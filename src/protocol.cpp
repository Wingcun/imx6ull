#include "protocol.hpp"

#include <sstream>

bool parseRequest(
    const std::string& line,
    ParsedRequest& request
) {
    std::istringstream stream(line);

    std::string prefix;

    if (!(stream >> prefix
                 >> request.id
                 >> request.command)) {
        return false;
    }

    if (prefix != "REQ") {
        return false;
    }

    request.args.clear();

    std::string argument;
    while (stream >> argument) {
        request.args.push_back(argument);
    }

    return true;
}

std::string processRequestLine(
    const std::string& line
) {
    ParsedRequest request;

    if (!parseRequest(line, request)) {
        return "RES 0 ERROR bad_request\n";
    }

    if (request.command == "PING" &&
        request.args.empty()) {
        return "RES " + request.id +
               " OK {\"pong\":true}\n";
    }

    if (request.command == "STATUS" &&
        request.args.size() == 1 &&
        request.args[0] == "GET") {
        return "RES " + request.id +
               " OK {\"server\":\"up\"}\n";
    }

    return "RES " + request.id +
           " ERROR unknown_command\n";
}