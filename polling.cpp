#include "server.h"
#include <poll.h>
#include <cstdio>
#include<iostream>
#include <cstdlib>
#include <unistd.h>
#define BUF_SIZE 1000
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
void PollingMsg(tcp::Server &serv, int listener, int fdSize)
{
    // const int fdSize = 
    int fdCount = 0;
    int ss;
    int newFd;
    struct pollfd *fds = new struct pollfd[fdSize];
    fds[0].events = POLLIN;
    fds[0].fd = listener;
    fdCount++;
    /**
     * Set up the timeout
    */

    /**
     * Polling here 
    */
    while (1)
    {
        // s = serv.Listen();
        int readyCount = poll(fds, fdSize, -1);
        if (readyCount <= 0)
        {
            if (readyCount == -1)
            {
                //readCount == -1 means error
                perror("poll");
                exit(EXIT_FAILURE);
            }
            if (readyCount == 0)
            {
                std::cerr << "POLL Timeout" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            for (int i = 0; i < fdCount; i++)
            {
                if (fds[i].revents & POLLIN)
                {
                    if (fds[i].fd == listener)
                    {
                        //accept logic
                        // Status s;
                        char m[BUF_SIZE];
                        serv.Accept(newFd);
                        if (newFd != -1)
                        {
                            ss = send(newFd, &m, sizeof(m), 0);
                            if (ss == -1)
                            {
                                perror("Send");
                                close(newFd);
                            }
                        }
                        addToFdSet(&fdCount, &fdSize, &fds, newFd);
                    }
                    else
                    {
                        //communication logic
                        char buf[BUF_SIZE];
                        std::cout << i << std::endl;
                        ss = recv(fds[i].fd, &buf, sizeof(buf), 0);
                        if (ss <= 0)
                        {
                            // the peer is closed or error occurred
                            if (ss == 0)
                            {
                                std::cerr << "Peer is closed" << std::endl;
                            }
                            else
                            {
                                std::cout << ss << std::endl;
                                perror("Recv");
                            }
                            close(fds[i].fd);
                            delFromFds(fds, i, &fdCount);
                        }
                        else
                        {
                            char mm[BUF_SIZE] = "TEST";
                            std::cout << "Recv " << buf << " Size:" << ss << std::endl;
                            ss = send(fds[i].fd, &mm, sizeof(mm), 0);
                            if (ss == -1)
                            {
                                perror("Send");
                                close(newFd);
                            }
                            std::cout << "send" << mm << std::endl;
                        }

                    } //End handle data from client
                }
            }
            //As for poll, we need to traverse the set
        } //end of if-else
    }     //end of while
    delete fds;
}