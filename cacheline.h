#ifndef CACHELINE_H
#define CACHELINE_H

#include <cstddef>

#ifndef L3_CACHE_LINE_SIZE
#    define L3_CACHE_LINE_SIZE 64
#endif

namespace L3 // Low Latency Library
{
    static constexpr size_t cache_line_size = L3_CACHE_LINE_SIZE;
}

#ifndef L3_CACHE_LINE
#    define L3_CACHE_LINE alignas(cache_line_size)
#endif

#endif
