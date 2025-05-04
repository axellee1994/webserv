#include "../../includes/config/ConfigParser.hpp"
#include "../../includes/constants.hpp"

std::string ConfigParser::removeConsecutiveSlashes(const std::string &path) 
{
    std::string cleanPath;

    for (size_t i = 0; i < path.length(); ++i) 
    {
        if (path[i] == '/' && i > 0 && path[i - 1] == '/') 
            continue;  
        else 
            cleanPath += path[i]; 
    }
    return cleanPath;
}

std::string ConfigParser::transformRoot(const std::string &root) 
{
    char cwd[1024];
    std::string res;

    if (root[0] != '/') 
    {
        if (getcwd(cwd, sizeof(cwd)) == NULL) 
            throw std::runtime_error("Failed to get current working directory");
        res = std::string(cwd) + "/" + root;
    } 
    else 
        res = root;  
    
    if (!res.empty() && res[res.size() - 1] != '/') 
        res += '/';

    return removeConsecutiveSlashes(res);
}

int     ConfigParser::countWords(std::string& lineValue)
{
    std::istringstream ss(lineValue);
    std::string word;
    int wordCount = 0;

    while (ss >> word)
    {
        wordCount++;
    }
    return (wordCount);
}

int    ConfigParser::isValidPortNumber(int port)
{
    if (port < 1 || port > 65535)
        return 0;
    else
        return 1;
}

int     ConfigParser::hasAlpha(std::string& s)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (isalpha(s[i]))
            return 1;
        else
            continue ;
    }
    return 0;
}

int     ConfigParser::isValidIP(std::string& s)
{
    size_t colonPos = s.find(':');

    std::string ipPart = s.substr(0, colonPos);
    std::string portPart = s.substr(colonPos + 1);

    int port = atoi(portPart.c_str());
    if (!isValidPortNumber(port))
        return 0;

    std::string octet;
    int octetCount = 0;
    std::stringstream ipStream;
    ipStream << ipPart;
    int octets[4] = { 0 };
    
    while (std::getline(ipStream, octet, '.'))
    {
        for (size_t i = 0; i < octet.size(); ++i)
        {
            if (!isdigit(octet[i]))
                return 0;
        }
        int num = atoi(octet.c_str());
        if (num < 0 || num > 255)
            return 0;
        octets[octetCount++] = num;
    }

    if (octetCount != 4)
        return 0;

    if ((octets[0] == 0 && octets[1] == 0 && octets[2] == 0 && octets[3] == 0)
        || (octets[0] == 127))
    {
        return 1;
    }
    else
        throw std::runtime_error("Valid ranges: 0.0.0.0 or (127.0.0.0 to 127.255.255.255)");
    return 1;
}

int     ConfigParser::isValidIP_alone(std::string& s)
{
    std::string octet;
    int octetCount = 0;
    std::stringstream ipStream;
    ipStream << s;
    int octets[4] = { 0 };
    
    while (std::getline(ipStream, octet, '.'))
    {
        for (size_t i = 0; i < octet.size(); ++i)
        {
            if (!isdigit(octet[i]))
                return 0;
        }
        int num = atoi(octet.c_str());
        if (num < 0 || num > 255)
            return 0;
        octets[octetCount++] = num;
    }

    if (octetCount != 4)
        return 0;

    if ((octets[0] == 0 && octets[1] == 0 && octets[2] == 0 && octets[3] == 0)
        || (octets[0] == 127))
    {
        return 1;
    }
    else
        throw std::runtime_error("Valid ranges: 0.0.0.0 or (127.0.0.0 to 127.255.255.255)");
    return 1;
}

void     ConfigParser::addValidIP(std::string& s, size_t& in_config)
{
    size_t colonPos = s.find(':');

    std::string ipPart = s.substr(0, colonPos);
    std::string portPart = s.substr(colonPos + 1);

    if (hasAlpha(portPart))
        throw std::runtime_error("port cannot contain alphabets!");
    int port = atoi(portPart.c_str());

    std::map<std::string, std::vector<int> >::iterator it = this->configs[in_config].ip_port.find(ipPart);

    if (it != this->configs[in_config].ip_port.end())
    {
        std::vector<int>& ipPorts = it->second;
        if (isDuplicateElement_Int(ipPorts, port))
        {
            throw std::runtime_error("duplicate port number detected!");
        }
        else
        {
            if (isValidPortNumber(port) == 1)
                ipPorts.push_back(port);
        }
    }
    else
    {
        std::vector<int> newPortVector;
        if (isValidPortNumber(port) == 1)
            newPortVector.push_back(port);
        this->configs[in_config].ip_port[ipPart] = newPortVector;
    }
}

size_t  ConfigParser::convertToBytes(std::string v)
{
    std::string alpha;

    if (isalpha(v[v.size() - 1]))
    {
        alpha = v.substr(v.size() - 1);
        v = v.substr(0, v.size() - 1);

        std::stringstream ss(v);
        unsigned long value;
        ss >> value;
        if (ss.fail() || !ss.eof())
        {
            throw std::runtime_error("invalid value, conversion failed.");
        }
        if (alpha[0] == 'k' || alpha[0] == 'K')
            value = value * 1024;
        else if (alpha[0] == 'm' || alpha[0] == 'M')
            value = value * 1048576;
        else if (alpha[0] == 'g' || alpha[0] == 'G')
            value = value * 1073741824;
        if (value > MAX_BODY_SIZE)
            throw std::runtime_error("client_max_body_size set too high!");
        return value;
    }
    std::stringstream ss(v);
    unsigned long value;
    ss >> value;
    if (ss.fail() || !ss.eof())
    {
        std::cerr << "Invalid input: conversion failed." << std::endl;
    }
    return value;
}

void    ConfigParser::wipeVector(std::vector<std::string>& index)
{
    index.clear();
    std::vector<std::string>().swap(index);
}

void    ConfigParser::chkExtension(std::string& str)
{
    size_t dotPos = str.find('.');

    if (dotPos != std::string::npos && dotPos != str.size() - 1)
    {
        std::string extension = str.substr(dotPos);

        if (extension == ".html" || extension == ".htm" || extension == ".php")
            return ;
        else
            throw std::runtime_error("Invalid extension! Only .html, .htm, .php are accepted.");
    }
    else
        throw std::runtime_error("Index parameters require an extension! .html, .htm, .php!");
}

void    ConfigParser::toUpperCase(std::string& str)
{
    for (size_t i = 0; i < str.size(); ++i)
        str[i] = std::toupper(static_cast<unsigned char>(str[i]));
}

int     ConfigParser::isDuplicateElement_Int(std::vector<int>& container, int nbr)
{
    for (std::vector<int>::iterator it = container.begin(); it != container.end(); ++it)
    {
        if (*it == nbr)
            return 1;
    }
    return 0;
}

int     ConfigParser::isDuplicateElement_Str(std::vector<std::string>& container, std::string str)
{
    for (std::vector<std::string>::iterator it = container.begin(); it != container.end(); ++it)
    {
        if (*it == str)
            return 1;
    }
    return 0;
}

void    ConfigParser::generateIpPortDomain(ServerConfig& server)
{
    std::map<std::string, std::vector<int> >::iterator mapIt;
    for (mapIt = server.ip_port.begin(); mapIt != server.ip_port.end(); ++mapIt) 
    {
        const std::string& key = mapIt->first;
        const std::vector<int>& values = mapIt->second;
        for (std::vector<int>::const_iterator vecIt = values.begin(); vecIt != values.end(); ++vecIt) 
        {
            std::stringstream ss;
            ss << key << ":" << *vecIt;
            server.ipPortVec.push_back(ss.str());
        }
    }
    for (std::vector<std::string>::iterator it1 = server.ipPortVec.begin(); it1 != server.ipPortVec.end(); ++it1) 
    {
        for (std::vector<std::string>::iterator it2 = server.domains.begin(); it2 != server.domains.end(); ++it2) 
        {
            std::stringstream ss;
            ss << *it1 << ":" << *it2;
            server.ipPortDomain.push_back(ss.str());
        }
    }
}

void    ConfigParser::checkDuplicateIpPortDomains()
{
    std::string check;
    int duplicateCount;

    for (size_t i = 0; i < this->configs.size(); ++i)
    {
        for (size_t j = 0; j < this->configs[i].ipPortDomain.size(); ++j)
        {
            check = this->configs[i].ipPortDomain[j];
            std::vector<std::string>& ptr_toVec = this->configs[i].ipPortDomain;
            duplicateCount = 0;
            for (size_t k = 0; k < this->configs.size(); ++k)
            {
                for (size_t l = 0; l < this->configs[k].ipPortDomain.size(); ++l)
                {
                    if (check == this->configs[k].ipPortDomain[l])
                    {
                        duplicateCount++;
                        if (duplicateCount > 1)
                        {
                            std::vector<std::string>::iterator it = std::find(ptr_toVec.begin(), ptr_toVec.end(), check);
                            if (it != ptr_toVec.end())
                            {
                                ptr_toVec.erase(it);
                            }
                            break;
                        }
                    }
                    else
                        continue ;
                }
            }
        }
    }
}

size_t  ConfigParser::convertStrToSize_t(std::string nbr)
{
    std::stringstream ss(nbr);
    size_t result = 0;
    ss >> result;

    for (size_t i = 0; i < nbr.length(); ++i)
    {
        if (!isdigit(nbr[i]))
            throw std::runtime_error("Non-numeric character detected in timeout parameter!");
    }
    return (result);
}