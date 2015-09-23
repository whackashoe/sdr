#!/bin/sh

g++ -std=c++11 sdrdb-server.cpp -Wall -Wextra -pedantic -o sdrdb-server -lsocket++
mv sdrdb-server ../../dist
echo "server built"