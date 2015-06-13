#ifndef TYPES_H
#define TYPES_H

#include <atomic>

namespace L3
{
    struct NoOp { void operator()() {}; };
    using Index = uint64_t;
    using Sequence = std::atomic<Index>;
}

#endif
