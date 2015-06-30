#ifndef DISRUPTOR_H
#define DISRUPTOR_H

#include "ring.h"
#include "barrier.h"
#include "cacheline.h"
#include "sequence.h"
#include "get.h"

namespace L3 // Low Latency Library
{
    template<typename T, size_t s, size_t t=0>
    struct Disruptor
    {
        using DISRUPTOR = Disruptor<T, s, t>;
        using Msg = T;
        using Ring = L3::Ring<Msg, s>;
        static const size_t size = Ring::size;
        L3_CACHE_LINE static Ring ring;
        
        using Iterator = typename Ring::template Iterator<ring>;

        L3_CACHE_LINE static L3::Sequence cursor;

        template<typename Barrier,
                 typename CommitPolicy,
                 typename ClaimSpinPolicy=NoOp,
                 typename CommitSpinPolicy=NoOp>
        using Put = L3::Put<DISRUPTOR,
                            Barrier,
                            CommitPolicy,
                            ClaimSpinPolicy,
                            CommitSpinPolicy>;

        template<size_t tag=0,
                 typename BARRIER=Barrier<DISRUPTOR>,
                 typename SpinPolicy=NoOp>
        using Get = Get<DISRUPTOR, tag, BARRIER, SpinPolicy>;
    };

    template<typename T, size_t s, size_t t>
    L3_CACHE_LINE typename Disruptor<T, s, t>::Ring
    Disruptor<T, s, t>::ring;

    template<typename T, size_t s, size_t t>
    L3_CACHE_LINE L3::Sequence
    Disruptor<T, s, t>::cursor{Disruptor<T, s, t>::Ring::size};
}

#endif
