#ifndef SCOPEDTIMER_H
#define SCOPEDTIMER_H

#include <chrono>

namespace L3
{
    template<typename Duration=std::chrono::microseconds,
             typename Clock=std::chrono::steady_clock>
    struct ScopedTimer
    {
        typename Clock::time_point _start;
        Duration& _result;

        ScopedTimer(Duration& result):
            _result(result),
            _start(Clock::now())
        {}

        ~ScopedTimer()
        {
            typename Clock::time_point end(Clock::now());
            
            _result = std::chrono::duration_cast<Duration>(end - _start);
        }
    };
}

#endif
