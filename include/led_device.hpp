#ifndef LED_DEVICE_HPP
#define LED_DEVICE_HPP

#include <string>

class LedDevice {
public:
    explicit LedDevice(const std::string& path);

    bool available() const;
    bool set(bool on) const;
    bool get(bool& on) const;
    bool readable() const;

private:
    std::string path_;
    std::string brightnessPath_;
    bool available_;
    bool characterDevice_;
};

#endif