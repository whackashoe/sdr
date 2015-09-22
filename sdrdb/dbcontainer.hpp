#ifndef DBCONTAINER_H
#define DBCONTAINER_H

#include <string>
#include <iostream>
#include <unordered_map>
#include "../includes/sdr.hpp"

struct db_container
{
    std::string name;
    sdr::bank bank;
    std::unordered_map<std::string, std::size_t> trait_names;
    std::unordered_map<std::size_t, std::string> trait_names_i;
    std::unordered_map<std::string, std::size_t> concept_names;
    std::unordered_map<std::size_t, std::string> concept_names_i;

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
};

#endif