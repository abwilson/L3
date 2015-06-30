#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <L3/util/types.h>

#include <atomic>
#include <algorithm>
#include <iostream>

namespace L3
{
    template<typename, size_t, typename, typename> struct Get;
    template<typename, typename, typename, typename, typename> struct Put;
    template<typename...> struct Barrier;

    namespace CommitPolicy { struct Shared; }
    
    class Sequence: std::atomic<Index>
    {
        template<typename, size_t, typename, typename>
        friend struct Get;
        template<typename, typename, typename, typename, typename>
        friend struct Put;
        template<typename...>
        friend struct Barrier;

        friend struct CommitPolicy::Shared;

    public:
        using std::atomic<Index>::operator Index;        
        Sequence(): std::atomic<Index>{} {}
        Sequence(Index i): std::atomic<Index>{i} {}
    };

}

#endif
