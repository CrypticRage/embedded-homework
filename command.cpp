 #include "command.hpp"

namespace Density {
const std::vector<std::string> Command::commandPatterns = {
    "INCR",
    "DECR",
    "OUTPUT"
};

Command *Command::parseCommand(const std::string &commandString)
{
    Command *command;
    
    for (std::string pattern : commandPatterns) {
        size_t index = commandString.find(pattern);
        if (index == std::string::npos) {
            continue;
        }

        if (pattern == "OUTPUT") {
            command = new OutputCommand(pattern);
            return command;
        }

        char spaceDelim = ' ';
        size_t spaceIndex = commandString.find(spaceDelim);
        int value = stoi(commandString.substr(spaceIndex + 1, std::string::npos));

        if (pattern == "INCR") {
            command = new IncrCommand(pattern, value);
            return command;
        }

        if (pattern == "DECR") {
            command = new DecrCommand(pattern, value);
            return command;
        }
    }

    return nullptr;
}
}
