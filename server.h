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
namespace serv{

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
    // Status 
    virtual ~Server(){shutdown(sockfd,SHUT_RDWR); close(sockfd);};
    
    // Status connect();
private:
    Status GetInfo();
    struct addrinfo* servInfo;
    char* servName;
    char* servPort;
    int sockfd;
    // std::vector<int> sockList;/
    int sendAll(void *&buf,int len);
    std::vector<int> connSock;
    std::vector<peer_Struct> peerInfo;
};

int readAll(int fd, void *buf, int bufSize, int readSize);


int sendAll(int fd, void *buf, int bufSize, int sendSize);


std::ostream& printServInfo(std::ostream&,Server&);


struct Message{
    int data1 = 1;
    int data2 = 2;
    char data3 = 'a';
};

struct EpollManager{
public:
    EpollManager() = default;
    int create();
private:
    struct epoll_event;
};
void epoll_event_add(int fd, int event, int epfd);
void epoll_event_del(int fd, int event, int epfd);
void epoll_event_mod(int fd, int event, int epfd);
}