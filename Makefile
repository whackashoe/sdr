CXX=g++
CC=gcc
CPPFLAGS=-std=c++11 -Wall -Wextra -pedantic -O3
CFLAGS=

SERVER_LDFLAGS=-lsocket++ -pthread
CLI_LDFLAGS=-lsocket++
BENCHMARK_LDFLAGS=-pthread

SERVER_DIR=db/server
CLI_DIR=db/cli
BENCHMARK_DIR=benchmark
DIST_DIR=dist

RM=rm -f


all: server cli benchmark
	@echo "\ncomplete"

server: $(SERVER_DIR)/sdrdb-server.o
	$(CXX) $(CPPFLAGS) $(SERVER_DIR)/sdrdb-server.o -o $(DIST_DIR)/sdrdb-server $(SERVER_LDFLAGS)
	@echo "\nsdrdb-server built\n"

cli: $(CLI_DIR)/sdrdb-cli.o $(CLI_DIR)/linenoise.o
	$(CXX) $(CPPFLAGS) $(CLI_DIR)/sdrdb-cli.o $(CLI_DIR)/linenoise.o -o $(DIST_DIR)/sdrdb-cli $(CLI_LDFLAGS)
	@echo "\nsdrdb-cli built\n"

benchmark: $(BENCHMARK_DIR)/bench.o
	$(CXX) $(CPPFLAGS) $(BENCHMARK_DIR)/bench.o -o $(DIST_DIR)/bench $(BENCHMARK_LDFLAGS)
	@echo "\nbench built\n"

$(SERVER_DIR)/sdrdb-server.o: $(SERVER_DIR)/sdrdb-server.cpp $(SERVER_DIR)/*.hpp
	$(CXX) $(CPPFLAGS) -c $(SERVER_DIR)/sdrdb-server.cpp -o $(SERVER_DIR)/sdrdb-server.o

$(CLI_DIR)/sdrdb-cli.o: $(CLI_DIR)/sdrdb-cli.cpp $(CLI_DIR)/*.hpp
	$(CXX) $(CPPFLAGS) -c $(CLI_DIR)/sdrdb-cli.cpp -o $(CLI_DIR)/sdrdb-cli.o

$(CLI_DIR)/linenoise.o: $(CLI_DIR)/linenoise.c $(CLI_DIR)/linenoise.h
	$(CC) $(CFLAGS) -c $(CLI_DIR)/linenoise.c -o $(CLI_DIR)/linenoise.o

$(BENCHMARK_DIR)/bench.o: $(BENCHMARK_DIR)/bench.cpp
	$(CXX) $(CPPFLAGS) -c $(BENCHMARK_DIR)/bench.cpp -o $(BENCHMARK_DIR)/bench.o

clean:
	$(RM) $(SERVER_DIR)/*.o $(CLI_DIR)/*.o $(BENCHMARK_DIR)/*.o $(DIST_DIR)/*
	@echo "\ncleaned"
