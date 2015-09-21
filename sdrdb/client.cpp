#include <iostream>
#include <string>
#include <libsocket/unixclientstream.hpp>
#include <libsocket/exception.hpp>

#include <unistd.h>
#include <stdlib.h>
#include <cstring>

// Builds a connection to /tmp/unixsocket and sends/receives data using it.

void serverloop(std::string & bindpath)
{
    constexpr std::size_t buffer_size { 128 };

    try {
        libsocket::unix_stream_client sock(bindpath);

        std::string send = "Hello World\n";
        sock << send;

        while(true) {
            std::string answer;
            answer.resize(buffer_size);

            sock >> answer;

            std::cout << answer;

            for(char c : answer) {
                if(c == 0) {
                    goto breaker;
                }
            }
        } breaker:;
    } catch (const libsocket::socket_exception & exc) {
        std::cerr << exc.mesg;
    }
}

int main(void)
{
    std::string bindpath { "/tmp/sdrdb.sock" };

    serverloop(bindpath);

    return EXIT_SUCCESS;
}