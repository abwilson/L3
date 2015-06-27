#ifndef DISRUPTOR_H
#define DISRUPTOR_H

#include "ring.h"
#include "cacheline.h"
#include "sequence.h"

namespace L3 // Low Latency Library
{
    template<typename T, size_t size, size_t tag>
    struct Disruptor
    {
        using Msg = T;
        using Ring = L3::Ring<Msg, size>;
        L3_CACHE_LINE static Ring ring;
        using Iterator = typename Ring::template Iterator<ring>;

        L3_CACHE_LINE static L3::Sequence commitCursor;
    };

    template<typename T, size_t s, size_t t>
    L3_CACHE_LINE typename Disruptor<T, s, t>::Ring
    Disruptor<T, s, t>::ring;

    template<typename T, size_t s, size_t t>
    L3_CACHE_LINE L3::Sequence
    Disruptor<T, s, t>::commitCursor{Disruptor<T, s, t>::Ring::size};
}

#endif
