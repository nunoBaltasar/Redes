#include "protocol.h"

#include <sys/socket.h>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <iostream>

using namespace std;

// UID must be exactly 6 decimal digits (leading zeros allowed, e.g. "012345")
bool validate_uid(const string& uid){
    if (uid.length() != 6) return false;
    for (char c : uid)
        if (!isdigit(c)) return false;
    return true;
}

// Password must be exactly 8 alphanumeric ASCII characters
bool validate_password(const string& pw){
    if (pw.length() != 8) return false;
    for (char c : pw)
        if (!isalnum(c)) return false;
    return true;
}

// Sends request to DS and waits for reply.
// Returns 0 on success, -1 on error (including signal interruption).
int sendAndReceive(char *buffer, int bufsize, const string& request,
                   int fd, addrinfo* res, sockaddr_in addr){
    ssize_t n;
    socklen_t addrlen;

    n = sendto(fd, request.c_str(), request.length(), 0,
               res->ai_addr, res->ai_addrlen);
    if (n == -1){
        if (errno != EINTR) perror("sendto");
        return -1;
    }

    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, bufsize - 1, 0,
                 (struct sockaddr*) &addr, &addrlen);
    if (n == -1){
        if (errno != EINTR) perror("recvfrom");
        return -1;
    }

    buffer[n] = '\0';
    return 0;
}
