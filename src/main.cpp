#include<iostream>
#include<unistd.h>
#include<sys/types.h>
#include"tcp_server.hpp"


int main() {
    TcpServer server;
    if(!server.start(8000)){
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    server.run();
    return 0;
}