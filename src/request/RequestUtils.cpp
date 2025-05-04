#include "../../includes/request/Request.hpp"

int Request::getState() const
{
    return state;
}

void Request::setState(int new_state)
{
    state = new_state;
}

void Request::setStateAndStatus(int new_state, int new_status)
{
    state = new_state;
    statusCode = new_status;
}

bool Request::isUnreserved(char c) 
{
    return std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '.' || c == '_' || c == '~';
}

bool Request::isSubDelimiter(char c) 
{
    return c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' || c == ')' || c == '*' || 
           c == '+' || c == ',' || c == ';' || c == '=';
}

bool Request::isPathReserved(char c) 
{
    return c == ':' || c == '/' || c == '@' || c == ';' || c == '=' || c == '+' || c == ',' || c == '$';
}

bool Request::isQueryReserved(char c) 
{
    return c == '/' || c == '?' || c == ':' || c == '@' || c == '&' || 
           c == '=' || c == '+' || c == ',' || c == '$' || c == ';';
}

bool Request::isTchar(char c) 
{
   return std::isalnum(c) || c == '!' || c == '#' || 
        c == '$' || c == '%' || c == '&' || c == '\'' ||
        c == '*' || c == '+' || c == '-' || c == '.' ||
        c == '^' || c == '_' ||c == '`' || c == '|' || c == '~';
}

bool Request::isFieldVchar(char c) 
{
    return std::isprint(c) || static_cast<unsigned char>(c) >= 128;
}

void Request::toLowerCase(std::string &str) 
{
    for (size_t i = 0; i < str.size(); ++i) 
        str[i] = std::tolower(str[i]);  
}

bool Request::isRepeatableHeader(const std::string& name) 
{
   return name == "accept" || name == "accept-charset" || name == "accept-encoding" || 
          name == "accept-language" || name == "www-authenticate" || name == "proxy-authenticate" || 
          name == "via" || name == "transfer-encoding" || name == "content-encoding" || 
          name == "content-language" || name == "cache-control" || name == "if-match" || 
          name == "if-none-match" || name == "vary" || name == "connection" || name == "upgrade" || 
          name == "link" || name == "prefer" || name == "set-cookie" || name == "cookie";
}


bool Request::isValidNumber(const std::string& str, unsigned long long& num)
{
    if (str.empty() || (str[0] == '0' && str.size() > 1))
        return false;

    const char* cstr = str.c_str();
    char* endPtr;
    num = std::strtoull(cstr, &endPtr, 10);
    
    return *endPtr == '\0';
}

void Request::isCGIReq()
{
    size_t cgiPos = path.find("/cgi-bin/");
    if (cgiPos != std::string::npos)
        requiresCGI = true;
}

bool Request::parseContentDisposition(const std::string &header) 
{
    size_t namePos = header.find("name=\"");
    if (namePos == std::string::npos)
        return false;

    namePos += 6;
    size_t endPos = header.find("\"", namePos);
    if (endPos == std::string::npos || endPos == namePos)
        return false;
      
    nameMPF = header.substr(namePos, endPos - namePos);

    size_t filenamePos = header.find("filename=\"");
    if (filenamePos != std::string::npos)
    {
        filenamePos += 10;
        endPos = header.find("\"", filenamePos);
        if (endPos == std::string::npos)
            return false;
    
        filename = header.substr(filenamePos, endPos - filenamePos);
        
        if (filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos)
            return false;
    }
    else
    {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char name[20];

        sprintf(name, "%04d%02d%02d_%02d%02d%02d.txt",
                t->tm_year + 1900,
                t->tm_mon + 1,
                t->tm_mday,
                t->tm_hour,
                t->tm_min,
                t->tm_sec);
        
        filename = std::string(name);
    }
    return true;
}

bool Request::parseContentType(const std::string &header)
{
    size_t pos = header.find(": ");
    if (pos == std::string::npos) 
        return false;

    size_t endPos = header.find("\r\n", pos);
    if (endPos == std::string::npos)
        return false;
    contentType = header.substr(pos + 2, endPos - pos - 2);
    return true;
}

bool Request::extractUri()
{   
    if (uri == "http://" || uri == "https://") 
        return false; 
    
    if (uri.substr(0, 7) == "http://" || uri.substr(0, 8) == "https://") 
    {
        size_t protocolEnd = uri.find("://");
        
        size_t pathStart = uri.find('/', protocolEnd + 3);
        if (pathStart == std::string::npos) 
            uri = "/";
        else 
            uri = uri.substr(pathStart);
    }
    return true;
}

