#ifndef SERVER_H
#define SERVER_H
#include<iostream>
#include<cstdlib>
// #include<boost.hpp>
#include<sys/socket.h>
#include<sys/types.h>
#include<vector>
#include<netdb.h>
#include<arpa/inet.h>
#include<string>
#include<map>
#include<unistd.h>
#include<fcntl.h>
#include<sys/epoll.h>
#define BACKLOG 5
#define SOCKUNAVAIL -1
// #define ADDRESSLEN 
namespace tcp{

enum class Status{
    OK,
    FAIL,
};
// typedef enum class Status Status;


class ServerBase{
public:
    virtual Status  Bind() = 0;
    virtual Status Listen() = 0;
    virtual Status Accept() = 0;
};
class Server{
public:
    using peer_Struct = std::map<int,std::string>;
    Server() = default;
    Server(char* name, char* port):servName(name),servPort(port),sockfd(-1){
        GetInfo();
    };
    Status Bind();
    // Status Wait4Message();
    int Listen();
    Status Accept(int &);
    Status Read();
    Status Write();
    int getSock(){return sockfd;};
    void lookupInfo(int,std::string&); 
    virtual ~Server(){shutdown(sockfd,SHUT_RDWR); close(sockfd);};
    
private:
    Status GetInfo();
    struct addrinfo* servInfo;
    char* servName;
    char* servPort;
    int sockfd;
    std::vector<int> connSock;
    std::vector<peer_Struct> peerInfo;
};

int readAll(int fd, void *buf, int bufSize);


int sendAll(int fd, void *buf, int& actualSend, int bufSize);


int sendAll(int fd, void *buf, int& actualSend, int bufSize);


void HandleAccept(int acceptFd, int epfd);
void HandleWrite(int fd, const void *buf, int size, int&actualSend);
void HandleRead(int fd, void *buf, int size, int&actualRecv);

std::ostream& printServInfo(std::ostream&,Server&);


struct Msg{
    int msgLen;
    char delimeter;
};


struct Con{
    std::string readed;
    size_t written;
    bool writeEnabled;
    std::vector<std::string> strVec;
    Con():written(0),writeEnabled(false){};
};

struct EpollManager{
public:
    EpollManager() = default;
    int create();
private:
    struct epoll_event;
};

void Epoll_event_add(int fd, int event, int epfd);
void Epoll_event_del(int fd, int event, int epfd);
void Epoll_event_mod(int fd, int event, int epfd);
void Epoll_wait(int epfd, int event, int maxEvents, int tiemout);
void Epoll_create(int &epfd);
void SetNonBlocking(int fd);
// extern struct Con connections;
}
#endif