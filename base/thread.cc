#include "thread.h"
#include "currentthread.h"

#include <semaphore.h>

std::atomic<int> Thread::_numCreated(0);

Thread::Thread(ThreadFunc func, const std::string &name)
: _started(false), _joined(false), _tid(0), _func(std::move(func)), _name(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if(_started && !_joined)
    {
        _thread->detach();
    }
}

void Thread::start()
{
    _started = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    _thread = std::shared_ptr<std::thread>(new std::thread([&](){
        _tid = CurrentThread::tid();
        sem_post(&sem);
        _func();
    }));
    sem_wait(&sem);
}

void Thread::join()
{
    _joined = true;
    _thread->join();
}

void Thread::setDefaultName()
{
    int num = ++_numCreated;
    if(_name.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        _name = buf;
    }
}