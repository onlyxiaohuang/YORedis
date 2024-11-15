#pragma once


#include "utils.hpp"

extern void err_handle(char* err) noexcept;
void do_work(int connfd);

extern void fd_set_nb(int fd);
extern void connection_io(Conn *conn) ;
extern int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd);

int main(){
 int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        err_handle("socket()");
    }

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        err_handle("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        err_handle("listen()");
    }

    // a map of all client connections, keyed by fd
    std::vector<Conn *> fd2conn;

    // set the listen fd to nonblocking mode
    fd_set_nb(fd);

    // the event loop
    std::vector<pollfd> poll_args;
    while (true) {
        // prepare the arguments of the poll()
        poll_args.clear();
        // for convenience, the listening fd is put in the first position
        pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        // connection fds
        for (Conn *conn : fd2conn) {
            if (!conn) {
                continue;
            }
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        // poll for active fds
        // the timeout argument doesn't matter here
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if (rv < 0) {
            err_handle("poll");
        }

        // process active connections
        for (size_t i = 1; i < poll_args.size(); ++i) {
            if (poll_args[i].revents) {
                Conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END) {
                    // client closed normally, or something bad happened.
                    // destroy this connection
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        // try to accept a new connection if the listening fd is active
        if (poll_args[0].revents) {
            (void)accept_new_conn(fd2conn, fd);
        }
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