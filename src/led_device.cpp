#include "led_device.hpp"

#include <fstream>
#include <fcntl.h>
#include <unistd.h>

LedDevice::LedDevice(const std::string& path)
    : path_(path),
      brightnessPath_(),
      available_(false),
      characterDevice_(path.find("/dev/") == 0) {
    if (characterDevice_) {
        available_ =
            access(path_.c_str(), R_OK | W_OK) == 0;
        return;
    }

    brightnessPath_ = path_ + "/brightness";

    available_ =
        access(brightnessPath_.c_str(), R_OK | W_OK) == 0;

    if (available_) {
        std::ofstream trigger(path_ + "/trigger");
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

    if (characterDevice_) {
        const char value = on ? '1' : '0';
        const int fd = open(path_.c_str(), O_WRONLY);

        if (fd < 0) {
            return false;
        }

        const ssize_t result =
            write(fd, &value, 1);

        close(fd);
        return result == 1;
    }

    std::ofstream output(brightnessPath_);
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

    if (characterDevice_) {
        char value[2] = {};
        const int fd = open(path_.c_str(), O_RDONLY);

        if (fd < 0) {
            return false;
        }

        const ssize_t result = read(fd, value, sizeof(value));
        close(fd);

        if (result < 1) {
            return false;
        }

        on = value[0] == '1';
        return true;
    }

    std::ifstream input(brightnessPath_);
    int value = 0;

    if (!(input >> value)) {
        return false;
    }

    on = value != 0;
    return true;
}

bool LedDevice::readable() const {
    return available_ ;
}