#include "clientUDP.h"
#include "validation.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>

#include <cstdlib>
#include <csignal>

#define DEF_DSPORT "59000"
#define DEF_DSIP "193.136.138.142"

using namespace std;

static volatile sig_atomic_t stopped = 0;

static void signalHandler(int sig){
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


static void checkResponseLogin(const string& reply, User& user, const string& uid, const string& pw){
    if      (reply == "RLI OK\n")  { cout << "successful login\n";    user.setUID(uid); user.setPW(pw); user.setLIN(); }
    else if (reply == "RLI NOK\n") { cout << "incorrect login\n";                                                      }
    else if (reply == "RLI REG\n") { cout << "new user registered\n"; user.setUID(uid); user.setPW(pw); user.setLIN(); }
    else if (reply == "ERR\n")     { cout << "clientUDP error\n";                                                       }
    else                           { cout << "unexpected reply: " << reply;                                            }
}

static void checkRespondeLOUT(const string& reply, User& user){
    if      (reply == "RLO OK\n")  { cout << "successful logout\n";    user.setLOUT(); }
    else if (reply == "RLO NLG\n") { cout << "user not logged in\n";                  }
    else if (reply == "RLO WRP\n") { cout << "incorrect password\n";                  }
    else if (reply == "RLO UNR\n") { cout << "unknown user\n";                        }
    else if (reply == "ERR\n")     { cout << "clientUDP error\n";                      }
    else                           { cout << "unexpected reply: " << reply;           }
}

static void checkResponseUNR(const string& reply, User& user){
    if      (reply == "RUR OK\n")  { cout << "successful unregister\n";         user.setLOUT(); }
    else if (reply == "RUR NOK\n") { cout << "unknown user or not logged in\n";                 }
    else if (reply == "RUR WRP\n") { cout << "incorrect password\n";                            }
    else if (reply == "RUR UNR\n") { cout << "user not registered\n";                           }
    else if (reply == "ERR\n")     { cout << "clientUDP error\n";                                }
    else                           { cout << "unexpected reply: " << reply;                     }
}

static void checkResponsePUB(const string& reply){
    if (reply == "RPB OK\n") { cout << "successful publication\n";}
    else if (reply == "RPB NOK\n") { cout << "unsuccesful publication\n";}
    else if (reply == "RPB NLG\n") { cout << "user not logged in\n";}
    else if (reply == "RPB WRP\n") { cout << "incorrect password\n";}
    else if (reply == "RPB UNR\n") {cout << "user not registered\n";}
    
}

static void checkResponseREM(const string& reply){
    if (reply == "RRM OK\n") { cout << "successful removal\n";}
    else if (reply == "RRM NOK\n") { cout << "resource not found\n";} //implica n poder remover pub de outras pessoas?
    else if (reply == "RRM UNR\n") {cout << "user not registered\n";}
    else if (reply == "RRM WRP\n") {cout << "incorrect password\n";}
    else if (reply == "RRM NLG\n") {cout << "user not logged in\n";}

}

static void cmd_login(const string& uid, const string& pw, const string& peerport,
                      User& user, ClientUDP& ds){
    if (user.getLIN()){
        cout << "Already logged in as " << user.getUID() << "\n";
        return;
    }
    if (!validate_uid(uid)){
        cout << "Invalid UID (must be exactly 6 numeric digits)\n";
        return;
    }
    if (!validate_password(pw)){
        cout << "Invalid password (must be exactly 8 alphanumeric characters)\n";
        return;
    }

    string reply;
    string request = "LIN " + uid + " " + pw + " " + peerport + '\n';
    if (!ds.sendAndReceive(request, reply)) return;
    checkResponseLogin(reply, user, uid, pw);
}

static void cmd_logout(User& user, ClientUDP& ds){
    if (!user.getLIN()){ cout << "No user is currently logged in\n"; return; }

    string reply;
    string request = "LOU " + user.getUID() + " " + user.getPW() + '\n';
    if (!ds.sendAndReceive(request, reply)) return;
    checkRespondeLOUT(reply, user);
}

static void cmd_unregister(User& user, ClientUDP& ds){
    if (!user.getLIN()){ cout << "No user is currently logged in\n"; return; }

    string reply;
    string request = "UNR " + user.getUID() + " " + user.getPW() + '\n';
    if (!ds.sendAndReceive(request, reply)) return;
    checkResponseUNR(reply, user);
}

static void cmd_publish(User& user, const string& filename, const string& label, 
                        ClientUDP& ds){
    if (!user.getLIN()){
        cout << "User not logged in\n";
        return;
    }
    streamsize fSize;
    ifstream file(filename.c_str(), ios::binary | ios::ate);
    
    if (file.is_open()) {
        fSize = file.tellg();
        file.close();
    } else {
        cout << "Error: file doesn't exit in current directory\n";
        return;
    } 
    string reply;
    string request = "PUB " + user.getUID() + " " + user.getPW() + " " + filename +" " + to_string(fSize) + " " + label + '\n';
    if (!ds.sendAndReceive(request, reply)) return;
    checkResponsePUB(reply);
}

static void cmd_remove(User& user, const string& filename, ClientUDP& ds){
    if (!user.getLIN()){
        cout << "User not logged in\n";
        return;
    }
    string reply;
    string request = "REM " + user.getUID() + " " + user.getPW() +" "+ filename +'\n';
    if (!ds.sendAndReceive(request, reply)) return;
    checkResponseREM(reply);
}

static void checkResponseLST(string reply){
    if (reply == "RLS NOK\n") { cout << "no published resources yet\n";}
}

static void cmd_list(ClientUDP& ds){ 
    char buffer[65536];
    string request = "LST\n";
    string reply;
    int i = 1;
    if (!ds.sendAndReceive(request, reply)) return;
    checkResponseLST(string(buffer));
    stringstream ss(reply);
    string word;
    ss >> word >> word;
    cout << "Available files:\n";
    while (ss >> word){
        if (i>=50) return;
        cout << "File " + to_string(i) + " - " + word << endl;
        i++;
    }
}



int main(int argc, char *argv[]){
    string peerport;
    string dsip   = DEF_DSIP;
    string dsport = DEF_DSPORT;
    bool hasPeerport = false;

    struct sigaction sa{};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signalHandler;
    sa.sa_flags   = 0;
    sigaction(SIGINT, &sa, nullptr);
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

    unique_ptr<ClientUDP> ds;
    try{
        ds.reset(new ClientUDP(dsip, dsport));
    }catch (const exception& e){
        cout << e.what() << "\n";
        exit(1);
    }

    string input, command;
    User user{};


    int fdTCP, n;
    addrinfo  hintsTCP{}, *resTCP;
    hintsTCP.ai_family = AF_INET;
    hintsTCP.ai_socktype = SOCK_STREAM;
    fdTCP = socket(AF_INET,  SOCK_STREAM, 0);
    int errcodeTCP = getaddrinfo(dsip.c_str(), dsport.c_str(), &hintsTCP, &resTCP);
    if (errcodeTCP != 0){
        throw runtime_error(string("getaddrinfo: ") + gai_strerror(errcodeTCP));
    }
    
    n = connect(fdTCP, resTCP->ai_addr, resTCP->ai_addrlen);
    if (n==-1) exit(1);
    timeval tmout{};
    tmout.tv_sec = 5;
    if (setsockopt(fdTCP, SOL_SOCKET, SO_RCVTIMEO, &tmout, sizeof(tmout)) < 0){
        freeaddrinfo(resTCP);
        close(fdTCP);
        //throw runtime_error(string("setsockopt: ") + strerror(errno));
    }

    while (!stopped && getline(cin, input)){
        stringstream ss(input);
        if (!(ss >> command)) continue;
        string w1, w2, extra;

        if (command == "login"){
            if (!(ss >> w1) || !(ss >> w2) || (ss >> extra)){
                cout << "login: expected 2 arguments (UID password)\n"; continue;
            }
            cmd_login(w1, w2, peerport, user, *ds);

        }else if (command == "logout"){
            if (ss >> extra){ cout << "logout: takes no arguments\n"; continue; }
            cmd_logout(user, *ds);

        }else if (command == "unregister"){
            if (ss >> extra){ cout << "unregister: takes no arguments\n"; continue; }
            cmd_unregister(user, *ds);

        }else if (command == "exit"){
            if (ss >> extra){ cout << "exit: takes no arguments\n"; continue; }
            if (user.getLIN()){ cout << "please logout before exiting\n"; continue; }
            break;

        }else if (command == "publish"){ //sq é preciso verificar input (label)
            if (!(ss >> w1) || !(ss >> w2) || (ss >> extra)){
                cout << "login: expected 2 arguments\n"; continue;
            }
            cmd_publish(user, w1, w2, *ds);
            
        } else if (command == "remove"){ 
            if (!(ss >> w1) || (ss >> extra)){
                cout << "login: expected 1 arguments\n"; continue;
            }
            cmd_remove(user, w1, *ds);
        } else if (command == "list"){
            if (ss >> extra){ cout << "list: takes no arguments\n"; continue; }
            cmd_list(*ds);

        } else if (command == "versions"){
            string filename;
            if (!(ss >> filename) || (ss >> extra)){
                cout << "login: expected 2 arguments\n"; continue;
            }
            
            string request = "VRS " + filename + '\n';
            char reply[128]; 
            n = write(fdTCP, request.c_str(), request.length());
            if (n==-1) exit(1);
            n = read(fdTCP, reply, 128);
            if (n==-1) exit(1);
            cout << reply << endl;

        }else{
            cout << "unknown command '" << command << "'\n";
        }
    }

    if (stopped) cout << "\nInterrupted.\n";

    freeaddrinfo(resTCP);
    close(fdTCP);
    return 0;
}
