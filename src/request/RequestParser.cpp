#include "../../includes/request/Request.hpp" 

void Request::parseRequest(const std::vector<unsigned char>& request)
{
    bool firstBlock = true;

    if (state == PROC_REQ_LINE)
    {
        if (firstBlock)
            rawReqLine.insert(rawReqLine.end(), request.begin(), request.end());
        firstBlock = false;
        parseRequestLine();   
    }
    
    if (state == PROC_HEADERS)
    {
        if (firstBlock)
            rawHeaders.insert(rawHeaders.end(), request.begin(), request.end());
        firstBlock = false;
        parseHeaders();
    }
    if (state == PROC_BODY)
    {
        if (firstBlock)
            body.insert(body.end(), request.begin(), request.end());
        parseBody();
    }
    if (state == CHUNK_SIZE || state == CHUNK_DATA)
    {
        if (firstBlock)
            rawChunks.insert(rawChunks.end(), request.begin(), request.end());
        parseChunkedBody();
    }
    if (state == MPF_BOUNDARY || state == MPF_HEADERS || state == MPF_BODY)
    {
        if (firstBlock)
            rawMPF.insert(rawMPF.end(), request.begin(), request.end());
        parseMPF();
    }
}

void Request::parseRequestLine()
{
    size_t reqLineEnd;
    bool found = false;
    
    for (size_t i = 0; i < rawReqLine.size() - 1; ++i) 
    {
        if (rawReqLine[i] == '\r' && rawReqLine[i + 1] == '\n') 
        {
            reqLineEnd = i;
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        return;
    }

    rawHeaders = rawReqLine;
    for (size_t i = 0; i < reqLineEnd; ++i) 
        reqLineStr += static_cast<char>(rawReqLine[i]);
    
    std::istringstream iss(reqLineStr);
    if (reqLineStr.empty() || !validReqLineFormat() || !(iss >> method >> uri >> version))
    {
        setStateAndStatus(REQ_DONE, 400);
        return;
    }
    if (!validMethod() || !validUri() || !validVersion())
        return;

    setState(PROC_HEADERS);
    return;
}

bool Request::validReqLineFormat()
{
    int spaceCount;
    
    spaceCount = 0;
    for (size_t i = 0; i < reqLineStr.length(); i++)
    {
        if (isspace(reqLineStr[i]))
        {
            if (reqLineStr[i] != ' ')
                return false;
            if (i != 0 && isspace(reqLineStr[i - 1]))
                return false;
            spaceCount++;
        }
    }
    return spaceCount == 2;
}

bool Request::validMethod()
{
    if (method.empty() || method[0] < 'A' || method[0] > 'Z') 
    {
        setStateAndStatus(REQ_DONE, 400);
        return false;
    }
    for (std::string::size_type i = 1; i < method.size(); ++i) 
    {
        char c = method[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||  
              c == '-' || c == '.' || c == '_' || c == '~'))
        {
            setStateAndStatus(REQ_DONE, 400);
            return false;
        }
    }

    if (method != "GET" && method != "POST" && method != "DELETE")
    {
        setStateAndStatus(REQ_DONE, 501);
        return false;
    }

    return true;
}

bool Request::validUri()
{
    if (!extractUri())
    {
        setStateAndStatus(REQ_DONE, 400);
        return false;
    }
    if (uri.length() > MAX_URI_LENGTH)
    {
        setStateAndStatus(REQ_DONE, 414);
        return false;
    }
    if (uri.find("../") != std::string::npos || uri.find("..\\") != std::string::npos)
    {
        std::cout << "Directory traversal attempt detected" << std::endl;
        setStateAndStatus(REQ_DONE, 403);
        return false;
    }
    bool hasTrailingSlash = (!uri.empty() && uri[uri.length() - 1] == '/');

    size_t queryPos = uri.find('?');
    path = (queryPos != std::string::npos) ? uri.substr(0, queryPos) : uri;
    query = (queryPos != std::string::npos) ? uri.substr(queryPos + 1) : "";
    if (hasTrailingSlash && !path.empty() && path[path.length() - 1] != '/') 
        path += '/';
    for (size_t i = 0; i < path.length(); i++)
    {
        if (!isUnreserved(path[i]) && !isSubDelimiter(path[i]) && !isPathReserved(path[i]))
        {
            setStateAndStatus(REQ_DONE, 400);
            return false;
        }
    }
    for (size_t i = 0; i < query.length(); i++)
    {
        const char c = query[i];
        if (!isUnreserved(c) && !isSubDelimiter(c) && !isQueryReserved(c))
        {
            setStateAndStatus(REQ_DONE, 400);
            return false;
        }
    }
    
    isCGIReq();
    return true;
}

bool Request::validVersion()
{
    if (version == "HTTP/1.1")
         return true;
    if (version == "HTTP/0.9" || version == "HTTP/1.0" || version == "HTTP/2" || version == "HTTP/2.0" || version == "HTTP/3" || version == "HTTP/3.0")
    { 
        setStateAndStatus(REQ_DONE, 505);
        return false;
    }
    setStateAndStatus(REQ_DONE, 400);
    return false;
}   

void Request::parseHeaders()
{
    std::vector<unsigned char> rawMessage = rawHeaders;
    size_t headersEnd = 0;
    bool foundHeadersEnd = false;
    for (size_t i = 0; i < rawMessage.size() - 3; ++i) 
    {
        if (rawMessage[i] == '\r' && rawMessage[i + 1] == '\n' && rawMessage[i + 2] == '\r' && rawMessage[i + 3] == '\n') 
        {
            headersEnd = i;
            foundHeadersEnd = true;
            break;
        }
    }    
    if (!foundHeadersEnd)
        return;
    std::vector<unsigned char> newHeaders;
    for (size_t i = 0; i <= headersEnd + 1; ++i)
        newHeaders.push_back(rawMessage[i]);
    
    rawHeaders = newHeaders;
    
    body.clear();
    for (size_t i = headersEnd + 4; i < rawMessage.size(); ++i) 
        body.push_back(rawMessage[i]);
    
    size_t reqLineEnd = 0;
    bool foundReqLineEnd = false;
    for (size_t i = 0; i < rawHeaders.size() - 1; ++i) 
    {
        if (rawHeaders[i] == '\r' && rawHeaders[i + 1] == '\n') 
        {
            reqLineEnd = i;
            foundReqLineEnd = true;
            break;
        }
    }
    
    if (!foundReqLineEnd)
    {
        setStateAndStatus(REQ_DONE, 400);
        return;
    }
    std::vector<unsigned char> headersWithoutReqLine;
    for (size_t i = reqLineEnd + 2; i < rawHeaders.size(); ++i) 
        headersWithoutReqLine.push_back(rawHeaders[i]);
    
    rawHeaders = headersWithoutReqLine;
    
    if (!populateHeaders())
    {
        setStateAndStatus(REQ_DONE, 400);
        return;
    }
    
    if (!checkHeader() || !extractHeaderVals())
        return;
        
    if (method != "POST")
        setStateAndStatus(REQ_DONE, 200);
    else if (chunked_encoding)
    {
        setState(CHUNK_SIZE);
        rawChunks = body;
        body.clear();
    }
    else if(headers.find("content-type") != headers.end() && headers["content-type"] == "multipart/form-data")
    {
        setState(MPF_BOUNDARY);
        rawMPF = body;
        body.clear();
    }
    else
        setState(PROC_BODY);
    return;
}

bool Request::populateHeaders()
{
    size_t colonPos;
    size_t pair;
    std::string headerPair;
    std::string name;
    std::string value;
    std::string headerStr;

    for (size_t i = 0; i < rawHeaders.size(); ++i) 
        headerStr += static_cast<char>(rawHeaders[i]);

    while (!headerStr.empty())
    { 
        pair = headerStr.find("\r\n");
        if (pair == std::string::npos)
            break;
        headerPair = headerStr.substr(0, pair);
        headerStr = headerStr.substr(pair + 2);

        colonPos = headerPair.find(":");
        if (colonPos == std::string::npos)
            return false;
        name = headerPair.substr(0, colonPos);
        value = headerPair.substr(colonPos + 1);
        if (!validHeaderName(name) || !validHeaderValue(value))
            return false;
        if (headers.find(name) != headers.end() && !isRepeatableHeader(name))
            return false;
        headers[name] = value;
    }
    
    return headerStr.empty();
}


bool Request::validHeaderName(std::string& name) 
{
    if (name.empty()) 
        return false;

    for (size_t i = 0; i < name.length(); ++i) 
    {
        if (!isTchar(name[i])) 
            return false;
    }
    toLowerCase(name);
    return true;
}

bool Request::validHeaderValue(std::string& value) 
{
    if (value.empty()) 
        return false;
    size_t start = 0;
    size_t end = value.length();
    while (start < end && (value[start] == ' ' || value[start] == '\t')) 
        ++start;
    while (end > start && (value[end-1] == ' ' || value[end-1] == '\t')) 
        --end;
    
    for (size_t i = start; i < end; ++i) 
    {
        char c = value[i];
        if (!isFieldVchar(c) && c != ' ' && c != '\t') 
            return false;
    }
    value = value.substr(start, end - start);
    return true;
}

bool Request::checkHeader()
{
    if (headers.find("host") == headers.end())
    {
        setStateAndStatus(REQ_DONE, 400);
        return false;
    }
    if (method == "POST")
    {
        const bool hasLength = headers.find("content-length") != headers.end();
        const bool hasTransfer = headers.find("transfer-encoding") != headers.end();
        if (hasLength == hasTransfer)
        {
            setStateAndStatus(REQ_DONE, hasLength ? 400 : 411); 
            return false;
        }
    }
    return true;
}


bool Request::extractHeaderVals()
{
    size_t colonPos = headers["host"].find(':');
    if (colonPos != std::string::npos)
    {
        host = headers["host"].substr(0, colonPos);
        if (!validPort(headers["host"].substr(colonPos + 1)))
            return false;
    }
    else
    {
        host = headers["host"];
        port = 80;
    }
    std::map<std::string, std::string>::iterator it;
    it = headers.find("content-length");
    if (it != headers.end() && !validContentLength(it->second))
        return false;
    it = headers.find("transfer-encoding");
    if (it != headers.end())
        chunked_encoding = it->second == "chunked";
    it = headers.find("connection");
    if (it != headers.end())
        keep_alive = it->second == "keep-alive";
    it = headers.find("content-type");
    if (it != headers.end() && it->second.find("multipart/form-data") != std::string::npos)
        return extractBoundary(it->second);

    return true;
}

bool Request::validPort(const std::string& str)
{
    unsigned long long num;
    if (!isValidNumber(str, num) || num <= 0 || num > 65535)
    {
        setStateAndStatus(REQ_DONE, 400);
        return false;
    }
    port = static_cast<int>(num);
    return true;
}

bool Request::validContentLength(const std::string& str)
{
    unsigned long long num;
    if (!isValidNumber(str, num) || num < 0)
    {
        setStateAndStatus(REQ_DONE, 400);
        return false;
    }    
    if (num > client_max_body_size)
    {
        setStateAndStatus(REQ_DONE, 413);
        return false;
    }
    contentLength = num;
    return true;
}

bool Request::extractBoundary(std::string &value)
{
    std::string trimmed;
    size_t semicolon_pos = value.find(';');
    if (semicolon_pos == std::string::npos)
        return false;
    else
        trimmed = value.substr(0, semicolon_pos);

    size_t pos = value.find("boundary=");
    if (pos == std::string::npos)
        return false;
    pos += 9;
    if (pos >= value.length())
        return false;

    size_t end;
    if (value[pos] == '"')
    {
        end = value.find('"', ++pos);
        if (end == std::string::npos)
            return false;
    } 
    else
    {
        end = value.find(';', pos);
        if (end == std::string::npos)
            end = value.length();
    }

    std::string boundaryStr = "--" + value.substr(pos, end - pos);
    boundary.clear();
    for (size_t i = 0; i < boundaryStr.length(); ++i) {
        boundary.push_back(static_cast<unsigned char>(boundaryStr[i]));
    }
    
    value = trimmed;
    return boundary.size() > 2;
}

void Request::parseBody()
{
    if (chunked_encoding)
    {
        parseChunkedBody();
    }
    else if (headers.find("content-type") != headers.end() && headers["content-type"].find("multipart/form-data") != std::string::npos)
    {
        if (body.size() > 0)
        {
            rawMPF.insert(rawMPF.end(), body.begin(), body.end());
        }        
        parseMPF();
    }
    else
    {
        if (body.size() > contentLength)
        {
            setStateAndStatus(REQ_DONE, 400);
        }
        else if (body.size() == contentLength)
            setStateAndStatus(REQ_DONE, 200);
    }
}

void Request::parseChunkedBody() 
{
    while (state != REQ_DONE && rawChunkPos < rawChunks.size()) 
    {
        switch (state) 
        {
            case CHUNK_SIZE:
                if (!parseChunkSize())
                    return;
                break;
                
            case CHUNK_DATA:
                if (!parseChunkData())
                    return;
                break;
        }
    }
}

bool Request::parseChunkSize() 
{
    size_t lineEnd;
    bool found = false;
    for (size_t i = rawChunkPos; i < rawChunks.size() - 1; ++i) 
    {
        if (rawChunks[i] == '\r' && rawChunks[i + 1] == '\n') 
        {
            lineEnd = i;
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    std::string hexSize;
    for (size_t i = rawChunkPos; i < lineEnd; ++i) 
        hexSize += static_cast<char>(rawChunks[i]);
    size_t ext = hexSize.find(';');
    if (ext != std::string::npos)
        hexSize = hexSize.substr(0, ext);

    std::istringstream iss(hexSize);
    if (!(iss >> std::hex >> chunkSize))
    {
        setStateAndStatus(REQ_DONE, 400);
        return false;
    }
    if (body.size() + chunkSize > client_max_body_size) 
    {
        setStateAndStatus(REQ_DONE, 413);
        return false;
    }
    if (chunkSize == 0) 
    {
        setStateAndStatus(REQ_DONE, 200);
        return true;
    }

    rawChunkPos = lineEnd + 2;
    state = CHUNK_DATA;
    bytesRead = 0;
    return true;
}

bool Request::parseChunkData() 
{
    size_t available;
    
    available = std::min(chunkSize - bytesRead, rawChunks.size() - rawChunkPos);
    
    for (size_t i = 0; i < available; ++i) 
        body.push_back(rawChunks[rawChunkPos + i]);
    
    rawChunkPos += available;
    bytesRead += available;

    if (bytesRead == chunkSize) 
    {  
        if ((rawChunks.size() - rawChunkPos) < 2)
            return false;

        if (rawChunks[rawChunkPos] != '\r' || rawChunks[rawChunkPos + 1] != '\n') 
        {
            setStateAndStatus(REQ_DONE, 400);
            return false;
        }
        rawChunkPos += 2;
        state = CHUNK_SIZE;
        chunkSize = bytesRead = 0;
        return true;
    }
    return false;
}


void Request::parseMPF() 
{
    if (rawMPF.size() == 0)
        return;
    
    while (state != REQ_DONE) 
    {
        switch (state) 
        {
            case MPF_BOUNDARY:
                if (!parseBoundary())
                    return;  
                break; 
            case MPF_HEADERS:
                if (!parseMultiHeaders())
                    return;
                break;
            case MPF_BODY:
                if (!parsePart())
                    return;
                break;
        }
    }
}

bool Request::parseBoundary()
{
    if (rawMPF.size() == 0)
    {
        return false;
    }
    
    size_t boundary_end;
    bool found = false;
    for (size_t i = 0; i < rawMPF.size() - 1; ++i) 
    {
        if (rawMPF[i] == '\r' && rawMPF[i + 1] == '\n') 
        {
            boundary_end = i;
            found = true;
            break;
        }
    }
    if (!found)
        return false;
    
    std::vector<unsigned char> currBound(rawMPF.begin(), rawMPF.begin() + boundary_end); 
    std::vector<unsigned char> newRawMPF(rawMPF.begin() + boundary_end + 2, rawMPF.end());
    
    rawMPF = newRawMPF;

    if (currBound.size() == boundary.size() && std::equal(currBound.begin(), currBound.end(), boundary.begin()))
    {
        setStateAndStatus(MPF_HEADERS, 200);
    }
    else
    {
        std::vector<unsigned char> finalBoundary = boundary;
        finalBoundary.push_back('-');
        finalBoundary.push_back('-');
        if (currBound.size() == finalBoundary.size() && std::equal(currBound.begin(), currBound.end(), finalBoundary.begin()))
            setStateAndStatus(REQ_DONE, 200);
        else
            setStateAndStatus(REQ_DONE, 400);
    }
    
    return true;
}

bool Request::parseMultiHeaders()
{
    size_t headersEnd;
    bool found = false;
    for (size_t i = 0; i < rawMPF.size() - 3; ++i) 
    {
        if (rawMPF[i] == '\r' && rawMPF[i + 1] == '\n' && rawMPF[i + 2] == '\r' && rawMPF[i + 3] == '\n') 
        {
            headersEnd = i;
            found = true;
            break;
        }
    }
    if (!found)
        return false;
    std::string currHeaders;
    for (size_t i = 0; i < headersEnd; ++i) 
        currHeaders += static_cast<char>(rawMPF[i]);
    std::vector<unsigned char> newRawMPF(rawMPF.begin() + headersEnd + 4, rawMPF.end());
    rawMPF = newRawMPF;

    if (currHeaders.find("Content-Disposition:") == std::string::npos || !parseContentDisposition(currHeaders)
        || (currHeaders.find("Content-Type:") == std::string::npos && !parseContentType(currHeaders)))
    {
        setStateAndStatus(REQ_DONE, 400);
        return true;
    }
    setStateAndStatus(MPF_BODY, 200);
    return true;
}


bool Request::parsePart()
{
    size_t nextBoundary = std::string::npos;
    for (size_t i = 0; i <= rawMPF.size() - boundary.size(); ++i) 
    {
        bool found = true;
        for (size_t j = 0; j < boundary.size(); ++j) 
        {
            if (rawMPF[i + j] != boundary[j]) 
            {
                found = false;
                break;
            }
        }
        if (found) 
        {
            nextBoundary = i;
            break;
        }
    }

    if (nextBoundary == std::string::npos) 
    {
        std::vector<unsigned char> finalBoundary = boundary;
        finalBoundary.push_back('-');
        finalBoundary.push_back('-');
        
        for (size_t i = 0; i <= rawMPF.size() - finalBoundary.size(); ++i) 
        {
            bool found = true;
            for (size_t j = 0; j < finalBoundary.size(); ++j) 
            {
                if (rawMPF[i + j] != finalBoundary[j]) 
                {
                    found = false;
                    break;
                }
            }
            if (found) 
            {
                nextBoundary = i;
                break;
            }
        }
    }

    if (nextBoundary == std::string::npos)
        return false;

    for (size_t i = 0; i < nextBoundary; ++i) 
        body.push_back(rawMPF[i]);

    std::vector<unsigned char> newRawMPF(rawMPF.begin() + nextBoundary, rawMPF.end());
    rawMPF = newRawMPF;

    setStateAndStatus(MPF_BOUNDARY, 200);
    return true;
}
size_t Request::getClientMaxBodySize() const
{
    return client_max_body_size;
}

void Request::setClientMaxBodySize(size_t size)
{
    client_max_body_size = size;
}