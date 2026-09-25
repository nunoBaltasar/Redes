#include "clientUDP.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <stdexcept>

using namespace std;

ClientUDP::ClientUDP(const string& host, const string& port, int timeoutSec)
    : fd(-1), res(nullptr){
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) throw runtime_error(string("socket: ") + strerror(errno));

    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    int errcode = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
    if (errcode != 0){
        close(fd);
        throw runtime_error(string("getaddrinfo: ") + gai_strerror(errcode));
    }

    timeval tmout{};
    tmout.tv_sec = timeoutSec;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tmout, sizeof(tmout)) < 0){
        freeaddrinfo(res);
        close(fd);
        throw runtime_error(string("setsockopt: ") + strerror(errno));
    }
}

ClientUDP::~ClientUDP(){
    if (res)     freeaddrinfo(res);
    if (fd != -1) close(fd);
}


bool ClientUDP::send(const string& request){
    ssize_t n = sendto(fd, request.c_str(), request.length(), 0,
                       res->ai_addr, res->ai_addrlen);
    if (n == -1){
        if (errno != EINTR) perror("sendto");
        return false;
    }
    return true;
}

bool ClientUDP::receive(string& reply){
    char buffer[BUFSIZE];
    sockaddr_in addr{};
    socklen_t addrlen = sizeof(addr);
    ssize_t n = recvfrom(fd, buffer, sizeof(buffer), 0,
                         reinterpret_cast<sockaddr*>(&addr), &addrlen);
    if (n == -1){
        if (errno == EAGAIN || errno == EWOULDBLOCK) fprintf(stderr, "recvfrom: timeout, DS did not reply\n");
        else if (errno != EINTR)                     perror("recvfrom");
        return false;
    }
    reply.assign(buffer, n);
    return true;
}

bool ClientUDP::sendAndReceive(const string& request, string& reply){
    return send(request) && receive(reply);
}
