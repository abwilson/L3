#ifndef DISRUPTOR_H
#define DISRUPTOR_H
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

#include <L3/util/cacheline.h>
#include <L3/util/ring.h>
#include <L3/util/types.h>

namespace L3 // Low Latency Library
{
    using Cursor = std::atomic<Index>;

    template<size_t size>
    class Barrier
    {
        const Cursor* const _cursors[size];

        Index least() const
        {
            Index result = cursors[0];
            for(auto c: cursors)
            {
                Index i = c.load(std::memory_order_acquire);
                if(i < result)
                {
                    result = i;
                }
            }
            return result;
        }
    };

    template<size_t size, typename SpinPolicy=NoOp>
    class Consumer
    {
        L3_CACHE_LINE Cursor _cursor;
        const Barrier<size> _barrier;

        Index cursor() const
        {
            //
            // When we read our cursor we know that only us - this
            // thread - can have modified the value therefore relaxed
            // memrory order is sufficient.
            //
            return cursor.load(std::memory_order_relaxed);
        }
        
        Index claim(Index begin)
        {
            //
            // The cursor is the start of a dependency chain
            // leading to reading a message from the ring
            // buffer. Therefore consume semantics are sufficient to
            // ensure synchronisation.
            //
            Index end;
            SpinPolicy sp;
            while((end = _barrier.least()) <= begin)
            {
                sp();
            }
            return end;
        }

        void cursor(Index end)
        {
            //
            // When writing to our cursor we have to ensure that the
            // new value is released to other threads to aquire.
            //
            cursor.store(end, std::memory_order_release);
        }

    public:
        friend class Get;
        
        //
        // RAII semantics to perform a get. Ensures that we always
        // commit.
        //
        class Get
        {
            Get(const Consumer& consumer):
                //
                // Only a single thread should be modifying the read cursor.
                //
                _begin{consumer.cursor.load(std::memory_order_relaxed)},
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

            using Iterator = T*;
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
    };

    //
    // Disruptor containing buffer of size Msg objects. Getters is the
    // number of consumers.
    //
    template<typename Msg, size_t size, size_t getters>
    class Disruptor
    {
    public:
        using Ring = L3::Ring<CacheLine<Msg>, size>;
        using Iterator = typename Ring::template Iterator<ring>;
        
        Ring<CacheLine<Msg>, size> _ring;
        //
        // Allow one extra space for the write cursor.
        //
        Barrier<getters + 1> getters;

        Cursor cursor;
    };

    template<typename Disruptor, typename Msg, size_t instance, >
    class Get
    {
    public:

        Cursor& cursor();
        Get(Disruptor& ):
            _begin{

        Msg* _begin;
        Msg* _end;

            
    };
}

#endif
