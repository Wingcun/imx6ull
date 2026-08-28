#include "key_device.hpp"

#include <cerrno>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <unistd.h>

KeyDevice::KeyDevice(const std::string& path)
    : fd_(-1) {
    fd_ = open(
        path.c_str(),
        O_RDONLY | O_NONBLOCK | O_CLOEXEC
    );
}

KeyDevice::~KeyDevice() {
    if (fd_ >= 0) {
        close(fd_);
    }
}

bool KeyDevice::available() const {
    return fd_ >= 0;
}

bool KeyDevice::waitEvent(
    int timeoutMs,
    bool& pressed
) {
    if (fd_ < 0) {
        return false;
    }

    pollfd descriptor{};
    descriptor.fd = fd_;
    descriptor.events = POLLIN;

    int result = poll(
        &descriptor,
        1,
        timeoutMs
    );

    if (result <= 0 ||
        !(descriptor.revents & POLLIN)) {
        return false;
    }

    while (true) {
        input_event event{};

        ssize_t size = read(
            fd_,
            &event,
            sizeof(event)
        );

        if (size == sizeof(event)) {
            if (event.type == EV_KEY &&
                (event.value == 0 ||
                 event.value == 1)) {
                pressed = event.value == 1;
                return true;
            }

            continue;
        }

        if (size < 0 &&
            (errno == EAGAIN ||
             errno == EWOULDBLOCK)) {
            return false;
        }

        return false;
    }
}