#ifndef DEVICE_MANAGER_HPP
#define DEVICE_MANAGER_HPP

#include <string>
#include "led_device.hpp"
#include "key_device.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <deque>


class DeviceManager {
public:
    std::string execute(const std::string& requestLine);
    bool popEvent(std::string& event);
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
    struct KeyEvent {
        std::uint64_t sequence;
        bool pressed;
        std::uint64_t timestampMs;
    };

    std::deque<KeyEvent> keyEvents;
    std::uint64_t nextEventSequence = 1;

    static constexpr std::size_t MAX_KEY_EVENTS = 64;
};
#endif