#include<iostream>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<string.h>
#include<cstdio>
#include<fcntl.h>  //set non-blocking
#include<sys/epoll.h>  //epoll related header files
#include "tcp_server.hpp"
#include<unordered_map>
#include<cerrno>
#include "protocol.hpp"

namespace{
    constexpr int MAX_EVENTS = 16;
    constexpr int BUFFER_SIZE = 1024;
    constexpr int EPOLL_TIMEOUT_MS = 1000; // 1 second
}

bool TcpServer::setNonBlocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1){
        return false;
    }
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
        return false;
    }
    return true;
}

//socket, bind, listen
bool TcpServer::start(uint16_t port) {
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd<0){
        perror("socket error");
        return false;
    }
    int reuse = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        perror("setsockopt error");
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind error");
        return false;
    }

    if(listen(listenfd, SOMAXCONN) < 0){
        perror("listen error");
        return false;
    }
    if(!setNonBlocking(listenfd)){
        perror("setNonBlocking error");
        return false;
    }
    epollfd = epoll_create1(0);
    if(epollfd < 0){
        perror("epoll_create1 error");
        return false;
    }

    epoll_event event{};
    event.data.fd = listenfd;
    event.events = EPOLLIN ; // Edge-triggered

    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event) < 0){
        perror("epoll_ctl error");
        return false;
    }

    std::cout << "server is listening on port" <<port<< std::endl;
    return true;
}

void TcpServer::broadcastDeviceEvents() {
    if (receiveBuffers.empty()) {
        return;
    }

    std::string event;

    while (deviceManager.popEvent(event)) {
        std::vector<int> clients;

        for (const auto& item : receiveBuffers) {
            clients.push_back(item.first);
        }

        for (int fd : clients) {
            const ssize_t sent = send(
                fd,
                event.data(),
                event.size(),
                MSG_NOSIGNAL
            );

            if (sent < 0) {
                perror("event send error");
                closeSocket(fd);
            }
        }
    }
}

void TcpServer::closeSocket(int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    receiveBuffers.erase(fd);
    close(fd);
    std::cout << "[INFO]Socket closed: " << fd << std::endl;
}


bool TcpServer::handleClientRead(int fd) {
    char buffer[BUFFER_SIZE]{};

    while (true) {
        ssize_t receivedBytes = recv(
            fd,
            buffer,
            sizeof(buffer),
            0
        );

        if (receivedBytes > 0) {
            std::string& pending = receiveBuffers[fd];

            pending.append(buffer, receivedBytes);

            if (pending.size() > 4096) {
                std::cerr << "[WARN] Frame is too large: "
                          << fd << std::endl;
                return false;
            }

            while (true) {
                std::size_t newline =
                    pending.find('\n');

                if (newline == std::string::npos) {
                    break;
                }

                std::string request =
                    pending.substr(0, newline);

                pending.erase(0, newline + 1);

                if (!request.empty() &&
                    request.back() == '\r') {
                    request.pop_back();
                }

                std::cout << "[INFO] Received from client "
                          << fd << ": "
                          << request
                          << std::endl;

                std::string response =deviceManager.execute(request);

                ssize_t sent = send(
                    fd,
                    response.data(),
                    response.size(),
                    0
                );

                if (sent < 0) {
                    perror("send error");
                    return false;
                }
            }

            continue;
        }

        if (receivedBytes == 0) {
            return false;
        }

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK) {
            return true;
        }

        perror("recv error");
        return false;
    }
}

void TcpServer::run(){
    epoll_event events[MAX_EVENTS];
    while(true){
        int ReadyCount= epoll_wait(epollfd, events, MAX_EVENTS, EPOLL_TIMEOUT_MS);
        if(ReadyCount < 0){
            if(errno == EINTR) continue; // Interrupted by signal, retry
            perror("epoll_wait error");
            return;  
        }
        for (int i=0;i<ReadyCount;++i){
            int currentfd=events[i].data.fd;
            uint32_t event_flags=events[i].events;
            if (currentfd == listenfd) {
                while (true) {
                    sockaddr_in client_addr{};
                    socklen_t client_len = sizeof(client_addr);

                    int clientfd = accept(
                        listenfd,
                        reinterpret_cast<sockaddr*>(&client_addr),
                        &client_len
                    );

                    if (clientfd < 0) {
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK) {
                            break;
                        }

                        perror("accept error");
                        break;
                    }

                    if (!setNonBlocking(clientfd)) {
                        perror("setNonBlocking error");
                        close(clientfd);
                        continue;
                    }

                    epoll_event client_event{};
                    client_event.data.fd = clientfd;
                    client_event.events = EPOLLIN | EPOLLRDHUP;

                    if (epoll_ctl(
                            epollfd,
                            EPOLL_CTL_ADD,
                            clientfd,
                            &client_event) < 0) {
                        perror("epoll_ctl add client error");
                        close(clientfd);
                        continue;
                    }
                    receiveBuffers[clientfd] = std::string();
                    std::cout << "[INFO] New client connected: "
                            << clientfd << std::endl;
                }
                continue;
            } 
            else{
                if (event_flags & EPOLLIN) {
                    if (!handleClientRead(currentfd)) {
                        closeSocket(currentfd);
                        continue;
                    }
                }

                if(event_flags & (EPOLLRDHUP|EPOLLERR|EPOLLHUP)){
                    closeSocket(currentfd);
                }

            }
        }
        broadcastDeviceEvents();
    }
}
 
                        




TcpServer::TcpServer() 
: listenfd(-1), epollfd(-1) {
    std::cout << "[INFO]TcpServer created!" << std::endl;

}

TcpServer::~TcpServer() {
    if(listenfd != -1){
        close(listenfd);
        std::cout << "[INFO]TcpServer closed!" << std::endl;
    }
    if(epollfd != -1){
        close(epollfd);
        std::cout << "[INFO]Epoll closed!" << std::endl;
    }
    std::cout << "[INFO]TcpServer closed!" << std::endl;

}



