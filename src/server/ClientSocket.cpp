#include "../../includes/server/ClientSocket.hpp"
#include "../../includes/server/Server.hpp"
#include "../../includes/server/Multiplexer.hpp"
#include "../../includes/request/Request.hpp"
#include "../../includes/response/Response.hpp"

ClientSocket::ClientSocket(int client_fd, int listen_fd, const struct sockaddr_in& addr, Server *serv_ptr, Multiplexer *multi_ptr) : 
    client_fd(client_fd), listen_fd(listen_fd), client_addr(addr), sp(serv_ptr), mp(multi_ptr), last_activity_time(time(NULL)), last_write_time(0)
{
    req = new Request(client_fd, this);

    if (sp)
    {
        res= new Response(client_fd, req, sp->getConfig(), mp, this);
        if(sp->getConfig())
            req->setClientMaxBodySize(sp->getConfig()->client_max_body_size);
    }
    else
        res= new Response(client_fd, req, NULL, mp, this);
}


ClientSocket::~ClientSocket() 
{
   std::cout << "Client Socket destructor called\n";
   delete req;
   delete res;
}

int ClientSocket::getListenFd() const 
{
   return listen_fd;
}

Request* ClientSocket::getRequest() const
{
   return req;
}

Response* ClientSocket::getResponse() const
{
   return res;
}

void ClientSocket::setServer(Server *server)
{
    sp = server;
}

void ClientSocket::ReadData()
{
    std::vector<unsigned char> buffer(65536);
    ssize_t bytes_read = recv(client_fd, buffer.data(), buffer.size(), MSG_DONTWAIT);
    if (bytes_read > 0) 
    {
        buffer.resize(bytes_read);
        if (req->getState() == NO_REQ)
        {
            req->setState(PROC_REQ_LINE);
            req->resetTimer();
        }
      
        req->parseRequest(buffer);
        if (req->getState() == REQ_DONE)
        {
            std::cout << "Request was complete. Status code: " << req->getStatusCode() << std::endl;
            if (req->getStatusCode() == 400)
                req->setKeepAlive(false);
            if (sp)
                sp->mp->modifyEpollEvent(client_fd, EPOLLOUT);
        }
    } 
    else if (bytes_read == 0) 
        throw DisconnectedException("Client disconnected");
    else 
    {
        req->setKeepAlive(false);
        req->setStateAndStatus(REQ_DONE, 500);
    }
}

void ClientSocket::WriteData()
{
    if (res->getState() == NO_RES || res->getState() == SPECIAL_ERR)
        res->init();
    if (res->getState() != RES_DONE)
        return;

    std::string response = res->getResponse();
    int old_sent = res->getBytesSent();
    ssize_t bytes_sent = send(client_fd, response.data() + old_sent, response.length() - old_sent, MSG_NOSIGNAL);
    if (bytes_sent < 0) 
        throw std::runtime_error("Failed to send response");
    
    last_write_time = time(NULL);
    res->setBytesSent(bytes_sent);

    if (res->getBytesSent() < response.length())
        return;
    if (req->shouldKeepAlive()) 
    {
        last_activity_time = time(NULL);
        res->reset();
        req->reset();
        sp->mp->modifyEpollEvent(client_fd, EPOLLIN);
    }
    else
    {
        if (sp)
            sp->removeClient(client_fd);
        else
            mp->removeUnassignedClient(this, client_fd);
    }
}

std::string ClientSocket::getClientIP() const
{
    uint32_t ip = ntohl(client_addr.sin_addr.s_addr);
    std::ostringstream oss;
    oss << ((ip >> 24) & 0xFF) << '.'
        << ((ip >> 16) & 0xFF) << '.'
        << ((ip >> 8) & 0xFF) << '.'
        << (ip & 0xFF);
    return oss.str();
}

int ClientSocket::getClientPort() const
{
    return ntohs(client_addr.sin_port);
}

bool ClientSocket::hasKeepAliveTimedOut() const
{
    if (!req)
        return false;
    if (req->getState() == NO_REQ && !req->isFirstReq()) 
    {
        if (time(NULL) - last_activity_time > KEEP_ALIVE_TIMEOUT)
        {
            std::cout << "keep alive timeout\n";
            return true;
        }
    }
    return false;
}

bool ClientSocket::hasSendTimedOut() const
{
    if (!res)
        return false;
    if (res->getState() == RES_DONE && last_write_time != 0)
    {
        if (time(NULL) - last_activity_time > SEND_TIMEOUT)
        {
            return true;
        }
    }     
    return false;     
}