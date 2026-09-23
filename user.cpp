#include <sstream>
#include <iostream>
#include <vector>

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
            this->pw = "\0";
            this->uid = "\0";
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
    if (ss.eof()){
        cout << "Too few arguments\n";
        return false;
    }
    ss >> uid;
    ss >> pw;
    if (ss >> word){
        cout << "Too many arguments\n";
        return false;
    }
    if (user.getLIN()){
        cout << "User already loggin in\n";
        return false;
    }

    if (uid.length() != 6) {
        cout << "Invalid UID\n";
        return false;
    }
    for (int i = 0; i <6; i++){
        if ((i == 0 && uid[i]=='0') || !isdigit(uid[i])){
            cout << "Invalid UID\n";
            return false;
        }
    }
    if (pw.length() != 8){
        cout << "Invalid password\n";
        return false;
    }
    for (int i = 0; i < 8; i++){
        if (!isalnum(pw[i])){
            cout << "Invalid password\n";
            return false;
        }
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
        cout << "Error: idk\n";
    }
}
void checkResponseUNR(char* buffer, User& user){
    if (!strcmp(buffer, "RUR OK\n")){
        cout << "successful unregister\n";
        user.setLOUT();
    }else if (!strcmp(buffer, "RUR NOK\n")){
        cout << "unknown user\n";
    }else if (!strcmp(buffer, "RUR WSP\n")){
        cout << "incorrect unregister attempt\n";
    }else if (!strcmp(buffer, "ERR\n")){
        cout << "Error:\n";
    }
}
void checkRespondeLOUT(char* buffer, User& user){  
    if (!strcmp(buffer, "RLO OK\n")){
        cout << "successful logout\n";
        user.setLOUT();
    }else if (!strcmp(buffer, "RLO NLG\n")){
        cout << "user not logged in\n";
    }else if (!strcmp(buffer, "RLO WRP\n")){
        cout << "unknown user\n";
    }
}

void sendAndReceive(char *buffer, User& user, string request, int fd, addrinfo* res, sockaddr_in addr){
    int n;
    socklen_t addrlen;
    n = sendto(fd, request.c_str(), request.length(), 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);
    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n >= 0) {buffer[n] = '\0';}
}

//é preciso tratar do sigpipe
int main(int argc, char *argv[]){
    string peerport;
    string dsip;
    string dsport;
    int i = 1, isDSPORT = 0, isDSIP = 0;
     
    while (i < argc){
        string arg = argv[i];
        if (arg == "-m"){
            i++;
            peerport = argv[i];
        }else if (arg == "-n"){
            i++;
            dsip = argv[i];
            isDSIP = 1;
        }else if (arg == "-p"){
            i++;
            dsport = argv[i];
            isDSPORT = 1;
        }
        i++;
    }
    if (!isDSPORT){
        dsport = "59000";
    }
    if (!isDSIP){
        dsip = "193.136.138.142"; //192.168.1.1
    }
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    struct addrinfo hints, *res;
    struct timeval tmout;
    tmout.tv_sec = 6.7;
    tmout.tv_usec = 0;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        cout << "ERR: ";
        exit(1);
    };
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype =  SOCK_DGRAM;
    errcode = getaddrinfo(dsip.c_str(), dsport.c_str(), &hints, &res);
    if (errcode != 0){ 
        cout << "ERR: fah\n";
        cout << errcode;
        exit(1);
    };
    string input;
    string command;
    User user{};
    char buffer[128];
    struct sigaction signalAction{};
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_handler = signalHandler;
    signalAction.sa_flags = 0;
    sigaction(SIGINT, &signalAction, nullptr);
    
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tmout, sizeof(tmout)) < 0){
        cout << "Error: socket timeout";
        freeaddrinfo(res);
        close(fd);
        return 0;
    }

    while (getline(cin, input)){
        stringstream ss(input);
        ss >> command;
        string word;
        
        if (command == "login"){
            if (!checkLogin(input, user)) continue;
            
            string request = "LIN " + user.getUID() + " " + user.getPW() + " " + peerport + '\n';
            
            sendAndReceive(buffer, user, request, fd, res, addr);

            checkResponseLogin(buffer, user);
             
        }else if (command == "unregister"){
            //nao sei como verificar reply nok e wrp
            if (ss >> word){
                cout << "Too many arguments\n";
                continue;
            }
            if (!user.getLIN()){
                cout << "user not logged in\n";
                continue;
            }       
            string request = "UNR " + user.getUID() +" " + user.getPW() + '\n';

            sendAndReceive(buffer, user, request, fd, res, addr);

            checkResponseUNR(buffer, user);

        }else if (command == "logout"){
            if (ss >> word){
                cout << "Too many arguments\n";
                continue;
            }
            if (!user.getLIN()){
                cout << "user not logged in\n";
                continue;
            }
            string request = "LOU " + user.getUID() +" " + user.getPW() + '\n';
            
            sendAndReceive(buffer, user, request, fd, res, addr);

            checkRespondeLOUT(buffer, user);


        }else if(command == "publish filename label"){
            string fn = "fillename";
            if (FILE *file = fopen(fn.c_str(), "r")) {
                fclose(file);
            } else {
                cout << "There is no file to send in the current directory\n";
                continue;
            }
            continue;
            
        }else if (command == "list"){ //nao sei qual o tamanho do buffer caso receba mt nomes
            string request = "LST\n";
            sendAndReceive(buffer, user, request, fd, res, addr);

            if (!strcmp(buffer, "RLS OK"))
            continue;
        }else if (command == "exit"){
            if (ss >> word){
                cout << "Too many arguments\n";
            }
            if (user.getLIN()){
                cout << "It is advised to logout before closing the program\n";
                continue;
            }
            break;
        }else{
            cout << "Invalid command\n";
        }

    }

    if (stopped){
        cout << "Program exiting safely\n";
    }

    freeaddrinfo(res);
    close(fd);
    return 0;
}