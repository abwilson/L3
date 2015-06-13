#ifndef CONSUMER_H
#define CONSUMER_H

#include "disruptor.h"

namespace L3
{
    template<typename Dstor, Dstor& dstor>
    struct ConsumerT
    {
        Sequence _tail{Dstor::size};

        constexpr ConsumerT(){}
    };

    template<typename C, C&... tail>
    struct ConsumerList
    {
        static bool atLeast(Index i) { return true; }
    };
    
    template<typename C, C& c, C&... tail>
    struct ConsumerList<C, c, tail...>: ConsumerList<C, tail...>
    {
        static bool atLeast(Index i)
        {
            return c._tail.load(std::memory_order_acquire) >= i &&
                ConsumerList<C, tail...>::atLeast(i);
        }
    };
}

#endif
