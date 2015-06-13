#ifndef PRODUCER_H
#define PRODUCER_H

#include "types.h"
#include "cacheline.h"
#include "disruptor.h"
#include "consumer.h"

namespace L3
{
    namespace ProducerType
    {
        struct Single {};
        struct Multi {};
    };

    template<typename ProducerType> struct ProducerTraits;

    template<>
    struct ProducerTraits<ProducerType::Single>
    {
        using HeadType = Index;
    };

    template<>
    struct ProducerTraits<ProducerType::Multi>
    {
        using HeadType = std::atomic<Index>;
    };

    template<typename ProducerType>
    using HeadType = typename ProducerTraits<ProducerType>::HeadType;

    template<typename Dstor,
             Dstor& dstor,
             typename ConsumerList,
             typename type = ProducerType::Single>
    class ProducerT
    {
        //
        // For when there is only a single thread putting.
        //
        // _head is only ever accessed by the single producing thread
        // and therefor does not need any synchronisation.
        //
        L3_CACHE_LINE HeadType<type> _head {Dstor::size + 1};

        Index incHead(ProducerType::Single)
        {
            //
            // For the single threaded producer no need to synchronise so
            // just inc.
            //
            return _head++;
        }

        void commit(ProducerType::Single, Index _slot)
        {
            //
            // To commit a put we advance the cursor to show that slot
            // is now available for reading. For single producer
            // increment doesn't need to be atomic but store must be
            // synchronised.
            //
            dstor._cursor.fetch_add(1, std::memory_order_relaxed);
            dstor._cursor.store(std::memory_order_release);
        }

        template<typename SpinProbe>
        Index incHead(ProducerType::Multi, SpinProbe&)
        {
            //
            // Multi-threaded producer must use atomic increment.
            //
            return _head.fetch_add(1, std::memory_order_acquire);
        }

        template<typename SpinProbe>
        void commit(ProducerType::Multi, Index slot, SpinProbe& sp)
        {
            //
            // For multiple producers it's possible that we've claimed a
            // slot while another producer is doing a put. If the other
            // producers have not yet committed then we must wait for
            // them. When cursor is 1 less than _slot we know that we are
            // next to commit.
            //
            SpinProbe probe;
            while(dstor._cursor.load(std::memory_order_acquire) < slot - 1)
            {
                sp();
            }                    
            //
            // No need to CAS. Only we could have been waiting for
            // this particular cursor value.
            //
            dstor._cursor.fetch_add(1, std::memory_order_release);
        }

    public:
        template<typename SpinProbe=NoOp>
        Index claim(SpinProbe& sp=SpinProbe())
        {
            //
            // Claim a slot. For the single producer we just do...
            //
            Index slot = incHead(type());
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
            Index wrapAt = slot - Dstor::size;
            //
            // We only need to know that this relation is satisfied at
            // this point. Since _tail monotonically increases the
            // condition cannot be invalidated by consumers.
            //
            while(ConsumerList::atLeast(wrapAt))
            {
                sp();
            }
            return slot;
        }

        template<typename SpinProbe=NoOp>
        void commit(Index slot, SpinProbe& sp=SpinProbe())
        {
            commit(type(), slot, sp);
        }

        template<typename ClaimProbe=NoOp, typename CommitProbe=NoOp>
        class Put
        {
            ProducerT& _p;
            Index _slot;
        public:
            using value_type = typename Dstor::value_type;
            
            Put(ProducerT& p): _p(p), _slot(p.claimHead(ClaimProbe())) {}
            ~Put() { _p.commitHead(_slot, CommitProbe()); }

            value_type& operator=(const value_type& rhs) const
            { return dstor._ring[_slot] = rhs; }

            operator value_type&() const
            { return dstor.ring[_slot]; }
        };
        
    };

    // Sequence s;
    // Producer<10, ConsumerList<s>> sp;
    // void h()
    // {
    //     sp.commit(sp.claim());
    // }

    // Producer<10, ConsumerList<s>, ProducerType::Multi> mp;
    // void i()
    // {
    //     mp.commit(mp.claim());
    // }
}

#endif
