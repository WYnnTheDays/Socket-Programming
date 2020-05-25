#include "server.h"
#define PORT "3490"
#define NAME "TEST"
using namespace serv;
#include <thread>
#include <poll.h>
#include <cstring>
#include <unistd.h>
#define MAX_EVENTS 5
#define LISENSIZE 10
#define BUF_SIZE 300
#define READ_SIZE 15
// #define BUFDIZE 1000

void addToFdSet(int *fdCount, int *fdSize, struct pollfd **fdset, int &fd)
{
    /**
     * Add new fd to the fdset
    */
    if (*fdCount == *fdSize)
    {
        //reach the end of the set
        *fdSize = *fdSize * 2;
        *fdset = (struct pollfd *)realloc(*fdset, sizeof(**fdset) * (*fdSize));
    }
    (*fdset)[*fdCount].fd = fd;
    (*fdset)[*fdCount].events = POLLIN;
    (*fdCount)++;
    std::cout << "Add " << fd << " to set. Current size is: " << *fdCount << "/" << *fdSize << std::endl;
}
void delFromFds(struct pollfd pfds[], int &i, int *fd_count)
{
    //copy the end to replace current fd
    pfds[i] = pfds[*fd_count - 1];
    (*fd_count)--;
}
int main(int argc, char **argv)
{
    pid_t sPid = getpid();
    std::cout << "Server run on " << sPid << std::endl;
    Server serv(NAME, PORT);
    Status s;
    int listener;
    ssize_t ss;
    int newFd;
    int connFd;
    int fdSize = 5;
    int fdCount = 0;
    int running = 1;
    s = serv.Bind();

    /**
     * Set up the set we want to listen
    */
    listener = serv.Listen(); //get the listener we want to poll on
    // struct pollfd *fds = new struct pollfd[fdSize];
    // fds[0].events = POLLIN;
    // fds[0].fd = listener;
    // fdCount++;
    /**
     * Set up the timeout
    */

    /**
     * Polling here 
    */
    // while (1)
    // {
    //     struct Message m;
    //     std::thread t;
    //     // s = serv.Listen();
    //     int readyCount = poll(fds, fdSize, -1);
    //     if (readyCount <= 0)
    //     {
    //         if (readyCount == -1)
    //         {
    //             //readCount == -1 means error
    //             perror("poll");
    //             exit(EXIT_FAILURE);
    //         }
    //         if (readyCount == 0)
    //         {
    //             std::cerr << "POLL Timeout" << std::endl;
    //             exit(EXIT_FAILURE);
    //         }
    //     }
    //     else
    //     {
    //         for (int i = 0; i < fdCount; i++)
    //         {
    //             if (fds[i].revents & POLLIN)
    //             {
    //                 if (fds[i].fd == listener)
    //                 {
    //                     //accept logic
    //                     s = serv.Accept(newFd);
    //                     if (newFd != -1)
    //                     {
    //                         ss = send(newFd, &m, sizeof(m), 0);
    //                         if (ss == -1)
    //                         {
    //                             perror("Send");
    //                             close(newFd);
    //                         }
    //                     }
    //                     addToFdSet(&fdCount, &fdSize, &fds, newFd);
    //                 }
    //                 else
    //                 {
    //                     //communication logic
    //                     char buf[BUF_SIZE];
    //                     std::cout << i << std::endl;
    //                     ss = recv(fds[i].fd, &buf, sizeof(buf), 0);
    //                     if (ss <= 0)
    //                     {
    //                         // the peer is closed or error occurred
    //                         if (ss == 0)
    //                         {
    //                             std::cerr << "Peer is closed" << std::endl;
    //                         }
    //                         else
    //                         {
    //                             std::cout << ss << std::endl;
    //                             perror("Recv");
    //                         }
    //                         close(fds[i].fd);
    //                         delFromFds(fds, i, &fdCount);
    //                     }
    //                     else
    //                     {
    //                         char mm[BUF_SIZE] = "TEST";
    //                         std::cout << "Recv " << buf << " Size:" << ss << std::endl;
    //                         ss = send(fds[i].fd, &mm, sizeof(mm), 0);
    //                         if (ss == -1)
    //                         {
    //                             perror("Send");
    //                             close(newFd);
    //                         }
    //                         std::cout << "send" << mm << std::endl;
    //                     }

    //                 } //End handle data from client
    //             }
    //         }
    //         //As for poll, we need to traverse the set
    //     } //end of if-else
    // }     //end of while
    // delete fds;

    int efd = epoll_create(1);
    if (efd == -1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = listener;
    ss = epoll_ctl(efd, EPOLL_CTL_ADD, listener, &ev);
    if (ss == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }
    std::map<int, int> recvInfo;
    while (running)
    {
        int size = epoll_wait(efd, events, MAX_EVENTS, 300000); //wait for listenfd
        std::cout << "Size " << size << std::endl;
        if (size <= 0)
        {
            if (size == 0)
            {
                std::cout << "Time Out" << std::endl;
                continue;
            }
            else
            {
                perror("epoll_wait");
                close(efd);
                exit(EXIT_FAILURE);
            }
        }
        for (int i = 0; i < size; i++)
        {
            // char *buf = new char[BUFSIZ];
            if (events[i].data.fd == listener)
            {
                serv.Accept(connFd);
                struct epoll_event connEv;
                connEv.events = EPOLLIN;
                connEv.data.fd = connFd;
                ss = epoll_ctl(efd, EPOLL_CTL_ADD, connFd, &connEv);
                if (ss == -1)
                {
                    perror("epoll_ctl");
                    exit(EXIT_FAILURE);
                }
                char buf[] = "Start";
                int e = sendAll(connFd, buf, sizeof(buf), 15);
                if (e == -1)
                {
                    perror("send");
                    exit(EXIT_FAILURE);
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                fprintf(stderr, "%d : EPOLLIN\n", events[i].data.fd);
                char buf[500 + 1];
                int error = readAll(events[i].data.fd, (void *)buf, 500, 15);

                if (error == -1)
                {
                    exit(EXIT_FAILURE);
                }
                // error = fcntl(events[i].data.fd, F_SETFD, O_NONBLOCK);
                // epoll_event_mod(events[i].data.fd, EPOLLIN | EPOLLOUT, efd);
                
                /**
                 * @brief write after read will cause blocking 
                 * 
                 */
                char sbuf[BUF_SIZE + 1];
                memset(sbuf, '4', BUF_SIZE + 1);
                 error = sendAll(events[i].data.fd, (void *)sbuf, BUF_SIZE, 20);
                if(error == -1){
                    exit(EXIT_FAILURE);
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                fprintf(stderr, "%d : EPOLLOUT\n", events[i].data.fd);
                char sbuf[BUF_SIZE + 1];
                memset(sbuf, '4', BUF_SIZE + 1);
                int error = sendAll(events[i].data.fd, (void *)sbuf, BUF_SIZE, 20);
                if(error == -1){
                    exit(EXIT_FAILURE);
                }
            }

        } //end of iteration through  ready event
    }
    if (close(efd))
    {
        std::cerr << "Failed to close epoll file descriptor" << std::endl;
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}