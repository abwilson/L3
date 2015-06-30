#ifndef BARRIER_H
#define BARRIER_H

#include "sequence.h"

namespace L3
{
    template<typename... Elems> struct Barrier;

    template<typename T >
    struct Barrier<T>
    {
        static constexpr Index least()
        {
            return T::cursor.load(std::memory_order_acquire);
        }
    };

    template<typename Head, typename... Tail>
    struct Barrier<Head, Tail...>: Barrier<Tail...>
    {
        static constexpr Index least()
        {
            return std::min(
                Head::cursor.load(std::memory_order_acquire),
                Barrier<Tail...>::least());
        }
    };
}

#endif
