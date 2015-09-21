#ifndef CHECKRESULT_H
#define CHECKRESULT_H

#include "resultcontainer.hpp"

struct checkresult
{
    bool success;
    resultcontainer rc;

    checkresult(const bool success)
    : success(success)
    , rc(resultcontainer())
    {}

    checkresult(const resultcontainer & rc)
    : success(false)
    , rc(rc)
    {}

    operator bool()
    {
        return success;
    }

    operator bool() const
    {
        return success;
    }

    bool operator!()
    {
        return !success;
    }

    resultcontainer & get_rc()
    {
        return rc;
    }
};

#endif