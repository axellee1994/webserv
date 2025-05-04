#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include "../constants.hpp"

#define ColourRed "\033[31m"
#define ColourOrange "\033[38;5;214m"
#define ColourYellow "\033[33m"
#define ColourGreen "\033[32m"
#define ColourBlue "\033[34m"
#define ColourReset "\033[0m"

typedef struct LocationStruct
{
    int type;
    std::string path;
    std::vector<std::string> index;
    int index_isWiped;
    std::map<int, std::string> ret;
    int ret_err_c;
    std::string ret_value;
    std::string root;
    std::string alias;
    int root_alias_flag;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    size_t cmbs_flag;
    bool autoindex;
    int autoindex_flag;
    std::vector<std::string> limit_except;
    int le_flag;
    std::map<std::string, std::string> cgi;
    std::string cgi1;
    std::string cgi2;

    LocationStruct() : type(0), index_isWiped(0), ret_err_c(0), root_alias_flag(0), client_max_body_size(1048576)
        , cmbs_flag(0), autoindex(false), autoindex_flag(0), le_flag(0) {}
} Location;

typedef struct ServerConfigStruct
{
    std::string name;
    std::vector<int> ports;
    std::string ipaddr;
    std::map< std::string, std::vector<int> > ip_port;
    std::vector<std::string> ipPortVec;
    std::vector<std::string> ipPortDomain;
    std::vector<std::string> domains;
    std::vector<std::string> index;
    int index_clear;
    std::map<std::string, std::string> cgi_ext;
    std::map<int, std::string> ret;
    int ret_err_c;
    std::string ret_value;
    std::string root;
    int root_flag;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    size_t cmbs_flag;
    bool autoindex;
    int autoindex_flag;
    std::vector<std::string> limit_except;
    std::map<std::string, Location> exact_loc;
    std::map<std::string, Location> prefer_loc;
    std::map<std::string, Location> prefix_loc;
    std::string upload_store;
    size_t max_file_size;
    std::vector<std::string> allowed_types;
    std::map<std::string, std::string> cgi;
    std::string cgi1;
    std::string cgi2;
    size_t  client_header_timeout;
    size_t  client_body_timeout;
    size_t  send_timeout;
    size_t  keepalive_timeout;
    size_t  cgi_read_timeout;
    size_t  cgi_send_timeout;
    size_t  cgi_connect_timeout;

    ServerConfigStruct() : index_clear(0), ret_err_c(0), root_flag(0), client_max_body_size(1048576), cmbs_flag(0)
        , autoindex(false), autoindex_flag(0), max_file_size(0) {}
} ServerConfig;

class ConfigParser
{
    private:
        std::vector<ServerConfig> configs;

        std::vector<int> error_codes;
        std::string loc_path;
        int location_flag;
        int location_type;
        int locblk_flag;
        std::string current_line;
        size_t line_number;

    public:
        ConfigParser(const std::string& filename);
        ~ConfigParser();

        int     configChecker(std::ifstream& file, int& server_nbr);
        void    chkServerBlock(std::string& line, int& server_nbr, int& server_flag, int& servblk_flag);
        void    initServerBlock(int& server_nbr, int& servblk_flag);
        void    addServerDirective(std::string& line, int& server_nbr, int& servblk_flag, int& server_flag);
        void    isServerDirective(std::string& lineValue, std::string& key, std::string& value, size_t& in_config, int& servblk_flag, int& server_flag);
        void    chkLocationBlock(std::string& lineValue, int& server_nbr);
        void    initLocationBlock(int& server_nbr, std::string& path);
        void    initLocationObject(std::map<std::string, Location>& inLocationMap, int& server_nbr, std::string& path);
        void    addLocationDirective(std::string& lineValue, int& server_nbr);
        void    isLocationDirective(Location& loc_struct, std::string& lineValue, std::string& key, std::string& value);
        int     countWords(std::string& lineValue);
        int     isValidIP(std::string& s);
        int     isValidIP_alone(std::string& s);
        void    addValidIP(std::string& s, size_t& in_config);
        int     hasAlpha(std::string& s);
        int     isValidPortNumber(int port);
        size_t  convertToBytes(std::string v);
        void    wipeVector(std::vector<std::string>& index);
        void    chkExtension(std::string& str);
        void    toUpperCase(std::string& str);
        int     isDuplicateElement_Int(std::vector<int>& container, int nbr);
        int     isDuplicateElement_Str(std::vector<std::string>& container, std::string str);
        void    checkDuplicates(ServerConfig& server);
        void    generateIpPortDomain(ServerConfig& server);
        void    checkDuplicateIpPortDomains();
        size_t  convertStrToSize_t(std::string nbr);

        std::vector<ServerConfig>& getConfigs();

        std::string removeConsecutiveSlashes(const std::string &path);
        std::string transformRoot(const std::string &root);

        void    printConfigs();
        void    printLocConfigs(std::map<std::string, Location>& exact_loc);
};

#endif