#ifndef DISRUPTOR_H
#define DISRUPTOR_H

#include "ring.h"
#include "cacheline.h"

#include <atomic>

namespace L3 // Low Latency Library
{
    struct NoOp { void operator()() {}; };
    
    template<typename Ring,
             typename PutSpinProbe=NoOp,
             typename GetSpinProbe=NoOp,
             typename CursorSpinProbe=NoOp> class Disruptor;

    template<typename T, char log2size,
             typename PutSpinProbe,
             typename GetSpinProbe,
             typename CursorSpinProbe
             >
    class Disruptor<
        Ring<T, log2size>,
        PutSpinProbe,
        GetSpinProbe,
        CursorSpinProbe>: private Ring<T, log2size>
    {
        using Base = Ring<T, log2size>;
        static_assert(log2size > 0, "Minimun ring size is 2");
    public:
        using Base::size;
        typedef T value_type;
        typedef typename Base::Index Index;
        using Sequence = std::atomic<Index>;
//    using Sequence = size_t;

        Disruptor():
            Base(),
            //
            // See Put::nextSlot for why initial values are based on size.
            //
            // The next put position.
            //
            _head{size + 1},
            //
            // The next get.
            //
            _tail{size + 1},
            //
            // The latest commited put.
            //
            _cursor{size}
        {}

        friend class Put;
        class Put
        {
        public:
            Put(Disruptor& d): _dr(d), _slot(nextSlot()) {}
            ~Put() { commit(); }

            T& operator=(const T& rhs) const { return _dr[_slot] = rhs; }
            operator T&() const              { return _dr[_slot]; }
        protected:
            Disruptor& _dr;
            Index _slot;

            Index nextSlot()
            {
                //
                // Claim a slot. For the single producer case we don't
                // care about memory order because only this thread
                // can write to _head.
                //
                Index slot = _dr._head.fetch_add(1, std::memory_order_relaxed);
                //
                // We have our slot but we cannot write to it until we
                // are sure we will not overwrite data that has not
                // yet been consumed - _head is about to lap _tail
                // around the ring.
                //
                // To prevent _head catching up with _tail we must ensure
                //
                //     _head - _tail < size
                //
                // We spin while this is not the case.
                //
                // To optimise the loop condition we rearrange the above to:
                //
                //     _tail <= _head - size
                //
                // and precompiute _head - size. To ensure that this
                // difference is always positive the Disruptor ctor
                // initialises _head and _tail using size as a
                // baseline - ie as if we have already done one lap
                // around the ring.
                //
                Index wrapAt = slot - size;
                //
                // Must ensure that _tail is synchronised with writes
                // from consumers. We only need to know that this
                // relation is satisfied at this point. Since _tail
                // monotonically increases the condition cannot be
                // invalidated by consumers.
                //
                while(_dr._tail.load(std::memory_order_acquire) <= wrapAt)
                {
                    PutSpinProbe()();
                }
                return slot;
            }

            void commit()
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
                // For single producer no other thread could have
                // commited puts. so don't need this code.
                //
#if 0
                while(_dr._cursor.load(std::memory_order_acquire) < _slot - 1)
                {
                    CursorSpinProbe()();
                }                    
#endif
                //
                // No need to CAS. Only we could have been waiting for
                // this particular cursor value.
                //
                _dr._cursor.fetch_add(1, std::memory_order_release);
            }
        };

        void put(const T& rhs)
        {
            Put(*this) = rhs;
        }

        friend class Get;
        class Get
        {
        public:
            Get(Disruptor& d): _dr(d), _slot(nextSlot()) {}
            ~Get() { commit(); }

            operator const T&() const { return _dr[_slot]; }

        protected:
            Disruptor& _dr;
            Index     _slot;

            Index nextSlot()
            {
                Index slot = _dr._tail.load(std::memory_order_acquire);
                while(slot > _dr._cursor.load(std::memory_order_acquire))
                {
                    GetSpinProbe()();
                }
                return slot;
            }

            void commit()
            {
                _dr._tail.fetch_add(1, std::memory_order_release);
            }
        };

        T get()
        {
            return Get(*this);
        }
            
    protected:
        L3_CACHE_LINE Sequence _head;
        L3_CACHE_LINE Sequence _tail;
        L3_CACHE_LINE Sequence _cursor;
    };
}

#endif
