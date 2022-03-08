#include "socket.hpp"
#include "command.hpp"
#include "defines.hpp"

#include <cstdio>
#include <cstdlib>
#include <csignal>

#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <memory>

using namespace Density;

int masterCount = 0;
MasterSocket masterSocket(PORT);
ClientSocketSet clientSockets(MAX_CLIENTS);
std::array<char, BUFFER_SIZE> buffer;

void signalHandler(int signum)
{
    std::cout << "caught signal: " << signum << std::endl;
    
    clientSockets.closeAll();
    masterSocket.close();

    std::cout << "exiting, final count: " << masterCount << std::endl;
    exit(0);
}

int main(void)
{
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    fd_set readDescSet;

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
            std::cout << "Connection established. Clients: " << clientSockets.size() << std::endl;
        }
        
        // check the client sockets
        std::vector<Socket> readSockets = clientSockets.getSocketsInSet(readDescSet);
        // std::cout << "Socket read count: " << readSockets.size() << std::endl;
        for (Socket &socket : readSockets) {
            int bytes = socket.read(buffer.data(), BUFFER_SIZE);
            if (bytes == 0) {
                std::cout << "closing socket: " << socket.desc() << std::endl;
                socket.close();
                clientSockets.remove(socket.desc());
            } 
            else {
                std::unique_ptr<Command> currentCommand = Command::parseCommand(std::string(buffer.data()));
                if (!currentCommand) {
                    continue;
                }

                currentCommand->execute(clientSockets, socket, masterCount);
            }
        }
    }

    return 0;
}
