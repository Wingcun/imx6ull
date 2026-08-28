#ifndef KEY_DEVICE_HPP
#define KEY_DEVICE_HPP

#include <string>

class KeyDevice {
public:
    explicit KeyDevice(const std::string& path);
    ~KeyDevice();

    KeyDevice(const KeyDevice&) = delete;
    KeyDevice& operator=(const KeyDevice&) = delete;

    bool available() const;
    bool waitEvent(int timeoutMs, bool& pressed);

private:
    int fd_;
};

#endif