#include "../../includes/config/ConfigParser.hpp"

void    ConfigParser::checkDuplicates(ServerConfig& server)
{
    for (std::vector<int>::iterator it1 = server.ports.begin(); it1 != server.ports.end(); ++it1)
    {
        for (std::vector<int>::iterator it2 = it1 + 1; it2 != server.ports.end(); ++it2)
        {
            if (*it1 == *it2)
                throw std::runtime_error("duplicate value found in ports!");
        }
    }

    for (std::vector<std::string>::iterator it1 = server.domains.begin(); it1 != server.domains.end(); ++it1)
    {
        for (std::vector<std::string>::iterator it2 = it1 + 1; it2 != server.domains.end(); ++it2)
        {
            if (*it1 == *it2)
                throw std::runtime_error("duplicate value found in domains!");
        }
    }

    for (std::vector<std::string>::iterator it1 = server.index.begin(); it1 != server.index.end(); ++it1)
    {
        for (std::vector<std::string>::iterator it2 = it1 + 1; it2 != server.index.end(); ++it2)
        {
            if (*it1 == *it2)
                throw std::runtime_error("duplicate value found in index!");
        }
    }

    for (std::vector<std::string>::iterator it1 = server.limit_except.begin(); it1 != server.limit_except.end(); ++it1)
    {
        for (std::vector<std::string>::iterator it2 = it1 + 1; it2 != server.limit_except.end(); ++it2)
        {
            if (*it1 == *it2)
                throw std::runtime_error("duplicate value found in limit_except!");
        }
    }
}