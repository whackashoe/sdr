#ifndef DBCONTAINER_H
#define DBCONTAINER_H

#include <string>
#include <iostream>
#include <unordered_map>
#include "../includes/sdr.hpp"

struct dbcontainer
{
    std::string name;
    sdr::bank bank;
    std::unordered_map<std::string, std::size_t> trait_names;
    std::unordered_map<std::size_t, std::string> trait_names_i;
    std::unordered_map<std::string, std::size_t> concept_names;
    std::unordered_map<std::size_t, std::string> concept_names_i;

    dbcontainer()
    : name("undefined")
    , bank(0)
    {}

    dbcontainer(const dbcontainer & dbc)
    : name(dbc.name)
    , bank(dbc.bank)
    {}

    dbcontainer(const std::string & name, const std::size_t width)
    : name(name)
    , bank(sdr::bank(width))
    {}

    void render_list()
    {
        std::cout
            << "width: " << bank.get_width() << std::endl
            << "rows:  " << bank.get_storage_size() << std::endl;
    }
};

#endif