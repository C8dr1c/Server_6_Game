#include<condition_variable>
#include<random>

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

typedef std::ranlux48 TRandomGenerator;

class CSemaphore
{
private:
   std::mutex mutex_;
   std::condition_variable condition_;
   unsigned int count_;

public:  
   CSemaphore(int);

   void reset();
   void notify();
   void wait();
};

#endif