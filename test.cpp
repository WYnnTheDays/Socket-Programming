#include "server.h"
#define PORT "3490"
#define NAME "TEST"
using namespace tcp;
#include <thread>
#include <poll.h>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include "msgQueue.h"
#include <condition_variable>
#define MAX_EVENTS 5
#define LISENSIZE 10
#define RECVSIZE 300
#define BUF_SIZE 300

void EventloopMain(int listenfd, tcp::Server s, int epfd)
{
    int connFd;
    int ss;
    struct epoll_event events[MAX_EVENTS];
    int running;
    while (running)
    {
        int size = epoll_wait(epfd, events, MAX_EVENTS, 300000);
        if (size <= 0)
        {
            if (size == 0)
            {
                std::cerr << "Timeout" << std::endl;
                continue;
            }
            else
            {
                std::cerr << __func__ << ":%s" << strerror(errno) << std::endl;
                std::cerr << "Process:" << getpid() << std::endl;
            }
        }
        for (int i = 0; i < size; i++)
        {
            if (events[i].data.fd & (EPOLLIN | EPOLLERR))
            {
                if (events[i].data.fd & EPOLLERR)
                {
                    std::cerr << "Socker error" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if (events[i].data.fd == listenfd)
                {
                    s.Accept(connFd);
                    Epoll_event_add(connFd, EPOLLIN, epfd);
                }
                else
                {
                    char buf[30];
                    int actualRecv;
                    HandleRead(events[i].data.fd, buf, sizeof(buf), actualRecv);
                    tcp::SetNonBlocking(events[i].data.fd);
                    Epoll_event_mod(events[i].data.fd, EPOLLOUT | EPOLLET, epfd);
                }
            }
            else if (events[i].data.fd & EPOLLOUT)
            {
                char buf[] = "I am wynn here";
                int actualSend;
                HandleWrite(events[i].data.fd, buf, sizeof(buf), actualSend);
                std::cout << "ActualSend:" << actualSend << std::endl;
            }
        }
    }
}

tcp::MsgQueue *rMq = new tcp::MsgQueue;
tcp::MsgQueue *wMq = new tcp::MsgQueue;
std::condition_variable cv4Read;
std::condition_variable cv4Write;

void ReadThread()
{
    Task *data = new Task;
    while (1)
    {
        //thread always run
        std::unique_lock<std::mutex> ul(rMq->qLock);
        std::cout << "Reader Thread ID:" << std::this_thread::get_id() << std::endl;
        cv4Read.wait(ul, [] { return rMq->hasTask(); });

        std::cout << "Read thread waiting for work" << std::endl;
        char *buf = new char[BUF_SIZE];
        int recvData;
        rMq->get(data);
        int fd = data->fd;
        int epfd = data->eventFd;
        int n;
        ul.unlock();
        if ((n = recv(fd, buf, BUF_SIZE, 0)) < 0)
        {
            if (errno == ECONNRESET)
                close(fd);
            fprintf(stderr, "[SERVER] Error: readline failed: %s\n", strerror(errno));
            if (data != NULL)
                delete data;
            // free(data);
        }
        else if (n == 0)
        {
            close(fd);
            if (data != NULL)
                free(data);
        }
        else
        {
            //successfully received
            std::string temp(buf);
            if (temp.find('#') != std::string::npos)
            {
                //end of the input
                Epoll_event_mod(fd, EPOLLOUT | EPOLLET, epfd);
            }
            else
            {
                Epoll_event_mod(fd, EPOLLIN | EPOLLET, epfd);
            }
            std::cout << "Over" << buf << std::endl;
        }
        delete[] buf;
    }
}
void WriteThread()
{
    Task *data;
    while (1)
    {
        std::unique_lock<std::mutex> ul(wMq->qLock);
        std::cout << "Writer Thread ID:" << std::this_thread::get_id() << std::endl;
        //thread always run
        cv4Write.wait(ul, [] { return wMq->hasTask(); });

        std::cout << "write thread starts working" << std::endl;
        char *buf = new char[BUF_SIZE];
        int sendData;
        // std::unique_lock<std::mutex> rL(wMq->qLock);
        wMq->get(data);
        int fd = data->fd;
        int epfd = data->eventFd;
        HandleWrite(fd, buf, BUF_SIZE, sendData);
        ul.unlock();
    }
}

int main(int argc, char **argv)
{
    pid_t sPid = getpid();
    std::cout << "Server run on " << sPid << std::endl;
    tcp::Server serv(NAME, PORT);
    Status s;
    ssize_t ss;

    int newFd, connFd, listener;
    int running = 1;
    epoll_event events[MAX_EVENTS];
    s = serv.Bind();
    // std::vector<std::thremaad> TList;
    

    /**
     * Set up the set we want to listen
    */
    listener = serv.Listen(); //get the listener we want to poll on
    int epfd;
    // std::cout<<"here"<<std::endl;
    Epoll_create(epfd);
    Epoll_event_add(listener, EPOLLIN, epfd);
    std::thread rT1(ReadThread);
    std::thread rT2(ReadThread);
    std::thread rT3(ReadThread);
    std::thread wT1(WriteThread);
    std::thread wT2(WriteThread);
    while (1)
    {
        // int size;
        // size = Epoll_wait(epfd,,MAX_EVENTS,100000);
        int size = epoll_wait(epfd, events, MAX_EVENTS, 10000);
        if (size <= 0)
        {
            if (size == 0)
            {
                std::cerr << "TimeOut" << std::endl;
                continue;
            }
            else
            {
                perror("epoll_wait");
                close(epfd);
                exit(EXIT_FAILURE);
            }
        }
        for (int i = 0; i < size; i++)
        {
            if (events[i].data.fd & (EPOLLIN))
            {
                if (events[i].data.fd < 0)
                    continue;
                // if (events[i].data.fd & EPOLLERR)
                // {

                //     std::cerr << "Error with fd:" << events[i].data.fd<<"/"<<strerror(errno) << std::endl;
                //     exit(EXIT_FAILURE);
                // }
                if (events[i].data.fd == listener)
                {
                    serv.Accept(connFd);
                    Epoll_event_add(connFd, EPOLLIN | EPOLLET, epfd); //why must ET
                    tcp::SetNonBlocking(connFd);
                    //master receive a connection
                    //put it in a connector pool or send it through pipe or msgQueue
                }
                else
                {
                    //generate an input Task
                    //if not ET pattern, one input came ,a task generate
                    fprintf(stderr, "%d : EPOLLIN\n", events[i].data.fd);
                    Task *newTask = new Task(events[i].data.fd, epfd);
                    // tcp::SetNonBlocking(connFd);
                    fprintf(stdout, "[SERVER] Put %d into read queue\n", events[i].data.fd);
                    std::lock_guard<std::mutex> gm(rMq->qLock);
                    rMq->Insert(newTask);
                    // fprintf(stderr, "%d : EPOLLIN overs\n", events[i].data.fd);
                    cv4Read.notify_all();
                    //  pthread_cond_broadcast(&r_condl);
                    // fprintf(stderr, "%d : EPOLLIN over\n", events[i].data.fd);
                }
            }
            else if (events[i].data.fd & (EPOLLOUT))
            {
                //EPOLLOUT and EPOLLIN may occur in same time
                fprintf(stderr, "%d : EPOLLOUT\n", events[i].data.fd);
                Task *newTask = new Task(events[i].data.fd, epfd);
                std::lock_guard<std::mutex> gm(wMq->qLock);
                fprintf(stdout, "[SERVER] Put %d into write queue\n", events[i].data.fd);
                wMq->Insert(newTask);
                // pthread_cond_broadcast(&w_condl);
                cv4Write.notify_one();
            }
        }
    }
    // Epoll_event_add(listener, EPOLLIN, epfd);//only one thread can create？
    // std::vector<std::thread> TList;
    // for (int i = 0; i < 5; i++)
    // {
    //     TList.push_back(std::move(std::thread(EventloopMain, listener, serv, epfd)));
    //     // std::thread t(EventloopMain,listener,serv);
    // }
    // for (int i = 0; i < 4; i++)
    // {
    //     if (TList[i].joinable())
    //     {
    //         TList[i].join();
    //     }
    //     else
    //     {
    //         std::cout << "Not Jointable" << std::endl;
    //         continue;
    //     }
    // }
    std::cout << "Here" << std::endl;
    // t1.join();
    // t2.join();
    // t3.join();
    // t4.join();

    // int efd = epoll_create(1);
    // if (efd == -1)
    // {
    //     perror("epoll_create");
    //     exit(EXIT_FAILURE);
    // }
    // struct epoll_event ev, events[MAX_EVENTS];
    // ev.events = EPOLLIN;
    // ev.data.fd = listener;
    // ss = epoll_ctl(efd, EPOLL_CTL_ADD, listener, &ev);
    // if (ss == -1)
    // {
    //     perror("epoll_ctl");
    //     exit(EXIT_FAILURE);
    // }
    // while (running)
    // {
    //     int size = epoll_wait(efd, events, MAX_EVENTS, 300000); //wait for listenfd
    //     std::cout << "Size " << size << std::endl;
    //     if (size <= 0)
    //     {
    //         if (size == 0)
    //         {
    //             std::cout << "Time Out" << std::endl;
    //             continue;
    //         }
    //         else
    //         {
    //             perror("epoll_wait");
    //             close(efd);
    //             exit(EXIT_FAILURE);
    //         }
    //     }
    //     for (int i = 0; i < size; i++)
    //     {
    //         if (events[i].data.fd == listener)
    //         {
    //             if (events[i].events & (EPOLLIN | EPOLLOUT))
    //             {
    //                 std::cout << "Listening:EPOLLIN,EPLOOOUT" << std::endl;
    //             }
    //             serv.Accept(connFd);
    //             // Epoll_event_add(connFd, EPOLLOUT|EPOLLET, efd);//EPOLLOUT ONLY OCCURS ONCE IN PINGPONG
    //             Epoll_event_add(connFd, EPOLLIN, efd); //EPOLLIN OCCUR MULTIPLE TIMES
    //             // Epoll_event_add(connFd,EPOLLIN|EPOLLET,efd);
    //             // Epoll_event_add(connFd, EPOLLOUT, efd);
    //             char buf[] = "Start";
    //             int actualSend;
    //             int e = sendAll(connFd, buf, actualSend, sizeof(buf));
    //             // Epoll_event_mod(events[i].data.fd, EPOLLOUT|EPOLLET, efd);
    //         }
    //         else if (events[i].events & EPOLLIN)
    //         {
    //             fprintf(stderr, "%d : EPOLLIN\n", events[i].data.fd);
    //             char buf[RECVSIZE + 1];
    //             int status = readAll(events[i].data.fd, (void *)buf, sizeof(buf));
    //             if (status < 0)
    //             {
    //                 // close(events[i].data.fd);
    //                 Epoll_event_del(events[i].data.fd, EPOLLIN | EPOLLET, efd);
    //                 close(events[i].data.fd);
    //             }
    //             else
    //             {
    //                 status = fcntl(events[i].data.fd, F_SETFL, O_NONBLOCK);
    //                 // Epoll_event_mod(events[i].data.fd, EPOLLIN|EPOLLET, efd);
    //                 // Epoll_event_mod(events[i].data.fd, EPOLLIN, efd);
    //                 // Epoll_event_mod(events[i].data.fd, EPOLLOUT, efd);
    //                 if (status == -1)
    //                 {
    //                     perror("FCNTL");
    //                     exit(EXIT_FAILURE);
    //                 }
    //                 Epoll_event_mod(events[i].data.fd, EPOLLOUT | EPOLLET, efd);
    //             }
    //             /**
    //              * @brief write after read will cause blocking
    //              */
    //         }
    //         else if (events[i].events & EPOLLOUT)
    //         {
    //             fprintf(stderr, "%d : EPOLLOUT\n", events[i].data.fd);
    //             char sbuf[BUF_SIZE + 1];
    //             int actualSend;
    //             memset(sbuf, '4', BUF_SIZE + 1);
    //             static int times = 0;
    //             times++;
    //             if (times == 50)
    //                 sbuf[BUF_SIZE + 1] = '$';
    //             int error = sendAll(events[i].data.fd, (void *)sbuf, actualSend, sizeof(sbuf));
    //             if (error == 0)
    //             {
    //                 continue;
    //             }
    //             Epoll_event_mod(events[i].data.fd, EPOLLOUT | EPOLLET, efd); //will make it equal to EPOLLLEVEL
    //         }
    //         else if (events[i].events & EPOLLRDHUP)
    //         {
    //             fprintf(stderr, "%d : EPOLLRDHUP\n", events[i].data.fd);
    //         }
    //         if (events[i].events & (EPOLLIN | EPOLLOUT))
    //         {
    //             std::cout << "EPOLLIN AND EPOLLOUT" << std::endl;
    //         }

    //     } //end of iteration through  ready event
    // }
    return EXIT_SUCCESS;
}
