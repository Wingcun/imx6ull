#include "device_manager.hpp"
#include "protocol.hpp"
#include <chrono>

DeviceManager::DeviceManager() {
    if (key.available()) {
        keyThread = std::thread(
            &DeviceManager::keyMonitorLoop,
            this
        );
    }
}

DeviceManager::~DeviceManager() {
    stopping.store(true);

    if (keyThread.joinable()) {
        keyThread.join();
    }
}

void DeviceManager::keyMonitorLoop() {
    while (!stopping.load()) {
        bool pressed = false;

        if (!key.waitEvent(200, pressed)) {
            continue;
        }

        const auto now =
            std::chrono::system_clock::now();

        const auto timestampMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count();

        std::lock_guard<std::mutex> lock(keyMutex);

        /*
         * 过滤相同状态的重复事件：
         * pressed -> pressed 不重复记录；
         * released -> released 不重复记录。
         */
        if (pressed == keyPressed) {
            continue;
        }

        keyPressed = pressed;
        ++keyEventCount;

        if (keyEvents.size() >= MAX_KEY_EVENTS) {
            keyEvents.pop_front();
        }

        keyEvents.push_back({
            nextEventSequence++,
            pressed,
            static_cast<std::uint64_t>(timestampMs)
        });
    }
}

bool DeviceManager::popEvent(std::string& event) {
    std::lock_guard<std::mutex> lock(keyMutex);

    if (keyEvents.empty()) {
        return false;
    }

    const KeyEvent keyEvent = keyEvents.front();
    keyEvents.pop_front();

    event =
        "EVENT KEY {\"sequence\":" +
        std::to_string(keyEvent.sequence) +
        ",\"state\":\"" +
        (keyEvent.pressed ? "pressed" : "released") +
        "\",\"timestamp_ms\":" +
        std::to_string(keyEvent.timestampMs) +
        "}\n";

    return true;
}

std::string DeviceManager::execute(const std::string& requestLine) {
    ParsedRequest request;

    if (!parseRequest(requestLine, request)) {
        return "RES 0 ERROR bad_request\n";
    }

    if (request.command == "LED") {
        const bool on = request.args[1] == "ON";

        if (request.args[1] != "ON" &&
            request.args[1] != "OFF") {
            return "RES " + request.id +
                " ERROR bad_request\n";
        }

        const bool simulated = !led.available();

        if (simulated) {
            ledOn = on;
        } else if (!led.set(on)) {
            return "RES " + request.id +
                " ERROR led_write_failed\n";
        } else {
            ledOn = on;
        }

        return "RES " + request.id +
            " OK {\"led\":\"" +
            (on ? "on" : "off") +
            "\",\"simulated\":" +
            (simulated ? "true" : "false") +
            "}\n";
    }

    if (request.command == "STATUS" &&
        request.args.size() == 1 &&
        request.args[0] == "GET") {
        bool on = ledOn;
        const bool simulated = !led.available();

        if (!simulated && led.readable() && !led.get(on)) {
            return "RES " + request.id +
                " ERROR led_read_failed\n";
        }

        return "RES " + request.id +
            " OK {\"led\":\"" +
            (on ? "on" : "off") +
            "\",\"simulated\":" +
            (simulated ? "true" : "false") +
            "}\n";
    }

    if (request.command == "KEY") {
        if (request.args.size() != 1 ||
            request.args[0] != "GET") {
            return "RES " + request.id +
                " ERROR bad_request\n";
        }

        if (!key.available()) {
            return "RES " + request.id +
                " OK {\"key\":\"released\","
                "\"available\":false,"
                "\"events\":0}\n";
        }

        std::lock_guard<std::mutex> lock(keyMutex);

        return "RES " + request.id +
            " OK {\"key\":\"" +
            (keyPressed ? "pressed" : "released") +
            "\",\"available\":true,"
            "\"events\":" +
            std::to_string(keyEventCount) +
            "}\n";
    }

    return processRequestLine(requestLine);
}