#include "server.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include<thread>
using namespace tcp;

std::map<int, tcp::Con> Conns;
// struct Con tcp::connections;
tcp::Con connections;

void tcp::SetNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    int ss = fcntl(fd, F_SETFL, flag);
    if (ss < 0)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

void *get_addr_in(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        //sockaddr_in likes a re-implementaion from sockaddr,using
        //the same memory space, but different data entries(based on
        //the protocol)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    else
    {
        // if is ipv6
        return &(((struct sockaddr_in6 *)sa)->sin6_addr);
    }
}

Status Server::GetInfo()
{
    struct addrinfo hints;
    // struct addrinfo *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; //fill in my ip for me
    // if(getaddrinfo())
    int status;
    if ((status = getaddrinfo(nullptr, servPort, &hints, &servInfo)) != 0)
    {
        // using namespace std;
        std::cerr << "getaddrinfo:" << gai_strerror(status) << std::endl;
        // exit(EXIT_FAILURE);
        return Status::FAIL;
    }
    std::cout << "Get Info Complete" << std::endl;
    return Status::OK;
}

Status Server::Bind()
{
    /**
     * Now we already have servInfo, we need to traverse the info list
    */
    struct addrinfo *res = nullptr;
    //    int sockfd;
    if (servInfo == nullptr)
        return Status::FAIL;
    for (res = servInfo; res != NULL; res = res->ai_next)
    {
        if ((sockfd = socket(res->ai_family, res->ai_socktype,
                             res->ai_protocol)) == -1)
        {
            perror("server:Socket");
            continue;
        }
        int yes;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            return Status::FAIL;
        }
        if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
        {
            perror("server:bind");
            continue;
        }
        // sockList.emplace_back(sockfd);
        break;
    }
    //    if(sockList.empty() == true) return Status::FAIL;
    if (res == nullptr)
        return Status::FAIL;
    freeaddrinfo(servInfo);
    servInfo = nullptr;
    return Status::OK;
}

int Server::Listen()
{
    if (sockfd == SOCKUNAVAIL)
        return -1;
    int status;
    status = listen(sockfd, BACKLOG);
    if (status != 0)
    {
        perror("Listen");
        exit(EXIT_FAILURE);
    }
    if (sockfd == -1)
    {
        std::cerr << "Error getting listeners" << std::endl;
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

/**
 * @brief accept a connection from peer
 * 
 * @param newFd : connect fd return by connect() call
 * @return Status 
 */
Status Server::Accept(int &newFd)
{

    if (sockfd == SOCKUNAVAIL)
        return Status::FAIL;
    int status;
    // int newFd;
    struct sockaddr_storage peerAddress;
    socklen_t peerAddrLen = sizeof(peerAddrLen);
    newFd = accept(sockfd, (struct sockaddr *)&peerAddress, &peerAddrLen);
    if (newFd == -1)
    {
        perror("Accept");
        exit(EXIT_FAILURE);
    }
    char s[INET6_ADDRSTRLEN];
    inet_ntop(peerAddress.ss_family, get_addr_in((struct sockaddr *)&peerAddress),
              s, sizeof(s));
    std::cout << "[Server]Get connection From " << s << std::endl;
    std::string ss(s);
    std::map<int, std::string> temp;
    temp[newFd] = ss;
    peerInfo.emplace_back(temp);
    return Status::OK;
}

// void Server::lookupInfo(int fd, std::string& info){

// }
static std::map<int, int> recvInfo;
static std::map<int, std::string> recvMsg;
int tcp::readAll(int fd, void *buf, int bufSize)
{
    int bufLeft = bufSize;
    char *recvStart = (char *)buf;
    std::cout << "Reading from fd:" << fd << std::endl;
    while (bufLeft > 0)
    {
        ssize_t byteCount = recv(fd, recvStart, bufLeft, 0);
        if (byteCount < 0)
        {
            if (errno == EAGAIN)
            {
                std::cerr << __func__ << ":" << byteCount << std::endl;
                std::cerr << __func__ << ":try again " << std::endl;
                return 0;
            }
            else
            {
                perror("recv");
                return -1;
            }
        }
        if (byteCount == 0)
        {
            std::cerr << __func__ << ":Peer shutdown" << std::endl;
            return -1;
            // continue;
        }
        else
        {
            recvInfo[fd] += byteCount;
            fprintf(stderr, "Receive: %d/%d\n", byteCount, bufLeft);
            std::cout << "Total receive:" << fd << "/" << recvInfo[fd] << std::endl;
            auto mesg = std::string(recvStart, recvStart + byteCount);
            std::cout << "Receive:" << mesg << std::endl;
            recvMsg[fd] += mesg;
            recvStart += byteCount;
            bufLeft -= byteCount;
        }
    }
    std::cout << __func__ << " Buffer is full" << std::endl;
    return 1;
}

static std::map<int, int> sendInfo;

// int serv::send(){

// }

/**
 * @brief send all required data in the buffer
 * 
 * @param fd 
 * @param buf 
 * @param len 
 * @param sendSize 
 * @return int 
 *         -1 when error occur
 *         0 if success
 *         1 if need to try again
 */
int tcp::sendAll(int fd, void *buf, int &actualSend, int len)
{
    //send by bytess
    // we don't need a buffer in write
    ssize_t byteSend = 0;
    ssize_t totalSend = 0;
    ssize_t sendLeft = len;
    char *sendStart = (char *)buf;
    while (sendLeft > 0)
    {
        byteSend = send(fd, sendStart, sendLeft, 0);
        if (byteSend < 0)
        {
            if (errno == EAGAIN)
            {
                std::cout << __func__ << ": try again" << std::endl;
                return 0;
            }
            perror("Send");
            exit(EXIT_FAILURE);
        }
        else
        {
            std::cout << "Send " << byteSend << "/" << sendLeft << " bytes" << std::endl;
            auto sendMsg = std::string(sendStart, sendStart + byteSend);
            std::cout << "Send " << sendMsg << std::endl;
            sendStart += byteSend;
            sendLeft -= byteSend;
            totalSend += byteSend;
        }
    }
    actualSend = totalSend;
    std::cout << __func__ << " Sent buffer is empty" << std::endl;
    std::cout << "Total send " << totalSend << std::endl;
    return 1;
}

// int sendInt(int fd,)

void tcp::HandleWrite(int connFd, const void *sendBuf, int bufSize,int &actualSend)
{
    if (connFd < 0)
    {
        std::cerr << "fd:" << connFd << " Descriptor Error" << std::endl;
        exit(EXIT_FAILURE);
    }
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
        actualSend += byteSend;
        std::cout << "Total send:" << connFd << "/" << sendInfo[connFd] << std::endl;
        if (bufLeft <= 0)
        {
            std::cerr << "Buffer full" << std::endl;
            return;
        }
    }
    if (byteSend < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }
        else
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
    }
}
void tcp::HandleRead(int fd, void *buf, int size, int &actualRecv)
{
    if (fd < 0 || buf == nullptr)
    {
        std::cerr << "fd:" << fd << " Descriptor Error" << std::endl;
        exit(EXIT_FAILURE);
    }
    char *start = (char *)buf;
    int bufLeft = size;
    ssize_t byteRead;
    int totalRecv;
    while ((byteRead = recv(fd, start, bufLeft, 0) > 0))
    {
        totalRecv += byteRead;
        fprintf(stderr, "Receive: %d/%d\n", byteRead, bufLeft);
        std::cout << std::this_thread::get_id()<<" Total receive:" << fd << "/" << totalRecv << std::endl;
        auto mesg = std::string(start, start + byteRead);
        std::cout << "Receive:" << mesg << std::endl;
        start += byteRead;
        bufLeft -= byteRead;
        connections.readed += byteRead;
        if (bufLeft <= 0)
        {
            std::cerr << "Buffer full" << std::endl;
            return;
        }
    }
    if (byteRead <= 0)
    {
        if (byteRead == 0)
        {
            std::cerr << __func__ << ":Peer shutdown" << std::endl;
            // exit(EXIT_FAILURE);
            close(fd);
        }
        else
        {
            if (errno == EAGAIN)
            {
                std::cerr << __func__ << ":" << byteRead << std::endl;
                std::cerr << __func__ << ":try again " << std::endl;
                return;
            }
            else
            {
                perror("recv");
                // exit(EXIT_FAILURE);
            }
        }
    }
}

void tcp::Epoll_create(int &epfd)
{
    epfd = epoll_create(1);
    if (epfd == -1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
}

// void Epoll_wait
int EpollManager::create()
{
    int efd = epoll_create(1);
    if (efd == -1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    return efd;
}

void tcp::Epoll_event_add(int fd, int event, int epfd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;
    int ss = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    if (ss == -1)
    {
        fprintf(stderr, "in %s, fd:%d %s:%s \n", __func__, fd, "epoll_ctl", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
void tcp::Epoll_event_del(int fd, int event, int epfd)
{
    int ss = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    if (ss == -1)
    {
        fprintf(stderr, "in %s, fd:%d %s:%s \n", __func__, fd, "epoll_ctl", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
void tcp::Epoll_event_mod(int fd, int event, int epfd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;
    int ss = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    if (ss == -1)
    {
        fprintf(stderr, "in %s, fd:%d %s:%s \n", __func__, fd, "epoll_ctl", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
