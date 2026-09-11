#include <array>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

static std::array<uint8_t, 7> glyph(char c) {
    switch (c) {
    case 'N': return {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    case 'E': return {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f};
    case 'T': return {0x1f,0x04,0x04,0x04,0x04,0x04,0x04};
    case 'L': return {0x10,0x10,0x10,0x10,0x10,0x10,0x1f};
    case 'D': return {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e};
    case 'O': return {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e};
    case 'F': return {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10};
    case 'K': return {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
    case 'Y': return {0x11,0x11,0x0a,0x04,0x04,0x04,0x04};
    case 'C': return {0x0f,0x10,0x10,0x10,0x10,0x10,0x0f};
    case 'U': return {0x11,0x11,0x11,0x11,0x11,0x11,0x0e};
    case 'R': return {0x1e,0x11,0x11,0x1e,0x14,0x12,0x11};
    case 'S': return {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e};
    case 'A': return {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11};
    case 'I': return {0x1f,0x04,0x04,0x04,0x04,0x04,0x1f};
    case 'M': return {0x11,0x1b,0x15,0x15,0x11,0x11,0x11};
    case 'X': return {0x11,0x11,0x0a,0x04,0x0a,0x11,0x11};
    case ' ': return {0,0,0,0,0,0,0};
    default:  return {0,0,0,0,0,0,0};
    }
}

int main() {
    constexpr int width = 1024;
    constexpr int height = 600;
    constexpr int lineBytes = 2048;

    const uint16_t background = 0x001f; // 蓝色
    const uint16_t foreground = 0xffff; // 白色
    const int scale = 5;

    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) return 1;

    const std::size_t size =
        static_cast<std::size_t>(lineBytes) * height;

    void* mapped = mmap(
        nullptr, size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, 0);

    if (mapped == MAP_FAILED) {
        close(fd);
        return 1;
    }

    auto* fb = static_cast<uint16_t*>(mapped);

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            fb[y * (lineBytes / 2) + x] = background;

    const char* lines[] = {
        "NET",
        "LED ON",
        "KEY",
        "KEY RELEASED",
        "CLIENT"
    };

    int y0 = 80;

    for (const char* line : lines) {
        int x0 = 80;

        for (const char* p = line; *p; ++p) {
            const auto rows = glyph(*p);

            for (int gy = 0; gy < 7; ++gy) {
                for (int gx = 0; gx < 5; ++gx) {
                    if (!(rows[gy] & (1 << (4 - gx))))
                        continue;

                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            int x = x0 + gx * scale + sx;
                            int y = y0 + gy * scale + sy;

                            fb[y * (lineBytes / 2) + x] =
                                foreground;
                        }
                    }
                }
            }

            x0 += 6 * scale;
        }

        y0 += 60;
    }

    msync(mapped, size, MS_SYNC);
    munmap(mapped, size);
    close(fd);
    return 0;
}