#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <utility>
#include <libsocket/inetserverstream.hpp>
#include <libsocket/exception.hpp>
#include <libsocket/socket.hpp>
#include <libsocket/select.hpp>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>

#include "../includes/sdr.hpp"
#include "dbcontainer.hpp"


#ifndef VERSION
#define VERSION "0.1-alpha"
#endif

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


bool running { true };
bool interactive_mode { true };
std::unordered_map<std::string, dbcontainer> databases;


std::chrono::system_clock::duration since_epoch()
{
    return std::chrono::system_clock::now().time_since_epoch();
}

// trim from start
std::string & ltrim(std::string & s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

bool is_number(const std::string & s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

// trim from end
std::string & rtrim(std::string & s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
std::string & trim(std::string & s)
{
    return ltrim(rtrim(s));
}

std::string tolower(const std::string & s)
{
    std::string ret { s };

    std::transform(ret.begin(), ret.end(), ret.begin(), [](const int c) {
        if(c >= 'A' && c <= 'Z') {
            return c - 'A' + 'a';
        }

        return c;
    });

    return ret;
}

std::size_t get_concept_id(const decltype(databases)::iterator dbsearch, const std::string & str)
{
    return is_number(str)
        ? std::stoi(str, nullptr)
        : (dbsearch->second).concept_names[str];
}

std::vector<std::size_t> concept_str_to_pos(const decltype(databases)::iterator dbsearch, const std::vector<std::string> & concept_strs)
{
    std::vector<std::size_t> concept_positions;

    for(const auto & i : concept_strs) {
        concept_positions.push_back(get_concept_id(dbsearch, i));
    }

    return concept_positions;
}

void render_error(const std::string & s, const std::string & piece)
{
    std::cerr
        << (interactive_mode ? ANSI_COLOR_RED : "") << s
        << (interactive_mode ? ANSI_COLOR_YELLOW : "") << " => "
        << (interactive_mode ? ANSI_COLOR_RESET : "") << piece
        << std::endl
        << std::endl;
}

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
    running = false;
}

bool create_database(const std::string & dbname, const std::string & dbwidth)
{
    const std::size_t width { static_cast<std::size_t>(std::stoi(dbwidth, nullptr)) };

    databases[dbname] = dbcontainer(dbname, width);

    std::cout << "database " << dbname << " created" << std::endl;

    return true;
}

bool drop_database(const std::string & dbname)
{
    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    databases.erase(dbname);

    std::cout << "database " << dbname << " dropped" << std::endl;

    return true;
}

bool list(const std::string & dbname)
{
    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    (dbsearch->second).render_list();

    return true;
}

bool clear(const std::string & dbname)
{
    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    (dbsearch->second).bank.clear();

    std::cout << "database " << dbname << " cleared" << std::endl;

    return true;
}

bool resize(const std::string & dbname, const std::string & dbwidth)
{
    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    const std::size_t width = std::stoi(dbwidth, nullptr);

    (dbsearch->second).bank.resize(width);

    std::cout << "database " << dbname << " resized" << std::endl;

    return true;
}

bool name_trait(const std::string & dbname, const std::string & traitpos, const std::string & traitname)
{
    const std::size_t pos { static_cast<std::size_t>(std::stoi(traitpos, nullptr)) };


    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    (dbsearch->second).trait_names[traitname] = pos;
    (dbsearch->second).trait_names_i[pos] = traitname;

    return true;
}

bool name_concept(const std::string & dbname, const std::string & conceptpos, const std::string & conceptname)
{
    const std::size_t pos { static_cast<std::size_t>(std::stoi(conceptpos, nullptr)) };

    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    (dbsearch->second).concept_names[conceptname] = pos;
    (dbsearch->second).concept_names_i[pos] = conceptname;

    return true;
}

bool rename_trait(const std::string & dbname, const std::string & traitname, const std::string & traitnewname)
{
    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    const auto traitsearch = (dbsearch->second).trait_names.find(traitname);
    if(traitsearch == (dbsearch->second).trait_names.end()) {
        render_error("trait not found", traitname);
        return false;
    }

    const auto oldval = (dbsearch->second).trait_names[traitname];
    (dbsearch->second).trait_names.erase(traitname);
    (dbsearch->second).trait_names[traitnewname] = oldval;
    (dbsearch->second).trait_names_i[oldval] = traitnewname;

    return true;
}

bool rename_concept(const std::string & dbname, const std::string & conceptname, const std::string & conceptnewname)
{
    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    const auto conceptsearch = (dbsearch->second).concept_names.find(conceptname);
    if(conceptsearch == (dbsearch->second).concept_names.end()) {
        render_error("concept not found", conceptname);
        return false;
    }

    auto oldval = (dbsearch->second).trait_names[conceptname];
    (dbsearch->second).concept_names.erase(conceptname);
    (dbsearch->second).concept_names[conceptnewname] = oldval;
    (dbsearch->second).concept_names_i[oldval] = conceptnewname;

    return true;
}

bool insert(const std::string & dbname, const std::string & conceptname, const std::vector<std::string> & concept_strs)
{
    const auto t_start = since_epoch();

    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    const std::vector<std::size_t> concept_positions { concept_str_to_pos(dbsearch, concept_strs) };
    const sdr::position_t position { (dbsearch->second).bank.insert(sdr::concept(concept_positions)) };

    if(! conceptname.empty()) {
        (dbsearch->second).concept_names[conceptname] = position;
    }

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    std::cout << "Query OK, 1 row affected (" << t_ms << "μs)" << std::endl;

    return true;
}

bool update(const decltype(databases)::iterator dbsearch, const std::size_t concept_id, const std::vector<std::string> & concept_strs)
{
    const auto t_start = since_epoch();

    const std::vector<std::size_t> concept_positions { concept_str_to_pos(dbsearch, concept_strs) };

    (dbsearch->second).bank.update(concept_id, sdr::concept(concept_positions));

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    std::cout << "Query OK, 1 row affected (" << t_ms << "μs)" << std::endl;

    return true;
}

std::size_t similarity(const std::string & dbname, const std::string & concept_astr, const std::string & concept_bstr)
{
    const auto t_start = since_epoch();

    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    const std::size_t concept_a_id { get_concept_id(dbsearch, concept_astr) };
    const std::size_t concept_b_id { get_concept_id(dbsearch, concept_bstr) };

    const std::size_t result { (dbsearch->second).bank.similarity(concept_a_id, concept_b_id) };


    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    std::cout << "Similarity: " << result << std::endl;
    std::cout << "Query OK, 2 concepts compared (" << t_ms << "μs)" << std::endl;

    return result;
}

std::size_t usimilarity(const std::string & dbname, const std::string & concept_str, const std::vector<std::string> & concept_strs)
{
    const auto t_start = since_epoch();

    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return false;
    }

    const std::size_t concept_id { get_concept_id(dbsearch, concept_str) };
    const std::vector<std::size_t> concept_positions { concept_str_to_pos(dbsearch, concept_strs) };
    const std::size_t result { (dbsearch->second).bank.union_similarity(concept_id, concept_positions) };


    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    std::cout << "Union Similarity: " << result << std::endl;
    std::cout << "Query OK, " << (1 + concept_positions.size()) << " concepts compared (" << t_ms << "μs)" << std::endl;

    return result;
}

std::vector<std::pair<std::size_t, std::size_t>> closest(const std::string & dbname, const std::size_t amount, const std::string & concept_str)
{
    const auto t_start = since_epoch();

    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return {};
    }

    const std::size_t concept_id { get_concept_id(dbsearch, concept_str) };
    const std::vector<std::pair<std::size_t, std::size_t>> results { (dbsearch->second).bank.closest(concept_id, amount) };

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    for(const auto i : results) {
        std::cout << i.first << "\t::\t" << i.second << std::endl;
    }
    std::cout << "Query OK, " << amount << " concepts sorted (" << t_ms << "μs)" << std::endl;

    return results;
}

std::vector<std::size_t> matching(const std::string & dbname, const std::vector<std::string> & concept_strs)
{
    const auto t_start = since_epoch();

    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return {};
    }

    const std::vector<std::size_t> concept_positions { concept_str_to_pos(dbsearch, concept_strs) };
    const std::vector<std::size_t> results { (dbsearch->second).bank.matching(sdr::concept(concept_positions)) };

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    for(const auto i : results) {
        std::cout << i << std::endl;
    }

    std::cout << "Query OK, " << (results.size()) << " concepts matched (" << t_ms << "μs)" << std::endl;

    return results;
}

std::vector<std::size_t> matchingx(const std::string & dbname, const std::size_t amount, const std::vector<std::string> & concept_strs)
{
    const auto t_start = since_epoch();

    const auto dbsearch = databases.find(dbname);
    if(dbsearch == databases.end()) {
        render_error("database not found", dbname);
        return {};
    }

    const std::vector<std::size_t> concept_positions { concept_str_to_pos(dbsearch, concept_strs) };
    const std::vector<std::size_t> results { (dbsearch->second).bank.matching(sdr::concept(concept_positions), amount) };

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    for(const auto i : results) {
        std::cout << i << std::endl;
    }

    std::cout << "Query OK, " << (results.size()) << " concepts matched (" << t_ms << "μs)" << std::endl;

    return results;
}

void parse_input(const std::string & input)
{
    std::istringstream iss(input);

    std::vector<std::string> pieces;

    {
        std::string piece;

        while(std::getline(iss, piece, ' ')) {
            pieces.push_back(piece);
        }
    }

    const std::string command { tolower(pieces[0]) };


    if(command == "help") {
        render_help();
    } else if(command == "exit") {
        exit_console();
    } else if(command == "create") {
        if(pieces.size() != 4) {
            render_error("wrong number of arguments", command);
            return;
        }

        if(tolower(pieces[1]) != "database") {
            render_error("bad syntax", pieces[1]);
            return;
        }

        const std::string dbname  { pieces[2] };
        const std::string dbwidth { pieces[3] };

        create_database(dbname, dbwidth);
    } else if(command == "drop") {
        if(pieces.size() != 3) {
            render_error("wrong number of arguments", command);
            return;
        }

        if(tolower(pieces[1]) != "database") {
            render_error("bad syntax", pieces[1]);
            return;
        }

        const std::string dbname { pieces[2] };

        drop_database(dbname);
    } else if(command == "list") {
        if(pieces.size() != 2) {
            render_error("wrong number of arguments", command);
            return;
        }

        const std::string dbname { pieces[1] };

        list(dbname);
    } else if(command == "clear") {
        if(pieces.size() != 2) {
            render_error("wrong number of arguments", command);
            return;
        }

        const std::string dbname { pieces[1] };

        clear(dbname);
     } else if(command == "resize") {
        if(pieces.size() != 3) {
            render_error("bad syntax", command);
            return;
        }

        const std::string dbname  { pieces[1] };
        const std::string dbwidth { pieces[2] };

        resize(dbname, dbwidth);
    } else if(command == "name") {
        if(pieces.size() != 6) {
            render_error("wrong number of arguments", command);
            return;
        }

        if(tolower(pieces[4]) != "in") {
            render_error("bad syntax", pieces[4]);
            return;
        }

        const std::string & ttype { tolower(pieces[1]) };
        const bool is_trait { ttype == "trait" };
        const bool is_concept { ttype == "concept" };

        if(! is_trait && ! is_concept) {
             render_error("bad syntax", pieces[1]);
             return;
        }

        const std::string itempos  { pieces[2] };
        const std::string itemname { pieces[3] };
        const std::string dbname   { pieces[5] };

        if(is_trait) {
            name_trait(dbname, itempos, itemname);
        }

        if(is_concept) {
            name_concept(dbname, itempos, itemname);
        }
     } else if(command == "rename") {
        if(pieces.size() != 6) {
            render_error("wrong number of arguments", command);
            return;
        }

        if(tolower(pieces[4]) != "in") {
            render_error("bad syntax", pieces[4]);
            return;
        }

        const std::string & ttype { tolower(pieces[1]) };

        const bool is_trait { ttype == "trait" };
        const bool is_concept { ttype == "concept" };

        if(! is_trait && ! is_concept) {
            render_error("bad syntax", ttype);
            return;
        }

        const std::string itemname    { pieces[2] };
        const std::string itemnewname { pieces[3] };
        const std::string dbname      { pieces[5] };

        if(is_trait) {
            rename_trait(dbname, itemname, itemnewname);
        }

        if(is_concept) {
            rename_concept(dbname, itemname, itemnewname);
        }
     } else if(command == "put") {
        if(pieces.size() < 4) {
            render_error("wrong number of arguments", command);
            return;
        }

        if(tolower(pieces[1]) != "into") {
            render_error("bad syntax", pieces[1]);
            return;
        }

        const std::string dbname { pieces[2] };

        std::string conceptname;
        bool named { false };

        if(tolower(pieces[3]) == "as") {
            if(pieces.size() < 5) {
                render_error("wrong number of arguments", command);
                return;
            }

            named = true;
            conceptname = pieces[4];
        }

        const std::vector<std::string> data(std::begin(pieces) + (named ? 5 : 3), std::end(pieces));;

        insert(dbname, conceptname, data);
     }  else if(command == "update") {
        if(pieces.size() < 4) {
            render_error("wrong number of arguments", command);
            return;
        }

        if(tolower(pieces[1]) != "concept") {
            render_error("bad syntax", pieces[1]);
            return;
        }

        if(tolower(pieces[3]) != "from") {
            render_error("bad syntax", pieces[3]);
            return;
        }

        const std::string dbname { pieces[4] };
        auto dbsearch = databases.find(dbname);
        if(dbsearch == databases.end()) {
            render_error("database not found", dbname);
            return;
        }

        const std::string conceptstr { pieces[2] };
        const std::string conceptname { conceptstr };
        const bool is_index { is_number(conceptstr) };

        if(! is_index) {
            auto conceptsearch = (dbsearch->second).concept_names.find(conceptstr);

            if(conceptsearch == (dbsearch->second).concept_names.end()) {
                render_error("concept not found", conceptname);
                return;
            }
        }

        const std::size_t concept_id {
            is_index
                ? static_cast<std::size_t>(std::stoi(conceptname, nullptr))
                : (dbsearch->second).concept_names[conceptname]
        };

        const std::vector<std::string> data(std::begin(pieces) + 5, std::end(pieces));

        update(dbsearch, concept_id, data);
     } else if(command == "query") {
        if(pieces.size() < 4) {
            render_error("wrong number of arguments", command);
            return;
        }

        bool weighted = false;
        std::size_t qtype_pos = 1;

        if(tolower(pieces[1]) == "weighted") {
            weighted = true;
            qtype_pos = 2;
        }


        const std::string qtype = pieces[qtype_pos];
        if(tolower(pieces[qtype_pos + 1]) != "from") {
            render_error("bad syntax", pieces[qtype_pos + 1]);
            return;
        }

        const std::string dbname { pieces[qtype_pos + 2] };

        if(qtype == "similarity") {
            if(pieces.size() != qtype_pos + 5) {
                render_error("bad syntax", command);
                return;
            }

            const std::string concept_a { pieces[qtype_pos + 3] };
            const std::string concept_b { pieces[qtype_pos + 4] };

            similarity(dbname, concept_a, concept_b);
        } else if(qtype == "usimilarity") {
            if(pieces.size() < qtype_pos + 5) {
                render_error("wrong number of arguments", command);
                return;
            }

            const std::string concept { pieces[qtype_pos + 3] };
            const std::vector<std::string> data(std::begin(pieces) + qtype_pos + 4, std::end(pieces));

            usimilarity(dbname, concept, data);
        } else if(qtype == "closest") {
            if(pieces.size() != qtype_pos + 5) {
                render_error("wrong number of arguments", command);
                return;
            }

            const std::size_t amount { static_cast<std::size_t>(std::stoi(pieces[qtype_pos + 3])) };
            const std::string concept { pieces[qtype_pos + 4] };

            closest(dbname, amount, concept);
        } else if(qtype == "matching") {
            if(weighted) {
                render_error("bad syntax", pieces[1]);
                return;
            }

            if(pieces.size() < qtype_pos + 4) {
                render_error("wrong number of arguments", command);
                return;
            }

            const std::vector<std::string> data(std::begin(pieces) + qtype_pos + 3, std::end(pieces));

            matching(dbname, data);
        } else if(qtype == "matchingx") {
            if(weighted) {
                render_error("bad syntax", pieces[1]);
                return;
            }

            if(pieces.size() < qtype_pos + 5) {
                render_error("wrong number of arguments", command);
                return;
            }

            const std::size_t amount { static_cast<std::size_t>(std::stoi(pieces[qtype_pos + 3], nullptr)) };
            const std::vector<std::string> data(std::begin(pieces) + qtype_pos + 4, std::end(pieces));

            matchingx(dbname, amount, data);
        } else {
            render_error("bad syntax", command);
        }
     } else if(command == "save") {

     } else {
        render_error("unknown command", command);
        return;
    }
}

bool inputloop()
{
    char * input = readline(interactive_mode ? "sdrdb" ANSI_COLOR_CYAN "> " ANSI_COLOR_RESET : "");

    if(input == nullptr) {
        return false;
    }

    std::string sinput(input);
    sinput = trim(sinput);

    if(sinput.length() > 0) {
        parse_input(sinput);

        if(interactive_mode) {
            add_history(input);
        }
    }

    return true;
}

void error(const char * msg)
{
    std::cerr << msg << std::endl;
    exit(1);
}

void display_usage()
{
    std::cout
        << "Usage: sdrdb [OPTION]..." << std::endl
        << "Options and arguments:" << std::endl
        << "-p arg        : set port number" << std::endl
        << "-h            : show this help" << std::endl;

}

void display_version()
{
    std::cout << "sdrdb " << VERSION << std::endl;

}

void serverloop(const std::string & host, const std::string & port)
{
    try {
        libsocket::inet_stream_server srv(host,port,LIBSOCKET_IPv6);

        libsocket::selectset<libsocket::inet_stream_server> set1;
        set1.add_fd(srv,LIBSOCKET_READ);

        while(true) {
            // Create pair (libsocket::fd_struct is the return type of selectset::wait()
            libsocket::selectset<libsocket::inet_stream_server>::ready_socks readypair;

            // Wait for a connection and save the pair to the var
            readypair = set1.wait();

            // Get the last fd of the LIBSOCKET_READ vector (.first) of the pair and cast the socket* to inet_stream_server*
            libsocket::inet_stream_server * ready_srv { dynamic_cast<libsocket::inet_stream_server*>(readypair.first.back()) };

            // delete the fd from the pair
            readypair.first.pop_back();
            std::cout << "Ready for accepting\n";


            libsocket::inet_stream * cl1 { ready_srv->accept() };
            *cl1 << "Hello\n";

            std::string answ(100, 0);
            *cl1 >> answ;
            std::cout << answ;

            cl1->destroy();
        }

        srv.destroy();
    } catch (const libsocket::socket_exception& exc) {
        std::cerr << exc.mesg << std::endl;
    }
}

int main(int argc, char ** argv)
{
    interactive_mode = isatty(STDIN_FILENO);

    std::string host { "::1" };
    std::string port { "8888" };
    bool server { false };

    int c;

    while ((c = getopt (argc, argv, "vsh:p:")) != -1) {
        switch (c) {
        case 'v':
            display_version();
            return EXIT_SUCCESS;
            break;
        case 'h':
            host = optarg;
            break;
        case 's':
            server = true;
            break;
        case 'p':
            port = optarg;
            break;
        case '?':
            display_usage();
            return EXIT_FAILURE;
        default:
            abort ();
         }
     }

    if(server) {
        serverloop(host, port);
        return EXIT_SUCCESS;
    }

    if(interactive_mode) {
        std::cout
            << "Welcome to the sdrdb interactive console. Commands end with \\n" << std::endl
            << "Server version: " << VERSION << std::endl
            << std::endl
            << "Type 'help' for help." << std::endl
            << std::endl;
    }

    while(running && inputloop());

    return EXIT_SUCCESS;
}