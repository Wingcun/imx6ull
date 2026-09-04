#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "imx6ull_device_uapi.h"

int main(void)
{
    int fd;
    int value;

    fd = open("/dev/imx6ull_device", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    value = 1;
    if (ioctl(fd, IMX6ULL_DEVICE_SET, &value) < 0) {
        perror("ioctl SET");
        close(fd);
        return 1;
    }

    value = 0;
    if (ioctl(fd, IMX6ULL_DEVICE_GET, &value) < 0) {
        perror("ioctl GET");
        close(fd);
        return 1;
    }

    printf("device state = %d\n", value);

    close(fd);
    return 0;
}