#pragma once
//WEBSOCKET
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>


extern void err_handle(char* err) noexcept;
int main(){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        err_handle("socket");
    }

    int port = 8080;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    
    int rv = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rv){
        err_handle("connect");
    }
    
    // send and recv
    while(true){

        char msg[] = "hello";
        write(fd,msg,std::strlen(msg));
        
        char rbuf[64] = {};
        ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
        if (n < 0){
            err_handle("read");
            break;
        }

        std::printf("recv: %s\n", rbuf);
    }

    close(fd);

}