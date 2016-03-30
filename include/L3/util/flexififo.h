#ifndef FLEXIFIFO_H
#define FLEXIFIFO_H
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

/*******************************************************************************
 * 
 * FlexiFifo is pluggable lock free queue. The ends are pluggable to
 * support 1 => 1, n => 1, 1 => n and n => n patterns.
 *
 ******************************************************************************/

#include "cacheline.h"
#include "ring.h"
#include "types.h"

#include <stack>
#include <array>

namespace L3 // Low Latency Library
{
    template<typename Ring>
    struct UniqueHead
    {
        static constexpr size_t size = Ring::size;
        L3_CACHE_LINE Counter _claim{size};
        L3_CACHE_LINE Counter _head{size};
        
        Index claim(Counter& tail)
        {
            Index slot = _claim.fetch_add(1, std::memory_order_relaxed); 
            Index min =  slot - size;
            while(tail.load(std::memory_order_acquire) <= min);
            return slot;
        }
        void commit(Index){ _head.fetch_add(1, std::memory_order_release); }
    };

    template<typename Ring>
    struct SharedHead
    {
        static constexpr size_t size = Ring::size;
        L3_CACHE_LINE Counter _claim{size};
        L3_CACHE_LINE Counter _head{size};
        
        Index claim(Counter& tail)
        {
            Index slot = _claim.fetch_add(1, std::memory_order_acquire); 
            Index min = slot - size;
            while(tail.load(std::memory_order_acquire) <= min);
            return slot;
        }
        void commit(Index slot)
        {
            //
            // Wait until it's our turn to commit.
            //
            while(_head.load(std::memory_order_acquire) < slot);
            _head.fetch_add(1, std::memory_order_release);
        }
    };

    template<typename Ring>
    struct UniqueTail
    {
        static constexpr size_t size = Ring::size;
        L3_CACHE_LINE Counter _tail{size};

        template<typename SpinProbe=NoOp>
        Index claim(Counter& head)
        {
            Index slot = _tail.load(std::memory_order_acquire);
            while(head.load(std::memory_order_acquire) <= slot);
            return slot;
        }
      
        void commitTail()
        {
            _tail.fetch_add(1, std::memory_order_release);
        }
    };

    template<typename Ring>
    struct SharedTail
    {
        static constexpr size_t size = Ring::size;
        L3_CACHE_LINE Counter _claim{size};
        L3_CACHE_LINE Counter _tail{size};

        Index claim(Counter& head)
        {
            Index slot = _claim.fetch_add(1, std::memory_order_acq_rel);
            while(head.load(std::memory_order_acquire) <= slot);
            return slot;
        }
      
        void commit(Index slot)
        {
            //
            // Wait until it's our turn to commit.
            //
            while(_tail.load(std::memory_order_acquire) < slot - 1);
            _tail.fetch_add(1, std::memory_order_release);
        }

        bool tryCommit(Index slot)
        {
            if(_tail.load(std::memory_order_acquire) == slot - 1)
            {
                _tail.fetch_add(1, std::memory_order_release);
                return true;
            }
            return false;
        }
    };

    template<typename Ring,
             template<typename> class Head,
             template<typename> class Tail>
    class FlexiFifo: Ring
    {
        Head<Ring> _head;
        Tail<Ring> _tail;
    public:
        Index claimHead() { return _head.claim(_tail._tail); }
        void commitHead(Index slot) { _head.commit(slot); }
        
        Index claimTail() { return _tail.claim(_head._head); }
        void commitTail(Index slot) { _tail.commit(slot); }
        bool tryCommitTail(Index slot) { return _tail.tryCommit(slot); }
    };

    /**
     * Simple single threaded queue.
     */
    template<typename T, size_t log2size>
    class Queue
    {
        using Ring = Ring<T, log2size>;
        Ring _ring;
        Index _head{0};   // Next availble slot to write.
        Index _tail{0};   // Next available to read.
        
    public:
        bool empty() const    { return _head == _tail; }
        bool full()  const    { return _head == _tail + Ring::size; }
        T&  tail()            { return _ring[_tail]; }

        void put(const T&& t) { _ring[_head++] = t; }
        void put(const T& t)  { _ring[_head++] = t; }
        T&   get()            { return _ring[_tail++]; }
        void pop()            { _tail++; }
    };
        
    template<typename Fifo, size_t commits = 2>
    class SharedConsumer
    {
        Fifo* _fifo;
        Queue<Index, commits>  _pendingCommits;

    public:
        SharedConsumer(Fifo* p): _fifo(p) {}        

        Index claim()
        {
            if(!_pendingCommits.empty())
            {
                commit();
            }
            if(_pendingCommits.full())
            {
                _fifo->commitTail(_pendingCommits.get());
            }
            Index result = _fifo->claimTail();
            _pendingCommits.put(result);
            return result;
        }

        void commit()
        {
            //
            // We know there must be at least one pending commit
            // otherwise this commit would be logically inconsistent.
            //
            if(_fifo->tryCommitTail(_pendingCommits.tail()))
            {
                _pendingCommits.pop();
                //
                // We don't know how many more there may be.
                //
                while(!_pendingCommits.empty() &&
                      _fifo->tryCommitTail(_pendingCommits.tail()))
                {
                    _pendingCommits.pop();
                }
            }
        }
    };    
}

#endif
