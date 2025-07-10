#ifndef NETWORK_COMM_H
#define NETWORK_COMM_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <string>
#include <memory>
#include "ObjectDatabase.h"

class UdpSender {
public:
    UdpSender(const std::string& ip, int port);
    ~UdpSender();
    
    bool sendData(const void* data, size_t size);
    bool sendString(const std::string& str);
    bool sendObjectData(const std::vector<ObjectFeatures>& objects);

private:
    int sockfd_;
    struct sockaddr_in server_addr_;
};

#endif // NETWORK_COMM_H