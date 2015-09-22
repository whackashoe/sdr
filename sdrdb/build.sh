#!/bin/sh

g++ -std=c++11 sdrdb-server.cpp -Wall -Wextra -pedantic -o sdrdb-server -lsocket++
g++ -std=c++11 sdrdb-cli.cpp -Wall -Wextra -pedantic -o sdrdb-cli -lreadline -lsocket++
