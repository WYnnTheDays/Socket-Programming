#include "msgQueue.h"
using tcp::MsgQueue;
using tcp::MsgTask;
int MsgQueue::Insert(Task *&t)
{
    // std::lock_guard<std::mutex> m(qLock);
    if (t == nullptr || t->fd < 0)
    {
        std::cout << "Empty task" << std::endl;
        return -1;
    }
    if (tHead == nullptr)
    {
        tHead = t;
        tTail = t;
    }
    else
    {
        tTail->next = t;
        tTail = t;
    }
    taskNum++;
    return 1;
}
int MsgQueue::get(Task *&t)
{
    // if we use a lock, we can simply set head = head->next when we pick a job
    t = tHead;

    // Task temp = *tHead;
    tHead = tHead->next;
    taskNum--;
    return 1;
}