#ifndef CLIENT_H
#define CLIENT_H
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


namespace cli{
class Client{
public:
    Client() = default;
    int Connect(int&);
    int GetInfo();

    ~Client(){};
private:
    char *servName;
    char *port;
    struct addrinfo *servInfo; //used to store information about the wanted server

};


struct Con{
    std::string readed;
    size_t written;
    bool writeEnabled;
    Con():written(0),writeEnabled(false){};
};


}
// namespace 
#endif