#include "device_manager.hpp"
#include "protocol.hpp"

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

        std::lock_guard<std::mutex> lock(keyMutex);

        keyPressed = pressed;
        ++keyEventCount;
    }
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
            simulatedLedOn = on;
        } else if (!led.set(on)) {
            return "RES " + request.id +
                " ERROR led_write_failed\n";
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
        bool on = simulatedLedOn;
        const bool simulated = !led.available();

        if (!simulated && !led.get(on)) {
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