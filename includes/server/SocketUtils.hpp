#ifndef SOCKETUTILS_HPP
#define SOCKETUTILS_HPP

#include <iostream>
#include <cstring>
#include <cerrno>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/tcp.h>

class SocketUtils
{
    protected:
        int initListeningSocket(const char* host, const char* port, int backlog = 128);
        void closeSocket(int& sockfd);
        SocketUtils();
        ~SocketUtils();

    private:
        SocketUtils(const SocketUtils &src);
        SocketUtils &operator=(const SocketUtils &rhs);

        struct addrinfo populateHints(const char* host);
        addrinfo* resolveAddress(const char* host, const char* port, const struct addrinfo& hints);
        int createBind(struct addrinfo* addressList);
        void setSocketOptions(int sockfd);
        void startListening(int sockfd, int backlog);
};

#endif