#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cctype>
#include <ctime>
#include "../constants.hpp"
#include "../request/Request.hpp"
#include "../server/Multiplexer.hpp"

enum ParserState 
{
    READING_HEADERS,
    READING_BODY,
    ERROR,
    COMPLETE
};

class Response;

class CGI 
{
    private:
        static std::map<pid_t, CGI*> cgi_instances;
        static void sigchldHandler(int sig, siginfo_t *info, void *extra);

        Response *res_ptr;
        Request *req_ptr;
        std::string scriptPath;
        std::string executorPath;
        std::string postBody;    
        char* argv[3];
        std::map<std::string, std::string>    env_map;
        char **envp;
        int input_socket[2];  
        int output_socket[2]; 
        pid_t   cgi_pid;
        size_t bytes_sent;
        size_t received;
        std::string output;
        std::string response;
        int parser_state;
        std::string raw_headers;
        std::string body;
        std::string status;
        bool status_found;
        std::string content_type;
        bool content_type_found;
        int error_code;
        time_t start_time;
        bool process_complete;
        bool timedOut;

        void setMandatoryEnv();
        void mapToArray();
        void processHeaders(const std::string& headers);
        void validHeaderLine(const std::string& header_line);
        std::string validStatusLine(const std::string& header_value);
        bool isProcessRunning() const;
        bool hasTimedOut() const;
        void closeSockets();


    public:
        CGI(Response *res, Request *rq, std::string &sp, std::string &ep);
        ~CGI();

        void initiateCGI();
        void createCGIProcess();
        void setPostBody(const std::vector<unsigned char>& vec_body);
        void sendPostBody();
        void readAvailOutput();
        int getErrorCode() const;
        bool isComplete() const;
        Response *getRes() const;
        std::string getBody() const;
        std::string getHeaders() const;
        std::string getStatus() const;
        void cleanChild();
        void parseOutput();
};

#endif