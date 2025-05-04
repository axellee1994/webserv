#include "../../includes/server/SocketUtils.hpp"

SocketUtils::SocketUtils()
{
}

SocketUtils::SocketUtils(const SocketUtils &src)
{
	(void)src;
}

SocketUtils &SocketUtils::operator=(const SocketUtils &rhs)
{
	(void)rhs;
	return (*this);
}


SocketUtils::~SocketUtils()
{
}

int SocketUtils::initListeningSocket(const char *host, const char *port, int backlog)
{
	struct addrinfo	*addressList;
	int				socketFD;
	struct addrinfo	hints;

	socketFD = -1;
	try
	{
		hints = populateHints(host);
		addressList = resolveAddress(host, port, hints);
		
		socketFD = createBind(addressList);
		startListening(socketFD, backlog);
	}
	catch (const std::exception& e)
	{
		if (addressList)
			freeaddrinfo(addressList);
		throw;
	}
	freeaddrinfo(addressList);
	return (socketFD);
}

struct addrinfo SocketUtils::populateHints(const char *host)
{
	struct addrinfo	hints;

	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if (host != NULL && std::strlen(host) > 0)
		hints.ai_flags = 0;
	else
		hints.ai_flags = AI_PASSIVE;
	return (hints);
}

struct addrinfo *SocketUtils::resolveAddress(const char *host, const char *port,
	const struct addrinfo &hints)
{
	struct addrinfo	*res;
	int				status;

	res = NULL;
	status = getaddrinfo(host, port, &hints, &res);
	if (status != 0)
		throw std::runtime_error("getaddrinfo failed");
	return (res);
}


int SocketUtils::createBind(struct addrinfo *addressList)
{
	int				sockfd;
	struct addrinfo	*addr;

	sockfd = -1;
	for (addr = addressList; addr != NULL; addr = addr->ai_next)
	{
        sockfd = socket(addr->ai_family, addr->ai_socktype | SOCK_NONBLOCK, addr->ai_protocol);
		if (sockfd < 0)
			continue ;
		
		try
		{
			setSocketOptions(sockfd);
			if (bind(sockfd, addr->ai_addr, addr->ai_addrlen) == 0)
                return sockfd;
			closeSocket(sockfd);
		}
		catch (const std::exception& e) 
        {
            closeSocket(sockfd);
			continue;
        }
	}
	throw std::runtime_error("Failed to create socket");
}

void SocketUtils::setSocketOptions(int sockfd)
{
	int	opt;

	opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("Failed to set socket options");
    
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("Failed to set TCP_NODELAY option");
    
    int rcvbuf = 262144;
    int sndbuf = 262144;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0)
        throw std::runtime_error("Failed to set SO_RCVBUF option");
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0)
        throw std::runtime_error("Failed to set SO_SNDBUF option");
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("Failed to set SO_KEEPALIVE option");
}

void SocketUtils::startListening(int sockfd, int backlog)
{
	if (listen(sockfd, backlog) < 0)
		throw std::runtime_error("Failed to listen on socket");
}

void SocketUtils::closeSocket(int &sockfd)
{
	if (sockfd != -1)
	{
		close(sockfd);
		sockfd = -1;
	}
}