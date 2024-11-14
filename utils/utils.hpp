# pragma once
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <assert.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>


extern void err_handle(char* err) noexcept;

const int k_max_args = 20;
const int k_max_msg = 1024;
enum{
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

enum{
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};  

static std::map<std::string,std::string> g_map;

struct Conn{
    int fd = -1;//句柄
    __uint32_t state = STATE_REQ;
    
    //读取的buffer
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    //写的buffer
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};

