#ifndef RESULTCONTAINER_H
#define RESULTCONTAINER_H

#include <string>
#include <vector>
#include <utility>
#include <cassert>

enum class result_type { NONE, BOOL, SIZET, STRING, VECSIZET, VECPAIRSIZETSIZET };

struct result_container
{
    result_type type;
    void * data;

    result_container()
    : type(result_type::NONE)
    , data(nullptr)
    {}

    result_container(const result_container & rc)
    : type(rc.type)
    , data(rc.data)
    {}

    result_container(const bool res)
    : type(result_type::BOOL)
    , data(static_cast<void*>(new bool(res)))
    {}

    result_container(const std::size_t res)
    : type(result_type::SIZET)
    , data(static_cast<void*>(new std::size_t(res)))
    {}

    result_container(const std::string & res)
    : type(result_type::STRING)
    , data(static_cast<void*>(new std::string(res)))
    {}

    result_container(const std::vector<std::size_t> & res)
    : type(result_type::VECSIZET)
    , data(static_cast<void*>(new std::vector<std::size_t>(res)))
    {}

    result_container(const std::vector<std::pair<std::size_t, std::size_t>> & res)
    : type(result_type::VECPAIRSIZETSIZET)
    , data(static_cast<void*>(new std::vector<std::pair<std::size_t, std::size_t>>(res)))
    {}

    operator bool*()
    {
        assert(type == result_type::BOOL);
        return static_cast<bool*>(data);
    }

    operator bool*() const
    {
        assert(type == result_type::BOOL);
        return static_cast<bool*>(data);
    }

    operator std::size_t*()
    {
        assert(type == result_type::SIZET);
        return static_cast<std::size_t*>(data);
    }

    operator std::size_t*() const
    {
        assert(type == result_type::SIZET);
        return static_cast<std::size_t*>(data);
    }

    operator std::string*()
    {
        assert(type == result_type::STRING);
        return static_cast<std::string*>(data);
    }

    operator std::string*() const
    {
        assert(type == result_type::STRING);
        return static_cast<std::string*>(data);
    }

    operator std::vector<std::size_t>*()
    {
        assert(type == result_type::VECSIZET);
        return static_cast<std::vector<std::size_t>*>(data);
    }

    operator std::vector<std::size_t>*() const
    {
        assert(type == result_type::VECSIZET);
        return static_cast<std::vector<std::size_t>*>(data);
    }

    operator std::vector<std::pair<std::size_t, std::size_t>>*()
    {
        assert(type == result_type::VECPAIRSIZETSIZET);
        return static_cast<std::vector<std::pair<std::size_t, std::size_t>>*>(data);
    }

    operator std::vector<std::pair<std::size_t, std::size_t>>*() const
    {
        assert(type == result_type::VECPAIRSIZETSIZET);
        return static_cast<std::vector<std::pair<std::size_t, std::size_t>>*>(data);
    }

    result_type get_type() const
    {
        return type;
    }


};

#endif