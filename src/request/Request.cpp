#include "../../includes/request/Request.hpp" 

Request::Request(int fd, ClientSocket *client) : client(client), client_fd(fd), state(NO_REQ), statusCode(-1),
    rawReqLine(), reqLineStr(""), method(""), uri(""), path(""), query(""), processed_uri(""), version(""), 
    rawHeaders(), headers(), host(""), port(-1), contentLength(-1), chunked_encoding(false), keep_alive(true),
    body(), rawChunks(), rawChunkPos(0), chunkSize(0), bytesRead(0), requiresCGI(false), 
    boundary(), rawMPF(), nameMPF(""), filename(""), contentType(""), client_max_body_size(MAX_BODY_SIZE), 
    start_time(time(NULL)), firstReq(true), timeOutDetected(false)
{
    std::cout << "Request Constructor called" << std::endl;
}

Request::Request(const Request &src)
{
    *this = src;
    std::cout << "Request Copy Constructor called" << std::endl;
}

Request &Request::operator=(const Request &rhs)
{
    if (this != &rhs)
    {
        client = rhs.client;
        client_fd = rhs.client_fd;
        state = rhs.state;
        statusCode = rhs.statusCode;
        rawReqLine = rhs.rawReqLine;
        reqLineStr = rhs.reqLineStr;
        method = rhs.method;
        uri = rhs.uri;
        path = rhs.path;
        query = rhs.query;
        processed_uri = rhs.processed_uri;
        version = rhs.version;
        rawHeaders = rhs.rawHeaders;
        headers = rhs.headers;
        host = rhs.host;
        port = rhs.port;
        contentLength = rhs.contentLength;
        chunked_encoding = rhs.chunked_encoding;
        keep_alive = rhs.keep_alive;
        body = rhs.body;
        rawChunks = rhs.rawChunks;
        rawChunkPos = rhs.rawChunkPos;
        chunkSize = rhs.chunkSize;
        bytesRead = rhs.bytesRead;
        requiresCGI = rhs.requiresCGI;
        boundary = rhs.boundary;
        rawMPF = rhs.rawMPF;
        nameMPF = rhs.nameMPF;
        filename = rhs.filename;
        contentType = rhs.contentType;
        client_max_body_size = rhs.client_max_body_size;
        start_time = rhs.start_time;
        firstReq = rhs.firstReq;
        timeOutDetected = rhs.timeOutDetected;
    }
    std::cout << "Request Copy Assignment Operator called" << std::endl;
    return (*this);
}

Request::~Request()
{
    std::cout << "Request Destructor called" << std::endl;
}

void Request::reset()
{
    state = NO_REQ;
    statusCode = -1;
    rawReqLine.clear();
    reqLineStr = "";
    method = "";
    uri = "";
    path = "";
    query = "";
    processed_uri = "";
    version = "";
    rawHeaders.clear();
    headers.clear();
    host = "";
    port = -1;
    contentLength = -1;
    chunked_encoding = false;
    keep_alive = true;
    body.clear();
    rawChunks.clear();
    rawChunkPos = 0;
    chunkSize = 0;
    bytesRead = 0;
    requiresCGI = false;
    boundary.clear();
    rawMPF.clear();
    nameMPF = "";
    filename = "";
    contentType = "";
    start_time = -1;
    firstReq = false;
    timeOutDetected = false;
}

const std::string& Request::getMethod() const
{
    return (method);
}

const std::string& Request::getUrl() const
{
    return (processed_uri);
}

const std::string& Request::getVersion() const
{
    return (version);
}

std::map<std::string, std::string>& Request::getHeaders()
{
    return (headers);
}

const std::vector<unsigned char>& Request::getBody() const
{
    return (body);
}


const std::string& Request::getUri() const
{
    return (uri);
}

int Request::getClientFd() const
{
    return (client_fd);
}

bool Request::getRequiresCGI() const
{
    return (requiresCGI);
}

void Request::setRequiresCGI(bool val)
{
    requiresCGI = val;
}


bool Request::shouldKeepAlive() const 
{ 
    return keep_alive; 
}

void Request::setKeepAlive(bool val)
{
    keep_alive = val;
}

int Request::getStatusCode() const
{
    return statusCode;
}

const std::string& Request::getFilename() const
{
    return (filename);
}

const std::string& Request::getContentType() const
{
    return (contentType);
}

const std::string& Request::getHost() const
{
    return (host);
}

const std::string Request::getPort() const
{
    std::ostringstream ss;
    ss << port;
    return ss.str();
}

const std::string& Request::getQuery() const
{
    return (query);
}

ClientSocket *Request::getClient() const
{
    return client;
}

bool Request::hasTimedOut() 
{
    if ((state != NO_REQ && state != REQ_DONE) || (state == NO_REQ && firstReq))
    {
        time_t current_time = time(NULL);
        if (current_time - start_time > REQ_TIMEOUT)
        {
            return true;
        }
    }
    return false;
}

void Request::resetTimer() 
{
    start_time = time(NULL);
}


bool Request::isFirstReq() const
{
    return firstReq;
}

bool Request::getTimeOutDetected() const
{
    return timeOutDetected;
}

void Request::setTimeOutDetected()
{
    timeOutDetected = true;
}