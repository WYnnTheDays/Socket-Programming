#include "server.h"
#include <cstring>
using namespace serv;
// Server::Server(){

// }

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
    // if (sockfd == -1)
    // return Status::FAIL;
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
int serv::readAll(int fd, void *buf, int bufSize, int readSize)
{
    int bufLeft = bufSize;
    char *recvStart = (char *)buf;
    // size_t readSize =
    // std::map<int, int> recvInfo;
    std::cout << "Reading from fd:" << fd << std::endl;
    // std::cout << recvStart << std::endl;
    while (bufLeft > 0)
    {
        ssize_t byteCount = recv(fd, recvStart, readSize, 0);
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
            recvInfo[fd] += byteCount;
            fprintf(stderr, "Receive: %d/%d\n", byteCount, bufLeft);
            std::cout << "Total receive:" << fd << "/" << recvInfo[fd] << std::endl;
            ssize_t ptrMove = ((bufLeft - byteCount) >= 0) ? byteCount : bufLeft;
            auto mesg = std::string(recvStart, recvStart + byteCount);
            std::cout << "Receive:" << mesg << std::endl;
            recvStart += ptrMove;
            bufLeft -= ptrMove;
            readSize = (bufLeft - readSize >= 0) ? readSize : bufLeft;
            // recvInfo[fd] += byteCount;
            // std::cout << "Total receive:"<<fd<<"/"<<recvInfo[fd]<<std::endl;
        }
    }
    std::cout << __func__ << " Buffer is full" << std::endl;
    // std::cout << recvStart << std::endl;
    return 1;
}

static std::map<int, int> sendInfo;
int serv::sendAll(int fd, void *buf, int len, int sendSize)
{
    //send by bytes
    char *sendStart = (char *)buf;
    ssize_t byteSend = 0;
    ssize_t totalSend = 0;
    ssize_t sendLeft = len;
    if (sendSize > len)
    {
        //avoid sending too much data
        sendSize = len;
    }
    while (sendLeft > 0)
    {
        byteSend = send(fd, sendStart, sendSize, 0);
        if (byteSend == -1)
        {
            perror("Send");
            return -1;
        }
        std::cout << "Send " << byteSend << " bytes" << std::endl;
        auto sendMsg = std::string(sendStart, sendStart + sendSize);
        std::cout << "Send " << sendMsg << std::endl;
        ssize_t ptrMove = (sendLeft - byteSend > 0) ? byteSend : sendLeft;
        sendStart += ptrMove;
        sendLeft -= ptrMove;
        sendSize = (sendLeft - sendSize > 0) ? sendSize : sendLeft;
        totalSend += byteSend;
    }
    std::cout << __func__ << " Sent buffer is empty" << std::endl;
    std::cout << "Total send " << totalSend << std::endl;
    if (totalSend != len)
    {
        std::cerr << "Wrong Sending" << std::endl;
        std::cerr << totalSend << "/" << len << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}

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

void serv::epoll_event_add(int fd, int event, int epfd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;
    int ss = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    if (ss == -1)
    {
        perror("EPOLL_CTL");
        exit(EXIT_FAILURE);
    }
}
void serv::epoll_event_del(int fd, int event, int epfd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;
    int ss = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
    if (ss == -1)
    {
        perror("EPOLL_CTL");
        exit(EXIT_FAILURE);
    }
}
void serv::epoll_event_mod(int fd, int event, int epfd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = event;
    int ss = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    if (ss == -1)
    {
        perror("EPOLL_CTL");
        exit(EXIT_FAILURE);
    }
}
