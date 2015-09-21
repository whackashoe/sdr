#ifndef RESULTCONTAINER_H
#define RESULTCONTAINER_H

#include <string>
#include <vector>
#include <utility>
#include <cassert>

enum class resulttype { NONE, BOOL, SIZET, STRING, VECSIZET, VECPAIRSIZETSIZET };

struct resultcontainer
{
    resulttype type;
    void * data;

    resultcontainer()
    : type(resulttype::NONE)
    , data(nullptr)
    {}

    resultcontainer(const resultcontainer & rc)
    : type(rc.type)
    , data(rc.data)
    {}

    resultcontainer(const bool res)
    : type(resulttype::BOOL)
    , data(static_cast<void*>(new bool(res)))
    {}

    resultcontainer(const std::size_t res)
    : type(resulttype::SIZET)
    , data(static_cast<void*>(new std::size_t(res)))
    {}

    resultcontainer(const std::string & res)
    : type(resulttype::STRING)
    , data(static_cast<void*>(new std::string(res)))
    {}

    resultcontainer(const std::vector<std::size_t> & res)
    : type(resulttype::VECSIZET)
    , data(static_cast<void*>(new std::vector<std::size_t>(res)))
    {}

    resultcontainer(const std::vector<std::pair<std::size_t, std::size_t>> & res)
    : type(resulttype::VECPAIRSIZETSIZET)
    , data(static_cast<void*>(new std::vector<std::pair<std::size_t, std::size_t>>(res)))
    {}

    operator bool*()
    {
        assert(type == resulttype::BOOL);
        return static_cast<bool*>(data);
    }

    operator bool*() const
    {
        assert(type == resulttype::BOOL);
        return static_cast<bool*>(data);
    }

    operator std::size_t*()
    {
        assert(type == resulttype::SIZET);
        return static_cast<std::size_t*>(data);
    }

    operator std::size_t*() const
    {
        assert(type == resulttype::SIZET);
        return static_cast<std::size_t*>(data);
    }

    operator std::string*()
    {
        assert(type == resulttype::STRING);
        return static_cast<std::string*>(data);
    }

    operator std::string*() const
    {
        assert(type == resulttype::STRING);
        return static_cast<std::string*>(data);
    }

    operator std::vector<std::size_t>*()
    {
        assert(type == resulttype::VECSIZET);
        return static_cast<std::vector<std::size_t>*>(data);
    }

    operator std::vector<std::size_t>*() const
    {
        assert(type == resulttype::VECSIZET);
        return static_cast<std::vector<std::size_t>*>(data);
    }

    operator std::vector<std::pair<std::size_t, std::size_t>>*()
    {
        assert(type == resulttype::VECPAIRSIZETSIZET);
        return static_cast<std::vector<std::pair<std::size_t, std::size_t>>*>(data);
    }

    operator std::vector<std::pair<std::size_t, std::size_t>>*() const
    {
        assert(type == resulttype::VECPAIRSIZETSIZET);
        return static_cast<std::vector<std::pair<std::size_t, std::size_t>>*>(data);
    }


};

#endif