#!/bin/sh

g++ -std=c++11 sdrdb-cli.cpp -Wall -Wextra -pedantic -o sdrdb-cli -lreadline -lsocket++
g++ -std=c++11 sdrdbd.cpp -Wall -Wextra -pedantic -o sdrdbd -lreadline -lsocket++