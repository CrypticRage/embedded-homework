#include "command.hpp"

namespace Density {
const std::vector<std::string> Command::commandPatterns = {
    "INCR",
    "DECR",
    "OUTPUT"
};

std::unique_ptr<Command> Command::parseCommand(const std::string &commandString)
{   
    for (std::string pattern : commandPatterns) {
        size_t index = commandString.find(pattern);
        if (index == std::string::npos) {
            continue;
        }

        if (pattern == "OUTPUT") {
            return std::make_unique<OutputCommand>(pattern);
        }

        char spaceDelim = ' ';
        size_t spaceIndex = commandString.find(spaceDelim);

        // stoi() is smart enought to stop parsing after the last numeric value,
        // even if there's more junk at the end of the string
        int value = stoi(commandString.substr(spaceIndex + 1, std::string::npos));

        if (pattern == "INCR") {
            return std::make_unique<IncrCommand>(pattern, value);
        }

        if (pattern == "DECR") {
            return std::make_unique<DecrCommand>(pattern, value);
        }
    }

    return std::unique_ptr<Command>(nullptr);
}
}
