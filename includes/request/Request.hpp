#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cctype>
#include <cstring>
#include <string>
#include <cstdlib>
#include <map>
#include "../constants.hpp"
#include "../server/ClientSocket.hpp"
#include <vector>
#include <algorithm>
#include <stdio.h>

enum RequestState
{
    NO_REQ,
    PROC_REQ_LINE,
    PROC_HEADERS,
    PROC_BODY,
    CHUNK_SIZE,
    CHUNK_DATA,
    MPF_BOUNDARY,
    MPF_HEADERS,
    MPF_BODY,
    REQ_DONE,
    SPECIAL_ERR   
};

class Request 
{
    private:
        ClientSocket *client;
        int client_fd;
        int state;
        int statusCode;
        std::vector<unsigned char> rawReqLine;
        std::string reqLineStr;
        std::string method;
        std::string uri;
        std::string path;
        std::string query;
        std::string processed_uri;
        std::string version;
        std::vector<unsigned char> rawHeaders;
        std::map<std::string, std::string> headers;
        std::string host;
        int port;
        unsigned long long contentLength;
        bool chunked_encoding;
        bool keep_alive;
        std::vector<unsigned char> body;
        std::vector<unsigned char> rawChunks;
        size_t rawChunkPos;
        size_t chunkSize;
        size_t bytesRead;
        bool requiresCGI;
        std::vector<unsigned char> boundary;
        std::vector<unsigned char> rawMPF;
        std::string nameMPF;
        std::string filename;
        std::string contentType;
        size_t client_max_body_size;
        time_t start_time;
        bool firstReq;
        bool timeOutDetected;
        
        void parseRequestLine();
        bool validReqLineFormat();
        bool validMethod();
        bool validVersion();
        bool validUri();
        bool isUnreserved(char c);
        bool isSubDelimiter(char c);
        bool isPathReserved(char c);
        bool isQueryReserved(char c);
        void parseHeaders();
        bool populateHeaders();
        bool validHeaderName(std::string& name);
        bool isTchar(char c); 
        void toLowerCase(std::string &str);
        bool validHeaderValue(std::string& value);
        bool isFieldVchar(char c);
        bool isRepeatableHeader(const std::string& name);
        bool checkHeader();
        bool extractHeaderVals();
        bool isValidNumber(const std::string& str, unsigned long long& num);
        bool validPort(const std::string& str);
        bool validContentLength(const std::string& str);
        bool extractBoundary(std::string &value);
        void parseBody();
        void parseChunkedBody();
        bool parseChunkSize();
        bool parseChunkData();
        void isCGIReq();
        void parseMPF();
        bool parseBoundary();
        bool parseMultiHeaders();
        bool parseContentDisposition(const std::string &header);
        bool parseContentType(const std::string &header);
        bool parsePart();
        bool extractUri();

    public:
        Request(int fd, ClientSocket *client);
        Request(const Request& src);
        Request& operator=(const Request& rhs);
        ~Request();

        void parseRequest(const std::vector<unsigned char>& request);
        void reset();

        const std::string& getMethod() const;
        const std::string& getUrl() const;
        const std::string& getVersion() const;
        std::map<std::string, std::string>& getHeaders();
        const std::vector<unsigned char>& getBody() const;
        const std::string& getUri() const;
        int getClientFd() const;        
        bool getRequiresCGI() const;
        void setRequiresCGI(bool val);
        bool shouldKeepAlive() const;
        int getState() const;
        void setState(int new_state);
        int getStatusCode() const;
        const std::string& getFilename() const;
        const std::string& getContentType() const;
        const std::string& getHost() const;
        void setKeepAlive(bool val);
        void setStateAndStatus(int new_state, int new_status);
        const std::string getPort() const;
        const std::string &getQuery() const;
        ClientSocket *getClient() const;
        size_t getClientMaxBodySize() const;
        void setClientMaxBodySize(size_t size);
        bool hasTimedOut();
        void resetTimer();
        bool isFirstReq() const;
        bool getTimeOutDetected() const;
        void setTimeOutDetected();
};

#endif 