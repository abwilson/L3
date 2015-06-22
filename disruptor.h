#ifndef DISRUPTOR_H
#define DISRUPTOR_H

#include "ring.h"
#include "cacheline.h"
#include "types.h"
#include "sequence.h"

#include <atomic>

namespace L3 // Low Latency Library
{
//    using Sequence = std::atomic<Index>;

    template<typename T, size_t log2size>
    class DisruptorT: Ring<T, log2size>
    {
        //
        // A valid ring buffer needs the cursor position to be
        // different from the head position. Therefore the minimum
        // size is 2 or 2^1.
        //
        static_assert(log2size > 0, "Minimun ring size is 2");
        //
        // In the implementation of a disruptor there's a window
        // between a slot has been claimed and the cursor advanced
        // where the value must be copied into the ring buffer. If
        // this operation fails and the cursor is not advanced then
        // the whole system will block. To ensure the value copy
        // cannot fail constrain T to being a POD.
        //
        static_assert(std::is_pod<T>::value, "PODs only");

      public:
        using Ring = Ring<T, log2size>;
        using value_type = T;

        using Ring::operator[];

        static constexpr Index size{Ring::size};
        L3_CACHE_LINE Sequence _cursor{size};
    };
}

#endif
