#ifndef COUNTER_H
#define COUNTER_H

#include <cstddef>

namespace L3
{
    template<size_t& counter>
    struct Counter
    {
        void operator()() const { ++counter; }
    };
}

#endif
