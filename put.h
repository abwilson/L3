#ifndef PUT_H
#define PUT_H

namespace L3
{
    template<Sequence& writeCursor, size_t size, typename UpStream>
    struct Put
    {
        Put(): _slot(claim()) {}
        ~Put(){ commit(_slot); }
        
        Index claim()
        {
            //
            // Claim a slot. For the single producer we just do...
            //
            Index slot = writeCursor.fetch_add(1, std::memory_order_consume);
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
            Index wrapAt = slot - size;
            //
            // We only need to know that this relation is satisfied at
            // this point. Since _tail monotonically increases the
            // condition cannot be invalidated by consumers.
            //
            while(UpStream::lessThanEqual(wrapAt))
            {
                sp();
            }
            return slot;
        }

        void commit(Index slot, const SpinProbe& sp=SpinProbe())
        {
            //
            // For multiple producers it's possible that we've claimed a
            // slot while another producer is doing a put. If the other
            // producers have not yet committed then we must wait for
            // them. When cursor is 1 less than _slot we know that we are
            // next to commit.
            //
            SpinProbe probe;
            while(waitForCursor(type(), slot))
            {
                sp();
            }                    
            //
            // No need to CAS. Only we could have been waiting for
            // this particular cursor value.
            //
            dstor._cursor.fetch_add(1, std::memory_order_release);
        }
    protected:
        Index _slot;
    };
}

#endif
