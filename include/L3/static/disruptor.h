#ifndef DISRUPTOR_H
#define DISRUPTOR_H
/*
The MIT License (MIT)

Copyright (c) 2015 Norman Wilson - Volcano Consultancy Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "barrier.h"
#include "get.h"
#include "put.h"
#include "sequence.h"

#include <L3/util/cacheline.h>
#include <L3/util/ring.h>

namespace L3 // Low Latency Library
{
    template<typename T, size_t s, typename TAG=void>
    struct Disruptor
    {
        using DISRUPTOR = Disruptor<T, s, TAG>;
        using Msg = T;
        using Tag = TAG;
        using Ring = L3::Ring<CacheLine<Msg>, s>;
        static const size_t size = Ring::size;
        L3_CACHE_LINE static Ring ring;
        
        using Iterator = typename Ring::template StaticIterator<ring>;

        L3_CACHE_LINE static L3::Sequence cursor;

        template<typename Tag=void,
                 typename BARRIER=Barrier<DISRUPTOR>,
                 typename SpinPolicy=NoOp>
        using Get = Get<DISRUPTOR, Tag, BARRIER, SpinPolicy>;

        template<typename BARRIER=Barrier<Get<>>,
                 typename COMMITPOLICY=CommitPolicy::Unique,
                 typename ClaimSpinPolicy=NoOp,
                 typename CommitSpinPolicy=NoOp>
        using Put = L3::Put<DISRUPTOR,
                            BARRIER,
                            COMMITPOLICY,
                            ClaimSpinPolicy,
                            CommitSpinPolicy>;
    };

    template<typename T, size_t s, typename Tag>
    L3_CACHE_LINE typename Disruptor<T, s, Tag>::Ring
    Disruptor<T, s, Tag>::ring;

    template<typename T, size_t s, typename Tag>
    L3_CACHE_LINE L3::Sequence
    Disruptor<T, s, Tag>::cursor{Disruptor<T, s, Tag>::Ring::size};
}

#endif
