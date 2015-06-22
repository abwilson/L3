#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "types.h"

#include <atomic>
#include <limits>
#include <algorithm>

namespace L3
{
    struct Sequence: std::atomic<Index>
    {
        Sequence(): std::atomic<Index>{} {}
        
        Sequence(Index i): std::atomic<Index>{i} {}

        bool operator<=(Index idx) const
        {
            return load(std::memory_order_acquire) <= idx;
        }
        bool operator>(Index idx) const { return !(*this <= idx); }
    };

//    using Sequence = std::atomic<Index>;

    template<const Sequence&... elems>
    struct SequenceList
    {
        static constexpr bool lessThanEqual(Index i) { return false; }

        static constexpr Index least() { return std::numeric_limits<Index>::max(); }
    };

    template<const Sequence& car, const Sequence&... cdr>
    struct SequenceList<car, cdr...>: SequenceList<cdr...>
    {
        static constexpr bool lessThanEqual(Index idx)
        {
            return car <= idx ||
                SequenceList<cdr...>::lessThanEqual(idx);
        }

        static constexpr Index least()
        {
            return std::min(
                car.load(std::memory_order_acquire),
                SequenceList<cdr...>::least());
        }
    };

    template<size_t size, const Sequence (&elems)[size]>
    struct SequenceArray
    {
        static bool lessThanEqual(Index idx)
        {
            for(auto& i: elems)
            {
                if(i <= idx)
                {
                    return true;
                }
            }
            return false;
        }
    };
}

#endif
