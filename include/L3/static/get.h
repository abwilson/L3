#ifndef GET_H
#define GET_H
/*
The MIT License (MIT)

Copyright (c) 2015 Norman Wilson - Volcano Consultancy Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "sequence.h"

#include <L3/util/cacheline.h>

#include <limits>

namespace L3
{
    template<typename Disruptor,
             typename Tag,
             typename Barrier,
             typename SpinPolicy=NoOp>
    struct Get
    {
        Get():
            //
            // Only a single thread should be modifying the read cursor.
            //
            _begin{cursor.load(std::memory_order_relaxed)},
            _end{claim(_begin)}
        {}
        
        Get(size_t maxBatchSize):
            _begin{cursor.load(std::memory_order_relaxed)},
            _end{std::min(claim(_begin), _begin + maxBatchSize)}
        {
        }

        enum NoBlock { noBlock };
        Get(size_t maxBatchSize, NoBlock):
            _begin(cursor.load(std::memory_order_relaxed)),
            _end(std::min(Barrier::least(), _begin + maxBatchSize))
        {
        }

        Get(NoBlock):
            _begin(cursor.load(std::memory_order_relaxed)),
            _end(Barrier::least())
        {}
        
        ~Get()
        {
            if(_begin != _end)
            {
                cursor.store(_end, std::memory_order_release);
            }
        }

        using Iterator = typename Disruptor::Iterator;
        Iterator begin() const { return _begin; }
        Iterator end() const { return _end; }

        L3_CACHE_LINE static L3::Sequence cursor;

    private:
        Iterator _begin;
        Iterator _end;

        Get(Index b, Index e):
            _begin(b),
            _end(e)
        {}
        
        static Index claim(Index begin)
        {
            //
            // The cursor is the start of a dependency chain
            // leading to reading a message from the ring
            // buffer. Therefore consume semantics are sufficient to
            // ensure synchronisation.
            //
            Index end;
            SpinPolicy sp;
            while((end = Barrier::least()) <= begin)
            {
                sp();
            }
            return end;
        }
    };

    template<typename Disruptor,
             typename Tag,
             typename Barrier,
             typename SpinPolicy>
    L3_CACHE_LINE L3::Sequence
    Get<Disruptor, Tag, Barrier, SpinPolicy>::cursor{Disruptor::size};
}

#endif
