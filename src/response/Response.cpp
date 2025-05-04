#include "../../includes/response/Response.hpp"
#include "../../includes/server/Multiplexer.hpp"
#include "../../includes/cgi/CGI.hpp"


Response::Response(int fd, Request *rp, ServerConfig *config, Multiplexer *multi, ClientSocket *cs) : client_fd(fd), mp(multi), req_ptr(rp), sc(config), client(cs), 
	state(NO_RES), statusCode(200), uri(""), url(""), method(""), loc(NULL), resBody(""), resHeaders(), 
	response(""), endpoints(), error_responses(), isLoc(false), bytes_sent(0),
	scriptName(""), scriptFileName(""), pathInfo(""), pathTranslated(""), interpreter(""), cgi(NULL)
{
	endpoints["/"].push_back("GET");
	endpoints["/login"].push_back("GET");
    endpoints["/uploads"].push_back("POST");
    endpoints["/uploads"].push_back("DELETE");
    endpoints["/uploads"].push_back("GET");
    endpoints["/cgi-bin"].push_back("GET");
    endpoints["/cgi-bin"].push_back("POST");
    endpoints["/error_pages"].push_back("GET");
	
    error_responses[200] = "<html><body><h1>200 OK</h1></body></html>";
	error_responses[201] = "<html><body><h1>201 Created</h1></body></html>";
	error_responses[204] = "<html><body><h1>204 No Content</h1></body></html>";
	error_responses[301] = "<html><body><h1>301 Moved Permanently</h1></body></html>";
	error_responses[302] = "<html><body><h1>302 Found</h1></body></html>";
	error_responses[303] = "<html><body><h1>303 See Other</h1></body></html>";
	error_responses[400] = "<html><body><h1>400 Bad Request</h1></body></html>";
	error_responses[403] = "<html><body><h1>403 Forbidden</h1></body></html>";
	error_responses[404] = "<html><body><h1>404 Not Found</h1></body></html>";
	error_responses[405] = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
	error_responses[408] = "<html><body><h1>408 Request Timeout</h1></body></html>";
	error_responses[411] = "<html><body><h1>411 Length Required</h1></body></html>";
	error_responses[413] = "<html><body><h1>413 Content Too Large</h1></body></html>";
	error_responses[414] = "<html><body><h1>414 URI Too Long</h1></body></html>";
	error_responses[415] = "<html><body><h1>415 Unsupported Media Type</h1></body></html>";
	error_responses[500] = "<html><body><h1>500 Internal Server Error</h1></body></html>";
	error_responses[501] = "<html><body><h1>501 Not Implemented</h1></body></html>";
	error_responses[502] = "<html><body><h1>502 Bad Gateway</h1></body></html>";
	error_responses[504] = "<html><body><h1>504 Gateway Timeout</h1></body></html>";
	error_responses[505] = "<html><body><h1>505 HTTP Version Not Supported</h1></body></html>";

	std::cout << "Response Constructor called" << std::endl;
}

Response::Response(const Response& src) : client_fd(src.client_fd), mp(src.mp), req_ptr(src.req_ptr), sc(src.sc), 
    state(src.state), statusCode(src.statusCode), uri(src.uri), url(src.url), method(src.method), 
    loc(src.loc), resBody(src.resBody), resHeaders(src.resHeaders), response(src.response), 
    endpoints(src.endpoints), error_responses(src.error_responses), isLoc(src.isLoc), 
    bytes_sent(src.bytes_sent), scriptName(src.scriptName), scriptFileName(src.scriptFileName), 
    pathInfo(src.pathInfo), pathTranslated(src.pathTranslated), interpreter(src.interpreter), 
    cgi(NULL)
{
    std::cout << "Response Copy Constructor called" << std::endl;
}

Response& Response::operator=(const Response& rhs)
{
    if (this != &rhs)
    {
        if (cgi)
        {
            delete cgi;
            cgi = NULL;
        }
        client_fd = rhs.client_fd;
        mp = rhs.mp;
        req_ptr = rhs.req_ptr;
        sc = rhs.sc;
        client = rhs.client;
        state = rhs.state;
        statusCode = rhs.statusCode;
        uri = rhs.uri;
        url = rhs.url;
        method = rhs.method;
        loc = rhs.loc;
        resBody = rhs.resBody;
        resHeaders = rhs.resHeaders;
        response = rhs.response;
        endpoints = rhs.endpoints;
        error_responses = rhs.error_responses;
        isLoc = rhs.isLoc;
        bytes_sent = rhs.bytes_sent;
        scriptName = rhs.scriptName;
        scriptFileName = rhs.scriptFileName;
        pathInfo = rhs.pathInfo;
        pathTranslated = rhs.pathTranslated;
        interpreter = rhs.interpreter;
    }
    std::cout << "Response Copy Assignment Operator called" << std::endl;
    return *this;
}

Response::~Response()
{
	if (cgi)
    	delete cgi;
}

void Response::reset() 
{
    state = NO_RES;
    statusCode = 200;
    uri = "";
    url = "";
    method = "";
    loc = NULL;
    resBody = "";
    resHeaders.clear();
    response = "";
    isLoc = false;
	bytes_sent = 0;
	scriptName = "";
	scriptFileName = "";
	pathInfo = "";
	pathTranslated = "";
	interpreter = "";	
	if (cgi)
	{
		delete cgi;
		cgi = NULL;
	}
    
    std::cout << "Response reset completed" << std::endl;
}


std::string Response::getResponse() const
{
	return response;
}

void Response::setServer(ServerConfig *config)	
{
	sc = config;
}

size_t Response::getBytesSent() const
{
	return bytes_sent;
}

void Response::setBytesSent(size_t bytes)
{
	bytes_sent += bytes;
}

int Response::getState() const
{
	return state;
}

const std::string& Response::getScriptName() const
{
	return scriptName;
}	

const std::string& Response::getScriptFileName() const
{
	return scriptFileName;
}	

const std::string& Response::getPathInfo() const
{
	return pathInfo;
}

const std::string& Response::getPathTranslated() const
{
	return pathTranslated;
}

Multiplexer* Response::getMultiplexer() const
{
	return mp;
}

ClientSocket *Response::getClient() const
{
    return client;
}

int Response::getClientFd() const
{
    return client_fd;
}   