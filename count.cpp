#include "socket.hpp"
#include "command.hpp"
#include "defines.hpp"

#include <cstdio>
#include <cstdlib>
#include <csignal>

#include <vector>
#include <iostream>
#include <string>

using namespace Density;

int masterCount = 0;
MasterSocket masterSocket(PORT);
ClientSocketSet clientSockets(MAX_CLIENTS);
fd_set readDescSet;
char buffer[BUFFER_SIZE];

void signalHandler(int signum)
{
    std::cout << "caught signal: " << signum << std::endl;
    exit(signum);
}

int main(void)
{
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    if (!masterSocket.init()) {
        exit(1);
    }

    while (true) {
        // clear the socket set
        FD_ZERO(&readDescSet);
        
        // add the master socket to the set
        if (!masterSocket.addToSet(readDescSet)) {
            exit(1);
        }

        // add the child sockets to the set
        clientSockets.addAllActiveToSet(readDescSet);

        // wait indefinetely for activity on a socket desc
        int maxDesc = clientSockets.maxDesc() > masterSocket.desc() ?
            clientSockets.maxDesc() : masterSocket.desc();
        int active = select(maxDesc + 1, &readDescSet, NULL, NULL, NULL);
        
        if ((active < 0) && (errno != EINTR)) {
            perror("select() failed\n");
        }

        // check the master socket
        if (masterSocket.isSet(readDescSet)) {
            int socketDesc = masterSocket.acceptConnection();
            if (socketDesc < 0) {
                perror("accept() failed\n");
                break;
            }
            clientSockets.addActive(socketDesc);
            std::cout << "Connection established!" << std::endl;
        }
        
        // check the client sockets
        std::vector<Socket> readSockets = clientSockets.getSocketsWithData(readDescSet);
        std::cout << "Socket read count: " << readSockets.size() << std::endl;
        for (Socket socket : readSockets) {
            int bytes = socket.read(buffer, BUFFER_SIZE);
            if (bytes == 0) {
                std::cout << "closing socket: " << socket.desc() << std::endl;
            }
            else {
                Command *command = Command::parseCommand(std::string(buffer));
                if (nullptr == command) {
                    continue;
                }

                command->execute(clientSockets, socket, masterCount);
                delete command;
            }
        }
    }

    std::cout << "Exiting" << std::endl;

    return 0;
}
