#ifndef CLIENTUDP_H
#define CLIENTUDP_H

#include <string>
#include <netdb.h>
#include <netinet/in.h>

bool validate_uid(const std::string& uid);
bool validate_password(const std::string& pw);

int sendUDP   (const std::string& request, int fd, addrinfo* res);
int receiveUDP(char* buffer, int bufsize,  int fd, sockaddr_in& addr);

int sendAndReceive(char* buffer, int bufsize, const std::string& request,
                   int fd, addrinfo* res, sockaddr_in& addr);

#endif
