#ifndef LCD_DISPLAY_HPP
#define LCD_DISPLAY_HPP

#include <cstddef>
#include <string>

#include "device_manager.hpp"

class LcdDisplay {
public:
    LcdDisplay();
    ~LcdDisplay();

    bool available() const;

    void update(
        const DeviceStatus& status,
        std::size_t clientCount,
        const std::string& networkState
    );

private:
    int framebufferFd_;
    void* framebuffer_;
    std::size_t framebufferSize_;
    bool available_;
};

#endif