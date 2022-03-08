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
Command *currentCommand = nullptr;

MasterSocket masterSocket(PORT);
ClientSocketSet clientSockets(MAX_CLIENTS);

fd_set readDescSet;
char buffer[BUFFER_SIZE];

void signalHandler(int signum)
{
    std::cout << "caught signal: " << signum << std::endl;
    
    // clean up
    if (nullptr != currentCommand) {
        delete currentCommand;
        currentCommand = nullptr;
    }

    std::cout << "exiting, final count: " << masterCount << std::endl;
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
        clientSockets.addAllToSet(readDescSet);

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
            clientSockets.add(socketDesc);
            std::cout << "Connection established!" << std::endl;
        }
        
        // check the client sockets
        std::vector<Socket> readSockets = clientSockets.getSocketsInSet(readDescSet);
        // std::cout << "Socket read count: " << readSockets.size() << std::endl;
        for (Socket socket : readSockets) {
            int bytes = socket.read(buffer, BUFFER_SIZE);
            if (bytes == 0) {
                std::cout << "closing socket: " << socket.desc() << std::endl;
                socket.close();
                clientSockets.remove(socket.desc());
            } 
            else {
                currentCommand = Command::parseCommand(std::string(buffer));
                if (nullptr == currentCommand) {
                    continue;
                }

                currentCommand->execute(clientSockets, socket, masterCount);
                delete currentCommand;
                currentCommand = nullptr;
            }
        }
    }

    return 0;
}
