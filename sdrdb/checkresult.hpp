#ifndef CHECKRESULT_H
#define CHECKRESULT_H

#include "resultcontainer.hpp"

struct check_result
{
    bool success;
    result_container rc;

    check_result(const bool success)
    : success(success)
    , rc(result_container())
    {}

    check_result(const result_container & rc)
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

    result_container & get_rc()
    {
        return rc;
    }
};

#endif