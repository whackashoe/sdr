#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <sstream>

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


struct help_block_arg
{
    std::string arg;
    std::string type;
    bool required;
    std::string desc;

    help_block_arg(
        const std::string & arg
      , const std::string & type
      , const bool required
      , const std::string & desc
    )
    : arg(arg)
    , type(type)
    , required(required)
    , desc(desc)
    {}

    void render() const
    {
        std::cout
            << ANSI_COLOR_YELLOW << arg << ANSI_COLOR_RESET
            << " (" << type << ") ";
        if(required) {
            std::cout << ANSI_COLOR_RED << "(required)" << ANSI_COLOR_RESET;
        }
        std::cout
            << " : "
            << desc
            << std::endl;
    }
};

struct help_block
{
    std::string cmd;
    std::string summary;
    std::vector<help_block_arg> args;
    std::vector<std::string> examples;

    help_block(
        const std::string & cmd
      , const std::string & summary
      , const std::vector<help_block_arg> & args
      , const std::vector<std::string> & examples
    )
    : cmd(cmd)
    , summary(summary)
    , args(args)
    , examples(examples)
    {}

    void render() const
    {
        std::cout << ANSI_COLOR_CYAN << cmd << ANSI_COLOR_RESET << " - " << summary << std::endl;
        for(auto & arg : args) {
            std::cout << "\t";
            arg.render();
        }

        std::cout << "EXAMPLES:" << std::endl << std::endl;

        for(auto & example : examples) {
            std::cout << example << std::endl << std::endl;
        }
    }
};


void render_help_commands_index()
{
    std::cout
        << "List of all sdrdb commands:" << std::endl
        << "Enter \"help COMMAND\" for more information" << std::endl
        << std::endl

        << "help\n\tShow this help" << std::endl
        << "exit\n\tExit interactive console" << std::endl

        << "create DBNAME AMOUNT\n\t" << std::endl
        << "drop DBNAME\n\t" << std::endl
        << "clear DBNAME\n\tEmpty database" << std::endl
        << "resize DBNAME WIDTH\n\tSet new width for database" << std::endl

        //<< "load from FILE into DBNAME\n\t" << std::endl
        //<< "save DBNAME to FILE\n\t" << std::endl

        << "put DBNAME TRAIT...\n\t" << std::endl
        << "update DBNAME CONCEPT_ID TRAIT....\n\t" << std::endl

        << "query DBNAME [WEIGHTED] similarity CONCEPT CONCEPT\n\t" << std::endl
        << "query DBNAME [WEIGHTED] usimilarity CONCEPT CONCEPT...\n\t" << std::endl
        << "query DBNAME [WEIGHTED] [ASYNC] closest AMOUNT CONCEPT\n\t" << std::endl
        << "query DBNAME matching TRAIT...\n\t" << std::endl
        << "query DBNAME [WEIGHTED] matchingx AMOUNT TRAIT...\n\t" << std::endl
        << std::endl;
}

void render_help_create()
{
    const help_block item("create", "create a new database", {
        help_block_arg("DBNAME", "string", true, "the name of the database to create"),
        help_block_arg("AMOUNT", "integer", true, "dimensions"),
    }, {
        "create newdb 1024",
        "create people 3200"
    });

    item.render();
}

void render_help_drop()
{
    const help_block item("drop", "delete a database and everything inside it", {
        help_block_arg("DBNAME", "string", true, "the name of the database to delete")
    }, {
        "drop newdb",
        "drop people"
    });

    item.render();
}

void render_help_clear()
{
    const help_block item("clear", "clear all items from database", {
        help_block_arg("DBNAME", "string", true, "the name of the database to empty")
    }, {
        "clear newdb",
        "clear people"
    });

    item.render();
}

void render_help_resize()
{
    const help_block item("resize", "change width of database", {
        help_block_arg("DBNAME", "string", true, "the name of the database to resize"),
        help_block_arg("WIDTH", "integer", true, "new dimension of database"),
    }, {
        "resize newdb 256",
        "resize people 400000"
    });

    item.render();
}

void render_help_put()
{
    const help_block item("put", "insert concept into database", {
        help_block_arg("DBNAME", "string", true, "the name of the database"),
        help_block_arg("TRAIT...", "[integer]", true, "list of traits between 1 and database width"),
    }, {
        "put newdb 1 2 4 6 78 100",
        "put people 663 3393 3"
    });

    item.render();
}

void render_help_update()
{
    const help_block item("update", "change a row in database", {
        help_block_arg("DBNAME", "string", true, "the name of the database"),
        help_block_arg("CONCEPT_ID", "string", true, "the concepts identifier you received from 'put'"),
        help_block_arg("TRAIT...", "[integer]", false, "same as put, if empty concept will be cleared"),
    }, {
        "update newdb 1 4 6 78",
        "update people 5 3"
    });

    item.render();
}

void render_help_query()
{
    std::cout << "try one of these:" << std::endl
        << "\thelp query similarity" << std::endl
        << "\thelp query usimilarity" << std::endl
        << "\thelp query closest" << std::endl
        << "\thelp query matching" << std::endl
        << "\thelp query matchingx" << std::endl;
}

void render_help_query_similarity()
{
    const help_block item("query similarity", "find amount of matching traits between two concepts", {
        help_block_arg("DBNAME", "string", true, "the name of the database"),
        //help_block_arg("WEIGHTED", "string", false, "whether or not to use weighting data for results"),
        help_block_arg("CONCEPT_A", "integer", true, "first concept id"),
        help_block_arg("CONCEPT_B", "integer", true, "second concept id"),
    }, {
        "query newdb similarity 1 2",
        "query people similarity 5 3"
    });

    item.render();
}

void render_help_query_usimilarity()
{
    const help_block item("query usimilarity", "find amount of matching traits between concept and union of concepts", {
        help_block_arg("DBNAME", "string", true, "the name of the database"),
        //help_block_arg("WEIGHTED", "string", false, "whether or not to use weighting data for results"),
        help_block_arg("CONCEPT_ID", "integer", true, "concept id to compare with"),
        help_block_arg("CONCEPT_ID...", "integer", true, "list of concept_ids OR'd with each other"),
    }, {
        "query newdb usimilarity 1 2 3 4",
        "query people usimilarity 5 3 2"
    });

    item.render();
}

void render_help_query_closest()
{
    const help_block item("query closest", "find N closest concepts to another concept", {
        help_block_arg("DBNAME", "string", true, "the name of the database"),
        //help_block_arg("WEIGHTED", "string", false, "whether or not to use weighting data for results"),
        //help_block_arg("ASYNC", "string", false, "whether or not to run this asynchronously"),
        help_block_arg("AMOUNT", "integer", true, "amount to find"),
        help_block_arg("CONCEPT_ID", "integer", true, "concept to find closest to"),
    }, {
        "query newdb closest 4 2",
        "query people closest 1 3"
    });

    item.render();
}

void render_help_query_matching()
{
    const help_block item("query matching", "find concepts matching specific traits", {
        help_block_arg("DBNAME", "string", true, "the name of the database"),
        //help_block_arg("WEIGHTED", "string", false, "whether or not to use weighting data for results"),
        help_block_arg("TRAITS...", "integer", true, "list of traits each concept must contain")
    }, {
        "query newdb matching 1 2",
        "query people matching 3 4"
    });

    item.render();
}

void render_help_query_matchingx()
{
    const help_block item("query matchingx", "find concepts matching at least N specific traits", {
        help_block_arg("DBNAME", "string", true, "the name of the database"),
        //help_block_arg("WEIGHTED", "string", false, "whether or not to use weighting data for results"),
        help_block_arg("AMOUNT", "integer", true, "amount of traits required for inclusion"),
        help_block_arg("TRAITS...", "integer", true, "list of traits concepts must contain with respect to AMOUNT")
    }, {
        "query newdb matchingx 1 1 2",
        "query people matching 2 3 4 5"
    });

    item.render();
}

void parse_help(const std::string & str)
{
    std::istringstream iss(str);

    std::vector<std::string> pieces;

    {
        std::string piece;

        while(std::getline(iss, piece, ' ')) {
            const std::string & trimmed { trim(piece) };

            if(! trimmed.empty()) {
                pieces.push_back(piece);
            }
        }
    }

    if(pieces.size() == 2) {
        const std::string & cmd { pieces[1] };

        if(cmd == "create") {
            render_help_create();
        } else if(cmd == "drop") {
            render_help_drop();
        } else if(cmd == "clear") {
            render_help_clear();
        } else if(cmd == "resize") {
            render_help_resize();
        } else if(cmd == "put") {
            render_help_put();
        } else if(cmd == "update") {
            render_help_update();
        } else if(cmd == "query") {
            render_help_query();
        } else {
            std::cerr << "help item: " << cmd << " not found" << std::endl;
        }
    } else if(pieces.size() == 3) {
        const std::string & cmd { pieces[1] };
        if(cmd == "query") {
            const std::string & qtype { pieces[2] };

            if(qtype == "similarity") {
                render_help_query_similarity();
            } else if(qtype == "usimilarity") {
                render_help_query_usimilarity();
            } else if(qtype == "closest") {
                render_help_query_closest();
            } else if(qtype == "matching") {
                render_help_query_matching();
            } else if(qtype == "matchingx") {
                render_help_query_matchingx();
            } else {
                std::cerr << "query type: " << qtype << " not found" << std::endl;
                render_help_query();
            }
        }
    } else {
        render_help_commands_index();
    }
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
        if(sinput.find("help") == 0) {
            parse_help(sinput);
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
