#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "device_manager.hpp"
#include "lcd_display.hpp"

class TcpServer{

public:
    TcpServer();
    ~TcpServer();
    bool start(uint16_t port = 8000);
    void run();

private:
    bool setNonBlocking(int fd);
    void closeSocket(int fd);
    bool handleClientRead(int fd);
    void broadcastDeviceEvents();
    DeviceManager deviceManager;
    LcdDisplay lcdDisplay;
    int listenfd;  
    int epollfd; 
    std::unordered_map<int,std::string> receiveBuffers; // int for fd,string for data buffer
};

#endif 