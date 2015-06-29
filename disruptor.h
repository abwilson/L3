#ifndef DISRUPTOR_H
#define DISRUPTOR_H

#include "ring.h"
#include "cacheline.h"
#include "sequence.h"
#include "get.h"

namespace L3 // Low Latency Library
{
    template<typename T, size_t s, size_t tag = 0>
    struct Disruptor
    {
        using Msg = T;
        static const size_t size = s;
        using Ring = L3::Ring<Msg, size>;
        L3_CACHE_LINE static Ring ring;
        using Iterator = typename Ring::template Iterator<ring>;

        L3_CACHE_LINE static L3::Sequence commitCursor;
        L3_CACHE_LINE static L3::Sequence writeCursor;

        template<typename UpStream,
                 typename CommitPolicy,
                 typename ClaimSpinPolicy=NoOp,
                 typename CommitSpinPolicy=NoOp>
        using Put = L3::Put<writeCursor,
                            commitCursor,
                            Iterator,
                            UpStream,
                            CommitPolicy,
                            ClaimSpinPolicy,
                            CommitSpinPolicy>;

        template<Sequence& readCursor,
                 typename UpStream,
                 typename SpinPolicy=NoOp>
        using Get = Get<readCursor, Iterator, UpStream, SpinPolicy>;
    };

    template<typename T, size_t s, size_t t>
    L3_CACHE_LINE typename Disruptor<T, s, t>::Ring
    Disruptor<T, s, t>::ring;

    template<typename T, size_t s, size_t t>
    L3_CACHE_LINE L3::Sequence
    Disruptor<T, s, t>::commitCursor{Disruptor<T, s, t>::Ring::size};

    template<typename T, size_t s, size_t t>
    L3_CACHE_LINE L3::Sequence
    Disruptor<T, s, t>::writeCursor{Disruptor<T, s, t>::Ring::size};
}

#endif
