#ifndef GET_H
#define GET_H

#include "sequence.h"

#include <L3/util/cacheline.h>
//#include "ring.h"

namespace L3
{
    template<typename Disruptor,
             size_t tag,
             typename Barrier,
             typename SpinPolicy=NoOp>
    struct Get
    {
        using Iterator = typename Disruptor::Iterator;
        
        Get():
            //
            // Only a single thread should be modifying the read cursor.
            //
            _begin{cursor.load(std::memory_order_relaxed)},
            _end{claim()}
        {
        }

        ~Get()
        {
            cursor.store(_end, std::memory_order_release);
        }

        Iterator begin() const { return _begin; }
        Iterator end() const { return _end; }

        L3_CACHE_LINE static L3::Sequence cursor;

    private:
        Iterator _begin;
        Iterator _end;
        
        Index claim()
        {
            //
            // The cursor is the start of a dependency chain
            // leading to reading a message from the ring
            // buffer. Therefore consume semantics are sufficient to
            // ensure synchronisation.
            //
            Index end;
            SpinPolicy sp;
            while(_begin >= (end = Barrier::least()))
            {
                sp();
            }
            return end;
        }
    };

    template<typename Disruptor,
             size_t tag,
             typename Barrier,
             typename SpinPolicy>
    L3_CACHE_LINE L3::Sequence
    Get<Disruptor, tag, Barrier, SpinPolicy>::cursor{Disruptor::size};
}

#endif
