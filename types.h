#ifndef TYPES_H
#define TYPES_H

#include <atomic>

namespace L3
{
    struct NoOp { void operator()() const {}; };
    using Index = uint64_t;
//    using Sequence = std::atomic<Index>;
}
#endif
