#pragma once

#include "defines.hpp"
#include "socket.hpp"

#include <string>
#include <vector>
#include <cstdio>

namespace Density {
class Command
{
    static const std::vector<std::string> commandPatterns;

    protected:
        std::string name_;
        char *buffer_;
        
    public:
        Command(const std::string &name) : name_(name) {
            buffer_ = (char *)malloc(sizeof(char) * BUFFER_SIZE);
        }

        ~Command() {
            free(buffer_);
        }

        virtual void execute(ClientSocketSet &sockets, Socket &client, int &masterCount) { };

        static Command *parseCommand(const std::string &commandString);
};

class OutputCommand : public Command
{
    public:
        OutputCommand(const std::string &name) : Command(name) { }

        void execute(ClientSocketSet &sockets, Socket &client, int &masterCount) {
            int n = sprintf(buffer_, "%d\n", masterCount);
            if (n > 0)
                client.send(buffer_, n);
        }
};

class IncrCommand : public Command
{
    int value_;

    public:
        IncrCommand(const std::string &name, int value) : value_(value), Command(name) { }

        void execute(ClientSocketSet &sockets, Socket &client, int &masterCount) {
            masterCount += value_;
            int n = sprintf(buffer_, "%d\n", masterCount);
            if (n > 0)
                sockets.broadcast(buffer_, n);
        }
};

class DecrCommand : public Command
{
    int value_;

    public:
        DecrCommand(const std::string &name, int value) : value_(value), Command(name) { }

        void execute(ClientSocketSet &sockets, Socket &client, int &masterCount) {
            masterCount -= value_;
            int n = sprintf(buffer_, "%d\n", masterCount);
            if (n > 0)
                sockets.broadcast(buffer_, n);
        }
};
}
