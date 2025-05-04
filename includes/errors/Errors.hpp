#ifndef ERRORS_HPP
# define ERRORS_HPP

#include <iostream>
#include <string>
#include <exception>

class ServerException : public std::exception
{
    private:
        std::string m_message;

    public:
        explicit ServerException(const std::string &message) : m_message(message) {}
        virtual ~ServerException() throw() {}
        virtual const char* what() const throw() { return m_message.c_str(); }
};

class DisconnectedException : public std::exception
{
    private:
        std::string m_message;

    public:
        explicit DisconnectedException(const std::string &message) : m_message(message) {}
        virtual ~DisconnectedException() throw() {}
        virtual const char* what() const throw() { return m_message.c_str(); }
};

void log(const std::string &message);

#endif