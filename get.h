#ifndef GET_H
#define GET_H

#include "sequence.h"
#include "ring.h"

namespace L3
{
    template<Sequence& readCursor,
             typename Iterator,
             typename UpStream,
             typename SpinPolicy=NoOp>
    struct Get
    {
        Get():
            //
            // Only a single thread should be modifying the read cursor.
            //
            _begin{readCursor.load(std::memory_order_relaxed)},
            _end{claim()}
        {
        }

        ~Get()
        {
            readCursor.store(_end, std::memory_order_release);
        }

        Iterator begin() const { return _begin; }
        Iterator end() const { return _end; }

    private:
        Iterator _begin;
        Iterator _end;
        
        Index claim()
        {
            //
            // The readCursor is the start of a dependency chain
            // leading to reading a message from the ring
            // buffer. Therefore consume semantics are sufficient to
            // ensure synchronisation.
            //
            Index end;
            SpinPolicy sp;
            while(_begin >= (end = UpStream::least()))
            {
                sp();
            }
            return end;
        }
    };
}

#endif
