#include "clientUDP.h"

#include <sys/socket.h>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>

using namespace std;

bool validate_uid(const string& uid){
    if (uid.length() != 6) return false;
    for (char c : uid)
        if (!isdigit(c)) return false;
    return true;
}

bool validate_password(const string& pw){
    if (pw.length() != 8) return false;
    for (char c : pw)
        if (!isalnum(c)) return false;
    return true;
}


int sendUDP(const string& request, int fd, addrinfo* res){
    ssize_t n = sendto(fd, request.c_str(), request.length(), 0,
                       res->ai_addr, res->ai_addrlen);
    if (n == -1){
        if (errno != EINTR) perror("sendto");
        return -1;
    }
    return 0;
}

int receiveUDP(char* buffer, int bufsize, int fd, sockaddr_in& addr){
    socklen_t addrlen = sizeof(addr);
    ssize_t n = recvfrom(fd, buffer, bufsize - 1, 0,
                         (struct sockaddr*) &addr, &addrlen);
    if (n == -1){
        if (errno != EINTR) perror("recvfrom");
        return -1;
    }
    buffer[n] = '\0';
    return 0;
}


int sendAndReceive(char* buffer, int bufsize, const string& request,
                   int fd, addrinfo* res, sockaddr_in& addr){
    if (sendUDP(request, fd, res)         == -1) return -1;
    if (receiveUDP(buffer, bufsize, fd, addr) == -1) return -1;
    return 0;
}
