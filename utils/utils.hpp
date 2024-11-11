# pragma once
# include <stdio.h>
# include <stdint.h>


const int k_max_msg = 1024;
enum{
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

struct Conn{
    int fd = -1;//句柄
    __uint32_t state = STATE_REQ;
    
    size_t rbuf_size = 0;
    uint8_t r[4 + k_max_msg];

    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};