#ifndef NUMBER_CONTAINER
#define NUMBER_CONTAINER

#include <string>
#include <stdexcept>
#include <exception>
#include <cstddef>
#include <cassert>
#include "result_container.hpp"

struct number_container
{
    std::string s;
    std::size_t n;
    std::string what_str;
    bool ok;

    number_container(const std::string & s)
    : s(s)
    , ok(false)

    {}

    bool parse()
    {
        try {
            n = std::stoul(s);
            ok = true;
        } catch(std::out_of_range & e) {
            ok = false;
            what_str = "ERR: out of range => " + s;
        } catch(std::invalid_argument & e) {
            ok = false;
            what_str = "ERR: invalid argument => " + s;
        } catch(std::exception & e) {
            ok = false;
            what_str = "ERR: unknown exception encountered => " + s;
        }

        return ok;
    }

    operator bool() const
    {
        return ok;
    }

    std::size_t get_n() const
    {
        assert(ok);
        return n;
    }

    std::string get_s() const
    {
        assert(ok);
        return s;
    }

    result_container err() const
    {
        assert(!ok);
        return result_container(what_str);
    }
};

#endif