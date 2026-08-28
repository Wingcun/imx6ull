#include "led_device.hpp"

#include <fstream>
#include <unistd.h>

LedDevice::LedDevice(const std::string& basePath)
    : brightnessPath(basePath + "/brightness"),
      available_(access(brightnessPath.c_str(), R_OK | W_OK) == 0) {
    if (available_) {
        std::ofstream trigger(basePath + "/trigger");
        if (trigger) {
            trigger << "none";
        }
    }
}

bool LedDevice::available() const {
    return available_;
}

bool LedDevice::set(bool on) const {
    if (!available_) {
        return false;
    }

    std::ofstream output(brightnessPath);
    if (!output) {
        return false;
    }

    output << (on ? 1 : 0);
    output.flush();
    return output.good();
}

bool LedDevice::get(bool& on) const {
    if (!available_) {
        return false;
    }

    std::ifstream input(brightnessPath);
    int value = 0;

    if (!(input >> value)) {
        return false;
    }

    on = value != 0;
    return true;
}