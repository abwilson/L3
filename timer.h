#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

namespace L3
{
    class Timer
    {
        Timer(): start{}
        {
            gettimeofday(&start, nullptr);
        }
        ~Timer()
        {
            timeval finish;
            gettimeofday(&finish, nullptr);
        }

        timeval start;
    };
}

#endif
