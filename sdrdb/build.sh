#!/bin/sh

g++ -std=c++11 client.cpp -o client -lsocket++
g++ -std=c++11 sdrdb.cpp -Wall -Wextra -o sdrdb -lreadline -lsocket++