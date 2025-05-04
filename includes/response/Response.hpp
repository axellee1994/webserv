#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <ctime>
#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <map>
#include "../config/ConfigParser.hpp"
#include "../request/Request.hpp"

enum ResponseState
{
    NO_RES,
    PROC_RES,
    CGI_RUNNING,
    RES_DONE
};

enum StatusCode 
{
    CONTINUE = 100,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    REQUEST_TIMEOUT = 408,
    CONFLICT = 409,
    GONE = 410,
    LENGTH_REQUIRED = 411,
    PRECONDITION_FAILED = 412,
    CONTENT_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNSUPPORTED_MEDIA_TYPE = 415,
    RANGE_NOT_SATISFIABLE = 416,
    EXPECTATION_FAILED = 417,
    UNPROCESSABLE_CONTENT = 422,
    TOO_EARLY = 425,
    UPGRADE_REQUIRED = 426,
    PRECONDITION_REQUIRED = 428,
    TOO_MANY_REQUESTS = 429,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    UNAVAILABLE_FOR_LEGAL_REASONS = 451,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
    VARIANT_ALSO_NEGOTIATES = 506,
    INSUFFICIENT_STORAGE = 507,
    LOOP_DETECTED = 508,
    NOT_EXTENDED = 510,
    NETWORK_AUTHENTICATION_REQUIRED = 511
};

class Multiplexer;
class CGI;

class Response 
{
    private:
        int client_fd;
        Multiplexer *mp;
        Request *req_ptr;
        ServerConfig *sc;
        ClientSocket *client;
        int state;
        int statusCode;
        std::string uri;
        std::string url;
        std::string method;
        Location *loc;
        std::string resBody;
        std::map<std::string, std::string> resHeaders;
        std::string response;
        typedef std::vector<std::string> MethodList;
        typedef std::map<std::string, MethodList> EndpointMap;
        EndpointMap endpoints;
        std::map<int, std::string> error_responses;
        bool isLoc;
        size_t bytes_sent;
        std::string scriptName;
        std::string scriptFileName;
        std::string pathInfo;
        std::string pathTranslated;
        std::string interpreter;
        CGI *cgi;

        void matchLocation();
        bool exactMatch();
        bool preferPrefixMatch(std::map<std::string, Location> &locMap);
        bool resolvePath();
        void generateAllowHeader(const std::vector<std::string>& methods);
        void normaliseSegments();
        bool validEndPoint();
        bool isDirectory(const std::string& path) const;
        bool pathExists(const std::string& path) const;
        bool hasReadPermission(const std::string& path) const;
        bool validateFileSize(const std::string& path);
        bool readFile(const std::string& path);
        void listDirectory(const std::string& path);
        void setBodyHeaders(const std::string &path);
        std::string getMimeType(const std::string &path) const;
        bool handleDeleteRequest();
        bool hasWritePermission(const std::string &path) const;
        bool hasExecPermission(const std::string &path) const;
        bool handlePostRequest();
        void standardResponse(std::string cgi_headers);
        std::string toString(size_t num) const;
        int stringToInt(const std::string& str);
        void setRequiredHeaders();
        void formatHTTPDate(char* buffer);
        std::string getStatusText() const;
        void parseCGIPaths();
        bool handleCGIRequest();
        void CGIError();
        std::string generateFilename(const std::string& contentType) const;


        template<typename T>
        bool prelim_check(const T *data) 
        {
            if (data->limit_except.size() > 0 && std::find(data->limit_except.begin(), data->limit_except.end(), method) == data->limit_except.end())
            {
                std::cout << "limit except error" << std::endl;
                statusCode = 405;
                generateAllowHeader(data->limit_except);
                return false;
            }
            if (data->ret.size() > 0)
            {
                statusCode = data->ret.begin()->first;
                if (statusCode == 301) 
                    resHeaders["Location"] = data->ret.begin()->second;
                return false;
            }
            return true;
        }

          
        template<typename T>
        bool handleGetRequest(const T *data) 
        {
            if (!pathExists(url))
            {
                std::cout << "GET Path: " << url << " does not exist" << std::endl;
                statusCode = 404;
                return false;
            }
            if (!hasReadPermission(url))
            {
                std::cout << "GET no read permission" << std::endl;
                statusCode = 403;
                return false;
            }
            if (!validateFileSize(url))
            {
                std::cout << "GET invalid file size" << std::endl;
                return false;
            }
                
            if (isDirectory(url)) 
            {
                for (size_t i = 0; i < data->index.size(); i++)
                {
                    std::string index_path = url + ((!url.empty() && url[url.length() - 1] != '/') ? "/" : "") + data->index[i];
                    if (!pathExists(index_path))
                        continue;
                    try
                    {
                        if (readFile(index_path)) 
                        {
                            setBodyHeaders(index_path);
                            return true;
                        }
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << '\n';
                    }
                    statusCode = 500;
                    return false;
                }
                if (data->autoindex)
                {
                    listDirectory(url);
                    resHeaders["Content-Type"] = "text/html";
                    resHeaders["Content-Length"] = toString(resBody.size());
                    statusCode = 200;
                    return true;
                }
                statusCode = 403;  
                return false;
            }
            try
            {
                if (!readFile(url)) 
                {
                    std::cout << "GET couldnt read file: " << url << std::endl;
                    statusCode = 403;  
                    return false;
                }
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                statusCode = 500;
                return false;
            }
            setBodyHeaders(url);
            return true;
        };


        template<typename T>
        void errorResponse(const T *data)
        {
            if (data && data->error_pages.find(statusCode) != data->error_pages.end())
            {
                std::string error_path = data->root + 
                    ((data->root.empty() || data->root[data->root.length() - 1] != '/') ? "/" : "") + 
                    (data->error_pages.at(statusCode)[0] == '/' ? data->error_pages.at(statusCode).substr(1) : data->error_pages.at(statusCode));
                if (!pathExists(error_path))
                {
                    statusCode = 404;
                    resBody += error_responses[404];
                }
                else if (!hasReadPermission(error_path))
                {
                    statusCode = 403;
                    resBody += error_responses[403];
                }
                else 
                {
                    try
                    {
                        if (!validateFileSize(error_path) || !readFile(error_path))
                            resBody += error_responses[statusCode];
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << e.what() << '\n';
                        resBody = error_responses[statusCode];
                    }
                }   
            }
            else 
            {
                resBody += error_responses[statusCode];
            }
            response += "HTTP/1.1 " + toString(statusCode) + " " + getStatusText() + "\r\n";
            setRequiredHeaders();
            resHeaders["Content-Type"] = "text/html";   
            resHeaders["Content-Length"] = toString(resBody.size());

            std::map<std::string, std::string>::const_iterator it;
            for (it = resHeaders.begin(); it != resHeaders.end(); ++it) 
                response += it->first + ": " + it->second + "\r\n";
            
            response += "\r\n";
            response += resBody;    
        }; 

       
        template<typename T>
        bool checkCGI(const T *data)
        { 
            std::string ext = "";
            size_t dot = scriptName.rfind('.');
            if (dot == std::string::npos)
            {
                statusCode = 403;
                return false;
            }
            
            ext = scriptName.substr(dot);  
            std::map<std::string, std::string>::const_iterator it = data->cgi.find(ext);

            if (it == data->cgi.end())
            {
                std::cout << "extension not found\n";
                statusCode = 403;
                return false;
            }
            interpreter = it->second;
            if (interpreter.empty() || !hasExecPermission(interpreter))
            {
                std::cout << "interpreter: " << interpreter << "exec permission is: " << !hasExecPermission(interpreter) << std::endl;
                statusCode = 500;
                return false;
            }
            if (!pathExists(scriptFileName))
            {
                std::cout << "path doesnt exist: " << scriptFileName << std::endl;
                statusCode = 404;
                return false;
            }
            if (!hasReadPermission(scriptFileName) || !hasExecPermission(scriptFileName))
            {
                std::cout << "readPermission: " << hasReadPermission(scriptFileName) << ", execPermission: " << hasExecPermission(scriptFileName) << std::endl;
                statusCode = 403;
                return false;
            }   
            return true;
        }
    
    public:
        Response(int fd, Request *rp, ServerConfig *config, Multiplexer *mp, ClientSocket *cs);
        Response(const Response& src);
        Response& operator=(const Response& rhs);
        ~Response();
        void reset();
        void init();  
        std::string getResponse() const;
        void setServer(ServerConfig *config);
        size_t getBytesSent() const;
        void setBytesSent(size_t bytes);
        int getState() const;
        const std::string &getScriptName() const;
        const std::string &getScriptFileName() const;
        const std::string &getPathInfo() const;
        const std::string &getPathTranslated() const;
        Multiplexer *getMultiplexer() const;
        void handleCGIRead();
        void handleCGIWrite();
        ClientSocket *getClient() const;
        int getClientFd() const;
};

#endif