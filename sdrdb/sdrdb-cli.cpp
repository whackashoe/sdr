#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <algorithm>

#include <libsocket/unixclientstream.hpp>
#include <libsocket/exception.hpp>
#include <readline/readline.h>
#include <readline/history.h>

#include <unistd.h>
#include <stdlib.h>

#include "util.hpp"


#ifndef SDRDB_VERSION
#define SDRDB_VERSION "0.1-alpha"
#endif

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

std::string bindpath { "/tmp/sdrdb.sock" };


void render_help()
{
    std::cout
        << "List of all sdrdb commands:" << std::endl
        << "Enter \"help COMMAND\" for more information" << std::endl
        << std::endl
        << "help\n\tShow this help" << std::endl
        << "exit\n\tExit interactive console" << std::endl
        << "create database DBNAME\n\t" << std::endl
        << "drop database DBNAME\n\t" << std::endl
        << "list DBNAME\n\tPrint data stats for database" << std::endl
        << "clear DBNAME\n\tEmpty database" << std::endl
        << "resize DBNAME WIDTH\n\tSet new width for database" << std::endl
        << "name trait POS NAME in DBNAME\n\tSet alias for trait" << std::endl
        << "name concept POS NAME in DBNAME\n\tSet alias for trait" << std::endl
        << "rename trait NAME NEWNAME in DBNAME\n\tRename alias for trait" << std::endl
        << "rename concept NAME NEWNAME in DBNAME\n\tRename alias for concept" << std::endl
        << "load database from FILE into DBNAME\n\t" << std::endl
        << "save database DBNAME to FILE\n\t" << std::endl
        << "put into DBNAME [as NAME] TRAIT...\n\t" << std::endl
        << "query [WEIGHTED] similarity from DBNAME CONCEPT CONCEPT\n\t" << std::endl
        << "query [WEIGHTED] usimilarity from DBNAME CONCEPT CONCEPT...\n\t" << std::endl
        << "query [WEIGHTED] closest from DBNAME AMOUNT CONCEPT\n\t" << std::endl
        << "query [WEIGHTED] matching from DBNAME TRAIT...\n\t" << std::endl
        << "query [WEIGHTED] matchingx from DBNAME AMOUNT TRAIT...\n\t" << std::endl
        << std::endl;
}

void exit_console()
{
    std::cout << "Goodbye :)" << std::endl;
}

void display_usage()
{
    std::cout
        << "Usage: sdrdb [OPTION]..." << std::endl
        << "Options and arguments:" << std::endl
        << "-h            : show this help" << std::endl
        << "-v            : show version" << std::endl
        << "-b arg        : set bindpath for server" << std::endl;
}

void display_version()
{
    std::cout << "sdrdb " << SDRDB_VERSION << std::endl;
}

void send_command(const std::string & cmd)
{
    constexpr std::size_t buffer_size { 128 };

    try {
        libsocket::unix_stream_client sock(bindpath);
        sock << cmd;

        while(true) {
            std::string answer;
            answer.resize(buffer_size);

            sock >> answer;

            std::cout << answer;

            for(char c : answer) {
                if(c == '\n') {
                    goto breaker;
                }
            }
        } breaker:;
    } catch (const libsocket::socket_exception & exc) {
        std::cerr << exc.mesg;
    }
}

bool inputloop()
{
    const char * input = readline("sdrdb" ANSI_COLOR_CYAN "> " ANSI_COLOR_RESET);

    std::string sinput(input);
    sinput = trim(sinput);

    if(sinput.length() > 0) {
        if(sinput == "help") {
            render_help();
        } else if(sinput == "exit") {
            exit_console();
            return false;
        } else {
            sinput += '\n';
            send_command(sinput);
        }

        add_history(input);
    }


    return true;
}

int main(int argc, char ** argv)
{
    {
        int c;

        while ((c = getopt (argc, argv, "vhb:")) != -1) {
            switch (c) {
            case 'v':
                display_version();
                return EXIT_SUCCESS;
                break;
            case 'h':
                display_usage();
                return EXIT_SUCCESS;
                break;
            case 'b':
                bindpath = optarg;
                break;
            case '?':
                std::cerr << "sdrdb: invalid option" << std::endl;
                display_usage();
                return EXIT_FAILURE;
            default:
                abort ();
            }
        }
    }

    std::cout
        << "Welcome to the sdrdb interactive console. Commands end with \\n" << std::endl
        << "Server version: " << SDRDB_VERSION << std::endl
        << std::endl
        << "Type 'help' for help." << std::endl
        << std::endl;

    while(inputloop());

    return EXIT_SUCCESS;
}
