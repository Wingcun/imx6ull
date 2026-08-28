#ifndef LED_DEVICE_HPP
#define LED_DEVICE_HPP

#include <string>

class LedDevice {
public:
    explicit LedDevice(const std::string& basePath);

    bool available() const;
    bool set(bool on) const;
    bool get(bool& on) const;

private:
    std::string brightnessPath;
    bool available_;
};

#endif