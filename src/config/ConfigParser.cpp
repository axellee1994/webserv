#include "../../includes/config/ConfigParser.hpp"
#include "../../includes/errors/Errors.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void    ConfigParser::isServerDirective(std::string& lineValue, std::string& key, std::string& value, size_t& in_config, int& servblk_flag, int& server_flag)
{
    int param = 1;
    int wordcount = countWords(lineValue);

    if (value.empty())
    {
        if (key == "}")
        {
            server_flag = 0;
            servblk_flag = 0;
            return ;
        }
        else
            throw std::runtime_error("server block directive requires parameters!");
    }

    std::stringstream ss;
    ss << value;
    std::string v;
    while (std::getline(ss, v, ' '))
    {
        if (v.empty())
            continue ;

        if (key.substr(0, 6) == "listen" && key.size() == 6 && wordcount == 2)
        {
            if (v.find('.') != std::string::npos && v.find(':') != std::string::npos)
            {
                if (isValidIP(v))
                    addValidIP(v, in_config);
                else
                    throw std::runtime_error("invalid IP address/Port!");
            }
            else if (v.find('.') != std::string::npos || v.find(':') != std::string::npos)
            {
                if (isValidIP_alone(v))
                {
                    v = v + ":8080";
                    addValidIP(v, in_config);
                }
            }
            else
            {
                v = "0.0.0.0:" + v;
                addValidIP(v, in_config);
            }
        }
        else if (key.substr(0, 11) == "server_name")
        {
            if (isDuplicateElement_Str(this->configs[in_config].domains, v) == 1)
            {
                continue ;
            }
            else
                this->configs[in_config].domains.push_back(v);
        }
        else if (key.substr(0, 5) == "index")
        {
            if (this->configs[in_config].index_clear == 0)
            {
                wipeVector(this->configs[in_config].index);
                this->configs[in_config].index_clear = 1;
            }
            chkExtension(v);
            if (isDuplicateElement_Str(this->configs[in_config].index, v))
            {
                continue ;
            }
            else
                this->configs[in_config].index.push_back(v);
        }
        else if (key.substr(0, 4) == "root")
        {
            if (this->configs[in_config].root_flag == 1)
                throw std::runtime_error("conflicting directive 'root' is used more than once!");
            if (wordcount == 2)
            {
                this->configs[in_config].root = transformRoot(v);
                this->configs[in_config].root_flag = 1;
            }
            else
                throw std::runtime_error("root directive error detected!");
        }
        else if (key.substr(0, 9) == "autoindex")
        {
            if (this->configs[in_config].autoindex_flag == 1)
                throw std::runtime_error("'autoindex' is used more than once!");
            if (v.substr(0, 2) == "on")
            {
                this->configs[in_config].autoindex = 1;
                this->configs[in_config].autoindex_flag = 1;
            }
            else if (v.substr(0, 3) == "off")
            {
                this->configs[in_config].autoindex = 0;
                this->configs[in_config].autoindex_flag = 1;
            }
            else
                throw std::runtime_error("autoindex parameters only allows 'on' or 'off'!");
        }
        else if (key.substr(0, 20) == "client_max_body_size")
        {
            if (this->configs[in_config].cmbs_flag == 1)
                throw std::runtime_error("'client_max_body_size' is used more than once!");
            this->configs[in_config].client_max_body_size = convertToBytes(v);
            this->configs[in_config].cmbs_flag = 1;
        }
        else if (key.substr(0, 10) == "error_page" && wordcount >= 3)
        {
            std::string path;
            param++;
            if (v[0] == '/' && param == wordcount)
            {
                path = v;
                for (std::vector<int>::iterator it = this->error_codes.begin(); it != this->error_codes.end(); ++it)
                {
                    this->configs[in_config].error_pages[*it] = removeConsecutiveSlashes(path);
                }
                this->error_codes.clear();
                std::vector<int>().swap(error_codes);
            }
            else if (hasAlpha(v) == 0 && (param >= 1 && param < wordcount))
            {
                int err_c = atoi(v.c_str());
                if (err_c >= 400 && err_c <= 599)
                {
                    this->error_codes.push_back(err_c);
                }
                else
                    throw std::runtime_error("invalid error code detected! Must be 400 - 599!");
            }
            else
                throw std::runtime_error("error_page directive error detected!");
        }
        else if (key.substr(0, 6) == "return")
        {
            if (wordcount == 2 && hasAlpha(v) == 0)
            {
                if (!this->configs[in_config].ret.empty())
                    continue ;
                int err_c = atoi(v.c_str());
                if (err_c >= 100 && err_c <= 599)
                {
                    this->configs[in_config].ret[err_c];
                }
                else
                    throw std::runtime_error("in return directive: invalid error code detected! Must be 400 - 599!");
            }
            else if (wordcount == 3)
                {
                    if (!this->configs[in_config].ret.empty())
                        continue ;
                    if (hasAlpha(v) == 0 && isdigit(v[0]))
                    {
                        int err_c = atoi(v.c_str());
                        if (err_c >= 100 && err_c <= 599)
                        {
                            this->configs[in_config].ret_err_c = err_c;
                        }
                        else
                            throw std::runtime_error("in return directive: invalid error code detected! Must be 400 - 599!");
                    }
                    else if (v.substr(0, 1) == "/")
                    {
                        this->configs[in_config].ret_value = v;
                        if (this->configs[in_config].ret_err_c >= 100 && !this->configs[in_config].ret_value.empty())
                        {
                            this->configs[in_config].ret[this->configs[in_config].ret_err_c] = removeConsecutiveSlashes(this->configs[in_config].ret_value);
                            this->configs[in_config].ret_err_c = 0;
                            this->configs[in_config].ret_value.clear();
                        }
                        else
                            throw std::runtime_error("return directive must have an error_code + URL/PATH!");
                    }
                    else if (v.substr(0, 4) == "http")
                    {
                        this->configs[in_config].ret_value = v;
                        if (this->configs[in_config].ret_err_c >= 100 && !this->configs[in_config].ret_value.empty())
                        {
                            this->configs[in_config].ret[this->configs[in_config].ret_err_c] = removeConsecutiveSlashes(this->configs[in_config].ret_value);
                            this->configs[in_config].ret_err_c = 0;
                            this->configs[in_config].ret_value.clear();
                        }
                        else
                            throw std::runtime_error("return directive must have an error_code + URL/PATH!");
                    }
                    else
                        throw std::runtime_error("invalid return parameter detected!");
                }
            else
                throw std::runtime_error("return directive error detected!");
        }
        else if (key.substr(0, 8) == "cgi_exec" && wordcount == 3)
        {
            if (v[0] == '.')
            {
                if (v.length() > 1)
                    this->configs[in_config].cgi1 = v;
            }
            else if (v[0] == '/')
                this->configs[in_config].cgi2 = removeConsecutiveSlashes(v);
            else
                throw std::runtime_error("cgi_ext parameters are invalid!");
            if (!this->configs[in_config].cgi1.empty() && !this->configs[in_config].cgi2.empty())
            {
                this->configs[in_config].cgi[this->configs[in_config].cgi1] = this->configs[in_config].cgi2;
                this->configs[in_config].cgi1.clear();
                this->configs[in_config].cgi2.clear();
            }
        }
        else
            throw std::runtime_error("unknown server directive");
        
    }
}

void    ConfigParser::addServerDirective(std::string& lineValue, int& server_nbr, int& servblk_flag, int& server_flag)
{
    size_t in_config = server_nbr - 1;
    std::stringstream ss;
    ss << lineValue;

    std::string key;
    ss >> key;
    std::string value;
    std::getline(ss, value);
    while (!value.empty() && std::isspace(value[0]))
    {
        value = value.substr(1);
    }
    if (size_t pos = value.find("#"))
    {
        if (pos != std::string::npos)
            value.erase(pos);
    }
    while (!value.empty() && (value[value.size() - 1] == ';' || std::isspace(value[value.size() - 1])))
    {
        value = value.substr(0, value.size() - 1);
    }
    while (!lineValue.empty() && (lineValue[lineValue.size() - 1] == ';' || std::isspace(lineValue[lineValue.size() - 1])))
    {
        lineValue = lineValue.substr(0, lineValue.size() - 1);
    }

    if (lineValue.substr(0, 8) == "location" || this->location_flag == 1)
    {
        chkLocationBlock(lineValue, server_nbr);
    }
    else
        isServerDirective(lineValue, key, value, in_config, servblk_flag, server_flag);
}

void    ConfigParser::initServerBlock(int& server_nbr, int& servblk_flag)
{
    (void)servblk_flag;

    std::ostringstream oss;
    oss << server_nbr;
    std::string str = oss.str();
    ServerConfig newServer;

    this->configs.push_back(newServer);
    this->configs[server_nbr - 1].name = "server" + str;

    this->configs[server_nbr - 1].client_max_body_size = MAX_BODY_SIZE;
    this->configs[server_nbr - 1].root = transformRoot("www");
    this->configs[server_nbr - 1].index.push_back("index.html");
    this->configs[server_nbr - 1].index.push_back("index.htm");
    this->configs[server_nbr - 1].limit_except.push_back("GET");
    this->configs[server_nbr - 1].limit_except.push_back("POST");
    this->configs[server_nbr - 1].limit_except.push_back("DELETE");
}

void    ConfigParser::chkServerBlock(std::string& line, int& server_nbr, int& server_flag, int& servblk_flag)
{
    std::stringstream ss;
    ss << line;
    std::string value;
    std::getline(ss, value);
    while (!value.empty())
    {
        if (servblk_flag == 1)
        {
            addServerDirective(value, server_nbr, servblk_flag, server_flag);
            break ;
        }
        if (value.substr(0, 6) == "server")
        {
            server_flag = 1;
            value = value.substr(6);
            while (!value.empty())
            {
                if (std::isspace(value[0]))
                {
                    value = value.substr(1);
                }
                else if (value[0] == '{')
                {
                    server_nbr++;
                    servblk_flag = 1;
                    value = value.substr(1);
                    initServerBlock(server_nbr, servblk_flag);
                }
                else
                    throw std::runtime_error("server block error!");
            }
        }
        else if (value[0] == '{' && server_flag == 1)
        {
            server_nbr++;
            servblk_flag = 1;
            value = value.substr(1);
            initServerBlock(server_nbr, servblk_flag);
        }
        else
            throw std::runtime_error("server block cannot be found!");
    }
}
int ConfigParser::configChecker(std::ifstream& file, int& server_nbr)
{
    std::string line;
    int flag = 0;
    int server_flag = 0;
    int conf_empty = 1;
    int servblk_flag = 0;

    while (std::getline(file, line))
    {
        while (!line.empty() && std::isspace(line[0]))
    	{
        	line = line.substr(1);
    	}
        if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t\n\r") == std::string::npos)
        {
            continue;
        }
		while (!line.empty() && line[line.size() - 1] == ' ')
    	{
        	line = line.substr(0, line.size() - 1);
    	}
        if (!line.empty())
            conf_empty = 0;
        if (line.at(line.length() - 1) == '{')
            flag++;
        else if (line.at(line.length() - 1) == '}')
            flag--;
        chkServerBlock(line, server_nbr, server_flag, servblk_flag);
    }
    if (flag != 0)
    {
        std::cerr << "Error: '{' is left opened." << std::endl;
        return 1;
    }
    if (conf_empty == 1)
        throw std::runtime_error("Config file is empty.");
    return 0;
}

ConfigParser::ConfigParser(const std::string& filename) : location_flag(0), location_type(0), locblk_flag(0), line_number(0)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open .conf file");

    int server_nbr = 0;
    if (configChecker(file, server_nbr) != 0)
        throw std::runtime_error("Syntax error. Fix .conf file.");

    for (size_t i = 0; i < this->configs.size(); ++i)
    {
        checkDuplicates(this->configs[i]);
    }
    for (size_t j = 0; j < this->configs.size(); ++j)
    {
        generateIpPortDomain(this->configs[j]);
    }
    checkDuplicateIpPortDomains();
    printConfigs();
    std::cout << ColourRed << "=== END ConfigParser ===" << ColourReset << std::endl;
}

ConfigParser::~ConfigParser()
{
    configs.clear();
}

std::vector<ServerConfig>& ConfigParser::getConfigs()
{
    return configs;
}

void    ConfigParser::printLocConfigs(std::map<std::string, Location>& container)
{
    for (std::map<std::string, Location>::iterator container_it = container.begin(); container_it != container.end(); ++container_it)
    {
        std::cout << ColourRed << "Key: " << ColourReset << container_it->first << std::endl;
        std::cout << ColourRed << "Container members: " << ColourReset << std::endl;
        if (container_it->second.type == 0)
        {
            std::cout << ColourBlue << "  type: " << ColourReset << "(empty)" << std::endl;
        }
        else
            std::cout << ColourBlue << "  type: " << ColourReset << container_it->second.type << std::endl;
        if (container_it->second.path.empty())
            std::cout << ColourBlue << "  path: " << ColourReset << "(empty)" << std::endl;
        else
            std::cout << ColourBlue << "  path: " << ColourReset << container_it->second.path << std::endl;
        if (container_it->second.root.empty())
            std::cout << ColourBlue << "  root: " << ColourReset << "(empty)" << std::endl;
        else
            std::cout << ColourBlue << "  root: " << ColourReset << container_it->second.root << std::endl;
        
        std::cout << ColourBlue << "  index: " << ColourReset;
        if (container_it->second.index.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::vector<std::string>::iterator it = container_it->second.index.begin(); it != container_it->second.index.end(); ++it)
            {
                std::cout << *it;
                if (it + 1 != container_it->second.index.end())
                    std::cout << ", ";
                else
                    std::cout << std::endl;
            }
        }
        if (container_it->second.alias.empty())
            std::cout << ColourBlue << "  alias: " << ColourReset << "(empty)" << std::endl;
        else
            std::cout << ColourBlue << "  alias: " << ColourReset << container_it->second.alias << std::endl;

        std::cout << ColourBlue << "  ret: " << ColourReset;
        if (container_it->second.ret.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::map<int, std::string>::iterator ret_it = container_it->second.ret.begin(); ret_it != container_it->second.ret.end(); ++ret_it)
            {
                std::cout << "(" << ret_it->first << ", " << ret_it->second << ")" << std::endl;
            }
        }

        std::cout << ColourBlue << "  error_pages:" << ColourReset << std::endl;
        if (container_it->second.error_pages.empty())
            std::cout << "  (empty)" << std::endl;
        else
        {
            for (std::map<int, std::string>::iterator error_pages_it = container_it->second.error_pages.begin(); error_pages_it != container_it->second.error_pages.end(); ++error_pages_it)
            {
                std::cout << "    " << error_pages_it->first << ", " << error_pages_it->second << std::endl;
            }
        }

        if (container_it->second.client_max_body_size == 0)
            std::cout << ColourBlue << "  client_max_body_size: " << ColourReset << "0" << std::endl;
        else
            std::cout << ColourBlue << "  client_max_body_size: " << ColourReset << container_it->second.client_max_body_size << std::endl;

        if (container_it->second.autoindex == 0)
            std::cout << ColourBlue << "  autoindex: " << ColourReset << container_it->second.autoindex << std::endl;
        else
            std::cout << ColourBlue << "  autoindex: " << ColourReset << container_it->second.autoindex << std::endl;

        std::cout << ColourBlue << "  limit_except: " << ColourReset;
        if (container_it->second.limit_except.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::vector<std::string>::iterator it = container_it->second.limit_except.begin(); it != container_it->second.limit_except.end(); ++it)
            {
                std::cout << *it;
                if (it + 1 != container_it->second.limit_except.end())
                    std::cout << ", ";
                else
                    std::cout << std::endl;
            }
        }

        std::cout << ColourBlue << "  cgi:\n" << ColourReset;
        if (container_it->second.cgi.empty())
            std::cout << "  (empty)" << std::endl;
        else
        {
            for (std::map<std::string, std::string>::iterator cgi_it = container_it->second.cgi.begin(); cgi_it != container_it->second.cgi.end(); ++cgi_it)
            {
                std::cout << "    " << cgi_it->first << ", " << cgi_it->second << std::endl;
            }
        }
    }
}

void    ConfigParser::printConfigs()
{
    for (size_t i = 0; i < configs.size(); ++i)
    {
        std::cout << "-------------------------------------" << std::endl;
        std::cout << ColourYellow << configs[i].name << ColourReset << std::endl;

        std::cout << ColourGreen << "ip_port:\n" << ColourReset;
        if (configs[i].ip_port.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::map<std::string, std::vector<int> >::iterator it = configs[i].ip_port.begin(); it != configs[i].ip_port.end(); ++it)
            {
                std::string ip = it->first;
                std::vector<int> ports = it->second;

                for (size_t j = 0; j < ports.size(); ++j)
                {
                    std::cout << ip << ":" << ports[j] << std::endl;
                }
            }
        }

        std::cout << ColourGreen << "server_name/domains: " << ColourReset;
        if (configs[i].domains.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::vector<std::string>::iterator domains_it = configs[i].domains.begin(); domains_it != configs[i].domains.end(); ++domains_it)
            {
                std::cout << *domains_it;
                if (domains_it + 1 != configs[i].domains.end())
                    std::cout << ", ";
                else
                    std::cout << std::endl;
            }
        }

        std::cout << ColourGreen << "ipPortDomains:\n" << ColourReset;
        if (configs[i].ipPortDomain.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::vector<std::string>::iterator ipd_it = configs[i].ipPortDomain.begin(); ipd_it != configs[i].ipPortDomain.end(); ++ipd_it)
                std::cout << *ipd_it << std::endl;
        }

        std::cout << ColourGreen << "index: " << ColourReset;
        if (configs[i].index.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::vector<std::string>::iterator index_it = configs[i].index.begin(); index_it != configs[i].index.end(); ++index_it)
            {
                std::cout << *index_it;
                if (index_it + 1 != configs[i].index.end())
                    std::cout << ", ";
                else
                    std::cout << std::endl;
            }
        }

        std::cout << ColourGreen << "root: " << ColourReset;
        if (configs[i].root.empty())
            std::cout << "(empty)" << std::endl;
        else
            std::cout << configs[i].root << std::endl;
        
        std::cout << ColourGreen << "ret: " << ColourReset;
        if (configs[i].ret.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::map<int, std::string>::iterator ret_it = configs[i].ret.begin(); ret_it != configs[i].ret.end(); ++ret_it)
            {
                std::cout << "(" << ret_it->first << ", " << ret_it->second << ")" << std::endl;
            }
        }
        
        std::cout << ColourGreen << "error_pages:\n" << ColourReset;
        if (configs[i].error_pages.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::map<int, std::string>::iterator error_pages_it = configs[i].error_pages.begin(); error_pages_it != configs[i].error_pages.end(); ++error_pages_it)
            {
                std::cout << error_pages_it->first << ", " << error_pages_it->second << std::endl;
            }
        }

        std::cout << ColourGreen << "client_max_body_size: " << ColourReset;
        if (configs[i].client_max_body_size == 0)
            std::cout << "0" << std::endl;
        else
            std::cout << configs[i].client_max_body_size << std::endl;

        if (configs[i].autoindex == 0)
            std::cout << ColourGreen << "autoindex: " << ColourReset << configs[i].autoindex << std::endl;
        else
            std::cout << ColourGreen << "autoindex: " << ColourReset << configs[i].autoindex << std::endl;

        std::cout << ColourGreen << "limit_except: " << ColourReset;
        if (configs[i].limit_except.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::vector<std::string>::iterator it = configs[i].limit_except.begin(); it != configs[i].limit_except.end(); ++it)
            {
                std::cout << *it;
                if (it + 1 != configs[i].limit_except.end())
                    std::cout << ", ";
                else
                    std::cout << std::endl;
            }
        }

        std::cout << ColourGreen << "cgi:\n" << ColourReset;
        if (configs[i].cgi.empty())
            std::cout << "(empty)" << std::endl;
        else
        {
            for (std::map<std::string, std::string>::iterator cgi_it = configs[i].cgi.begin(); cgi_it != configs[i].cgi.end(); ++cgi_it)
            {
                std::cout << cgi_it->first << ", " << cgi_it->second << std::endl;
            }
        }

        std::cout << ColourYellow << "Locations:\n" << ColourReset;
        if (configs[i].exact_loc.empty())
            std::cout << "exact_loc: (empty)" << std::endl;
        else
        {
            std::cout << ColourOrange << "* in exact_loc *" << ColourReset << std::endl;
            printLocConfigs(configs[i].exact_loc);
            std::cout << "Map size = " << configs[i].exact_loc.size() << std::endl;
        }

        if (configs[i].prefer_loc.empty())
            std::cout << "prefer_loc: (empty)" << std::endl;
        else
        {
            std::cout << ColourOrange << "* in prefer_loc *" << ColourReset << std::endl;
            printLocConfigs(configs[i].prefer_loc);
            std::cout << "Map size = " << configs[i].prefer_loc.size() << std::endl;
        }

        if (configs[i].prefix_loc.empty())
            std::cout << "prefix_loc: (empty)" << std::endl;
        else
        {
            std::cout << ColourOrange << "* in prefix_loc *" << ColourReset << std::endl;
            printLocConfigs(configs[i].prefix_loc);
            std::cout << "Map size = " << configs[i].prefix_loc.size() << std::endl;
        }
        std::cout << "-------------------------------------" << std::endl;
    }
}
