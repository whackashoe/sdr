#!/bin/sh

g++ -std=c++11 bench.cpp -O3 -Weffc++ -Wall -Wextra -pthread -DNDEBUG -o bench
mv bench ../dist
echo "benchmark built"