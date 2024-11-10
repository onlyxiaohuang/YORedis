#pragma once

//WEBSOCKETS
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <cstring>

extern void err_handle(char* err) noexcept;
void do_work(int connfd);


int main(){
    //USE IPV4 & TCP
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    //SOL_SOCKET & SO_REUSEADDR specfies which option to set
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    //struct sockaddr_in holds an IPv4 address and port. You must initialize the structure as shown in the sample code. 
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    //The ntohs() and ntohl() functions convert numbers to the required big endian format. 
    
    int port = 8080;
    addr.sin_port = ntohs(port);
    addr.sin_addr.s_addr = ntohl(0);// wildcard address 0.0.0.0
    int rv = bind(fd,(const sockaddr*)&addr,sizeof(addr));
    if (rv)
    {
        err_handle("bind()");
    }
    std::cout << "listening on port " << port << std::endl;

    //LISTEN FOR CONNECTIONS
    rv = listen(fd,SOMAXCONN);
    if (rv){
        err_handle("listen()");
    }

    while(true){
        //ACCEPT CONNECTION
        sockaddr_in client_addr = {};
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(fd,(sockaddr*)&client_addr,&client_addr_len);
        if (client_fd < 0){
            err_handle("accept()");
        }
        std::cout << "accepted connection from " << ntohs(client_addr.sin_addr.s_addr) << ":" << ntohs(client_addr.sin_port) << std::endl;
        do_work(client_fd);
    }

    return 0;
}

void do_work(int connfd){
    char rbuf[64] = {};
    ssize_t n = read(connfd,rbuf,sizeof(rbuf) - 1);
    if (n < 0){
        err_handle("read() error");
        return ;
    }
    std::printf("client says: %s\n",rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
    return ;
}