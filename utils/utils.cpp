#include "utils.hpp"



void msg(const char *msg){
    fprintf(stdout,"%s\n",msg);
}

static bool cmd_is(const std::string &word, const char *cmd) {
    return 0 == strcasecmp(word.c_str(), cmd);
}

static uint32_t do_get(const std::vector<std::string> &cmd,uint8_t *res,uint32_t *reslen){
    if (!g_map.count(cmd[1])) {
        return RES_NX;
    }
    std::string &val = g_map[cmd[1]];
    assert(val.size() <= k_max_msg);
    memcpy(res, val.data(), val.size());
    *reslen = (uint32_t)val.size();
    return RES_OK;
}

static uint32_t do_set(
    const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    (void)res;
    (void)reslen;
    g_map[cmd[1]] = cmd[2];
    return RES_OK;
}

static uint32_t do_del(
    const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    (void)res;
    (void)reslen;
    g_map.erase(cmd[1]);
    return RES_OK;
}

static uint8_t parse_req(const uint8_t *data,uint32_t len,std::vector<std::string> &out){
    if(len < 4){
        return -1;
    }
    uint32_t n = 0;
    memcpy(&n,&data[0],4);
    if(n > k_max_args){
        return -1;
    }
    
    size_t pos = 4;
    while(n --){
        if(pos + 4 > len){
            return -1;
        }
        uint32_t sz = 0;
        memcpy(&sz,&data[pos],4);
        if(pos + 4 + sz > len){
            return -1;
        }
        out.push_back(std::string((char *)&data[pos + 4],sz));
        pos += 4 + sz;
    }

    if(pos != len){
        return -1;
    }
    
    return 0;
}

static int32_t do_request(const uint8_t *req,uint32_t reqlen,uint32_t *rescode,uint8_t *res,uint32_t *reslen){
    std::vector<std::string> cmd;
    if(parse_req(req,reqlen,cmd) != 0){
        msg("bad req");
        return -1;
    }
    
    //指令的识别
    if(cmd.size() == 2 && cmd_is(cmd[0], "get")){
        *rescode = do_get(cmd,res,reslen);
    }else if(cmd.size() == 3 && cmd_is(cmd[0], "set")){
        *rescode = do_set(cmd,res,reslen);
    }else if(cmd.size() == 2 && cmd_is(cmd[0],"del")){
        *rescode = do_del(cmd,res,reslen);
    }else{
        *rescode = RES_ERR;
        const char *msg = "Unknown command";
        strcpy((char *)res,msg);
        *reslen = strlen(msg);
        return 0;    
    }
    return 0;
}

static bool try_one_request(Conn *conn){
    if (conn -> rbuf_size < 4)
    {
        return false;
    }

    __uint32_t len = 0;
    std::memcpy(&len, &conn -> rbuf[0], 4);
    
    if(len > k_max_msg){
        msg("too long");
        conn -> state = STATE_END;
        return false;
    }

    if(4 + len > conn -> rbuf_size){
        return false;
    }

    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = do_request(&conn -> rbuf[4],len,&rescode,&conn -> wbuf[4 + 4],&wlen);
    
    if(err){
        conn -> state = STATE_END;
        return false;
    }
    wlen += 4;
    memcpy(&conn -> wbuf[0],&wlen,4);
    memcpy(&conn -> wbuf[4],&rescode,4);
    conn -> wbuf_size = wlen + 4;

    size_t remain = conn -> rbuf_size - 4;
    if (remain){
        memmove(conn -> rbuf, &conn -> rbuf[4 + len], remain);
    }
    conn -> rbuf_size = remain;

    conn -> state = STATE_RES;
    state_res(conn);
    
    return (conn -> state == STATE_REQ);

}

static void state_res(Conn *conn) {
    while (try_flush_buffer(conn)) {}
}

static bool try_flush_buffer(Conn *conn) {
    ssize_t rv = 0;
    do {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);
    if (conn->wbuf_sent == conn->wbuf_size) {
        // response was fully sent, change state back
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    // still got some data in wbuf, could try to write again
    return true;
}