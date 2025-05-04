#include "../../includes/server/Multiplexer.hpp"
#include "../../includes/server/Server.hpp"
#include "../../includes/server/ClientSocket.hpp"
#include "../../includes/response/Response.hpp"

Multiplexer::Multiplexer(std::vector<ServerConfig>& configsRef) : configs(configsRef) 
{
    epoll_fd = epoll_create(1);
    initServers();
    monitorEvents();
}

Multiplexer::~Multiplexer()
{
    for (std::map<std::string, int>::iterator it = ipport_listen.begin(); it != ipport_listen.end(); ++it) 
    {
        if (it->second >= 0) 
        {
            close(it->second);
        }
    }
    ipport_listen.clear();
    for (std::map<int, Response*>::iterator it = cgi_sockets.begin(); it != cgi_sockets.end(); ++it)
    {
        if (it->first >= 0)
        {
            close(it->first);
        }
    }
    cgi_sockets.clear();
    
    for (std::map<int, ClientSocket*>::iterator client_it = unassigned_clients.begin(); 
         client_it != unassigned_clients.end(); ++client_it) 
    {
        if (client_it->first >= 0)
        {
            close(client_it->first);
        }
        delete client_it->second;
    }
    unassigned_clients.clear();
    
    std::set<Server*> deleted_servers;
    for (std::map<int, std::vector<Server*> >::iterator it = listen_servers.begin(); it != listen_servers.end(); ++it) 
    {
        std::vector<Server*>& servers = it->second;
        for (std::vector<Server*>::iterator server_it = servers.begin(); 
             server_it != servers.end(); ++server_it) 
        {
            if (deleted_servers.find(*server_it) == deleted_servers.end()) 
            {
                deleted_servers.insert(*server_it);
                delete *server_it;
            }
        }
        servers.clear();
    }
    listen_servers.clear();
    deleted_servers.clear();
    
    client_to_server.clear();

    if (epoll_fd != -1)
    {
        close(epoll_fd);
        epoll_fd = -1;
    }
}

void Multiplexer::initServers() 
{
    for (std::vector<ServerConfig>::iterator it = configs.begin(); it != configs.end(); ++it) 
    {
        Server* server = new Server(this, &(*it));
        
        for (std::map<std::string, std::vector<int> >::iterator ip_it = it->ip_port.begin(); ip_it != it->ip_port.end(); ++ip_it) 
        {
            const std::string &ip = ip_it->first;
            for (std::vector<int>::iterator port_it = ip_it->second.begin(); port_it != ip_it->second.end(); ++port_it) 
            {
        
                int port = *port_it;
                std::string port_str = toString(port);
                std::string endpoint = ip + ":" + port_str;
                int listen_fd;
                
                if (ipport_listen.find(endpoint) == ipport_listen.end()) 
                {
                    listen_fd = initListeningSocket(ip.c_str(), port_str.c_str());
                    ipport_listen[endpoint] = listen_fd;
                    addFdToEpoll(listen_fd, EPOLLIN);
                    std::cout << "Listening on host: " << ip << " and port: " << port << std::endl; 
                } 
                else 
                    listen_fd = ipport_listen[endpoint];

                server->addListenFd(listen_fd); 
                listen_servers[listen_fd].push_back(server);
            }
        }
    }
}

void Multiplexer::monitorEvents()
{
    epoll_event events[1024];
    int timeout = 100;

    int num_events = epoll_wait(epoll_fd, events, 1024, timeout);
    
    if (num_events > 0)
    {
        for (int i = 0; i < num_events; ++i)
            dispatchEvent(events[i]);
    }
    checkTimeouts();
}


void Multiplexer::checkTimeouts()
{
    for (std::map<int, ClientSocket*>::iterator it = unassigned_clients.begin(); it != unassigned_clients.end();)
    {
        int client_fd = it->first;
        ClientSocket* client = it->second;
        
        std::map<int, ClientSocket*>::iterator current = it++;
        (void)current;

        if (client->getRequest()->hasTimedOut() && !client->getRequest()->getTimeOutDetected())
        {
            client->getRequest()->setTimeOutDetected();
            client->getRequest()->setKeepAlive(false);
            client->getRequest()->setStateAndStatus(SPECIAL_ERR, 408);
            modifyEpollEvent(client_fd, EPOLLOUT);
        }
        else if(client->hasKeepAliveTimedOut() || client->hasSendTimedOut())
            removeUnassignedClient(client, client_fd);
    }
    
    for (std::map<int, Server*>::iterator it = client_to_server.begin(); it != client_to_server.end();)
    {
        int client_fd = it->first;
        Server* server = it->second;
        
        std::map<int, Server*>::iterator current = it++;
        (void)current;

        ClientSocket* client = server->getClient(client_fd);

        if (!client)
            continue;
        
        if (client && client->getRequest()->hasTimedOut() && !client->getRequest()->getTimeOutDetected())
        {
            client->getRequest()->setTimeOutDetected();
            client->getRequest()->setKeepAlive(false);
            client->getRequest()->setStateAndStatus(REQ_DONE, 408);
            modifyEpollEvent(client_fd, EPOLLOUT);
        }
        else if(client->hasKeepAliveTimedOut() || client->hasSendTimedOut())
        {
            deregisterClient(client_fd);
            server->removeClient(client_fd);
        }
    }
}


void Multiplexer::dispatchEvent(epoll_event& event)
{
    if (event.events & EPOLLIN)
    {
        if (listen_servers.find(event.data.fd) != listen_servers.end())
            acceptClient(event.data.fd);
        else if (cgi_sockets.find(event.data.fd) != cgi_sockets.end())
        {
            std::map<int, Response*>::iterator cgi_it = cgi_sockets.find(event.data.fd);
            if (cgi_it != cgi_sockets.end() && cgi_it->second) 
                cgi_it->second->handleCGIRead();     
            else
                deregisterCGISocket(event.data.fd);
        }
        else if (unassigned_clients.find(event.data.fd) != unassigned_clients.end())
            assignClient(event);
        else if (client_to_server.find(event.data.fd) != client_to_server.end())
            client_to_server.find(event.data.fd)->second->handleClient(event);
        else
            removeFdFromEpoll(event.data.fd);
    }
    else if (event.events & EPOLLOUT)
    {
        if (cgi_sockets.find(event.data.fd) != cgi_sockets.end()) 
        {
            std::map<int, Response*>::iterator cgi_it = cgi_sockets.find(event.data.fd);
            if (cgi_it != cgi_sockets.end() && cgi_it->second) 
                cgi_it->second->handleCGIWrite();
            else 
                deregisterCGISocket(event.data.fd);
        }
        else if (unassigned_clients.find(event.data.fd) != unassigned_clients.end())
            unassigned_clients[event.data.fd]->WriteData();
        else if (client_to_server.find(event.data.fd) != client_to_server.end())
            client_to_server.find(event.data.fd)->second->handleClient(event);
        else
            removeFdFromEpoll(event.data.fd);
    }  
}

void Multiplexer::addFdToEpoll(int fd, uint32_t events)
{
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) >= 0)
    {
        std::cout << "Added to epoll: " << fd << std::endl;
    }
}

void Multiplexer::removeFdFromEpoll(int fd)
{
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) >= 0)
    {
        std::cout << "Removed from epoll: " << fd << std::endl;
    }
}

void Multiplexer::modifyEpollEvent(int fd, uint32_t new_events)
{
    struct epoll_event event;
    event.events = new_events;
    event.data.fd = fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) >= 0)
    {
        std::cout << "Modified epoll: " << fd << std::endl;
    }   
}

void Multiplexer::registerClient(int client_fd, Server *server) 
{
    client_to_server[client_fd] = server;
}

void Multiplexer::deregisterClient(int client_fd)
{
   client_to_server.erase(client_fd);
}



void Multiplexer::acceptClient(int listen_fd)
{
    struct sockaddr_in client_address;
    socklen_t addr_len = sizeof(client_address);
    
    for (int i = 0; i < 16; i++)
    {
        char* p = (char*)&client_address;
        for (size_t j = 0; j < sizeof(client_address); ++j)
            p[j] = 0;

        int client_fd = accept(listen_fd, (struct sockaddr *)&client_address, &addr_len);
        
        if (client_fd == -1)
            break;

        char client_ip[16];
        uint32_t ip = ntohl(client_address.sin_addr.s_addr);
        sprintf(client_ip, "%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
        
        std::cout << "Accepted new connection from " << client_ip << ":" 
                << ntohs(client_address.sin_port) << "." << std::endl;

        ClientSocket* new_client = new ClientSocket(client_fd, listen_fd, client_address, NULL, this);
        unassigned_clients[client_fd] = new_client;
        addFdToEpoll(client_fd, EPOLLIN);
    }
}


void Multiplexer::assignClient(epoll_event& event) 
{
    int client_fd = event.data.fd;
    std::map<int, ClientSocket*>::iterator client_it = unassigned_clients.find(client_fd);
    ClientSocket *client = client_it->second;

    try 
    {
        if (event.events & EPOLLIN)
        {
            client->ReadData();  
            if (client->getRequest()->getState() == REQ_DONE) 
            {
                int listen_fd = client->getListenFd();
                std::string host = client->getRequest()->getHost(); 
                std::vector<Server*>& servers = listen_servers.find(listen_fd)->second;
                Server *matched_server = NULL;

                for (std::vector<Server*>::iterator it = servers.begin(); it != servers.end(); ++it) 
                {
                    std::vector<std::string>& domains = (*it)->getConfig()->domains;        
                    for (std::vector<std::string>::iterator name_it = domains.begin(); name_it != domains.end(); ++name_it) 
                    {
                        if (*name_it == host) 
                        {
                            matched_server = *it;
                            break;
                        }
                    }
                    if (matched_server != NULL)
                        break;  
                }
                if (matched_server == NULL) 
                    matched_server = servers[0];
                client->setServer(matched_server);
                client->getResponse()->setServer(matched_server->getConfig());
                unassigned_clients.erase(client_it);
                registerClient(client_fd, matched_server);
                matched_server->addClient(client_fd, client);
                modifyEpollEvent(client_fd, EPOLLOUT);
            }
        }
        if (event.events & EPOLLOUT)  
        {
            client->WriteData();
        }
    }
    catch (const DisconnectedException& e) 
    {
        removeUnassignedClient(client, client_fd);
        return;
    }
    catch (const std::exception& e) 
    {
        client->getRequest()->setStateAndStatus(SPECIAL_ERR, 500);
        modifyEpollEvent(client_fd, EPOLLOUT);
    }
}


std::string Multiplexer::toString(size_t num) const
{
    std::ostringstream ss;
    ss << num;
    return ss.str();
}

void Multiplexer::registerCGISocket(int fd, Response *res_ptr)
{
    cgi_sockets[fd] = res_ptr;
}

void Multiplexer::deregisterCGISocket(int fd)
{
    std::map<int, Response*>::iterator it = cgi_sockets.find(fd);
    if (it != cgi_sockets.end()) 
    {
        removeFdFromEpoll(fd);
        cgi_sockets.erase(it);
        close(fd);
    }
}


void Multiplexer::removeUnassignedClient(ClientSocket *client, int client_fd)
{
    delete client;
    std::map<int, ClientSocket*>::iterator client_it = unassigned_clients.find(client_fd);
    unassigned_clients.erase(client_it);
    removeFdFromEpoll(client_fd);
    close(client_fd);
}