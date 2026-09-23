#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <netdb.h>
#include <netinet/in.h>

// Validation
bool validate_uid(const std::string& uid);
bool validate_password(const std::string& pw);

// UDP communication — returns 0 on success, -1 on error
int sendAndReceive(char *buffer, int bufsize, const std::string& request,
                   int fd, addrinfo* res, sockaddr_in addr);

#endif
