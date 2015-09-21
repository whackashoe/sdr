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
#include <cstdint>
#include <limits>
#include <memory>

#include <stdlib.h>
#include <unistd.h>

#include <libsocket/exception.hpp>
#include <libsocket/unixclientstream.hpp>
#include <libsocket/unixserverstream.hpp>
#include <readline/readline.h>
#include <readline/history.h>

#include "../includes/sdr.hpp"
#include "dbcontainer.hpp"


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

bool running { true };
bool interactive_mode { true };
bool verbose { true };
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

void render_error(const std::string & s, const std::string & piece)
{
    std::cerr
        << (interactive_mode ? ANSI_COLOR_RED : "") << s
        << (interactive_mode ? ANSI_COLOR_YELLOW : "") << " => "
        << (interactive_mode ? ANSI_COLOR_RESET : "") << piece
        << std::endl;
}

std::size_t get_trait_id(const dbcontainer & db_it, const std::string & str)
{
    if(is_number(str)) {
        return std::stoi(str, nullptr);
    } else {
        const auto search = db_it.trait_names.find(str);
        if(search == db_it.trait_names.end()) {
            render_error("trait not found", str);
            return false;
        }

        return search->second;
    }
}

std::size_t get_concept_id(const dbcontainer & db_it, const std::string & str)
{
    if(is_number(str)) {
        return std::stoi(str, nullptr);
    } else {
        const auto search = db_it.concept_names.find(str);
        if(search == db_it.concept_names.end()) {
            render_error("concept not found", str);
            return false;
        }

        return search->second;
    }
}

std::vector<std::size_t> concept_str_to_pos(const dbcontainer & db_it, const std::vector<std::string> & concept_strs)
{
    std::vector<std::size_t> concept_positions;

    for(const auto & i : concept_strs) {
        const std::size_t concept_id { get_concept_id(db_it, i) };
        concept_positions.push_back(concept_id);
    }

    return concept_positions;
}

bool syntax_eq_check(const std::string & a, const std::string & b)
{
    if(a != tolower(b)) {
        std::stringstream ss;
        ss << "expected: " << a << " encountered: " << b;
        render_error("bad syntax", ss.str());
        return false;
    } else {
        return true;
    }
}

bool number_check(const std::string & str)
{
    if(! is_number(str)) {
        render_error("not a number", str);
        return false;
    } else {
        return true;
    }
}

bool argument_length_check_lt(const std::vector<std::string> & pieces, const std::size_t len)
{
    if(pieces.size() < len) {
        std::stringstream ss;
        ss << "expected: " << len << " encountered: " << pieces.size();
        render_error("wrong number of arguments", ss.str());
        return false;
    } else {
        return true;
    }
}

bool argument_length_check_eq(const std::vector<std::string> & pieces, const std::size_t len)
{
    if(pieces.size() != len) {
        std::stringstream ss;
        ss << "expected: " << len << " encountered: " << pieces.size();
        render_error("wrong number of arguments", ss.str());
        return false;
    } else {
        return true;
    }
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

bool create_database(const std::string & name, const std::size_t width)
{
    databases[name] = dbcontainer(name, width);

    // add empty row for 0
    databases[name].bank.insert(sdr::concept({}));

    if(verbose) {
        std::cout << "database " << name << " created" << std::endl;
    }

    return true;
}

bool drop_database(const dbcontainer & db_it)
{
    const std::string & db_name { db_it.name };
    databases.erase(db_name);

    if(verbose) {
        std::cout << "database " << db_name << " dropped" << std::endl;
    }

    return true;
}

bool list(const dbcontainer & db_it)
{
    if(verbose) {
        std::cout
            << "width: " << db_it.bank.get_width() << std::endl
            << "rows:  " << db_it.bank.get_storage_size() << std::endl;
    }

    return true;
}

bool clear(dbcontainer & db_it)
{
    const std::string & db_name { db_it.name };

    db_it.bank.clear();

    if(verbose) {
        std::cout << "database " << db_name << " cleared" << std::endl;
    }

    return true;
}

bool resize(dbcontainer & db_it, const std::size_t width)
{
    const std::string & db_name { db_it.name };

    db_it.bank.resize(width);

    if(verbose) {
        std::cout << "database " << db_name << " resized" << std::endl;
    }

    return true;
}

bool name_trait(dbcontainer & db_it, const std::size_t trait_id, const std::string & name)
{
    db_it.trait_names[name]       = trait_id;
    db_it.trait_names_i[trait_id] = name;

    return true;
}

bool name_concept(dbcontainer & db_it, const std::size_t concept_id, const std::string & name)
{
    db_it.concept_names[name]         = concept_id;
    db_it.concept_names_i[concept_id] = name;

    return true;
}

bool rename_trait(dbcontainer & db_it, const std::string & name, const std::string & new_name)
{
    const auto oldval = db_it.trait_names[name];
    db_it.trait_names.erase(name);
    db_it.trait_names[new_name] = oldval;
    db_it.trait_names_i[oldval] = new_name;

    return true;
}

bool rename_concept(dbcontainer & db_it, const std::string & name, const std::string & new_name)
{
    auto oldval = db_it.trait_names[name];
    db_it.concept_names.erase(name);
    db_it.concept_names[new_name] = oldval;
    db_it.concept_names_i[oldval] = new_name;

    return true;
}

std::size_t insert(dbcontainer & db_it, const std::string & concept_name, const std::vector<std::size_t> & concept_positions)
{
    const auto t_start = since_epoch();

    const sdr::position_t position { db_it.bank.insert(sdr::concept(concept_positions)) };

    if(! concept_name.empty()) {
        db_it.concept_names[concept_name] = position;
    }

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    if(verbose) {
        std::cout << "OK, concept " << position << " created (" << t_ms << "μs)" << std::endl;
    }

    return position;
}

bool update(dbcontainer & db_it, const std::size_t concept_id, const std::vector<std::size_t> & concept_positions)
{
    const auto t_start = since_epoch();

    db_it.bank.update(concept_id, sdr::concept(concept_positions));

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    if(verbose) {
        std::cout << "OK, concept " << concept_id << " affected (" << t_ms << "μs)" << std::endl;
    }

    return true;
}

std::size_t similarity(const dbcontainer & db_it, const std::size_t concept_a_id, const std::size_t concept_b_id)
{
    const auto t_start = since_epoch();

    const std::size_t result { db_it.bank.similarity(concept_a_id, concept_b_id) };

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    if(verbose) {
        std::cout << result << std::endl;
        std::cout << "OK, 2 concepts compared (" << t_ms << "μs)" << std::endl;
    }

    return result;
}

std::size_t usimilarity(const dbcontainer & db_it, const std::size_t concept_id, const std::vector<std::size_t> & concept_positions)
{
    const auto t_start = since_epoch();

    const std::size_t result { db_it.bank.union_similarity(concept_id, concept_positions) };

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    if(verbose) {
        std::cout << result << std::endl;
        std::cout << "OK, " << (1 + concept_positions.size()) << " concepts compared (" << t_ms << "μs)" << std::endl;
    }

    return result;
}

std::vector<std::pair<std::size_t, std::size_t>> closest(const dbcontainer & db_it, const std::size_t amount, const std::size_t concept_id)
{
    const auto t_start = since_epoch();

    const std::vector<std::pair<std::size_t, std::size_t>> results { db_it.bank.closest(concept_id, amount) };

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    if(verbose) {
        std::cout << "[";
        for(std::size_t i=0; i<results.size(); ++i) {
            const std::pair<std::size_t, std::size_t> item { results[i] };

            std::cout << "[" << item.first << ":" << item.second << "]";
            if(i != results.size() - 1) {
                std::cout << ",";
            }
        }
        std::cout << "]" << std::endl;

        std::cout << "OK, " << amount << " concepts sorted (" << t_ms << "μs)" << std::endl;
    }

    return results;
}

std::vector<std::size_t> matching(const dbcontainer & db_it, const std::vector<std::size_t> & concept_positions)
{
    const auto t_start = since_epoch();

    const std::vector<std::size_t> results { db_it.bank.matching(sdr::concept(concept_positions)) };

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    if(verbose) {
        std::cout << "[";
        for(std::size_t i=0; i<results.size(); ++i) {
            const std::size_t item { results[i] };

            std::cout << item;
            if(i != results.size() - 1) {
                std::cout << ",";
            }
        }
        std::cout << "]" << std::endl;

        std::cout << "OK, " << (results.size()) << " concepts matched (" << t_ms << "μs)" << std::endl;
    }

    return results;
}

std::vector<std::size_t> matchingx(const dbcontainer & db_it, const std::size_t amount, const std::vector<std::size_t> & concept_positions)
{
    const auto t_start = since_epoch();

    const std::vector<std::size_t> results { db_it.bank.matching(sdr::concept(concept_positions), amount) };

    const auto t_end = since_epoch();
    const auto t_ms = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();

    if(verbose) {
        std::cout << "[";
        for(std::size_t i=0; i<results.size(); ++i) {
            const std::size_t item { results[i] };

            std::cout << item;
            if(i != results.size() - 1) {
                std::cout << ",";
            }
        }
        std::cout << "]" << std::endl;;

        std::cout << "OK, " << (results.size()) << " concepts matched (" << t_ms << "μs)" << std::endl;
    }

    return results;
}

void parse_input(const std::string & input)
{
    std::istringstream iss(input);

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

    // empty input shouldnt be passed here to start with
    // but.. how can we trust that?
    if(pieces.size() == 0) {
        return;
    }

    // comment
    if(pieces[0][0] == '#') {
        return;
    }

    const std::string & command { tolower(pieces[0]) };

    if(command == "help") {
        render_help();
    } else if(command == "create") {
        if(! argument_length_check_eq(pieces, 4)) {
            return;
        }

        if(! syntax_eq_check("database", pieces[1])) {
            return;
        }

        const std::string & db_name  { pieces[2] };
        const std::string & db_width { pieces[3] };
        if(! number_check(db_width)) {
            return;
        }

        const std::size_t width { static_cast<std::size_t>(std::stoi(db_width, nullptr)) };

        create_database(db_name, width);
    } else if(command == "drop") {
        if(! argument_length_check_eq(pieces, 3)) {
            return;
        }

        if(! syntax_eq_check("database", pieces[1])) {
            return;
        }

        const std::string & db_name { pieces[2] };

        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }

        drop_database(db->second);
    } else if(command == "list") {
        if(! argument_length_check_eq(pieces, 2)) {
            return;
        }

        const std::string & db_name { pieces[1] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }

        list(db->second);
    } else if(command == "clear") {
        if(! argument_length_check_eq(pieces, 2)) {
            return;
        }

        const std::string & db_name { pieces[1] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }

        clear(db->second);
     } else if(command == "resize") {
        if(! argument_length_check_eq(pieces, 3)) {
            return;
        }

        const std::string & db_name { pieces[1] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }

        const std::string & db_width { pieces[2] };
        if(! number_check(db_width)) {
            return;
        }

        const std::size_t width { static_cast<std::size_t>(std::stoi(db_width, nullptr)) };

        resize(db->second, width);
    } else if(command == "name") {
        if(! argument_length_check_eq(pieces, 6)) {
            return;
        }

        if(! syntax_eq_check("in", pieces[4])) {
            return;
        }

        const std::string & ttype { tolower(pieces[1]) };
        const bool is_trait       { ttype == "trait" };
        const bool is_concept     { ttype == "concept" };

        if(! is_trait && ! is_concept) {
             render_error("bad syntax", pieces[1]);
             return;
        }

        const std::string & item_pos { pieces[2] };
        if(! number_check(item_pos)) {
            return;
        }

        const std::string & item_name  { pieces[3] };
        const std::string & db_name    { pieces[5] };

        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }
        dbcontainer & db_it { db->second };

        if(is_trait) {
            const std::size_t trait_id  { get_trait_id(db_it, item_pos) };
            if(! trait_id) {
                render_error("trait not found", item_name);
                return;
            }

            name_trait(db_it, trait_id, item_name);
        }

        if(is_concept) {
            const std::size_t concept_id  { get_concept_id(db_it, item_pos) };
            if(! concept_id) {
                render_error("concept not found", item_name);
                return;
            }

            name_concept(db_it, concept_id, item_name);
        }
     } else if(command == "rename") {
        if(! argument_length_check_eq(pieces, 6)) {
            return;
        }

        if(! syntax_eq_check("in", pieces[4])) {
            return;
        }

        const std::string & ttype { tolower(pieces[1]) };
        const bool is_trait       { ttype == "trait" };
        const bool is_concept     { ttype == "concept" };

        if(! is_trait && ! is_concept) {
            render_error("bad syntax", ttype);
            return;
        }

        const std::string & item_name    { pieces[2] };
        const std::string & item_newname { pieces[3] };
        const std::string & db_name      { pieces[5] };

        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }
        dbcontainer & db_it { db->second };

        if(is_trait) {
            if(! get_trait_id(db_it, item_name)) {
                render_error("trait not found", item_name);
                return;
            }

            rename_trait(db_it, item_name, item_newname);
        }

        if(is_concept) {
            if(! get_concept_id(db_it, item_name)) {
                render_error("concept not found", item_name);
                return;
            }

            rename_concept(db_it, item_name, item_newname);
        }
     } else if(command == "put") {
        if(! argument_length_check_lt(pieces, 4)) {
            return;
        }

        if(! syntax_eq_check("into", pieces[1])) {
            return;
        }

        const std::string & db_name { pieces[2] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }
        dbcontainer & db_it { db->second };

        const bool named                 { tolower(pieces[3]) == "as" };
        const std::string & concept_name { named ? pieces[4] : "" };

        if(named && ! argument_length_check_lt(pieces, 5)) {
            render_error("wrong number of arguments", command);
            return;
        }

        const std::vector<std::string> concept_strs(std::begin(pieces) + (named ? 5 : 3), std::end(pieces));;
        const std::vector<std::size_t> concept_positions { concept_str_to_pos(db_it, concept_strs) };

        insert(db_it, concept_name, concept_positions);
     }  else if(command == "update") {
        if(   ! argument_length_check_lt(pieces, 4)
           || ! syntax_eq_check("concept", pieces[1])
           || ! syntax_eq_check("from", pieces[3]))
        {
            return;
        }

        const std::string & db_name { pieces[4] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }
        dbcontainer & db_it { db->second };

        const std::string & concept_str  { pieces[2] };
        const std::string & concept_name { concept_str };
        const std::size_t   concept_id   { get_concept_id(db_it, concept_name) };
        if(! concept_id) {
            render_error("concept not found", concept_str);
            return;
        }
        const std::vector<std::string> concept_strs(std::begin(pieces) + 5, std::end(pieces));
        const std::vector<std::size_t> concept_positions { concept_str_to_pos(db_it, concept_strs) };

        update(db_it, concept_id, concept_positions);
     } else if(command == "query") {
        if(! argument_length_check_lt(pieces, 4)) {
            return;
        }

        const bool weighted         { tolower(pieces[1]) == "weighted" };
        const std::size_t qtype_pos { static_cast<std::size_t>(weighted ? 2 : 1) };

        const std::string & qtype { pieces[qtype_pos] };
        if(! syntax_eq_check("from", pieces[qtype_pos + 1])) {
            return;
        }

        const std::string & db_name { pieces[qtype_pos + 2] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            render_error("database not found", db_name);
            return;
        }
        const dbcontainer & db_it { db->second };

        if(qtype == "similarity") {
            if(! argument_length_check_eq(pieces, qtype_pos + 5)) {
                return;
            }

            const std::string & concept_a_str { pieces[qtype_pos + 3] };
            const std::size_t   concept_a_id  { get_concept_id(db_it, concept_a_str) };
            if(! concept_a_id) {
                render_error("concept not found", concept_a_str);
                return;
            }

            const std::string & concept_b_str { pieces[qtype_pos + 4] };
            const std::size_t   concept_b_id  { get_concept_id(db_it, concept_b_str) };
            if(! concept_b_id) {
                render_error("concept not found", concept_b_str);
                return;
            }

            similarity(db_it, concept_a_id, concept_b_id);
        } else if(qtype == "usimilarity") {
            if(! argument_length_check_lt(pieces, qtype_pos + 5)) {
                return;
            }

            const std::string & concept_str { pieces[qtype_pos + 3] };
            const std::size_t   concept_id  { get_concept_id(db_it, concept_str) };
            if(! concept_id) {
                render_error("concept not found", concept_str);
                return;
            }
            const std::vector<std::string> concept_strs(std::begin(pieces) + qtype_pos + 4, std::end(pieces));
            const std::vector<std::size_t> concept_positions { concept_str_to_pos(db_it, concept_strs) };

            usimilarity(db_it, concept_id, concept_positions);
        } else if(qtype == "closest") {
            if(! argument_length_check_eq(pieces, qtype_pos + 5)) {
                return;
            }

            const std::string & amount_str { pieces[qtype_pos + 3] };
            if(! number_check(amount_str)) {
                return;
            }

            const std::size_t   amount      { static_cast<std::size_t>(std::stoi(amount_str)) };
            const std::string & concept_str { pieces[qtype_pos + 4] };
            const std::size_t   concept_id  { get_concept_id(db_it, concept_str) };
            if(! concept_id) {
                render_error("concept not found", concept_str);
                return;
            }

            closest(db_it, amount, concept_id);
        } else if(qtype == "matching") {
            if(weighted) {
                render_error("matching cannot be weighted", pieces[1]);
                return;
            }

            if(! argument_length_check_lt(pieces, qtype_pos + 4)) {
                return;
            }

            const std::vector<std::string> concept_strs(std::begin(pieces) + qtype_pos + 3, std::end(pieces));
            const std::vector<std::size_t> concept_positions { concept_str_to_pos(db_it, concept_strs) };


            matching(db_it, concept_positions);
        } else if(qtype == "matchingx") {
            if(weighted) {
                render_error("matchingx cannot be weighted", pieces[1]);
                return;
            }

            if(  ! argument_length_check_lt(pieces, qtype_pos + 5)
              || ! number_check(pieces[qtype_pos + 3]))
            {
                return;
            }

            const std::size_t amount { static_cast<std::size_t>(std::stoi(pieces[qtype_pos + 3], nullptr)) };
            const std::vector<std::string> concept_strs(std::begin(pieces) + qtype_pos + 4, std::end(pieces));
            const std::vector<std::size_t> concept_positions { concept_str_to_pos(db_it, concept_strs) };

            matchingx(db_it, amount, concept_positions);
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
    const char * input = readline(interactive_mode ? "sdrdb" ANSI_COLOR_CYAN "> " ANSI_COLOR_RESET : "");

    if(input == nullptr) {
        return false;
    }

    std::string sinput(input);
    sinput = trim(sinput);

    if(sinput.length() > 0) {
        if(sinput.length() == 1) {
            if(sinput == "help") {
                render_help();
            } else if(sinput == "exit") {
                exit_console();
            }
        } else {
            parse_input(sinput);
        }

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
        << "-h            : show this help" << std::endl
        << "-v            : show version" << std::endl
        << "-b arg        : set bindpath for server" << std::endl
        << "-s            : set server mode" << std::endl;

}

void display_version()
{
    std::cout << "sdrdb " << SDRDB_VERSION << std::endl;

}

int serverloop(const std::string & bindpath)
{
    constexpr std::size_t buffer_size { 128 };

    try {
        libsocket::unix_stream_server server(bindpath);

        while(true) {
            std::unique_ptr<libsocket::unix_stream_client> client(server.accept());

            while(true) {
                std::string answer;
                answer.resize(buffer_size);
                *client >> answer;

                std::cout << answer;

                for(char c : answer) {
                    if(c == 0) {
                        goto breaker;
                    }
                }
            } breaker:;

            *client << "Hello back from server!\n";
        }

        server.destroy();
    } catch (const libsocket::socket_exception& exc) {
        std::cerr << exc.mesg << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char ** argv)
{
    interactive_mode = isatty(STDIN_FILENO);

    bool server { false };
    std::string bindpath { "/tmp/sdrdb.sock" };
    {
        int c;

        while ((c = getopt (argc, argv, "vhsb:")) != -1) {
            switch (c) {
            case 'v':
                display_version();
                return EXIT_SUCCESS;
                break;
            case 'h':
                display_usage();
                return EXIT_SUCCESS;
                break;
            case 's':
                server = true;
                interactive_mode = false;
                break;
            case 'b':
                bindpath = optarg;
                break;
            case '?':
                display_usage();
                return EXIT_FAILURE;
            default:
                abort ();
             }
         }
     }

    if(server) {
        return serverloop(bindpath);
    }

    if(interactive_mode) {
        std::cout
            << "Welcome to the sdrdb interactive console. Commands end with \\n" << std::endl
            << "Server version: " << SDRDB_VERSION << std::endl
            << std::endl
            << "Type 'help' for help." << std::endl
            << std::endl;
    }

    while(running && inputloop());

    return EXIT_SUCCESS;
}
