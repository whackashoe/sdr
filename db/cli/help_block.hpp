#ifndef HELP_BLOCK
#define HELP_BLOCK

#include <string>
#include <iostream>
#include <vector>

#include "help_block_arg.hpp"
#include "ansi_colors.hpp"

struct help_block
{
    std::string cmd;
    std::string summary;
    std::vector<help_block_arg> args;
    std::vector<std::string> examples;

    help_block(
        const std::string & cmd
      , const std::string & summary
      , const std::vector<help_block_arg> & args
      , const std::vector<std::string> & examples
    )
    : cmd(cmd)
    , summary(summary)
    , args(args)
    , examples(examples)
    {}

    void render() const
    {
        std::cout << ANSI_COLOR_CYAN << cmd << ANSI_COLOR_RESET << " - " << summary << std::endl;
        for(auto & arg : args) {
            std::cout << "\t";
            arg.render();
        }

        std::cout << "EXAMPLES:" << std::endl << std::endl;

        for(auto & example : examples) {
            std::cout << example << std::endl << std::endl;
        }
    }
};

#endif