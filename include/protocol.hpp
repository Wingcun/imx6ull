#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP
#include<vector>
#include<string>

struct ParsedRequest
{
    std::string id;
    std::string command;
    std::vector<std::string> args;
};

bool parseRequest(const std::string& line, ParsedRequest& request);

std::string processRequestLine(const std::string& line);


#endif