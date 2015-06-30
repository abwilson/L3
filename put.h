#ifndef PUT_H
#define PUT_H

#include "ring.h"
#include "sequence.h"
#include "barrier.h"

namespace L3
{
    namespace CommitPolicy
    {
        struct Unique
        {
            template<typename Cursor>
            bool operator()(Cursor&, Index) { return false; }
            static const std::memory_order order{std::memory_order_relaxed};
        };

        struct Shared
        {
            template<typename Cursor>
            bool operator()(Cursor& commitCursor, Index slot)
            {
                //
                // For multiple producers it's possible that we've
                // claimed a slot while another producer is doing a
                // put. If the other producers have not yet committed
                // then we must wait for them. When cursor is 1 less
                // than _slot we know that we are next to commit.
                //
                return commitCursor.load(std::memory_order_acquire) < slot;
            }
            static const std::memory_order order{std::memory_order_consume};
        };
    }

    template<
        typename Disruptor,
        typename Barrier,
        typename CommitPolicy,
        typename ClaimSpinPolicy=NoOp,
        typename CommitSpinPolicy=NoOp>
    struct Put
    {
        Put(): _slot(claim()) {}
        ~Put(){ commit(_slot); }

        template<typename T>
        Put& operator=(const T& rhs) { *_slot = rhs; return *this; }

        L3_CACHE_LINE static L3::Sequence cursor;

    private:
        using Iterator = typename Disruptor::Iterator;
        const Iterator _slot;
        
        Index claim()
        {
            //
            // Claim a slot. For the single producer we just do...
            //
            Index slot = cursor.fetch_add(1, CommitPolicy::order);
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
            //     _tail > _head - size
            //
            // We spin while this is not the case.
            //
            // To optimise the loop condition precompute _head -
            // size. To ensure that this difference is always positive
            // initialises _head and _tail using size as a baseline -
            // ie as if we have already done one lap around the ring.
            //
            Index wrapAt = slot - Disruptor::size;
            //
            // We only need to know that this relation is satisfied at
            // this point. Since _tail monotonically increases the
            // condition cannot be invalidated by consumers.
            //
            ClaimSpinPolicy sp;
            while(Barrier::least() <= wrapAt)
            {
                sp();
            }
            return slot;
        }

        void commit(Index slot)
        {
            //
            // For multiple producers it's possible that we've claimed a
            // slot while another producer is doing a put. If the other
            // producers have not yet committed then we must wait for
            // them. When cursor is 1 less than _slot we know that we are
            // next to commit.
            //
            CommitSpinPolicy sp;
            CommitPolicy shouldSpin;
            while(shouldSpin(Disruptor::cursor, slot))
            {
                sp();
            }
            //
            // No need to CAS. Only we could have been waiting for
            // this particular cursor value.
            //
            Disruptor::cursor.fetch_add(1, std::memory_order_release);
        }
    };

    template<typename Disruptor,
             typename Barrier,
             typename CommitPolicy,
             typename ClaimSpinPolicy,
             typename CommitSpinPolicy>
    L3_CACHE_LINE L3::Sequence
    Put<Disruptor,
        Barrier,
        CommitPolicy,
        ClaimSpinPolicy,
        CommitSpinPolicy>::cursor{Disruptor::size};
}

#endif
