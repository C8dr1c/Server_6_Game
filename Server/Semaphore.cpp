#include "Semaphore.h"

CSemaphore::CSemaphore(int c=0) : count_(c)
{
}

void CSemaphore::reset()
{
    std::unique_lock<std::mutex> lock(mutex_);
    count_=0;
}

void CSemaphore::notify()
{
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    condition_.notify_one();
}

void CSemaphore::wait()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while(!count_)
        condition_.wait(lock);
    --count_;
}