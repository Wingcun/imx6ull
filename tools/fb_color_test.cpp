#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    const int width = 1024;
    const int height = 600;
    const int lineBytes = 2048;
    const uint16_t color = 0x001f; // RGB565 蓝色

    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) {
        perror("open /dev/fb0");
        return 1;
    }

    const std::size_t size =
        static_cast<std::size_t>(lineBytes) * height;

    void* mapped = mmap(
        nullptr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    auto* framebuffer =
        static_cast<uint16_t*>(mapped);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            framebuffer[y * (lineBytes / 2) + x] = color;
        }
    }

    msync(mapped, size, MS_SYNC);
    munmap(mapped, size);
    close(fd);

    std::printf("blue framebuffer test completed\n");
    return 0;
}