#ifndef DEVICE_MANAGER_HPP
#define DEVICE_MANAGER_HPP

#include <string>
#include "led_device.hpp"
#include "key_device.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>


class DeviceManager {
public:
    std::string execute(const std::string& requestLine);
    DeviceManager();
    ~DeviceManager();

private:
    LedDevice led{"/dev/imx6ull_device"};
    bool ledOn = false;

    void keyMonitorLoop();

    KeyDevice key{"/dev/input/event2"};

    std::atomic<bool> stopping{false};
    std::thread keyThread;
    std::mutex keyMutex;

    bool keyPressed = false;
    std::uint64_t keyEventCount = 0;
};
#endif