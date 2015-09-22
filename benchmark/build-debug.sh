#!/bin/sh

g++ -std=c++11 bench.cpp -O0 -Weffc++ -Wall -Wextra -pthread -g -o bench
