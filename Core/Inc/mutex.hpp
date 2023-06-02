#ifndef _MUTEX_H
#define _MUTEX_H

#include "Port.hpp"
#include "CriticalRegion.hpp"

class mutex
    {
    bool flag;
    Port mwait;

    public:

    mutex() : flag(false)
        {
        }

    inline void lock()
        {
        CRITICAL_REGION(InterruptLock)
            {
            while(flag)
                {
                mwait.suspend();
                }
            flag = true;
            }
        }

    inline void unlock()
        {
        flag = false;
        if(mwait)
            {
            mwait.resume();
            }
        }
    };

#endif
