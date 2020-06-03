#ifndef MSGQUEUE_H
#define MSGQUEUE_H
#include "server.h"
#include <mutex>
#define MSG_MAX 100
#define MAX_DATA 500
namespace tcp
{
    struct MsgTask
    {
        int fd;      //which connections
        int eventFd; // may need this
        struct MsgTask *next;
        char data[MAX_DATA];
        MsgTask() = default;
        MsgTask& operator=(MsgTask&);
        MsgTask(int f, int eFd) : fd(f), eventFd(eFd), next(nullptr) {}
    };
    using Task = struct MsgTask;

    //do we need to separate read and write
    struct MsgQueue
    {
    public:
        bool hasTask()
        {
            // std::lock_guard<std::mutex> m(qLock);
            // std::cout << taskNum << std::endl;
            return taskNum;
        }
        bool checkTaskNum(){
            // std::lock_guard<std::mutex> m(qLock);
            return taskNum;
        }
        MsgQueue() = default;
        int Insert(Task *&);
        int get(Task *&);
        // std::lock_guard<mutex> g{}
        std::mutex qLock;

    private:
        Task *tHead = NULL;
        Task *tTail = NULL;
        size_t taskNum = 0;
    };

}; // namespace tcp

#endif