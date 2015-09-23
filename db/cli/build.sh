#!/bin/sh

g++ -std=c++11 sdrdb-cli.cpp -Wall -Wextra -pedantic -o sdrdb-cli -lreadline -lsocket++
mv sdrdb-cli ../../dist
echo "cli built"