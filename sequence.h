#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "types.h"

#include <atomic>
#include <algorithm>
#include <iostream>

namespace L3
{
    class Sequence;    
    template<Sequence&, class, class, class> struct Get;

    template<Sequence&, Sequence&, class, class, class, class, class>
    struct Put;

    template<const Sequence&...> struct Barrier;

    namespace CommitPolicy { struct Shared; }
    
    class Sequence: std::atomic<Index>
    {
        template<Sequence&, class, class, class> friend struct Get;
        template<Sequence&, Sequence&, class, class, class, class, class>
        friend struct Put;
        template<const Sequence&...> friend struct Barrier;

        friend struct CommitPolicy::Shared;

    public:
        using std::atomic<Index>::operator Index;        
        Sequence(): std::atomic<Index>{} {}
        Sequence(Index i): std::atomic<Index>{i} {}
    };

}

#endif
