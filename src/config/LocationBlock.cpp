#include "../../includes/config/ConfigParser.hpp"

void    ConfigParser::isLocationDirective(Location& loc_struct, std::string& lineValue, std::string& key, std::string& value)
{
    int param = 1;
    int le_pushflag = 0;
    int wordcount = countWords(lineValue);

    if (value.empty())
    {
        if (key == "}")
        {
            this->location_flag = 0;
            this->locblk_flag = 0;
            return ;
        }
        else
            throw std::runtime_error("location block directive requires parameters!");
    }

    std::stringstream ss;
    ss << value;
    std::string v;

    while (std::getline(ss, v, ' '))
    {
        if (v.empty())
            continue ;

        if (key.substr(0, 5) == "index")
        {
            chkExtension(v);
            if (loc_struct.index_isWiped == 0)
            {
                wipeVector(loc_struct.index);
                loc_struct.index_isWiped = 1;
                if (isDuplicateElement_Str(loc_struct.index, v))
                {
                    continue ;
                }
                else
                    loc_struct.index.push_back(v);
            }
            else
            {
                if (isDuplicateElement_Str(loc_struct.index, v))
                    throw std::runtime_error("(location) duplicate index detected!");
                else
                    loc_struct.index.push_back(v);
            }
        }
        else if (key.substr(0, 4) == "root")
        {
            if (loc_struct.root_alias_flag == 1)
                throw std::runtime_error("conflicting directive 'root' and 'alias' in location block,\nor 'root' is used more than once!");
            if (wordcount == 2)
            {
                loc_struct.root = transformRoot(v);
                loc_struct.root_alias_flag = 1;
            }
            else
                throw std::runtime_error("root directive error detected!");
        }
        else if (key.substr(0, 12) == "limit_except")
        {
            if (loc_struct.le_flag == 1)
                throw std::runtime_error("(location) 'limit_except' is used more than once!");
            if (le_pushflag == 0)
            {
                wipeVector(loc_struct.limit_except);
                le_pushflag = 1;
            }

            param++;
            toUpperCase(v);
            if (le_pushflag == 1 && (param >= 1 && param < wordcount))
            {
                if (v.substr(0, 3) == "GET" || v.substr(0, 4) == "POST" || v.substr(0, 6) == "DELETE")
                {
                    if (isDuplicateElement_Str(loc_struct.limit_except, v) == 1)
                        continue ;
                    else
                    {
                        loc_struct.limit_except.push_back(v);
                    }
                }
                else
                    throw std::runtime_error("unknown parameters.");
            }
            else if (le_pushflag == 1 && (param == wordcount))
            {
                if (v.substr(0, 3) == "GET" || v.substr(0, 4) == "POST" || v.substr(0, 6) == "DELETE")
                {
                    if (isDuplicateElement_Str(loc_struct.limit_except, v) == 1)
                    {
                        le_pushflag = 0;
                        continue ;
                    }
                    else
                    {
                        loc_struct.limit_except.push_back(v);
                        le_pushflag = 0;
                        loc_struct.le_flag = 1;
                    }
                }
                else
                    throw std::runtime_error("unknown parameters.");
            }
        }
        else if (key.substr(0, 9) == "autoindex")
        {
            if (loc_struct.autoindex_flag == 1)
                throw std::runtime_error("(Location) 'autoindex' is used more than once!");
            if (v.substr(0, 2) == "on")
            {
                loc_struct.autoindex = 1;
                loc_struct.autoindex_flag = 1;
            }
            else if (v.substr(0, 3) == "off")
            {
                loc_struct.autoindex = 0;
                loc_struct.autoindex_flag = 1;
            }
            else
                throw std::runtime_error("autoindex parameters only allows 'on' or 'off'!");
        }
        else if (key.substr(0, 20) == "client_max_body_size")
        {
            if (loc_struct.cmbs_flag == 1)
                throw std::runtime_error("(Location) 'client_max_body_size' is used more than once!");
            loc_struct.client_max_body_size = convertToBytes(v);
            loc_struct.cmbs_flag = 1;
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
                    loc_struct.error_pages[*it] = removeConsecutiveSlashes(path);
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
        else if (key.substr(0, 5) == "alias")
        {
            if (loc_struct.root_alias_flag == 1)
                throw std::runtime_error("conflicting directive 'root' and 'alias' in location block,\nor 'alias' is used more than once!");
            if (v[0] == '/' && wordcount == 2)
            {
                loc_struct.alias = transformRoot(v);
                loc_struct.root_alias_flag = 1;
                loc_struct.root = "";
            }
            else
                throw std::runtime_error("alias directive error detected!");
        }
        else if (key.substr(0, 6) == "return")
        {
            if (wordcount == 2 && hasAlpha(v) == 0)
            {
                if (!loc_struct.ret.empty())
                    continue ;
                int err_c = atoi(v.c_str());
                if (err_c >= 100 && err_c <= 599)
                {
                    loc_struct.ret[err_c];
                }
                else
                    throw std::runtime_error("(location) in return directive: invalid error code detected! Must be 400 - 599!");
            }
            else if (wordcount == 3)
                {
                    if (!loc_struct.ret.empty())
                        continue ;
                    if (hasAlpha(v) == 0 && isdigit(v[0]))
                    {
                        int err_c = atoi(v.c_str());
                        if (err_c >= 100 && err_c <= 599)
                        {
                            loc_struct.ret_err_c = err_c;
                        }
                        else
                            throw std::runtime_error("(location) in return directive: invalid error code detected! Must be 400 - 599!");
                    }
                    else if (v.substr(0, 1) == "/")
                    {
                        loc_struct.ret_value = v;
                        if (loc_struct.ret_err_c >= 100 && !loc_struct.ret_value.empty())
                        {
                            loc_struct.ret[loc_struct.ret_err_c] = removeConsecutiveSlashes(loc_struct.ret_value);
                            loc_struct.ret_err_c = 0;
                            loc_struct.ret_value.clear();
                        }
                        else
                            throw std::runtime_error("(location) return directive must have an error_code + URL/PATH!");
                    }
                    else if (v.substr(0, 4) == "http")
                    {
                        loc_struct.ret_value = v;
                        if (loc_struct.ret_err_c >= 100 && !loc_struct.ret_value.empty())
                        {
                            loc_struct.ret[loc_struct.ret_err_c] = removeConsecutiveSlashes(loc_struct.ret_value);
                            loc_struct.ret_err_c = 0;
                            loc_struct.ret_value.clear();
                        }
                        else
                            throw std::runtime_error("(location) return directive must have an error_code + URL/PATH!");
                    }
                    else
                        throw std::runtime_error("(location) invalid return parameter detected!");
                }
            else
                throw std::runtime_error("return directive error detected!");
        }
        else if (key.substr(0, 8) == "cgi_exec" && wordcount == 3)
        {
            if (v[0] == '.')
            {
                if (v.length() > 1)
                    loc_struct.cgi1 = v;
            }
            else if (v[0] == '/')
            loc_struct.cgi2 = removeConsecutiveSlashes(v);
            else
                throw std::runtime_error("cgi_ext parameters are invalid!");
            if (!loc_struct.cgi1.empty() && !loc_struct.cgi2.empty())
            {
                loc_struct.cgi[loc_struct.cgi1] = loc_struct.cgi2;
                loc_struct.cgi1.clear();
                loc_struct.cgi2.clear();
            }
        }
        else
            throw std::runtime_error("unknown location directive!");
    }
}

void    ConfigParser::addLocationDirective(std::string& lineValue, int& server_nbr)
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

    std::map<std::string, Location>::iterator it;
    it = this->configs[in_config].exact_loc.find(this->loc_path);
    if (it != this->configs[in_config].exact_loc.end() && it->second.path == this->loc_path)
    {
        isLocationDirective(it->second, lineValue, key, value);
        return;
    }

    it = this->configs[in_config].prefer_loc.find(this->loc_path);
    if (it != this->configs[in_config].prefer_loc.end() && it->second.path == this->loc_path)
    {
        isLocationDirective(it->second, lineValue, key, value);
        return;
    }

    it = this->configs[in_config].prefix_loc.find(this->loc_path);
    if (it != this->configs[in_config].prefix_loc.end() && it->second.path == this->loc_path)
    {
        isLocationDirective(it->second, lineValue, key, value);
        return;
    }
}

void    ConfigParser::initLocationObject(std::map<std::string, Location>& inLocationMap, int& server_nbr, std::string& path)
{
    Location    newLocation;

    inLocationMap.insert(std::make_pair(path, newLocation));

    inLocationMap[path].type = this->location_type;
    inLocationMap[path].path = path;

    inLocationMap[path].client_max_body_size
        = this->configs[server_nbr - 1].client_max_body_size;
    inLocationMap[path].le_flag = 0;
    inLocationMap[path].limit_except
        = this->configs[server_nbr - 1].limit_except;
    inLocationMap[path].error_pages
        = this->configs[server_nbr - 1].error_pages;
    inLocationMap[path].autoindex
        = this->configs[server_nbr - 1].autoindex;
    inLocationMap[path].index
        = this->configs[server_nbr - 1].index;
    inLocationMap[path].index_isWiped = 0;
    inLocationMap[path].root
        = this->configs[server_nbr - 1].root;
    inLocationMap[path].root_alias_flag = 0;
    inLocationMap[path].cgi
        = this->configs[server_nbr - 1].cgi;
}

void    ConfigParser::initLocationBlock(int& server_nbr, std::string& path)
{
    if (this->location_type == 0)
    {
        throw std::runtime_error("initLocationBlock unexpected error!");
    }
    else if (this->location_type == 1)
    {
        initLocationObject(this->configs[server_nbr - 1].exact_loc, server_nbr, path);
    }
    else if (this->location_type == 2)
    {
        initLocationObject(this->configs[server_nbr - 1].prefer_loc, server_nbr, path);
    }
    else if (this->location_type == 3)
    {
        initLocationObject(this->configs[server_nbr - 1].prefix_loc, server_nbr, path);
    }
}

void    ConfigParser::chkLocationBlock(std::string& lineValue, int& server_nbr)
{
    while (!lineValue.empty())
    {
        if (locblk_flag == 1)
        {
            if (this->location_type == 0 || this->loc_path.empty())
                throw std::runtime_error("invalid location type and/or path is empty!");
            else
            {
                addLocationDirective(lineValue, server_nbr);
                break ;
            }
        }
        if (lineValue.substr(0, 8) == "location")
        {
            this->location_flag = 1;
            this->location_type = 0;
            lineValue = lineValue.substr(8);

            while (!lineValue.empty())
            {
                if (std::isspace(lineValue[0]))
                {
                    lineValue = lineValue.substr(1);
                }
                else if (lineValue.substr(0, 1) == "=")
                {
                    if (this->location_type == 0)
                    {
                        this->location_type = 1;
                        lineValue = lineValue.substr(1);
                    }
                    else
                        throw std::runtime_error("on 'location' line!");
                }
                else if (lineValue.substr(0, 2) == "^~")
                {
                    if (this->location_type == 0)
                    {
                        this->location_type = 2;
                        lineValue = lineValue.substr(2);
                    }
                    else
                        throw std::runtime_error("on 'location' line!");
                }
                else if (lineValue.substr(0, 1) == "/")
                {
                    std::stringstream l_ss;
                    l_ss << lineValue;
                    std::string path;

                    getline(l_ss, path, ' ');
                    if (this->location_type == 0)
                    {
                        this->location_type = 3;
                    }
                    if (path[path.size() - 1] == '{')
                    {
                        path.erase(path.size() - 1);
                        this->loc_path = path;
                        lineValue = lineValue.substr(path.size());
                    }
                    else
                    {
                        this->loc_path = path;
                        lineValue = lineValue.substr(path.size());
                    }
                }
                else if (lineValue[0] == '{' && this->location_type != 0)
                {
                    this->locblk_flag = 1;
                    lineValue = lineValue.substr(1);
                    initLocationBlock(server_nbr, this->loc_path);
                }
                else
                {
                    throw std::runtime_error("location block error!");
                }
            }
        }
        else if (lineValue[0] == '{' && this->location_flag == 1)
        {
            this->locblk_flag = 1;
            lineValue = lineValue.substr(1);
            initLocationBlock(server_nbr, this->loc_path);
        }
        else
            throw std::runtime_error("location block cannot be found!");
    }
}