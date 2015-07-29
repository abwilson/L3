#ifndef CACHELINE_H
#define CACHELINE_H

#include <cstddef>
#include <type_traits>

#ifndef L3_CACHE_LINE_SIZE
#    define L3_CACHE_LINE_SIZE 64
#endif

namespace L3 // Low Latency Library
{
    static constexpr size_t cache_line_size = L3_CACHE_LINE_SIZE;

#ifndef L3_CACHE_LINE
#    define L3_CACHE_LINE alignas(L3::cache_line_size)
#endif

    template<typename T, bool isClass>
    struct CacheLineImpl;

    template<typename T>
    struct CacheLineImpl<T, true>: T
    {
        using T::operator=;
        // CacheLineImpl<T, true>& operator=(const T& rhs)
        // {
        //     T::operator=(rhs);
        //     return *this;
        // }

        // CacheLineImpl<T, true>& operator=(T&& rhs)
        // {
        //     T::operator=(rhs);
        //     return *this;
        // }
    };

    template<typename T>
    struct CacheLineImpl<T, false>
    {
        T value;

        CacheLineImpl<T, false>& operator=(const T& rhs)
        {
            value = rhs;
            return *this;
        }

        CacheLineImpl<T, false>& operator=(T&& rhs)
        {
            value = rhs;
            return *this;
        }
        
        operator T&() { return value; }
        operator const T&() const { return value; }
    };
    
    template<typename T>
    struct L3_CACHE_LINE CacheLine: CacheLineImpl<T, std::is_class<T>::value>
    {
        CacheLine& operator=(const T& rhs)
        {
            CacheLineImpl<T, std::is_class<T>::value>::operator=(rhs);
            return *this;
        }

        CacheLine& operator=(T&& rhs)
        {
            CacheLineImpl<T, std::is_class<T>::value>::operator=(rhs);
            return *this;
        }
    };
}


#endif
