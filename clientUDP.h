#ifndef CLIENTUDP_H
#define CLIENTUDP_H

#include <string>
#include <netdb.h>

// Socket UDP ligado ao DS. Abre no construtor e fecha no destrutor (RAII).
class ClientUDP{
    public:
        // Lança std::runtime_error se não conseguir criar o socket ou resolver o DS
        ClientUDP(const std::string& host, const std::string& port, int timeoutSec = 6);
        ~ClientUDP();

        ClientUDP(const ClientUDP&)            = delete;
        ClientUDP& operator=(const ClientUDP&) = delete;

        bool send(const std::string& request);
        bool receive(std::string& reply);
        bool sendAndReceive(const std::string& request, std::string& reply);

    private:
        // Chega para a maior resposta UDP (RLS com 50 filenames ~ 1257 bytes)
        static const int BUFSIZE = 2048;

        int       fd;
        addrinfo* res;
};

#endif
