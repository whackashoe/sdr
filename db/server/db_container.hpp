#ifndef DBCONTAINER_H
#define DBCONTAINER_H

#include <string>
#include <iostream>
#include "../../includes/sdr.hpp"

struct db_container
{
    std::string name;
    sdr::bank bank;

    db_container()
    : name("undefined")
    , bank(0)
    {}

    db_container(const db_container & dbc)
    : name(dbc.name)
    , bank(dbc.bank)
    {}

    db_container(const std::string & name, const std::size_t width)
    : name(name)
    , bank(sdr::bank(width))
    {}

    std::size_t get_storage_size() const
    {
        return bank.get_storage_size();
    }

    std::size_t get_width() const
    {
        return bank.get_width();
    }
};

#endif