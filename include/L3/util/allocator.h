#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "ring.h"

#include <atomic>

namespace L3
{
    template<typename T>
    struct Slot
    {
        alignas(T) char _space[sizeof(T)];
        std::atomic<bool> _inUse;
    };
    
    template<typename T, char log2size>
    struct Allocator
    {
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using const_pointer = const T*;
        using const_reference = const T&;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        pointer allocate(size_type n, const_pointer hint=0)
        {
            size_t result;
            //
            // This could spin forever if allocator is full.
            //
            while(_ring[result = _nextFree++]._inUse);
            return reinterpret_cast<pointer>(&_ring[result]._space);
        }

        Ring<Slot<T>, log2size> _ring;
        using Idx = Ring<Slot<T>, log2size>::Idx<_ring>;
        std::atomic<typename Ring<Slot<T>, log2size>::Index> _nextFree;
    };
}

#endif
