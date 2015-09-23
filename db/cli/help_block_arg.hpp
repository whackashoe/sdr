#ifndef HELP_BLOCK_ARG
#define HELP_BLOCK_ARG

#include <string>
#include <iostream>
#include "ansi_colors.hpp"

struct help_block_arg
{
    std::string arg;
    std::string type;
    bool required;
    std::string desc;

    help_block_arg(
        const std::string & arg
      , const std::string & type
      , const bool required
      , const std::string & desc
    )
    : arg(arg)
    , type(type)
    , required(required)
    , desc(desc)
    {}

    void render() const
    {
        std::cout
            << ANSI_COLOR_YELLOW << arg << ANSI_COLOR_RESET
            << " (" << type << ") ";
        if(required) {
            std::cout << ANSI_COLOR_RED << "(required)" << ANSI_COLOR_RESET;
        }
        std::cout
            << " : "
            << desc
            << std::endl;
    }
};

#endif