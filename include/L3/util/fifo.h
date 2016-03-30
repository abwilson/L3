#ifndef FIFO_H
#define FIFO_H
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
 * Fifo is a simple lock free queue. It's constructed using the same
 * techniques as the Disruptor pattern but lacks its pluggability. As
 * such it's a good introduction to how the disruptor works without
 * getting bogged down in the template code involved in separating out
 * the producers from the consumers complete with the difference
 * waiting strategies.
 *
 ******************************************************************************/

#include "cacheline.h"
#include "ring.h"
#include "types.h"

namespace L3 // Low Latency Library
{
    template<typename T, size_t log2size>
    class Fifo
    {
        //
        // A valid ring buffer needs the cursor position to be
        // different from the head position. Therefore the minimum
        // size is 2 or 2^1.
        //
        static_assert(log2size > 0, "Minimun ring size is 2");
        //
        // In the implementation of a disruptor there's a window
        // between a slot has been claimed and the cursor advanced
        // where the value must be copied into the ring buffer. If
        // this operation fails and the cursor is not advanced then
        // the whole system will block. To ensure the value copy
        // cannot fail constrain T to being a POD.
        //
        static_assert(std::is_pod<T>::value, "PODs only");

    public:
        using Ring = Ring<T, log2size>;
        using value_type = T;

        static constexpr Index size = Ring::size;

        L3_CACHE_LINE Ring _ring;
        //
        // See Put::claim for why initial values are based on size.
        //
        // The next put position.
        //
        L3_CACHE_LINE std::atomic<Index> _head{size + 1};
        //
        // The next get.
        //
        L3_CACHE_LINE std::atomic<Index> _tail{size + 1};
        //
        // The latest commited put.
        //
        L3_CACHE_LINE std::atomic<Index> _cursor{size};

        template<typename SpinProbe=NoOp>
        Index claimHead(SpinProbe spinProbe=SpinProbe())
        {
            //
            // Claim a slot. For the single producer case we don't
            // care about memory order because only this thread can
            // write to _head. More generally if there are multiple
            // threads putting to head then we must ensure that this
            // increment is synchronised.
            //
            Index slot = _head.fetch_add(1, std::memory_order_acquire);
            //
            // We have our slot but we cannot write to it until we
            // are sure we will not overwrite data that has not
            // yet been consumed. In other words _head cannot lap
            // _tail around the ring.
            //
            // To prevent _head catching up with _tail we must
            // ensure that the gap between _head and _tail is never
            // greater than size. Or
            //
            //     _tail <= _head - size
            //
            // We spin while this is not the case.
            //
            // To optimise the loop condition precompiute _head -
            // size. To ensure that this difference is always
            // positive the Disruptor ctor initialises _head and
            // _tail using size as a baseline - ie as if we have
            // already done one lap around the ring.
            //
            Index wrapAt = slot - size;
            //
            // Must ensure that _tail is synchronised with writes
            // from consumers. We only need to know that this
            // relation is satisfied at this point. Since _tail
            // monotonically increases the condition cannot be
            // invalidated by consumers.
            //
            while(_tail.load(std::memory_order_acquire) <= wrapAt)
            {
                spinProbe();
            }
            return slot;
        }

        template<typename SpinProbe=NoOp>
        void commitHead(Index slot, SpinProbe spinProbe=SpinProbe())
        {
            //
            // To commit a put we advance the cursor to show that
            // slot is now available for reading.
            //
            // If there are multiple producers it's possible that
            // we've claimed a slot while another producer is
            // doing a put. If the other producers have not yet
            // commited then we must wait for them. When cursor is
            // 1 less than _slot we know that we are next to
            // commit.
            //
            --slot;
            //
            // For single producer no other thread could have
            // commited puts. So this code would be unnecessary.
            //
            while(_cursor.load(std::memory_order_acquire) < slot)
            {
                spinProbe();
            }
            //
            // No need to CAS. Only we could have been waiting for
            // this particular cursor value.
            //
            _cursor.fetch_add(1, std::memory_order_release);
        }

        template<typename ClaimProbe=NoOp, typename CommitProbe=NoOp>
        class Put
        {
            Fifo& _fifo;
            Index _slot;
        public:
            Put(Fifo& fifo): _fifo(fifo), _slot(fifo.claimHead(ClaimProbe())) {}
            ~Put() { _fifo.commitHead(_slot, CommitProbe()); }

            value_type& operator=(const T& rhs) const
            { return _fifo._ring[_slot] = rhs; }

            value_type& operator=(const T&& rhs) const
            { return _fifo._ring[_slot] = rhs; }
            
            operator value_type&() const
            { return _fifo.ring[_slot]; }

            operator value_type&&() const
            { return _fifo.ring[_slot]; }
        };

        void put(const T& rhs)
        {
            Put<>(*this) = rhs;
        }

        template<typename SpinProbe=NoOp>
        Index claimTail(SpinProbe spinProbe=SpinProbe())
        {
            Index slot = _tail.load(std::memory_order_acquire);
            while(_cursor.load(std::memory_order_acquire) < slot)
            {
                spinProbe();
            }
            return slot;
        }
      
        void commitTail()
        {
            _tail.fetch_add(1, std::memory_order_release);
        }

        template<typename ClaimProbe=NoOp>
        class Get
        {
            Fifo& _fifo;
            Index _slot;
            
        public:
            Get(Fifo& fifo): _fifo(fifo), _slot(fifo.claimTail(ClaimProbe())) {}
            ~Get() { _fifo.commitTail(); }

            operator const value_type&() const { return _fifo._ring[_slot]; }
        };            

        T get()
        {
            return Get<>(*this);
        }
    };
}

#endif
