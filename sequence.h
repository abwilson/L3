#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "types.h"

#include <atomic>
//#include <limits>
#include <algorithm>
#include <iostream>

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

    template<const Sequence&... elems>
    struct SequenceList
    {
        static constexpr bool lessThanEqual(Index i) { return false; }
        static constexpr Index least();
    };

    template<const Sequence& _1, const Sequence&... tail>
    struct SequenceList<_1, tail...>: SequenceList<tail...>
    {
        static constexpr bool lessThanEqual(Index idx)
        {
            return _1 <= idx || SequenceList<tail...>::lessThanEqual(idx);
        }

        static constexpr Index least()
        {
            return _1.load(std::memory_order_acquire);
        }
    };

    template<const Sequence& _1, const Sequence& _2, const Sequence&... tail>
    struct SequenceList<_1, _2, tail...>: SequenceList<_2, tail...>
    {
        static constexpr bool lessThanEqual(Index idx)
        {
            return _1 <= idx || SequenceList<_2, tail...>::lessThanEqual(idx);
        }

        static constexpr Index least()
        {
            return std::min(
                _1.load(std::memory_order_acquire),
                SequenceList<_2, tail...>::least());
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
