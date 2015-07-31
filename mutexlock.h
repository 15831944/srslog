#pragma once

#include <semaphore.h>

#define DISSALLOW_COPY_AND_ASSIGN(classname)  classname(const classname &);\
                                              classname & operator=(const classname &)

class MutexLock
{
public:
    MutexLock()
    {
        sem_init(&mu_ , 0, 1);
    }

    ~MutexLock()
    {
        sem_close(&mu_);
        sem_destroy(&mu_);
    }

    void Lock()
    {
        sem_wait(&mu_);
    }

    void UnLock()
    {
        sem_post(&mu_);
    }

private:
    DISSALLOW_COPY_AND_ASSIGN(MutexLock);

    sem_t mu_;
};
