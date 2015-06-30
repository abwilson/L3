#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "types.h"

#include <atomic>
#include <algorithm>
#include <iostream>

namespace L3
{
    class Sequence;    
    template<class, size_t, class, class> struct Get;
    template<class, class, class, class, class> struct Put;
    template<typename...> struct Barrier;

    namespace CommitPolicy { struct Shared; }
    
    class Sequence: std::atomic<Index>
    {
        template<class, size_t, class, class> friend struct Get;
        template<class, class, class, class, class> friend struct Put;
        template<typename...> friend struct Barrier;

        friend struct CommitPolicy::Shared;

    public:
        using std::atomic<Index>::operator Index;        
        Sequence(): std::atomic<Index>{} {}
        Sequence(Index i): std::atomic<Index>{i} {}
    };

}

#endif
