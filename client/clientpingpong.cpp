
#include "client.h"
#define PORT "3490"
#define NAME "TEST"
using namespace cli;
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <string>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#define NONBLOCKING 1
#define FAIL (-1)
#define READ_SIZE 5
#define SEND_SIZE 40
#define BUF_SIZE 9000
static std::map<int, std::string> recvInfo;
static std::map<int, int> sendInfo;
void *get_addr_in(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    else
    {
        // if is ipv6
        return &(((struct sockaddr_in6 *)sa)->sin6_addr);
    }
}

/**
 * 
*/
int Client::Connect(int &connFd)
{
    GetInfo();
    if (servInfo == nullptr)
        return -1;
    struct addrinfo *res;
    // int connFd; // socket handler for the connection
    int s;
    for (res = servInfo; res != nullptr; res = res->ai_next)
    {
        connFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (connFd == FAIL)
        {
            perror("Socket");
            continue;
        }
        // std::cout << "Client trying to connect to "<<get_addr_in(res->ai_addr)<<std::endl;;
        s = connect(connFd, res->ai_addr, res->ai_addrlen);
        if (s == FAIL)
        {
            perror("Connect");
            continue;
        }
        break;
    }
    if (res == nullptr)
    {
        //can't create from any server
        std::cerr << "Can't create socket from the infor list" << std::endl;
        exit(EXIT_FAILURE);
    }
    char ss[INET6_ADDRSTRLEN];

    inet_ntop(res->ai_family, get_addr_in(res->ai_addr), ss, sizeof(ss));
    std::cout << "Connecting to:" << ss << std::endl;

    freeaddrinfo(servInfo);
    servInfo = nullptr;
    return 0;
}
int Client::GetInfo()
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_addr = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    int s;
    s = getaddrinfo(NULL, PORT, &hints, &servInfo);
    if (s != 0)
    {
        std::cerr << "getaddrinfo:" << gai_strerror(s) << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Client get info finish" << std::endl;
    return 0;
}

void SendOut(int connFd, const void *sendBuf, int bufSize)
{
    ssize_t bufLeft = bufSize;
    char *sendStart = (char *)sendBuf;
    ssize_t byteSend;
    while ((byteSend = send(connFd, sendStart, bufLeft, 0)) >= 0)
    {
        std::cout << "Send " << byteSend << "/" << bufLeft << " bytes" << std::endl;
        auto sendMsg = std::string(sendStart, sendStart + byteSend);
        std::cout << "Send " << sendMsg << std::endl;
        sendStart += byteSend;
        bufLeft -= byteSend;
        sendInfo[connFd] += byteSend;
        std::cout << "Total send:" << connFd << "/" << sendInfo[connFd] << std::endl;
        if (bufLeft <= 0)
        {
            std::cerr << "Buffer full" << std::endl;
            return;
        }
    }
    if (byteSend < 0)
    {
        //error occur
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
        }
        else
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
    }
}

void HandleRecv(int connFd, void *recvBuf, int bufSize)
{
    int bufLeft = bufSize;
    char *recvStart = (char *)recvBuf;
    int totalRecv;
    std::cout << "Reading from fd:" << connFd << std::endl;
    ssize_t byteRecv;
    while ((byteRecv = recv(connFd, recvStart, bufLeft, 0)) > 0)
    {
        totalRecv += byteRecv;
        fprintf(stderr, "Receive: %d/%d\n", byteRecv, bufLeft);
        std::cout << "Total receive:" << connFd << "/" << totalRecv << std::endl;
        auto mesg = std::string(recvStart, recvStart + byteRecv);
        std::cout << "Receive:" << mesg << std::endl;
        recvInfo[connFd] += mesg;
        recvStart += byteRecv;
        bufLeft -= byteRecv;
        if (bufLeft <= 0)
        {
            std::cerr << "Buffer full" << std::endl;
            return;
        }
    }
    if (byteRecv < 0)
    {
        if (errno == EAGAIN)
        {
            std::cerr << __func__ << ":" << byteRecv << std::endl;
            std::cerr << __func__ << ":try again " << std::endl;
            return;
        }
        else
        {
            perror("recv");
            exit(EXIT_FAILURE);
        }
    }
    if (byteRecv == 0)
    {
        std::cerr << __func__ << ":Peer shutdown" << std::endl;
        exit(EXIT_FAILURE);
        // continue;
    }
}

int main(int argc, char *argv)
{
    pid_t sPid = getpid();
    std::cout << "Client run on " << sPid << std::endl;
    Client c;
    int connFd;
    c.Connect(connFd);
    if (NONBLOCKING)
    {
        int s = fcntl(connFd, F_SETFL, O_NONBLOCK);
        if (s == -1)
        {
            printf("[error] fcntl(O_NONBLOCK), [errno %d]\n", errno);
            exit(EXIT_FAILURE);
        }
    }
    const int msgSize = 1024;
    char sendBuf[msgSize];
    char recvBuf[msgSize];
    int pingpongTime = 20;
    memset(sendBuf, '9', sizeof(sendBuf));
    // ssize_t byteSend = send(connFd,sendBuf,sizeof(sendBuf),0);
    // while(1);
    while (1)
    {
        SendOut(connFd, sendBuf, sizeof(sendBuf));
        while(1);
        HandleRecv(connFd, recvBuf, sizeof(recvBuf));
        std::string recvStr = recvBuf;
        if (recvStr.find('$') != std::string::npos)
        {
            std::cout <<recvStr<<std::endl;
            std::cout << "Find it"<<std::endl;
            break;
        }
        else
        {
            std::cout << recvBuf << std::endl;
            memcpy(sendBuf, recvBuf, sizeof(recvBuf));
        }
    }
    return 0;
}