#include "../includes/config/ConfigParser.hpp"
#include "../includes/server/Multiplexer.hpp"
#include <signal.h>
#include <iostream>

static bool running = true;

void signal_handler(int signum)
{
    (void)signum;
    running = false;
}


int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Please use: ./webserv [configuration file]" << std::endl;
        return 1;
    }
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    try
    {
        ConfigParser cp(argv[1]);
        Multiplexer* mp = new Multiplexer(cp.getConfigs());

        while (running)
        {
            mp->monitorEvents();
        }

        std::cout << "\nShutting down server gracefully..." << std::endl;
        delete mp;
        for (int fd = 3; fd < 1024; fd++) 
            close(fd);

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}