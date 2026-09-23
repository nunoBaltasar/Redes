#include "protocol.h"

#include <sstream>
#include <iostream>

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <csignal>

using namespace std;

volatile sig_atomic_t stopped = 0;

void signalHandler(int sig){
    (void)sig;
    stopped = 1;
}

class User{
    private:
        string pw;
        string uid;
        int isLIN;
    public:
        User(){
            this->pw    = "\0";
            this->uid   = "\0";
            this->isLIN = 0;
        }
        void setPW(string pw){
            this->pw = pw;
        }
        void setUID(string uid){
            this->uid = uid;
        }
        void setLIN(){
            this->isLIN = 1;
        }
        void setLOUT(){
            this->isLIN = 0;
        }
        string getPW(){
            return this->pw;
        }
        string getUID(){
            return this->uid;
        }
        int getLIN(){
            return this->isLIN;
        }
};

bool checkLogin(const string& input, User& user){
    stringstream ss(input);
    string word, uid, pw;
    ss >> word;

    if (!(ss >> uid)){
        cout << "Too few arguments\n";
        return false;
    }
    if (!(ss >> pw)){
        cout << "Too few arguments\n";
        return false;
    }
    if (ss >> word){
        cout << "Too many arguments\n";
        return false;
    }
    if (user.getLIN()){
        cout << "User already loggin in\n";
        return false;
    }
    if (!validate_uid(uid)){
        cout << "Invalid UID\n";
        return false;
    }
    if (!validate_password(pw)){
        cout << "Invalid password\n";
        return false;
    }

    user.setPW(pw);
    user.setUID(uid);
    return true;
}

void checkResponseLogin(char* buffer, User& user){
    if (!strcmp(buffer, "RLI OK\n")){
        cout << "successful login\n";
        user.setLIN();
    }else if (!strcmp(buffer, "RLI NOK\n")){
        cout << "incorrect login\n";
    }else if (!strcmp(buffer, "RLI REG\n")){
        cout << "new user registered\n";
        user.setLIN();
    }else if (!strcmp(buffer, "ERR\n")){
        cout << "protocol error\n";
    }else{
        cout << "unexpected reply: " << buffer;
    }
}

void checkResponseUNR(char* buffer, User& user){
    if (!strcmp(buffer, "RUR OK\n")){
        cout << "successful unregister\n";
        user.setLOUT();
    }else if (!strcmp(buffer, "RUR NOK\n")){
        cout << "unknown user or not logged in\n";
    }else if (!strcmp(buffer, "RUR WRP\n")){  // fixed: was "WSP"
        cout << "incorrect password\n";
    }else if (!strcmp(buffer, "RUR UNR\n")){  // added missing case
        cout << "user not registered\n";
    }else if (!strcmp(buffer, "ERR\n")){
        cout << "protocol error\n";
    }else{
        cout << "unexpected reply: " << buffer;
    }
}

void checkRespondeLOUT(char* buffer, User& user){
    if (!strcmp(buffer, "RLO OK\n")){
        cout << "successful logout\n";
        user.setLOUT();
    }else if (!strcmp(buffer, "RLO NLG\n")){
        cout << "user not logged in\n";
    }else if (!strcmp(buffer, "RLO WRP\n")){ 
        cout << "incorrect password\n";
    }else if (!strcmp(buffer, "RLO UNR\n")){ 
        cout << "unknown user\n";
    }else if (!strcmp(buffer, "ERR\n")){
        cout << "protocol error\n";
    }else{
        cout << "unexpected reply: " << buffer;
    }
}

int main(int argc, char *argv[]){
    string peerport;
    string dsip   = "193.136.138.142";
    string dsport = "59000";
    bool hasPeerport = false;

    struct sigaction signalAction{};
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_handler = signalHandler;
    signalAction.sa_flags = 0;
    sigaction(SIGINT, &signalAction, nullptr);

    signal(SIGPIPE, SIG_IGN);

    for (int i = 1; i < argc; i++){
        string arg = argv[i];
        if (arg == "-m"){
            if (i + 1 >= argc){ cout << "Missing value for -m\n"; exit(1); }
            peerport = argv[++i];
            hasPeerport = true;
        }else if (arg == "-n"){
            if (i + 1 >= argc){ cout << "Missing value for -n\n"; exit(1); }
            dsip = argv[++i];
        }else if (arg == "-p"){
            if (i + 1 >= argc){ cout << "Missing value for -p\n"; exit(1); }
            dsport = argv[++i];
        }else{
            cout << "Unknown argument: " << arg << "\n";
            cout << "Usage: " << argv[0] << " -m peerport [-n DSIP] [-p DSport]\n";
            exit(1);
        }
    }

    if (!hasPeerport){
        cout << "Usage: " << argv[0] << " -m peerport [-n DSIP] [-p DSport]\n";
        exit(1);
    }

    int fd, errcode;
    struct sockaddr_in addr;
    struct addrinfo hints, *res;
    struct timeval tmout;
    tmout.tv_sec  = 6;
    tmout.tv_usec = 0;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1){ perror("socket"); exit(1); }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    errcode = getaddrinfo(dsip.c_str(), dsport.c_str(), &hints, &res);
    if (errcode != 0){
        cout << "getaddrinfo: " << gai_strerror(errcode) << "\n";
        close(fd);
        exit(1);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tmout, sizeof(tmout)) < 0){
        cout << "Error: socket timeout\n";
        freeaddrinfo(res);
        close(fd);
        return 1;
    }

    string input, command;
    User user{};
    char buffer[128];

    while (!stopped && getline(cin, input)){
        stringstream ss(input);
        if (!(ss >> command)) continue;
        string word;

        if (command == "login"){
            if (user.getLIN()){
                cout << "Already logged in as " << user.getUID() << "\n";
                continue;
            }
            if (!checkLogin(input, user)) continue;

            string request = "LIN " + user.getUID() + " " + user.getPW() + " " + peerport + '\n';
            if (sendAndReceive(buffer, sizeof(buffer), request, fd, res, addr) == -1) continue;
            checkResponseLogin(buffer, user);

        }else if (command == "unregister"){
            if (ss >> word){ cout << "Too many arguments\n"; continue; }
            if (!user.getLIN()){ cout << "No user is currently logged in\n"; continue; }

            string request = "UNR " + user.getUID() + " " + user.getPW() + '\n';
            if (sendAndReceive(buffer, sizeof(buffer), request, fd, res, addr) == -1) continue;
            checkResponseUNR(buffer, user);

        }else if (command == "logout"){
            if (ss >> word){ cout << "Too many arguments\n"; continue; }
            if (!user.getLIN()){ cout << "No user is currently logged in\n"; continue; }

            string request = "LOU " + user.getUID() + " " + user.getPW() + '\n';
            if (sendAndReceive(buffer, sizeof(buffer), request, fd, res, addr) == -1) continue;
            checkRespondeLOUT(buffer, user);

        }else if (command == "exit"){
            if (ss >> word){ cout << "Too many arguments\n"; continue; }
            if (user.getLIN()){ cout << "Please logout before exiting\n"; continue; }
            break;

        }else{
            cout << "Invalid command\n";
        }
    }

    if (stopped) cout << "\nInterrupted.\n";

    freeaddrinfo(res);
    close(fd);
    return 0;
}
