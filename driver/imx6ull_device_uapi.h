#ifndef IMX6ULL_DEVICE_UAPI_H
#define IMX6ULL_DEVICE_UAPI_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

#define IMX6ULL_DEVICE_IOC_MAGIC 'N'

#define IMX6ULL_DEVICE_GET \
    _IOR(IMX6ULL_DEVICE_IOC_MAGIC, 0x01, int)

#define IMX6ULL_DEVICE_SET \
    _IOW(IMX6ULL_DEVICE_IOC_MAGIC, 0x02, int)

#endif
