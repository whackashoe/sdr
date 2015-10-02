#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <chrono>
#include <utility>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <cstddef>
#include <stdexcept>

#include <stdlib.h>
#include <unistd.h>

#include <sparsehash/dense_hash_map>
#include <libsocket/exception.hpp>
#include <libsocket/unixclientstream.hpp>
#include <libsocket/unixserverstream.hpp>

#include "../../includes/sdr.hpp"
#include "db_container.hpp"
#include "result_container.hpp"
#include "check_result.hpp"
#include "number_container.hpp"

#ifndef SDRDB_VERSION
#define SDRDB_VERSION "0.1-alpha"
#endif


bool verbose { false };
template <typename T, typename U>
using hash_map = google::dense_hash_map<T, U, std::hash<T>>;

hash_map<std::string, db_container> databases;

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

bool is_number(const std::string & s)
{
    std::string::const_iterator it = s.begin();

    while (it != s.end() && std::isdigit(*it)) {
        ++it;
    }
    return ! s.empty() && it == s.end();
}

std::string trim(const std::string & s)
{
    std::string ret { s };
    ret.erase(ret.begin(), std::find_if(ret.begin(), ret.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    ret.erase(std::find_if(ret.rbegin(), ret.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), ret.end());
    return ret;
}

std::vector<std::size_t> unpack(const std::vector<number_container> & v)
{
    std::vector<std::size_t> ret;
    ret.reserve(v.size());

    for(auto & i : v) {
        ret.push_back(i.get_n());
    }

    return ret;
}

result_container render_error(const std::string & s, const std::string & piece)
{
    std::stringstream ss;
    ss  << "ERR: " <<  s << " => " << piece << std::endl;

    const std::string rstr { ss.str() };

    if(verbose) {
        std::cerr << rstr;
    }

    return result_container(rstr);
}

check_result syntax_eq_check(const std::string & a, const std::string & b)
{
    if(a != tolower(b)) {
        std::stringstream ss;
        ss << "expected: " << a << " encountered: " << b;
        return check_result(render_error("bad syntax", ss.str()));
    } else {
        return check_result(true);
    }
}

check_result number_check(const std::string & str)
{
    if(! is_number(str)) {
        return check_result(render_error("not a number", str));
    } else {
        return check_result(true);
    }
}

check_result argument_length_check_lt(const std::vector<std::string> & pieces, const std::size_t len)
{
    if(pieces.size() < len) {
        std::stringstream ss;
        ss << "expected: " << len << " encountered: " << pieces.size();
        return check_result(render_error("wrong number of arguments", ss.str()));
    } else {
        return check_result(true);
    }
}

check_result argument_length_check_eq(const std::vector<std::string> & pieces, const std::size_t len)
{
    if(pieces.size() != len) {
        std::stringstream ss;
        ss << "expected: " << len << " encountered: " << pieces.size();
        return check_result(render_error("wrong number of arguments", ss.str()));
    } else {
        return check_result(true);
    }
}

check_result positions_smaller_than_width_check(const db_container & db_it, const std::vector<number_container> & positions)
{
    for(const number_container pos : positions) {
        if(pos.get_n() >= db_it.get_width()) {
            return check_result(render_error("position too large for db", pos.get_s()));
        }
    }

    return check_result(true);
}

bool create_database(const std::string & name, const std::size_t width)
{
    databases[name] = db_container(name, width);

    // add empty row for 0
    databases[name].bank.insert(sdr::concept({}));

    if(verbose) {
        std::cout << "database " << name << " created" << std::endl;
    }

    return true;
}

bool drop_database(const db_container & db_it)
{
    const std::string & db_name { db_it.name };
    databases.erase(db_name);

    if(verbose) {
        std::cout << "database " << db_name << " dropped" << std::endl;
    }

    return true;
}

bool clear(db_container & db_it)
{
    const std::string & db_name { db_it.name };

    db_it.bank.clear();

    // add empty row for 0
    databases[db_name].bank.insert(sdr::concept({}));

    if(verbose) {
        std::cout << "database " << db_name << " cleared" << std::endl;
    }

    return true;
}

bool resize(db_container & db_it, const std::size_t width)
{
    const std::string & db_name { db_it.name };

    db_it.bank.resize(width);

    if(verbose) {
        std::cout << "database " << db_name << " resized" << std::endl;
    }

    return true;
}

std::size_t insert(db_container & db_it, const std::vector<std::size_t> & trait_positions)
{
    const sdr::position_t position { db_it.bank.insert(sdr::concept(trait_positions)) };

    if(verbose) {
        std::cout << position << std::endl;
    }

    return position;
}

bool update(db_container & db_it, const std::size_t concept_id, const std::vector<std::size_t> & trait_positions)
{
    db_it.bank.update(concept_id, sdr::concept(trait_positions));

    if(verbose) {
        std::cout << "OK" << std::endl;
    }

    return true;
}

std::size_t similarity(const db_container & db_it, const std::size_t concept_a_id, const std::size_t concept_b_id)
{
    const std::size_t result { db_it.bank.similarity(concept_a_id, concept_b_id) };

    if(verbose) {
        std::cout << result << std::endl;
    }

    return result;
}

std::size_t usimilarity(const db_container & db_it, const std::size_t concept_id, const std::vector<std::size_t> & concept_positions)
{
    const std::size_t result { db_it.bank.union_similarity(concept_id, concept_positions) };

    if(verbose) {
        std::cout << result << std::endl;
    }

    return result;
}

std::vector<std::pair<std::size_t, std::size_t>> closest(const db_container & db_it, const std::size_t amount, const std::size_t concept_id)
{
    const std::vector<std::pair<std::size_t, std::size_t>> results { db_it.bank.closest(concept_id, amount) };

    if(verbose) {
        for(std::size_t i=0; i<results.size(); ++i) {
            const std::pair<std::size_t, std::size_t> item { results[i] };

            std::cout << item.first << ":" << item.second;
            if(i != results.size() - 1) {
                std::cout << " ";
            }
        }

        std::cout << std::endl;
    }

    return results;
}

std::vector<std::size_t> matching(const db_container & db_it, const std::vector<std::size_t> & trait_positions)
{
    const std::vector<std::size_t> results { db_it.bank.matching(sdr::concept(trait_positions)) };

    if(verbose) {
        for(std::size_t i=0; i<results.size(); ++i) {
            const std::size_t item { results[i] };

            std::cout << item;
            if(i != results.size() - 1) {
                std::cout << " ";
            }
        }

        std::cout << std::endl;
    }

    return results;
}

std::vector<std::size_t> matchingx(const db_container & db_it, const std::size_t amount, const std::vector<std::size_t> & trait_positions)
{
    const std::vector<std::size_t> results { db_it.bank.matching(sdr::concept(trait_positions), amount) };

    if(verbose) {
        for(std::size_t i=0; i<results.size(); ++i) {
            const std::size_t item { results[i] };

            std::cout << item;
            if(i != results.size() - 1) {
                std::cout << " ";
            }
        }

        std::cout << std::endl;
    }

    return results;
}

result_container parse_input(const std::string & input)
{
    std::istringstream iss(input);

    std::vector<std::string> pieces;

    {
        std::string piece;

        while(std::getline(iss, piece, ' ')) {
            const std::string & trimmed { trim(piece) };

            if(! trimmed.empty()) {
                pieces.push_back(trimmed);
            }
        }
    }

    // empty input shouldnt be passed here to start with
    // but.. how can we trust that?
    if(pieces.size() == 0) {
        return result_container();
    }

    // comment
    if(pieces[0][0] == '#') {
        return result_container();
    }

    const std::string & command { tolower(pieces[0]) };

    if(command == "create") {
        {
            check_result check { argument_length_check_eq(pieces, 3) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        const std::string & db_name  { pieces[1] };
        const auto db = databases.find(db_name);
        if(db != databases.end()) {
            return render_error("database already exists", db_name);
        }

        const std::string & db_width { pieces[2] };

        {
            check_result check { number_check(db_width) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        number_container width(db_width);
        if(! width.parse()) {
            return width.err();
        }

        return result_container(create_database(db_name, width.get_n()));
    } else if(command == "drop") {
        {
            check_result check { argument_length_check_eq(pieces, 2) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        const std::string & db_name { pieces[1] };

        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            return render_error("database not found", db_name);
        }

        return result_container(drop_database(db->second));
    } else if(command == "clear") {
        {
            check_result check { argument_length_check_eq(pieces, 2) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        const std::string & db_name { pieces[1] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            return render_error("database not found", db_name);
        }

        return result_container(clear(db->second));
     } else if(command == "resize") {
        {
            check_result check { argument_length_check_eq(pieces, 3) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        const std::string & db_name { pieces[1] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            return render_error("database not found", db_name);
        }

        const std::string & db_width { pieces[2] };
        {
            check_result check { number_check(db_width) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        number_container width(db_width);
        if(! width.parse()) {
            return width.err();
        }

        return result_container(resize(db->second, width.get_n()));
     } else if(command == "put") {
        {
            check_result check { argument_length_check_lt(pieces, 3) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        const std::string & db_name { pieces[1] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            return render_error("database not found", db_name);
        }
        db_container & db_it { db->second };

        const std::vector<std::string> trait_strs(std::begin(pieces) +  2, std::end(pieces));
        std::vector<number_container> trait_positions(std::begin(trait_strs), std::end(trait_strs));
        for(number_container & n :  trait_positions) {
            std::cout << "pre parse" << std::endl;
            if(! n.parse()) {
                return n.err();
            }
        }

        std::cout << "past parse" << std::endl;

        {
            check_result check { positions_smaller_than_width_check(db_it, trait_positions) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        std::cout << "post pos small" << std::endl;


        return result_container(insert(db_it, unpack(trait_positions)));
     }  else if(command == "update") {
        //update DBNAME CONCEPT_ID TRAITS....
        {
            check_result check { argument_length_check_lt(pieces, 3) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }


        const std::string & db_name { pieces[1] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            return render_error("database not found", db_name);
        }
        db_container & db_it { db->second };

        const std::string & concept_str  { pieces[2] };
        number_container concept_id(concept_str);
        if(! concept_id.parse()) {
            return concept_id.err();
        }

        if(! concept_id.get_n()) {
            return render_error("concept not found", concept_str);
        }
        if(concept_id.get_n() >= db_it.get_storage_size()) {
            return render_error("id larger than amount in storage", concept_str);
        }

        const std::vector<std::string> trait_strs(std::begin(pieces) + 3, std::end(pieces));
        std::vector<number_container> trait_positions(std::begin(trait_strs), std::end(trait_strs));
        for(number_container & n :  trait_positions) {
            if(! n.parse()) {
                return n.err();
            }
        }

        {
            check_result check { positions_smaller_than_width_check(db_it, trait_positions) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        return result_container(update(db_it, concept_id.get_n(), unpack(trait_positions)));
     } else if(command == "query") {
        {
            check_result check { argument_length_check_lt(pieces, 4) };
            if(! check) {
                return result_container(check.get_rc());
            }
        }

        const std::string & db_name { pieces[1] };
        const auto db = databases.find(db_name);
        if(db == databases.end()) {
            return render_error("database not found", db_name);
        }
        const db_container & db_it { db->second };

        const bool weighted         { tolower(pieces[2]) == "weighted" };
        const bool async            { tolower(pieces[weighted ? 3 : 2]) == "async" };
        const std::size_t qtype_pos { static_cast<std::size_t>(2 + weighted + async) };

        const std::string & qtype { pieces[qtype_pos] };

        if(qtype == "similarity") {
            if(async) {
                return render_error("similarity cannot be async", "async");
            }
            {
                check_result check { argument_length_check_eq(pieces, qtype_pos + 3) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            const std::string & concept_a_str { pieces[qtype_pos + 1] };
            number_container concept_a_id(concept_a_str);
            if(! concept_a_id.parse()) {
                return concept_a_id.err();
            }

            if(! concept_a_id.get_n()) {
                return render_error("concept not found", concept_a_str);
            }
            if(concept_a_id.get_n() >= db_it.get_storage_size()) {
                return render_error("id larger than amount in storage", concept_a_str);
            }

            const std::string & concept_b_str { pieces[qtype_pos + 2] };
            number_container concept_b_id(concept_b_str);
            if(! concept_b_id.parse()) {
                return concept_b_id.err();
            }

            if(! concept_b_id.get_n()) {
                return render_error("concept not found", concept_b_str);
            }
            if(concept_b_id.get_n() >= db_it.get_storage_size()) {
                return render_error("id larger than amount in storage", concept_b_str);
            }

            return result_container(similarity(db_it, concept_a_id.get_n(), concept_b_id.get_n()));
        } else if(qtype == "usimilarity") {
            if(async) {
                return render_error("usimilarity cannot be async", "async");
            }
            {
                check_result check { argument_length_check_lt(pieces, qtype_pos + 3) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            const std::string & concept_str { pieces[qtype_pos + 1] };
            number_container concept_id(concept_str);
            if(! concept_id.parse()) {
                return concept_id.err();
            }

            if(! concept_id.get_n()) {
                return render_error("concept not found", concept_str);
            }
            if(concept_id.get_n() >= db_it.get_storage_size()) {
                return render_error("id larger than amount in storage", concept_str);
            }

            const std::vector<std::string> concept_strs(std::begin(pieces) + qtype_pos + 2, std::end(pieces));
            std::vector<number_container> concept_positions(std::begin(concept_strs), std::end(concept_strs));
            for(number_container & n :  concept_positions) {
                if(! n.parse()) {
                    return n.err();
                }
            }

            return result_container(usimilarity(db_it, concept_id.get_n(), unpack(concept_positions)));
        } else if(qtype == "closest") {
            {
                check_result check { argument_length_check_eq(pieces, qtype_pos + 3) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            const std::string & amount_str { pieces[qtype_pos + 1] };
            {
                check_result check { number_check(amount_str) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            number_container amount(amount_str);
            if(! amount.parse()) {
                return amount.err();
            }

            const std::string & concept_str { pieces[qtype_pos + 2] };
            number_container concept_id(concept_str);
            if(! concept_id.parse()) {
                return concept_id.err();
            }

            if(! concept_id.get_n()) {
                return render_error("concept not found", concept_str);
            }
            if(concept_id.get_n() >= db_it.get_storage_size()) {
                return render_error("id larger than amount in storage", concept_str);
            }

            return result_container(closest(db_it, amount, concept_id.get_n()));
        } else if(qtype == "matching") {
            if(weighted) {
                return render_error("matching cannot be weighted", "weighted");
            }

            if(async) {
                return render_error("matching cannot be async", "async");
            }

            {
                check_result check { argument_length_check_lt(pieces, qtype_pos + 2) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            const std::vector<std::string> trait_strs(std::begin(pieces) + qtype_pos + 1, std::end(pieces));
            std::vector<number_container> trait_positions(std::begin(trait_strs), std::end(trait_strs));
            for(number_container & n :  trait_positions) {
                if(! n.parse()) {
                    return n.err();
                }
            }

            {
                check_result check { positions_smaller_than_width_check(db_it, trait_positions) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            return result_container(matching(db_it, unpack(trait_positions)));
        } else if(qtype == "matchingx") {
            if(async) {
                return render_error("matchingx cannot be async", "async");
            }

            {
                check_result check { argument_length_check_lt(pieces, qtype_pos + 3) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            {
                check_result check { number_check(pieces[qtype_pos + 1]) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            const std::string & amount_str { pieces[qtype_pos + 1] };

            number_container amount(amount_str);
            if(! amount.parse()) {
                return amount.err();
            }

            const std::vector<std::string> trait_strs(std::begin(pieces) + qtype_pos + 2, std::end(pieces));
            std::vector<number_container> trait_positions(std::begin(trait_strs), std::end(trait_strs));
            for(number_container & n :  trait_positions) {
                if(! n.parse()) {
                    return n.err();
                }
            }

            {
                check_result check { positions_smaller_than_width_check(db_it, trait_positions) };
                if(! check) {
                    return result_container(check.get_rc());
                }
            }

            return result_container(matchingx(db_it, amount.get_n(), unpack(trait_positions)));
        } else {
            return render_error("bad syntax", command);
        }
     } else if(command == "save") {
        return result_container();
     } else {
        return render_error("unknown command", command);
    }
}

int serverloop(const std::string & bindpath)
{
    constexpr std::size_t buffer_size { 128 };

    try {
        libsocket::unix_stream_server server(bindpath);

        while(true) {
            std::unique_ptr<libsocket::unix_stream_client> client(server.accept());
            std::string input;

            while(true) {
                std::string answer;
                answer.resize(buffer_size);
                *client >> answer;


                if(verbose) {
                    std::cout << answer;
                }
                input.append(answer);

                for(char c : answer) {
                    if(c == '\n') {
                        goto breaker;
                    }
                }
            } breaker:;

            result_container res { parse_input(input) };
            switch(res.get_type()) {
                case result_type::NONE:
                    break;
                case result_type::BOOL:
                    {
                        const bool * m { res };
                        const std::string s { *m ? "1\n" : "0\n" };
                        *client << s;
                    }
                    break;
                case result_type::SIZET:
                    {
                        const std::size_t * m { res };
                        std::stringstream ss;
                        ss << *m << "\n";
                        *client << ss.str();
                    }
                    break;
                case result_type::STRING:
                    {
                        const std::string * s { res };
                        *client << *s << "\n";
                    }
                    break;
                case result_type::VECSIZET:
                    {
                        const std::vector<std::size_t> * vec { res };
                        const std::size_t vsize { (*vec).size() };
                        std::stringstream ss;

                        for(std::size_t i=0; i<vsize; ++i) {
                            ss << (*vec)[i];

                            if(i < vsize - 1) {
                                ss << " ";
                            }
                        }

                        ss << "\n";

                        *client << ss.str();
                    }
                    break;
                case result_type::VECPAIRSIZETSIZET:
                    {
                        const std::vector<std::pair<std::size_t, std::size_t>> * vec { res };
                        const std::size_t vsize { (*vec).size() };
                        std::stringstream ss;

                        for(std::size_t i=0; i<(*vec).size(); ++i) {
                            const std::pair<std::size_t, std::size_t> & item = (*vec)[i];
                            ss << item.first << ":" << item.second;

                            if(i < vsize - 1) {
                                ss << " ";
                            }
                        }

                        ss << "\n";

                        *client << ss.str();
                    }
                    break;
                default:
                    break;
            }
        }

        server.destroy();
    } catch (const libsocket::socket_exception & exc) {
        std::cerr << exc.mesg << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void display_usage()
{
    std::cout
        << "Usage: sdrdb-server [OPTION]..." << std::endl
        << "Options and arguments:" << std::endl
        << "-h            : show this help" << std::endl
        << "-v            : show version" << std::endl
        << "-V            : set verbose" << std::endl
        << "-b arg        : set bindpath for server" << std::endl
        << "-d            : run as daemon" << std::endl;
}

void display_version()
{
    std::cout << "sdrdb-server " << SDRDB_VERSION << std::endl;
}

int main(int argc, char ** argv)
{
    databases.set_empty_key("");

    std::string bindpath { "/tmp/sdrdb.sock" };
    bool daemonize { false };
    {
        int c;

        while ((c = getopt (argc, argv, "vVhb:d")) != -1) {
            switch (c) {
            case 'v':
                display_version();
                return EXIT_SUCCESS;
                break;
            case 'V':
                verbose = true;
                break;
            case 'h':
                display_usage();
                return EXIT_SUCCESS;
                break;
            case 'b':
                bindpath = optarg;
                break;
            case 'd':
                daemonize = true;
                break;
            case '?':
                std::cerr << "sdrdb-server: invalid option" << std::endl;
                display_usage();
                return EXIT_FAILURE;
            default:
                abort ();
            }
        }
    }

    std::cout << "sdrdb-server started" << std::endl;

    if(daemonize) {
        if(daemon(0, 0) != 0) {
            std::cerr << "error during daemonization: " << errno << std::endl;
            return EXIT_FAILURE;
        }
    }

    return serverloop(bindpath);

    return EXIT_SUCCESS;
}
