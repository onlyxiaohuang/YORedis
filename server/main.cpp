#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include "err.hpp"

int main(){
    //USE IPV4 & TCP
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    //SOL_SOCKET & SO_REUSEADDR specfies which option to set
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    //struct sockaddr_in holds an IPv4 address and port. You must initialize the structure as shown in the sample code. 
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = ntohl(0);// wildcard address 0.0.0.0
    int rv = bind(fd,(const sockaddr*)&addr,sizeof(addr));
    if (rv)
    {
        err_handle("bind()");
    }
    std::cout << "listening on port 8080" << std::endl;

    return 0;
}