#pragma once

#include "defines.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <iostream>
#include <vector>

#define MAX_PENDING_CONNECTIONS 10

namespace Density {
class Socket
{
    protected:
        int desc_;

    public:
        Socket() : desc_(0) { }
        int desc() { return desc_; }
        void setDesc(int d) { desc_ = d; }

        int read(char *buffer, int count) {
            if (buffer == nullptr) return 0;
            if (count < 0) return 0;
            if (desc_ == 0) return 0;

            return ::read(desc_, buffer, count);
        }

        int send(char *buffer, int count) {
            if (buffer == nullptr) return 0;
            if (count < 0) return 0;
            if (desc_ == 0) return 0;

            return ::send(desc_, buffer, count, 0);
        }

        bool addToSet(fd_set &descSet) {
            if (desc_ > 0) {
                FD_SET(desc_, &descSet);
                return true;
            }
            return false;
        }

        bool isSet(fd_set &descSet) {
            if (desc_ > 0) {
                return FD_ISSET(desc_, &descSet);
            }
            return false;
        }

        void close() {
            if (desc_ > 0)
                ::close(desc_);
        }
};

class MasterSocket : public Socket
{
    struct sockaddr_in addr_;
    uint16_t port_;

    public:
        MasterSocket(uint16_t port) : port_(port) { }

        bool init() {
            if ((desc_ = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
                perror("socket() failed");
                return false;
            }

            const int opt = TRUE;
            if (setsockopt(desc_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                perror("setsocketopt() failed");
                return false;
            }

            addr_.sin_family = AF_INET;
            addr_.sin_addr.s_addr = INADDR_ANY;
            addr_.sin_port = htons(port_);            

            if (bind(desc_, (struct sockaddr *)&addr_, sizeof(addr_)) < 0) {
                perror("bind() failure");
                return false;
            }

            if (listen(desc_, MAX_PENDING_CONNECTIONS) < 0) {
                perror("listen() failed");
                return false;
            }

            std::cout << "master socket: listening on port " << port_ << std::endl;
            return true;
        }

        int acceptConnection() {
            int addrLength = sizeof(addr_);
            int newSocket = accept(desc_, (struct sockaddr *)&addr_, (socklen_t *)&addrLength);

            if (newSocket < 0) {
                perror("accept() failed");
                return -1;
            }

            return newSocket;
        }
};

class ClientSocketSet
{
    std::vector<Socket> sockets_;
    int maxSockets_;
    int maxSocketDesc_;
    int nextIndex_;

    public:
        ClientSocketSet(int maxSockets) : maxSockets_(maxSockets), nextIndex_(0) {
            for (int i = 0; i < maxSockets; i++) {
                sockets_.emplace_back();
            }
        }

        void broadcast(char *buffer, int count) {
            if (sockets_.empty()) return;
            if (buffer == nullptr) return;
            if (count < 0) return;

            for (Socket socket : sockets_) {
                if (socket.desc() > 0) {
                    socket.send(buffer, count);
                }
            }
        }

        void addActive(int desc) {
            if (nextIndex_ >= maxSockets_) return;
            sockets_[nextIndex_].setDesc(desc);
            if (desc > maxSocketDesc_) maxSocketDesc_ = desc;

            for (int i = 0; i < sockets_.size(); i++) {
                if (sockets_[i].desc() == 0) nextIndex_ = i;
            }
        }

        bool addAllActiveToSet(fd_set &descSet) {
            for (Socket socket : sockets_) {
                if(socket.desc() > 0) {
                    if (!socket.addToSet(descSet)) return false;
                }
            }            
            return true;
        }

        std::vector<Socket> getSocketsWithData(fd_set &descSet) {
            std::vector<Socket> dataSockets;
            for (Socket socket : sockets_) {
                if (socket.isSet(descSet)) dataSockets.push_back(socket);
            }
            return dataSockets;
        }

        int maxDesc() { return maxSocketDesc_; }
        int size() { return sockets_.size(); }
};
}
