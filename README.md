# i.MX6ULL Network Device Control

面向 i.MX6ULL 的 Embedded Linux 网络化设备监控与控制项目。

项目在 Ubuntu 上完成 C++ 主机版验证，再通过 ARM 交叉编译部署到开发板，实现：

- TCP/epoll 多客户端通信
- 自定义文本协议
- LED 字符设备控制
- GPIO 按键事件采集
- Device Tree 与 platform driver
- LCD framebuffer 状态显示
- Qt HMI 远程监控与控制
- SysV init 开机自启动

## 系统架构

```text
PC Qt HMI / TCP Client
          │
          ▼
   TCP Server + epoll
          │
          ▼
     Protocol Parser
          │
          ▼
     DeviceManager
      ┌───┼────┬────┐
      ▼   ▼    ▼    ▼
     LED KEY  LCD  Events
      │   │    │    │
      ▼   ▼    ▼    ▼
  /dev/  input /dev/  TCP
  imx6ull event2 fb0  push
  device
      │
      ▼
 platform_driver
      │
      ▼
 Device Tree GPIO
      │
      ▼
   i.MX6ULL Hardware
```

## 已完成功能

- 非阻塞 TCP Server
- epoll 多客户端事件循环
- TCP 半包和粘包处理
- 请求 ID 与换行分帧
- 结构化响应和错误响应
- TCP 主动事件推送
- 多客户端事件广播
- 完整发送处理
- LED 字符设备 `read/write`
- LED `ioctl/poll` 接口
- platform driver `probe/remove`
- Device Tree GPIO 资源获取
- KEY input event 异步监控
- 按键事件去重和事件队列
- LCD framebuffer RGB565 显示
- Qt6 TCP HMI
- Qt HMI 断线自动重连
- SysV init 开机自启动
- ARM 交叉编译和板端部署
- 协议冒烟测试脚本

## 目录结构

```text
.
├── CMakeLists.txt
├── cmake/                         # ARM 工具链配置
├── src/                           # C++ 应用实现
├── include/                       # C++ 头文件
├── config/                        # 运行配置
├── driver/                        # Linux 内核模块
├── tools/                         # 测试工具和脚本
├── app/qt_hmi/                    # Qt6 HMI 客户端
├── scripts/                       # 部署和运行脚本
├── docs/                          # 开发记录
├── build/                         # x86-64 构建目录，不提交
└── build-arm/                     # ARM 构建目录，不提交
```

## 主机版编译

主机版用于验证 TCP、协议和设备抽象：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
```

启动：

```bash
./build/device-control
```

主机没有开发板设备节点时，LED 会进入模拟模式：

```text
"simulated":true
```

## ARM 交叉编译

确认 `cmake/arm-linux-gnueabihf-toolchain.cmake` 中的工具链路径正确：

```bash
cmake -S . -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-arm -j2
```

检查架构：

```bash
file build-arm/device-control
```

应显示：

```text
ELF 32-bit
ARM
```

不要把以下主机程序复制到开发板：

```text
build/device-control
```

## 内核模块编译

```bash
make -C driver clean
make -C driver
```

检查模块：

```bash
file driver/platform_probe.ko
modinfo -F vermagic driver/platform_probe.ko
```

目标内核版本：

```text
Linux 4.1.15
ARMv7
```

## Device Tree 和 platform driver

测试节点：

```dts
platform-probe-test {
    compatible = "training,imx6ull-platform-probe";
    led-gpios = <&gpio1 3 GPIO_ACTIVE_LOW>;
    status = "okay";
};
```

驱动通过：

```c
of_get_named_gpio_flags()
```

获取 GPIO，并在 `probe()` 中完成：

```text
gpio_request()
gpio_direction_output()
misc_register()
```

成功日志：

```text
platform_probe: probe successful, led_gpio=3, active_low=1
```

## 字符设备接口

设备节点：

```text
/dev/imx6ull_device
```

支持：

```text
open
read
write
ioctl
poll
```

示例：

```bash
echo 1 > /dev/imx6ull_device
cat /dev/imx6ull_device

echo 0 > /dev/imx6ull_device
cat /dev/imx6ull_device
```

预期：

```text
1
0
```

## LED、KEY 和事件

LED 使用：

```text
/dev/imx6ull_device
```

KEY 使用：

```text
/dev/input/event2
```

按键事件处理流程：

```text
/dev/input/event2
        ↓
poll()
        ↓
KeyDevice
        ↓
DeviceManager 后台线程
        ↓
状态去重
        ↓
事件队列
        ↓
TCP EVENT 推送
```

主动事件格式：

```text
EVENT KEY {"sequence":1,"state":"pressed","timestamp_ms":...}
EVENT KEY {"sequence":2,"state":"released","timestamp_ms":...}
```

## 通信协议

请求格式：

```text
REQ <id> <command> [arguments...]
```

每行一个请求，使用 `LF` 换行，也兼容 `CRLF`。

当前命令：

```text
REQ 1 PING
REQ 2 STATUS GET
REQ 3 LED SET ON
REQ 4 LED SET OFF
REQ 5 KEY GET
REQ 6 CLIENTS GET
```

响应示例：

```text
RES 1 OK {"pong":true}
RES 2 OK {"led":"off","simulated":false}
RES 5 OK {"key":"released","available":true,"events":0}
RES 6 OK {"clients":1}
```

错误示例：

```text
RES 9 ERROR bad_request
RES 10 ERROR unknown_command
RES 11 ERROR led_write_failed
```

客户端必须区分：

```text
RES ...
EVENT ...
```

不能假设一次 `recv()` 只返回一条消息。

## 协议冒烟测试

开发板服务器运行后，在 Ubuntu 执行：

```bash
python3 tools/protocol_smoke.py
```

测试内容包括：

```text
PING
STATUS GET
KEY GET
CLIENTS GET
LED SET ON
LED SET OFF
```

真实硬件模式下，LED 响应应包含：

```text
"simulated":false
```

## 部署到开发板

复制 ARM 应用：

```bash
scp build-arm/device-control \
    root@<BOARD_IP>:/tmp/device-control-latest
```

安装：

```bash
ssh root@<BOARD_IP>

cp /usr/local/bin/device-control \
   /usr/local/bin/device-control.backup

cp /tmp/device-control-latest \
   /usr/local/bin/device-control

chmod 755 /usr/local/bin/device-control
```

复制内核模块：

```bash
scp driver/platform_probe.ko \
    root@<BOARD_IP>:/tmp/platform_probe.ko
```

安装模块：

```bash
mkdir -p /usr/local/lib
cp /tmp/platform_probe.ko \
   /usr/local/lib/platform_probe.ko
chmod 644 /usr/local/lib/platform_probe.ko
```

## SysV 开机自启动

启动脚本：

```text
/etc/init.d/device-control
```

服务启动前会尝试加载：

```text
/usr/local/lib/platform_probe.ko
```

手动操作：

```bash
/etc/init.d/device-control start
/etc/init.d/device-control stop
/etc/init.d/device-control restart
/etc/init.d/device-control status
```

检查端口：

```bash
netstat -lnt 2>/dev/null | grep ':8000'
```

检查进程：

```bash
pidof device-control
```

开机验收：

```bash
reboot
```

重启后确认：

```bash
/etc/init.d/device-control status
netstat -lnt 2>/dev/null | grep ':8000'
```

## LCD framebuffer

LCD 设备：

```text
/dev/fb0
```

当前参数：

```text
驱动：mxs-lcdif
分辨率：1024×600
色深：16 bpp
格式：RGB565
行长度：2048 bytes
刷新率：约 60 Hz
```

检查：

```bash
fbset -i
```

纯色测试：

```bash
/tmp/fb_color_test
```

状态显示测试：

```bash
/tmp/fb_status
```

LCD 状态显示模块通过 `LcdDisplay` 访问：

```text
/dev/fb0
```

显示内容包括：

```text
Network
LED
KEY
Events
Clients
```

## Qt HMI

Qt 客户端目录：

```text
app/qt_hmi
```

编译：

```bash
cd app/qt_hmi

qmake6 qt_hmi.pro
make -j2
```

运行：

```bash
./qt_hmi
```

Qt HMI 支持：

- TCP 连接
- LED 开关控制
- KEY 状态显示
- 按键事件日志
- 事件计数
- 当前客户端数量
- 断线自动重连

## 常见问题

### Connection refused

服务器未监听端口：

```bash
netstat -lnt 2>/dev/null | grep ':8000'
```

### unknown_command

开发板运行的是旧版 ARM 程序，需要重新编译并部署：

```bash
cmake --build build-arm --clean-first
```

### simulated:true

表示：

```text
/dev/imx6ull_device 不存在
```

常见原因：

- platform_probe 模块未加载；
- 默认 DTB 没有 `platform-probe-test`；
- 当前使用的是原厂 DTB；
- 服务启动顺序未加载模块。

### Exec format error

将 x86-64 主机程序复制到了 ARM 开发板。应使用：

```text
build-arm/device-control
```

### .ko 加载失败

检查：

```bash
uname -r
modinfo -F vermagic driver/platform_probe.ko
```

并确认：

```text
内核版本
CONFIG_MODULES
CONFIG_MODVERSIONS
Module.symvers
```

### Qt 显示 Network: disconnected

检查：

```bash
nc -vz <BOARD_IP> 8000
```

确认开发板服务器正在监听 8000 端口。

## 日常构建

只修改 C++ 源码：

```bash
cmake --build build
```

修改 `CMakeLists.txt`、增加源文件或切换编译器：

```bash
cmake -S . -B build
```

ARM 版本：

```bash
cmake --build build-arm
```

## Git 管理

建议不要提交：

```text
build/
build-arm/
*.o
*.ko
*.mod
Module.symvers
modules.order
Qt 可执行文件
完整 Linux 内核源码
工具链
```

`.gitignore` 至少包含：

```gitignore
/build/
/build-arm/
*.o
*.ko
*.mod
*.mod.c
Module.symvers
modules.order
.vscode/
app/qt_hmi/qt_hmi
tools/fb_color_test
tools/fb_status
```

提交示例：

```bash
git status
git add .
git commit -m "update project"
git push
```

## 已验证环境

主机：

```text
Ubuntu 24.04
C++14/C++17
CMake
Qt 6.4.2
x86-64
```

开发板：

```text
i.MX6ULL
ARMv7
Linux 4.1.15
EMMC 启动
7 寸 1024×600 RGB LCD
```

设备：

```text
/dev/imx6ull_device
/dev/input/event2
/dev/fb0
```

## 当前项目状态

```text
TCP/epoll                 完成
字符设备驱动              完成
Device Tree               完成
platform_driver           完成
LED/KEY                   完成
事件队列                  完成
多客户端广播              完成
LCD framebuffer            完成
Qt HMI                    完成
开机自启动                 完成
冒烟测试脚本                完成
默认 DTB 永久部署           需谨慎处理
```

本项目面向学习和设备软件工程实践，当前协议运行在可信开发网段，未实现认证和 TLS。