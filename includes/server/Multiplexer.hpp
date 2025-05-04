#ifndef MULTIPLEXER_HPP
#define MULTIPLEXER_HPP

#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <set>
#include "SocketUtils.hpp"
#include "../config/ConfigParser.hpp"
#include "../errors/Errors.hpp"

class Server;
class ClientSocket;
class Response;

class Multiplexer : protected SocketUtils
{
    private:
        int epoll_fd;
        std::vector<ServerConfig>& configs;
        std::map<std::string, int> ipport_listen;
        std::map<int, std::vector<Server*> > listen_servers;
        std::map<int, ClientSocket*> unassigned_clients;
        std::map<int, Server*> client_to_server; 
        std::map<int, Response*> cgi_sockets;

        std::string toString(size_t num) const;
        void checkTimeouts();

    public:
        Multiplexer(std::vector<ServerConfig>& configsRef);
        ~Multiplexer();

        void initServers();
        void monitorEvents();
        void dispatchEvent(epoll_event& event);
        void addFdToEpoll(int fd, uint32_t events);
        void removeFdFromEpoll(int fd);
        void modifyEpollEvent(int fd, uint32_t new_events);
        void registerClient(int client_fd, Server *server);
        void deregisterClient(int client_fd);
        void acceptClient(int listen_fd);
        void assignClient(epoll_event& event);
        void registerCGISocket(int fd, Response *res_ptr);
        void deregisterCGISocket(int fd);
        void removeUnassignedClient(ClientSocket *client, int client_fd);
};

#endif 
