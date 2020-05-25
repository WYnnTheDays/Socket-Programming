#include "client.h"
#define PORT "3490"
#define NAME "TEST"
using namespace cli;
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include<string>
#define FAIL (-1)
#define READ_SIZE 5
#define SEND_SIZE 40
#define BUF_SIZE 500
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
        return FAIL;
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
        return -1;
    }
    std::cout << "Client get info finish" << std::endl;
    return 0;
}

int main(int argc, char **argv)
{
    pid_t sPid = getpid();
    std::cout << "Server run on " << sPid << std::endl;
    Client c;
    int connFd;
    c.Connect(connFd);
    ssize_t count;
    char buf[BUF_SIZE];
    memset(buf, '3', BUF_SIZE);
    ssize_t ss;
    char recvBuf[BUF_SIZE];
    int sendLeft = BUF_SIZE;
    ss = recv(connFd, recvBuf, sizeof(recvBuf), 0);
    if (ss == 0)
    {
        std::cerr << "Connection is closed" << std::endl;
        exit(EXIT_FAILURE);
    }
    else if (ss == -1)
    {
        perror("Recv");
        exit(EXIT_FAILURE);
    }
    recvBuf[ss + 1] = '\0';
    std::cout << "Recv:" << recvBuf << std::endl;
    int times = 3;
    char *start = buf;
    int sendCount = SEND_SIZE;
    int totalSend = 0;
    while (sendLeft >= 0)
    {
        count = send(connFd, start, sendCount, 0);
        if (count == -1)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
        if (count == 0)
        {
            std::cerr << "Send over" << std::endl;
            break;
        }
        if (count != SEND_SIZE)
        {
            std::cerr << "Partial Send" << std::endl;
        }

        char *temp = new char[count];
        memmove(temp, start, count);
        std::cout << "Sent:" << count << " Bytes" << std::endl;
        std::cout << "Sent:" << temp << std::endl;
        sendLeft -= count;
        start += count;
        totalSend += count;
        sendCount = (sendLeft > SEND_SIZE) ? SEND_SIZE : sendLeft;
        delete[] temp;
    }
    std::cout << "Total send:" << totalSend << std::endl;

    // memset(recvBuf,'0',sizeof(recvBuf));
    // std::cout << recvBuf<<std::endl;
    int bufLeft = BUF_SIZE;
    char recvBuffer[BUF_SIZE];
    memset(recvBuffer,'0',sizeof(recvBuf));
    char *recvStart = recvBuffer;
    size_t readSize = 15;
    int totalRecv = 0;
    while (bufLeft > 0)
    {
        ssize_t byteCount = recv(connFd, recvStart, readSize, 0);
        if (byteCount <= 0)
        {
            if (byteCount == 0)
            {
                std::cerr << __func__ << ":Read Complete" << std::endl;
                return 0;
                // continue;
            }
            else
            {
                perror("recv");
                return -1;
            }
            // return -1;
        }
        else
        {
            totalRecv+=byteCount;
            fprintf(stderr, "Receive: %d/%d totalReceive:%d\n", byteCount, bufLeft,totalRecv);
            // std::cout << recvStart<<std::endl;
            ssize_t ptrMove = ((bufLeft - byteCount) >= 0) ? byteCount : bufLeft;
            auto mesg = std::string(recvStart, recvStart + byteCount);
            std::cout << "Receive:" << mesg << std::endl;
            recvStart += ptrMove;
            bufLeft -= ptrMove;
            readSize = (bufLeft - readSize >= 0) ? readSize : bufLeft;
            // totalRecv+=byteCount;
        }
    }
    // std::cout << recvStart << std::endl;
    std::cout << totalRecv <<std::endl;
    // while(1);
    close(connFd);
    return EXIT_SUCCESS;
}