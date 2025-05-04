#ifndef CLIENTSOCKET_HPP
#define CLIENTSOCKET_HPP

#include <arpa/inet.h>
#include <string>
#include <sstream>
#include <ctime>
#include "Server.hpp"
#include "../errors/Errors.hpp"

class Server;
class Response;
class Request;
class Multiplexer;

class ClientSocket
{
    private:
        int client_fd;
        int listen_fd;
        struct sockaddr_in client_addr;
        Server  *sp;
        Request *req;   
        Response *res; 
        Multiplexer *mp;
        time_t last_activity_time;        
        time_t last_write_time;

    public:
        ClientSocket(int client_fd, int listen_fd, const struct sockaddr_in& addr, Server *serv_ptr,  Multiplexer *multi_ptr);
        ~ClientSocket();

        void ReadData();
        void WriteData();

        int getListenFd() const;
        Request* getRequest() const;
        Response* getResponse() const;
        const struct sockaddr_in& getClientAddr() const;
        void setServer(Server *server);
    
        std::string getClientIP() const;        
        int getClientPort() const;
        bool hasKeepAliveTimedOut() const;
        bool hasSendTimedOut() const;

};
#endif