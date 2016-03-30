#ifndef SCOPEDTIMER_H
#define SCOPEDTIMER_H

#include <chrono>

namespace L3
{
    template<typename C=std::chrono::steady_clock,
             typename D=typename C::duration>
    struct ScopedTimer
    {
        using duration = D;
        using clock = C;

        typename clock::time_point _start;
        
        duration& _result;

        ScopedTimer(duration& result):
            _result(result),
            _start(clock::now())
        {
        }

        ~ScopedTimer()
        {
            typename clock::time_point end(clock::now());
            _result = std::chrono::duration_cast<duration>(end - _start);
        }
    };
}

#endif
