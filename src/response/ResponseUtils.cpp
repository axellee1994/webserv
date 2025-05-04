#include "../../includes/response/Response.hpp"

bool Response::exactMatch()
{
    std::map<std::string, Location>::iterator it;
    for(it = sc->exact_loc.begin(); it != sc->exact_loc.end(); ++it) 
    {
        if(it->first == uri) 
        {
            loc = &(it->second);
            isLoc = true;
            return isLoc;
        }
    }
    return false;
}

bool Response::preferPrefixMatch(std::map<std::string, Location> &locMap) 
{
    std::map<std::string, Location>::iterator it;
    std::string prefix;
    size_t bestLength = 0;

    for (it = locMap.begin(); it != locMap.end(); ++it) 
    {
        prefix = it->first;
        if (uri.substr(0, prefix.length()) == prefix && prefix.length() > bestLength) 
        {
            loc = &(it->second);
            bestLength = prefix.length();
        }
    }
    if (bestLength > 0) 
    {
        isLoc = true;
        return isLoc;
    }
    return false;
}


void Response::generateAllowHeader(const std::vector<std::string>& methods) 
{
    std::string header;
    
    for (std::vector<std::string>::const_iterator it = methods.begin(); it != methods.end(); ++it) 
    {
        if (it != methods.begin())
            header += ", ";
        header += *it;
    }
    resHeaders["Allow"] = header;
}


void Response::normaliseSegments() 
{
    bool trail;
    size_t pos; 
    size_t nextSlash;
    std::string seg;
    std::vector<std::string> stack;
    std::string result;

    trail = !url.empty() && url[url.length() - 1] == '/';
    
    pos = 0;
    while (pos < url.length()) 
    {
        nextSlash = url.find('/', pos);
        seg = (nextSlash == std::string::npos) ? url.substr(pos) : url.substr(pos, nextSlash - pos);
        
        if (!seg.empty()) 
        {  
            if (seg == ".." && !stack.empty())
                stack.pop_back();
            else if (seg != "." && seg != "..")
                stack.push_back(seg); 
        }
        pos = (nextSlash == std::string::npos) ? url.length() : nextSlash + 1;
    }

    if (stack.empty()) 
        result = "/";
    else 
    {
        for (size_t i = 0; i < stack.size(); i++) 
            result += "/" + stack[i];
    }
    if (trail)
        result += "/";
    url = result;  
}

bool Response::validEndPoint()
{
    std::string match;
    size_t maxLength = 0; 

    for (EndpointMap::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it) 
    {
        const std::string &endpoint = it->first;
        if (uri.length() >= endpoint.length() && 
            uri.substr(0, endpoint.length()) == endpoint &&
            (endpoint == "/" ||
            uri.length() == endpoint.length() ||
            (uri.length() > endpoint.length() && uri[endpoint.length()] == '/')) &&
            endpoint.length() > maxLength)
        {
            match = endpoint;
            maxLength = endpoint.length(); 
        }
    }
    if (match.empty()) 
    {
        std::cout << "uri: " << uri << " invalid endpoint" << std::endl;
        statusCode = 404;  
        return false;  
    }

    const MethodList &allowed = endpoints.find(match)->second;
    for (MethodList::const_iterator it = allowed.begin(); it != allowed.end(); ++it) 
    {
        if (*it == method) 
            return true;
    }
    std::cout << "Method now allowed" << std::endl;  
    statusCode = 405;  
    generateAllowHeader(allowed);
    return false;  
}

bool Response::isDirectory(const std::string &path) const 
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

bool Response::pathExists(const std::string &path) const
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

bool Response::hasReadPermission(const std::string &path) const 
{
    return (access(path.c_str(), R_OK) == 0);
}

bool Response::hasWritePermission(const std::string &path) const 
{
    return (access(path.c_str(), W_OK) == 0);
}

bool Response::hasExecPermission(const std::string &path) const 
{
    return (access(path.c_str(), X_OK) == 0);
}

bool Response::validateFileSize(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        if (st.st_size > MAX_BODY_SIZE)
        {
            statusCode = CONTENT_TOO_LARGE;
            return false;
        }
        return true;
    }
    statusCode = INTERNAL_SERVER_ERROR;
    return false;
}

bool Response::readFile(const std::string &path) 
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file)
        return false;

    const size_t bufferSize = 1024;
    char buffer[bufferSize];
    std::string content;
    
    time_t start_time = time(NULL);

    while (file.read(buffer, bufferSize)) 
    {
        if (time(NULL) - start_time > READ_TIMEOUT) 
        {
            file.close();
            throw std::runtime_error("Read timeout");
        }
        content.append(buffer, file.gcount());
    }
    if (file.gcount() > 0) 
    {
        if (time(NULL) - start_time > READ_TIMEOUT) 
        {
            file.close();
            throw std::runtime_error("Read timeout");
        }
        content.append(buffer, file.gcount());
    }
    
    resBody = content;
    file.close();
    return true;
}

void Response::listDirectory(const std::string &path) 
{
    resBody = "<html><body><ul>";

    DIR* dir = opendir(path.c_str());
    if (dir) 
    {
        struct dirent* entry;
        while ((entry = readdir(dir))) 
        {
            std::string name = entry->d_name;

            if (name == "." || name == "..")
                continue;
            
            resBody += "<li><a href=\"";
            resBody += name; 
            resBody += "\">";
            resBody += name; 
            resBody += "</a></li>";
        }
        closedir(dir); 
    } 
    resBody += "</ul></body></html>";
}

void Response::setBodyHeaders(const std::string &path)
{
    statusCode = 200;
    resHeaders["Content-Type"] = getMimeType(path);
    resHeaders["Content-Length"] = toString(resBody.size());
}

std::string Response::getMimeType(const std::string &path) const
{
    size_t dot = path.find_last_of(".");
    if (dot == std::string::npos)
        return "text/plain";
    
    std::string ext = path.substr(dot + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    
    return "text/plain";
}

void Response::setRequiredHeaders()
{
    resHeaders["Connection"] = req_ptr->shouldKeepAlive() ? "keep-alive" : "close";
    resHeaders["Server"] = "webserv/1.0";
    char date[32];
    formatHTTPDate(date);
    resHeaders["Date"] = date;
}

void Response::formatHTTPDate(char* buffer) 
{
    time_t now = time(NULL);
    struct tm* tm_info = gmtime(&now);
    
    if (tm_info) 
        strftime(buffer, 32, "%a, %d %b %Y %H:%M:%S GMT", tm_info);
    else 
        strcpy(buffer, "Thu, 01 Jan 1970 00:00:00 GMT");
}

std::string Response::toString(size_t num) const
{
    std::ostringstream ss;
    ss << num;
    return ss.str();
}

int Response::stringToInt(const std::string& str)
{
    std::stringstream ss(str);
    int result;
    ss >> result;
    return result;
}


std::string Response::getStatusText() const
{
    switch (statusCode)
    {
        case CONTINUE:
            return "Continue";
        case OK:
            return "OK";
        case CREATED:
            return "Created";
        case ACCEPTED:
            return "Accepted";
        case NO_CONTENT:
            return "No Content";
        case MULTIPLE_CHOICES:
            return "Multiple Choices";
        case MOVED_PERMANENTLY:
            return "Moved Permanently";
        case FOUND:
            return "Found";
        case SEE_OTHER:
            return "See Other";
        case NOT_MODIFIED:
            return "Not Modified";
        case TEMPORARY_REDIRECT:
            return "Temporary Redirect";
        case PERMANENT_REDIRECT:
            return "Permanent Redirect";
        case BAD_REQUEST:
            return "Bad Request";
        case UNAUTHORIZED:
            return "Unauthorized";
        case FORBIDDEN:
            return "Forbidden";
        case NOT_FOUND:
            return "Not Found";
        case METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case NOT_ACCEPTABLE:
            return "Not Acceptable";
        case PROXY_AUTHENTICATION_REQUIRED:
            return "Proxy Authentication Required";
        case REQUEST_TIMEOUT:
            return "Request Timeout";
        case CONFLICT:
            return "Conflict";
        case GONE:
            return "Gone";
        case LENGTH_REQUIRED:
            return "Length Required";
        case PRECONDITION_FAILED:
            return "Precondition Failed";
        case CONTENT_TOO_LARGE:
            return "Content Too Large";
        case URI_TOO_LONG:
            return "URI Too Long";
        case UNSUPPORTED_MEDIA_TYPE:
            return "Unsupported Media Type";
        case RANGE_NOT_SATISFIABLE:
            return "Range Not Satisfiable";
        case EXPECTATION_FAILED:
            return "Expectation Failed";
        case UNPROCESSABLE_CONTENT:
            return "Unprocessable Content";
        case TOO_EARLY:
            return "Too Early";
        case UPGRADE_REQUIRED:
            return "Upgrade Required";
        case PRECONDITION_REQUIRED:
            return "Precondition Required";
        case TOO_MANY_REQUESTS:
            return "Too Many Requests";
        case REQUEST_HEADER_FIELDS_TOO_LARGE:
            return "Request Header Fields Too Large";
        case UNAVAILABLE_FOR_LEGAL_REASONS:
            return "Unavailable For Legal Reasons";
        case INTERNAL_SERVER_ERROR:
            return "Internal Server Error";
        case NOT_IMPLEMENTED:
            return "Not Implemented";
        case BAD_GATEWAY:
            return "Bad Gateway";
        case SERVICE_UNAVAILABLE:
            return "Service Unavailable";
        case GATEWAY_TIMEOUT:
            return "Gateway Timeout";
        case HTTP_VERSION_NOT_SUPPORTED:
            return "HTTP Version Not Supported";
        case VARIANT_ALSO_NEGOTIATES:
            return "Variant Also Negotiates";
        case INSUFFICIENT_STORAGE:
            return "Insufficient Storage";
        case LOOP_DETECTED:
            return "Loop Detected";
        case NOT_EXTENDED:
            return "Not Extended";
        case NETWORK_AUTHENTICATION_REQUIRED:
            return "Network Authentication Required";
        default:
            return "Unknown Status Code";
    }
}

std::string Response::generateFilename(const std::string& contentType) const
{
    time_t now = time(0);
    struct tm *t = localtime(&now);

    char basename[20];
    sprintf(basename, "%04d%02d%02d_%02d%02d%02d",
            t->tm_year + 1900,
            t->tm_mon + 1,
            t->tm_mday,
            t->tm_hour,
            t->tm_min,
            t->tm_sec);
    std::string extension;
    if (contentType == "text/html")
        extension = ".html";
    else if (contentType == "text/css")
        extension = ".css";
    else if (contentType == "application/javascript")
        extension = ".js";
    else if (contentType == "image/jpeg")
        extension = ".jpg";
    else if (contentType == "image/png")
        extension = ".png";
    else if (contentType == "image/gif")
        extension = ".gif";
    else if (contentType == "application/pdf")
        extension = ".pdf";
    else if (contentType == "application/xml")
        extension = ".xml";
    else if (contentType == "application/json")
        extension = ".json";
    else
        extension = ".txt";
    return std::string(basename) + extension;
}