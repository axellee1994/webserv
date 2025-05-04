#include "../../includes/response/Response.hpp"
#include "../../includes/cgi/CGI.hpp"

void Response::init()
{    
    statusCode = req_ptr->getStatusCode();
    state = PROC_RES;
    uri = req_ptr->getUri();
    method = req_ptr->getMethod();
    if (req_ptr->getState() == SPECIAL_ERR)
    {
        errorResponse((ServerConfig*)NULL);
        std::cout << "Special error response - Status: " << statusCode << std::endl;
        state = RES_DONE;
        return;
    }

    matchLocation();
    if (statusCode != 200 || !resolvePath()) 
    {
        std::cout << "Path resolution failed - Status: " << statusCode << std::endl;
        loc ? errorResponse(loc) : errorResponse(sc);
        state = RES_DONE;
        return;
    }
    
    bool res = false;
    if (req_ptr->getRequiresCGI() && method == "POST")
    {
        parseCGIPaths();
        res = loc ? checkCGI(loc) : checkCGI(sc);
        if (!res || !handleCGIRequest())
        {
            isLoc ? errorResponse(loc) : errorResponse(sc);
            state = RES_DONE;
            return;
        }
        state = CGI_RUNNING;
        return;
    } 
    else if (method == "GET")
        res = loc ? handleGetRequest(loc) : handleGetRequest(sc);
    else if (method == std::string("POST"))
        res = handlePostRequest();
    else if (method == "DELETE")
        res = handleDeleteRequest();
    res ?  standardResponse(""): (isLoc ? errorResponse(loc) : errorResponse(sc));
    state = RES_DONE;
}


void Response::matchLocation()
{
    if (exactMatch())
        return;
    if (preferPrefixMatch(sc->prefer_loc))
        return;
    if (preferPrefixMatch(sc->prefix_loc))
        return;
}


bool Response::resolvePath()
{    
    bool res = isLoc ? prelim_check(loc) : prelim_check(sc);
    if (!res)
    {
        std::cout << "Preliminary check failed - Status: " << statusCode << std::endl;
        return false;
    }

    if (isLoc && loc->alias.length() > 0)
    {
        std::string remaining = uri.substr(loc->path.length());
        if (!remaining.empty() && remaining[0] == '/') 
            remaining = remaining.substr(1);
        url = loc->alias + (loc->alias[loc->alias.size() - 1] == '/' ? "" : "/") + remaining;
    }

    if (url.empty())
    {
        if (isLoc && loc->root.length() > 0)
            url = loc->root + uri;
        else if (sc->root.length() > 0)
            url = sc->root + uri;
        else
        {
            statusCode = 500;
            std::cout << "Root path resolution failed - Status: " << statusCode << std::endl;
            return false;
        }
    }

    normaliseSegments(); 

    if (!validEndPoint())
    {
        std::cout << "Invalid endpoint - Status: " << statusCode << std::endl;
        return false;
    }
    if (isDirectory(url) && !url.empty() && url[url.length() - 1] != '/')
    {
        std::cout << "Directory redirect request" << std::endl;
        url += "/";
        statusCode = 301;
        resHeaders["Location"] = uri += "/";
        return false;
    }
    return true;
}

bool Response::handlePostRequest()
{
    const std::map<std::string, std::string>& headers = req_ptr->getHeaders();
    std::map<std::string, std::string>::const_iterator it = headers.find("content-type");
    if (it == headers.end())  
    {  
        statusCode = 415;  
        return false;  
    }
    std::string fn = req_ptr->getFilename();
    if (req_ptr->getFilename().empty())
        fn = generateFilename(it->second);
    if (!url.empty() && url[url.length() - 1] != '/') 
        url += "/";
    url += fn;
    statusCode = pathExists(url) ? 200 : 201;  
    std::ofstream file(url.c_str(), std::ios::binary | std::ios::trunc);
    if (!file)  
    {
        std::cout << "Unable to open file" << std::endl;
        statusCode = 500; 
        return false;  
    }

    const std::vector<unsigned char>& body = req_ptr->getBody();
    const size_t chunkSize = 1048576;  
    size_t offset = 0;
    time_t start_time = time(NULL);

    while (offset < body.size()) 
    {
        if (time(NULL) - start_time > WRITE_TIMEOUT) 
        {
            std::cout << "Write operation timed out" << std::endl;
            file.close();
            statusCode = 500; 
            return false;
        }
        
        size_t remaining = body.size() - offset;  
        size_t currChunkSize = std::min(remaining, chunkSize);  
        file.write(reinterpret_cast<const char*>(&body[offset]), currChunkSize);
        if (!file) 
        {  
            file.close();
            statusCode = 500;  
            return false;
        }
        offset += currChunkSize;  
    }
    file.close();

    resBody = "<html><body>Success: File posted</body></html>";
    resHeaders["Content-Type"] = "text/html";
    resHeaders["Content-Length"] = toString(resBody.size());
    if (statusCode == 201)  
        resHeaders["Location"] = uri + ((!uri.empty() && uri[uri.length() - 1] != '/') ? "/" : "") + req_ptr->getFilename();
    return true;
}


bool Response::handleDeleteRequest()
{
    if (!pathExists(url))
    {
        statusCode = 404;
        return false;
    }
    if (isDirectory(url))
    {
        std::cout << "Directory deletion is not allowed" << std::endl;
        statusCode = 403;
        return false;
    }
    std::string parent_dir = url.substr(0, url.find_last_of("/"));
    if (parent_dir.empty())
        parent_dir = ".";
    if (!hasWritePermission(parent_dir) || !hasExecPermission(parent_dir))
    {
        statusCode = 403;
        return false;
    } 
    if (remove(url.c_str()) != 0)
    {
        statusCode = 500;
        return false;
    }
    statusCode = 200;
    resBody = error_responses[200];
    resHeaders["Content-Type"] = "text/html";
    resHeaders["Content-Length"] = toString(resBody.size());
    return true;
}

void Response::standardResponse(std::string cgi_headers) 
{   
    setRequiredHeaders();

    response += "HTTP/1.1 " + toString(statusCode) + " " + getStatusText() + "\r\n";
    
    std::map<std::string, std::string>::const_iterator it;
    for (it = resHeaders.begin(); it != resHeaders.end(); ++it) 
        response += it->first + ": " + it->second + "\r\n";
     
    if (!cgi_headers.empty())
        response += cgi_headers; 
    
    response += "\r\n";

    if (!resBody.empty()) 
        response += resBody;
    std::cout << "Response complete - Status: " << statusCode << " " << getStatusText() << std::endl;
}


void Response::parseCGIPaths() 
{
    size_t queryPos = url.find('?');
    std::string path = (queryPos != std::string::npos) ? url.substr(0, queryPos) : url;

    size_t cgiPos = path.find("/cgi-bin/");
    size_t afterCGIBin = cgiPos + 9;
    size_t nextSlash = path.find('/', afterCGIBin);

    if (nextSlash == std::string::npos) 
    {
        scriptName = path.substr(cgiPos); 
        scriptFileName = path;  
        pathInfo = "";
        pathTranslated = "";
    } 
    else 
    {
        scriptName = path.substr(cgiPos, nextSlash - cgiPos);
        scriptFileName = path.substr(0, nextSlash);
        pathInfo = path.substr(nextSlash); 
        pathTranslated = path.substr(0, cgiPos) + pathInfo;
    }
}

bool Response::handleCGIRequest()
{
    if (cgi != NULL)
        delete cgi;
    cgi = new CGI(this, req_ptr, scriptFileName, interpreter);
    try 
    {   
        if (req_ptr->getMethod() == "POST")
            cgi->setPostBody(req_ptr->getBody());
        cgi->initiateCGI();
        cgi->createCGIProcess(); 
        return true;
    }
    catch (const std::exception& e) 
    {
        CGIError();
        return false;
    }
}

void Response::handleCGIRead()
{
    try 
    {   
        if (cgi == NULL || state == RES_DONE)
            return;
        cgi->readAvailOutput();
        if (cgi->isComplete() || cgi->getErrorCode() == 504)
        {
            if (cgi->getErrorCode() == 504) {
                statusCode = 504;
                errorResponse(sc);
                req_ptr->setKeepAlive(false);
                mp->modifyEpollEvent(client_fd, EPOLLOUT);
            } else {
                cgi->parseOutput();
                statusCode = stringToInt(cgi->getStatus());
                resBody = cgi->getBody();
                resHeaders["Content-Length"] = toString(resBody.size());
                standardResponse(cgi->getHeaders());
            }
            state = RES_DONE;
            delete cgi;
            cgi = NULL;
        }
    }
    catch (const std::exception& e) 
    {
        state = RES_DONE;
        statusCode = cgi->getErrorCode() ? cgi->getErrorCode() : 502;
        req_ptr->setKeepAlive(false);
        errorResponse(sc);
        mp->modifyEpollEvent(client_fd, EPOLLOUT);
        delete cgi;
        cgi = NULL;
    }
}

void Response::handleCGIWrite()
{
    try
    {   if (cgi == NULL)
            return;
        cgi->sendPostBody();
    }
    catch (const std::exception& e) 
    {
        if (cgi != NULL)
            cgi->cleanChild();
        CGIError();
    }
}

void Response::CGIError()
{
    statusCode = cgi->getErrorCode();
    delete cgi;
    cgi = NULL;
    isLoc ? errorResponse(loc) : errorResponse(sc);
    state = RES_DONE;
}


