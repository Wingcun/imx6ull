# i.MX6ULL Network Device Control

一个面向 i.MX6ULL 的 Embedded Linux 网络化设备监控与控制项目。项目在 Ubuntu 上完成 C++14 主机版验证，再通过 ARM 交叉编译部署到开发板，实现 TCP/epoll 通信、LED 控制和按键状态读取。

## 功能

- 非阻塞 TCP Server 与 epoll 多客户端事件循环
- 以换行符分帧，处理 TCP 半包和粘包
- 请求 ID、命令解析、结构化响应和错误响应
- LED 控制：开发板使用 Linux LED sysfs，主机版可回退到模拟状态
- KEY 监听：通过 `poll()` 读取 Linux input event 设备
- SysV init 开机自启动
- Ubuntu x86-64 主机版与 ARM 目标版分离构建

## 软件架构

```text
TCP client
    -> TcpServer + epoll
    -> 按行拆帧
    -> Protocol
    -> DeviceManager
       -> LedDevice -> /sys/class/leds/sys-led/brightness
       -> KeyDevice -> /dev/input/event2
```

## 目录

```text
.
├── CMakeLists.txt
├── cmake/                         # ARM 工具链配置
├── src/                           # C++ 应用实现
├── include/                       # C++ 头文件
├── config/                        # 运行配置
├── driver/                        # 内核模块实验代码
├── scripts/                       # 部署和运行脚本
├── docs/                          # 开发记录
├── build/                         # Ubuntu 主机构建产物，不提交
└── build-arm/                     # ARM 构建产物，不提交
```

## 主机版编译

Ubuntu 主机版用于验证网络、协议和设备抽象。首次配置或修改 `CMakeLists.txt` 时执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
```

启动：

```bash
./build/device-control
```

## ARM 交叉编译

将 `cmake/arm-linux-gnueabihf-toolchain.cmake` 中的工具链根目录改为本机实际路径，然后执行：

```bash
cmake -S . -B build-arm \
  -DCMAKE_TOOLCHAIN_FILE=cmake/arm-linux-gnueabihf-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build-arm -j2
file build-arm/device-control
```

`file` 应显示 `ELF 32-bit` 和 `ARM`。不要把 `build/device-control`（x86-64）复制到 ARM 开发板。

## 部署到开发板

将 `<BOARD_IP>` 替换为开发板当前地址：

```bash
scp build-arm/device-control root@<BOARD_IP>:/tmp/device-control.new
ssh root@<BOARD_IP>
```

在开发板执行：

```bash
mv /tmp/device-control.new /usr/local/bin/device-control
chmod 755 /usr/local/bin/device-control
/usr/local/bin/device-control
```

正式部署使用 `/usr/local/bin/device-control`，不要依赖 `/tmp`，因为 `/tmp` 的内容可能在重启后丢失。

## 通信协议

一行是一个请求，行尾使用 `LF`（也兼容 `CRLF`）：

```text
REQ <id> <command> [arguments...]
```

当前命令示例：

```text
REQ 1 PING
REQ 2 STATUS GET
REQ 3 LED SET ON
REQ 4 LED SET OFF
REQ 5 KEY GET
```

响应示例：

```text
RES 1 OK {"pong":true}
RES 3 OK {"led":"on","simulated":false}
RES 5 OK {"key":"released","available":true,"events":0}
RES 9 ERROR unknown_command
```

## 快速测试

开发板 Server 运行后，在 Ubuntu 或其他同网段客户端执行：

```bash
python3 - <<'PY'
import socket

board_ip = "<BOARD_IP>"
requests = [
    b"REQ 1 PING\n",
    b"REQ 2 STATUS GET\n",
    b"REQ 3 LED SET ON\n",
    b"REQ 4 KEY GET\n",
]

with socket.create_connection((board_ip, 8000), timeout=3) as client:
    for request in requests:
        client.sendall(request)
        print(client.recv(1024).decode().strip())
PY
```

按键事件计数应在按下、松开后增加；`LED SET ON/OFF` 应对应实体 LED 的亮灭。

## 开机自启动

目标系统使用 SysV init。安装启动脚本后：

```bash
chmod 755 /etc/init.d/device-control
ln -s /etc/init.d/device-control /etc/rc5.d/S50device-control
```

手动验证：

```bash
/etc/init.d/device-control start
/etc/init.d/device-control status
/etc/init.d/device-control stop
```

确认手动启动和停止正常后再重启验收。重启后检查：

```bash
pidof device-control
netstat -lnt 2>/dev/null | grep ':8000'
```

## 构建目录和版本管理

日常修改 C++ 源码后只需：

```bash
cmake --build build
```

修改 `CMakeLists.txt`、新增源文件或切换编译器后重新配置：

```bash
cmake -S . -B build
```

不要提交构建产物、工具链或完整 Linux 内核源码。建议 `.gitignore` 至少包含：

```gitignore
/build/
/build-arm/
*.o
*.ko
*.mod
Module.symvers
modules.order
.vscode/
```

## 常见问题

- `Connection refused`：目标端口没有 Server 监听，先检查进程和端口。
- 收到旧的 `Hello from server!`：运行了旧程序，统一使用 `./build/device-control` 或 `/usr/local/bin/device-control`。
- `Exec format error`：把 x86-64 程序复制到了 ARM 板，重新使用 `build-arm/device-control`。
- `available:false`：目标设备节点不存在，检查 `/sys/class/leds/sys-led` 或 `/dev/input/event2`。
- `.ko` 加载失败：检查运行内核版本、`vermagic`、`.config` 和 `Module.symvers` 是否匹配。

## 已验证环境

- Ubuntu 主机版：C++14、CMake、x86-64
- 目标板：i.MX6ULL、ARM、Linux 4.1.15
- 目标输入设备：`/dev/input/event2`
- 目标 LED 接口：`/sys/class/leds/sys-led`

本项目面向学习和设备软件工程实践，当前协议运行在可信开发网段，未实现认证和 TLS。
