#include "../../includes/server/Server.hpp"
#include "../../includes/server/ClientSocket.hpp"
#include "../../includes/server/Multiplexer.hpp"


Server::Server(Multiplexer *multiplexer_ptr, ServerConfig* conf) : config(conf), mp(multiplexer_ptr)   
{
}

Server::~Server()
{
    std::map<int, ClientSocket*>::iterator it = clients.begin();
    while (it != clients.end())
    {
        int fd = it->first;
        mp->removeFdFromEpoll(fd);
        mp->deregisterClient(fd);
        delete it->second;
        close(fd);
        ++it;
    }
    clients.clear();
    for (std::vector<int>::iterator fd_it = listen_fds.begin(); fd_it != listen_fds.end(); ++fd_it)
    {
        if (*fd_it >= 0)
        {
            close(*fd_it);        
            *fd_it = -1;          
        }
    }
    listen_fds.clear();     
}


void Server::addListenFd(int fd)
{
    listen_fds.push_back(fd);
}

void Server::addClient(int client_fd, ClientSocket *client)
{
    clients[client_fd] = client;
}

ServerConfig* Server::getConfig() const 
{ 
    return config; 
}

ClientSocket* Server::getClient(int client_fd) const
{
    std::map<int, ClientSocket*>::const_iterator it = clients.find(client_fd);
    if (it != clients.end())
        return it->second;
    return NULL; 
}

void Server::handleClient(epoll_event& event) 
{
    if (clients.find(event.data.fd) == clients.end()) 
    {
        std::cerr << "Warning: Client not found for fd: " << event.data.fd << std::endl;
        return;
    }
    if (event.events & EPOLLIN)
    {
        try
        {
            clients[event.data.fd]->ReadData();
        }
        catch (const DisconnectedException &e)
        {
            std::cout << e.what() << std::endl;
            removeClient(event.data.fd);
        }
    }
    if (event.events & EPOLLOUT)
    {
        try
        {
            clients[event.data.fd]->WriteData();
        }
        catch(const std::exception& e)
        {
            std::cout << e.what() << std::endl;
            removeClient(event.data.fd);
        }
    }
}

void Server::removeClient(int client_fd) 
{
    mp->removeFdFromEpoll(client_fd);
    mp->deregisterClient(client_fd);
    close(client_fd);
    delete clients[client_fd];
    clients.erase(client_fd);
}