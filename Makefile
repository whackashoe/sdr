CXX=g++
CPPFLAGS=-std=c++11 -Wall -Wextra -pedantic -O3

SERVER_LDFLAGS=-lreadline -lsocket++ -pthread
CLI_LDFLAGS=-lreadline -lsocket++
BENCHMARK_LDFLAGS=-pthread

SERVER_DIR=db/server
CLI_DIR=db/cli
BENCHMARK_DIR=benchmark
DIST_DIR=dist

RM=rm -f


all: server cli benchmark
	@echo "\n\ncomplete"

server: $(SERVER_DIR)/sdrdb-server.o
	$(CXX) $(CPPFLAGS) $(SERVER_DIR)/sdrdb-server.o -o $(DIST_DIR)/sdrdb-server $(SERVER_LDFLAGS)
	@echo "\n\nsdrdb-server built\n\n"

cli: $(CLI_DIR)/sdrdb-cli.o
	$(CXX) $(CPPFLAGS) $(CLI_DIR)/sdrdb-cli.o -o $(DIST_DIR)/sdrdb-cli $(CLI_LDFLAGS)
	@echo "\n\nsdrdb-cli built\n\n"

benchmark: $(BENCHMARK_DIR)/bench.o
	$(CXX) $(CPPFLAGS) $(BENCHMARK_DIR)/bench.o -o $(DIST_DIR)/bench $(BENCHMARK_LDFLAGS)
	@echo "\n\nbench built\n\n"

$(SERVER_DIR)/sdrdb-server.o: $(SERVER_DIR)/*.hpp
	$(CXX) $(CPPFLAGS) -c $(SERVER_DIR)/sdrdb-server.cpp -o $(SERVER_DIR)/sdrdb-server.o

$(CLI_DIR)/sdrdb-cli.o: $(CLI_DIR)/*.hpp
	$(CXX) $(CPPFLAGS) -c $(CLI_DIR)/sdrdb-cli.cpp -o $(CLI_DIR)/sdrdb-cli.o

$(BENCHMARK_DIR)/bench.o:
	$(CXX) $(CPPFLAGS) -c $(BENCHMARK_DIR)/bench.cpp -o $(BENCHMARK_DIR)/bench.o

clean:
	$(RM) $(SERVER_DIR)/*.o $(CLI_DIR)/*.o $(BENCHMARK_DIR)/*.o $(DIST_DIR)/*
	@echo "\n\ncleaned\n\n"
