#include "lcd_display.hpp"

#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace {
constexpr int WIDTH = 1024;
constexpr int HEIGHT = 600;
constexpr int LINE_BYTES = 2048;

constexpr std::uint16_t BLUE = 0x001f;
constexpr std::uint16_t GREEN = 0x07e0;
constexpr std::uint16_t RED = 0xf800;
constexpr std::uint16_t YELLOW = 0xffe0;
constexpr std::uint16_t GRAY = 0x8410;
constexpr std::uint16_t WHITE = 0xffff;
}

LcdDisplay::LcdDisplay()
    : framebufferFd_(-1),
      framebuffer_(nullptr),
      framebufferSize_(0),
      available_(false) {
    framebufferFd_ = open("/dev/fb0", O_RDWR);
    if (framebufferFd_ < 0) {
        return;
    }

    framebufferSize_ =
        static_cast<std::size_t>(LINE_BYTES) * HEIGHT;

    framebuffer_ = mmap(
        nullptr,
        framebufferSize_,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        framebufferFd_,
        0
    );

    if (framebuffer_ == MAP_FAILED) {
        framebuffer_ = nullptr;
        close(framebufferFd_);
        framebufferFd_ = -1;
        return;
    }

    available_ = true;
}

LcdDisplay::~LcdDisplay() {
    if (framebuffer_) {
        munmap(framebuffer_, framebufferSize_);
    }

    if (framebufferFd_ >= 0) {
        close(framebufferFd_);
    }
}

bool LcdDisplay::available() const {
    return available_;
}

void LcdDisplay::update(
    const DeviceStatus& status,
    std::size_t clientCount,
    const std::string& networkState) {
    if (!available_) {
        return;
    }

    auto* fb =
        static_cast<std::uint16_t*>(framebuffer_);

    const int stride = LINE_BYTES / 2;

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            fb[y * stride + x] = BLUE;
        }
    }

    const std::uint16_t networkColor =
        networkState == "UP" ? GREEN : RED;

    const std::uint16_t ledColor =
        status.ledOn ? GREEN : GRAY;

    const std::uint16_t keyColor =
        status.keyPressed ? YELLOW : GRAY;

    /*
     * 三个横向状态条：
     * 网络、LED、按键。
     */
    for (int y = 80; y < 160; ++y) {
        for (int x = 80; x < 944; ++x) {
            fb[y * stride + x] = networkColor;
        }
    }

    for (int y = 200; y < 280; ++y) {
        for (int x = 80; x < 944; ++x) {
            fb[y * stride + x] = ledColor;
        }
    }

    for (int y = 320; y < 400; ++y) {
        for (int x = 80; x < 944; ++x) {
            fb[y * stride + x] = keyColor;
        }
    }

    /*
     * 使用客户端数量改变底部白色指示条长度，
     * 暂时不绘制文字。
     */
    const std::size_t limitedClients =
        std::min<std::size_t>(clientCount, 16);

    const int clientWidth =
        static_cast<int>(limitedClients) * 50;

    for (int y = 440; y < 500; ++y) {
        for (int x = 80; x < 80 + clientWidth; ++x) {
            fb[y * stride + x] = WHITE;
        }
    }

    msync(framebuffer_, framebufferSize_, MS_SYNC);
}