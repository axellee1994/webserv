#include "../includes/cgi/CGI.hpp"
#include "../includes/response/Response.hpp"
#include "../includes/server/Multiplexer.hpp"

std::map<pid_t, CGI*> CGI::cgi_instances;

void CGI::sigchldHandler(int sig, siginfo_t *info, void *extra) 
{
    (void)sig;
    (void)info;
    (void)extra;
    
    pid_t pid;
    int status;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (cgi_instances.find(pid) != cgi_instances.end())
        {
            CGI* cgi = cgi_instances[pid];
            
            if (WIFSIGNALED(status))
            {
                if (cgi->timedOut)
                    cgi->error_code = 504;
                else
                    cgi->error_code = 502;
                cgi->parser_state = ERROR;
            }
            cgi->process_complete = true;
        }
    }
}

CGI::CGI(Response *res, Request *rq, std::string &sp, std::string &ep) : res_ptr(res), req_ptr(rq), 
    scriptPath(sp), executorPath(ep), postBody(), argv(), envp(NULL), input_socket(), output_socket(), cgi_pid(-1), bytes_sent(0), 
    received(0), output(""), response(""), parser_state(READING_HEADERS), raw_headers(""), body(""), status(""), status_found(false), 
    content_type(""), content_type_found(false), error_code(0), start_time(0), process_complete(false), timedOut(false)
{
    input_socket[0] = -1;
    input_socket[1] = -1;
    output_socket[0] = -1;
    output_socket[1] = -1;
}

CGI::~CGI() 
{
    cleanChild();
    
    if (cgi_pid > 0 && cgi_instances.find(cgi_pid) != cgi_instances.end()) 
        cgi_instances.erase(cgi_pid);
    
    if (envp != NULL) 
    {
        for (size_t i = 0; envp[i] != NULL; ++i) 
            delete[] envp[i];
        delete[] envp;
        envp = NULL;
    }
    for (int i = 0; i < 2; ++i) 
    {
        if (input_socket[i] != -1)
            close(input_socket[i]);
        if (output_socket[i] != -1)
            close(output_socket[i]);
    }
}


void CGI::setPostBody(const std::vector<unsigned char>& vec_body)
{
    postBody = std::string(vec_body.begin(), vec_body.end());
}

int CGI::getErrorCode() const
{
    return error_code;
}

bool CGI::isComplete() const
{
    return parser_state == COMPLETE;
}

Response *CGI::getRes() const
{
    return res_ptr;
}

std::string CGI::getBody() const
{
    return body;
}

std::string CGI::getHeaders() const
{
    return raw_headers;
}

std::string CGI::getStatus() const
{
    return status;
}


void CGI::initiateCGI()
{
    argv[0] = const_cast<char *>(executorPath.c_str());
    argv[1] = const_cast<char *>(scriptPath.c_str());
    argv[2] = NULL;

    setMandatoryEnv();
    mapToArray();

    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, input_socket) < 0 ||
        socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, output_socket) < 0)
    {
        closeSockets();
        error_code = 500;
        throw std::runtime_error("Socket pair creation failed");
    }
    struct sigaction sa;
    sa.sa_sigaction = &CGI::sigchldHandler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
    {
        closeSockets();
        error_code = 500;
        throw std::runtime_error("Failed to set up signal handler");
    }
}


void CGI::setMandatoryEnv()
{
    env_map["SERVER_SOFTWARE"] = "Webserv/1.0";
    env_map["SERVER_NAME"] = req_ptr->getHost();
    env_map["GATEWAY_INTERFACE"] = "CGI/1.1";
    env_map["SERVER_PROTOCOL"] = "HTTP/1.1";
    env_map["SERVER_PORT"] = req_ptr->getPort();
    env_map["REQUEST_METHOD"] = req_ptr->getMethod();
    env_map["PATH_INFO"] = res_ptr->getPathInfo();
    env_map["PATH_TRANSLATED"] = res_ptr->getPathTranslated();   
    env_map["SCRIPT_NAME"] = res_ptr->getScriptName();
    env_map["SCRIPT_FILENAME"] = scriptPath;
    env_map["QUERY_STRING"] = req_ptr->getQuery(); 
    env_map["REMOTE_ADDR"] = req_ptr->getClient()->getClientIP();
    env_map["REDIRECT_STATUS"] = "200";
    if (req_ptr->getHeaders().find("content-type") != req_ptr->getHeaders().end())
        env_map["CONTENT_TYPE"] = req_ptr->getHeaders()["content-type"];
    else
        env_map["CONTENT_TYPE"] = "plain/text"; 
    std::stringstream ss;
    ss << postBody.length();
    env_map["CONTENT_LENGTH"] = ss.str();
}

void CGI::mapToArray() 
{
    envp = new char*[env_map.size() + 1];
    
    size_t i = 0;
    std::string tmp;
    for (std::map<std::string, std::string>::iterator it = env_map.begin(); it != env_map.end(); it++)
    {
        tmp = it->first + "=" + it->second;
        envp[i] = new char[tmp.length() + 1];
        std::strcpy(envp[i], tmp.c_str());
        i++;
    }
    envp[i] = NULL;
}


void CGI::createCGIProcess() 
{  
    cgi_pid = fork();
    
    if (cgi_pid == -1)
    {
        error_code = 500;
        closeSockets();
        throw std::runtime_error("Fork failed");
    }
    else if (cgi_pid == 0)
    {  
        close(input_socket[1]); 
        input_socket[1] = -1;
        close(output_socket[0]);  
        output_socket[0] = -1;

        if (dup2(input_socket[0], STDIN_FILENO) < 0 || dup2(output_socket[1], STDOUT_FILENO) < 0) 
        {
            kill(getpid(), SIGTERM);
            return; 
        }

        close(input_socket[0]);
        input_socket[0] = -1;
        close(output_socket[1]);
        output_socket[1] = -1;
        execve(executorPath.c_str(), argv, envp);
        kill(getpid(), SIGTERM);
    }
    else
    {  
        start_time = time(NULL);   
        close(input_socket[0]);   
        input_socket[0] = -1;
        close(output_socket[1]);  
        output_socket[1] = -1;
        cgi_instances[cgi_pid] = this;

        res_ptr->getMultiplexer()->registerCGISocket(output_socket[0], res_ptr);
        res_ptr->getMultiplexer()->addFdToEpoll(output_socket[0], EPOLLIN);
        res_ptr->getMultiplexer()->registerCGISocket(input_socket[1], res_ptr);
        res_ptr->getMultiplexer()->addFdToEpoll(input_socket[1], EPOLLOUT);
    }
}


void CGI::sendPostBody()
{
    if (hasTimedOut())
    {
        timedOut = true;
        cleanChild();
        throw std::runtime_error("CGI timeout occured");
    }

    size_t remaining = postBody.length() - bytes_sent;
    if (remaining == 0) 
    {
        res_ptr->getMultiplexer()->removeFdFromEpoll(input_socket[1]);
        close(input_socket[1]);
        input_socket[1] = -1;
        return;
    }

    ssize_t sent = send(input_socket[1], postBody.c_str() + bytes_sent, std::min(remaining, size_t(16384)), 0);

    if (sent > 0) 
        bytes_sent += sent;
    else if (sent < 0) 
    {
        error_code = 500;
        cleanChild();
        throw std::runtime_error("Send failed to CGI process");
    }
}

void CGI::readAvailOutput() 
{
    if (hasTimedOut())
    {
        timedOut = true;
        cleanChild();
        throw std::runtime_error("CGI timeout occured");
    }

    char buffer[16384];
    received = recv(output_socket[0], buffer, sizeof(buffer), 0);
    if (received > 0) 
    {
        buffer[received] = '\0';
        output.append(buffer, static_cast<size_t>(received));
    }
    else if (received == 0) 
    {
        parser_state = COMPLETE;
        cleanChild();
    }
    else if (received < 0)
    {
        cleanChild();
        error_code = 500;
        throw std::runtime_error("Error reading from CGI");
    }    
}


void CGI::parseOutput() 
{
    size_t header_end = output.find("\r\n\r\n");
    if (header_end == std::string::npos) 
    {
        error_code = 502;
        throw std::runtime_error("Process completed without proper headers");
    }
    
    std::string headers = output.substr(0, header_end + 2);
    body = output.substr(header_end + 4);
    
    processHeaders(headers);
    
    if (body.length() > MAX_BODY_SIZE) 
    {
        error_code = 502;
        throw std::runtime_error("CGI response body too large");
    }
}

void CGI::processHeaders(const std::string& headers)
{
    if (headers.length() > MAX_HEADER_LENGTH) 
    {
        error_code = 502;
        throw std::runtime_error("CGI response headers too long");
    }

    size_t pos = 0;
    while (pos < headers.length()) 
    {
        size_t next_pos = headers.find("\r\n", pos);
        if (next_pos == std::string::npos) 
        {
            error_code = 502;
            throw std::runtime_error("Malformed header line");
        }

        validHeaderLine(headers.substr(pos, next_pos - pos));
        pos = next_pos + 2;
    }

    if (!status_found) 
        status = "200";
    if (!content_type_found) 
    {
        error_code = 502;
        throw std::runtime_error("Missing Content-Type header");
    }
}

void CGI::validHeaderLine(const std::string& header_line)
{
    if (header_line.empty()) 
    {
        error_code = 502;
        throw std::runtime_error("Empty header line");
    }

    size_t colon_pos = header_line.find(": ");
    if (colon_pos == std::string::npos || colon_pos == 0)
    {
        error_code = 502;
        throw std::runtime_error("Invalid header format");
    } 

    std::string header_name = header_line.substr(0, colon_pos);
    std::string header_value = header_line.substr(colon_pos + 2);
    if (header_name.empty() || header_value.empty()) 
    {
        error_code = 502;
        throw std::runtime_error("Empty header value");
    }
    std::string lower_header_name = header_name;
    std::transform(lower_header_name.begin(), lower_header_name.end(), lower_header_name.begin(), ::tolower);
    if (lower_header_name== "status") 
    {
        status = validStatusLine(header_value);
        status_found = true;
    }
    else if (lower_header_name== "content-type") 
    {
        content_type = header_value;
        content_type_found = true;
    }
}

std::string CGI::validStatusLine(const std::string& header_value)
{
    std::stringstream ss(header_value);
    std::string code, message, extra;
    
    if (!(ss >> code >> message) || (ss >> extra) || code.length() != 3)
    {
        error_code = 502;
        throw std::runtime_error("Malformed status line");
    }
       
    for (size_t i = 0; i < code.length(); ++i) 
    {
        if (!isdigit(code[i]))
        {
            error_code = 502;
            throw std::runtime_error("Malformed status line");        
        }
    }

    for (size_t i = 0; i < message.length(); ++i) 
    {
        if (!isalpha(message[i]))
        {
            error_code = 502;
            throw std::runtime_error("Malformed status line");
        }
    }

    return code;
}


bool CGI::isProcessRunning() const
{
    if (cgi_pid <= 0 || process_complete) 
        return false; 

    return (kill(cgi_pid, 0) == 0);
}

bool CGI::hasTimedOut() const
{
    if (start_time == 0)
        return false;
        
    time_t current_time = time(NULL);
    return (current_time - start_time) > CGI_TIMEOUT;
}


void CGI::cleanChild()
{
    closeSockets();

    if (cgi_pid > 0 && isProcessRunning())
    {
        kill(cgi_pid, SIGTERM);

        int proc_status;
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        select(0, NULL, NULL, NULL, &tv);
        
        if (waitpid(cgi_pid, &proc_status, WNOHANG) == 0)
        {
            kill(cgi_pid, SIGKILL);
            waitpid(cgi_pid, &proc_status, 0);
        }
        if (cgi_instances.find(cgi_pid) != cgi_instances.end()) 
            cgi_instances.erase(cgi_pid);
    }
}

void CGI::closeSockets()
{
    if (output_socket[0] != -1) 
    {
        try 
        {
            res_ptr->getMultiplexer()->removeFdFromEpoll(output_socket[0]);
            res_ptr->getMultiplexer()->deregisterCGISocket(output_socket[0]);

        } 
        catch (...) 
        {
        }
        close(output_socket[0]);
        output_socket[0] = -1;
    }
    if (input_socket[1] != -1) 
    {
        try 
        {
            res_ptr->getMultiplexer()->removeFdFromEpoll(input_socket[1]);
            res_ptr->getMultiplexer()->deregisterCGISocket(input_socket[1]);

        } 
        catch (...) 
        {
        }
        close(input_socket[1]);
        input_socket[1] = -1;
    }
}
