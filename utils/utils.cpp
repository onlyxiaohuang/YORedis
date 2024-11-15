#include "utils.hpp"


static void state_res(Conn *conn);
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
static void state_res(Conn *conn) {
    while (try_flush_buffer(conn)) {}
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

int32_t send_req(int fd, const char *text) {
    uint32_t len = (uint32_t)strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text, len);
    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

int32_t read_res(int fd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}

void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        err_handle("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        err_handle("fcntl error");
    }
}

bool try_fill_buffer(Conn *conn) {
    // try to fill the buffer
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);
    if (rv < 0 && errno == EAGAIN) {
        // got EAGAIN, stop.
        return false;
    }
    if (rv < 0) {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if (rv == 0) {
        if (conn->rbuf_size > 0) {
            msg("unexpected EOF");
        } else {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    // Try to process requests one by one.
    // Why is there a loop? Please read the explanation of "pipelining".
    while (try_one_request(conn)) {}
    return (conn->state == STATE_REQ);
}

 void state_req(Conn *conn) {
    while (try_fill_buffer(conn)) {}
}
void connection_io(Conn *conn) {
    if (conn->state == STATE_REQ) {
        state_req(conn);
    } else if (conn->state == STATE_RES) {
        state_res(conn);
    } else {
        assert(0);  // not expected
    }
}

void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn) {
    if (fd2conn.size() <= (size_t)conn->fd) {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}
int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd) {
    // accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0) {
        msg("accept() error");
        return -1;  // error
    }

    // set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    // creating the struct Conn
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if (!conn) {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}