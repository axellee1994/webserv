#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "../config/ConfigParser.hpp"
#include "SocketUtils.hpp"


class ClientSocket;
class Multiplexer;

class Server : protected SocketUtils
{
    private:
        std::vector<int> listen_fds;
        std::map<int, ClientSocket*> clients;
        ServerConfig* config;

    public:
        Multiplexer *mp;
        Server(Multiplexer *multiplexer_ptr, ServerConfig* conf);
        ~Server();
        
        void addListenFd(int fd);
        void addClient(int client_fd, ClientSocket *client);
        ServerConfig* getConfig() const;
        ClientSocket* getClient(int client_fd) const;
        void removeClient(int client_fd);  
        void handleClient(epoll_event& event);
};

#endif
