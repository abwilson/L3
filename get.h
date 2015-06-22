#ifndef GET_H
#define GET_H

#include "sequence.h"

namespace L3
{
    template<Sequence& readCursor, typename UpStream, typename SpinProbe=NoOp>
    struct Get
    {
        Get():
            _begin(readCursor.load(std::memory_order_relaxed)),
            _end(claim())
        {
        }

        ~Get()
        {
            readCursor.store(_end, std::memory_order_release);
        }

        Index begin() const { return _begin; }
        Index end() const { return _end; }

    protected:
        Index _begin;
        Index _end;

        Index claim()
        {
            Index begin = readCursor.load(std::memory_order_consume);

            Index end;
            SpinProbe sp;
            while(begin > (end = UpStream::least()))
            {
                sp;
            }
            return end;
        }
    };
}

#endif
