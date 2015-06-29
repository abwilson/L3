#ifndef BARRIER_H
#define BARRIER_H

#include "sequence.h"

namespace L3
{
    template<const Sequence&... elems> struct Barrier;

    template<const Sequence& s>
    struct Barrier<s>
    {
        static constexpr Index least()
        {
            return s.load(std::memory_order_acquire);
        }
    };

    template<const Sequence& head, const Sequence&... tail>
    struct Barrier<head, tail...>: Barrier<tail...>
    {
        static constexpr Index least()
        {
            return std::min(
                head.load(std::memory_order_acquire),
                Barrier<tail...>::least());
        }
    };
}

#endif
