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
#include <L3/disruptor/disruptor.h>
#include <L3/disruptor/selector.h>
#include <L3/disruptor/consume.h>

#include <iostream>
#include <thread>

using Msg = size_t;
constexpr size_t log2size = 1;

using D1 = L3::Disruptor<Msg, log2size, 1>;
using D2 = L3::Disruptor<Msg, log2size, 2>;

template<typename D>
struct L3_CACHE_LINE MsgTestSequence
{
    using Put = typename D::template Put<>;
    using Get = typename D::template Get<>;
    
    const Msg eos{0};
    const Msg first;
    const Msg last;
    const Msg increment;
    const size_t tag;
    Msg previous;
    
    MsgTestSequence(Msg f, Msg l, Msg i, size_t t):
        first{f},
        last{l},
        increment{i},
        tag{t},
        previous{f - increment}
    {}

    void put(Msg i)
    {
        Put() = i;
    }

    void produce()
    {
        for(Msg i = first; i < last; i += increment)
        {
            Put() = i;
        }
        Put() = eos;
    }

    void operator()(Msg& m)
    {
        if(m != eos && previous + increment != m)
        {
            std::cout << "FAIL: tag: " << tag
                      << ", msg: " << m
                      << ", previous: " << previous
                      << std::endl;
            abort();
        }
        previous = m;
    }

    bool done() const { return previous == eos; }
};

const Msg loops{10};// * 1000};// * 1000};

int
main()
{
    MsgTestSequence<D1> odd{3, loops, 2, 1};
    MsgTestSequence<D2> even{4, loops, 2, 2};
#if 0
    std::cout << D1::ring << std::endl;
    std::cout << D2::ring << std::endl;
    odd.put(1);
    even.put(2);

    std::cout << D1::ring << std::endl;
    std::cout << D2::ring << std::endl;

    L3::select(1, even, odd);
        
    std::cout << D1::ring << std::endl;
    std::cout << D2::ring << std::endl;
    odd.put(3);
    even.put(4);

    std::cout << D1::ring << std::endl;
    std::cout << D2::ring << std::endl;

    L3::select(1, odd, even);
        
    std::cout << D1::ring << std::endl;
    std::cout << D2::ring << std::endl;
    
    odd.put(3);
    even.put(4);

    std::cout << D1::ring << std::endl;
    std::cout << D2::ring << std::endl;

    L3::select(1, odd, even);
        
    std::cout << D1::ring << std::endl;
    std::cout << D2::ring << std::endl;

    return 0;
#endif    
    std::thread p2([&]{ even.produce(); });
    std::thread p1([&]{ odd.produce(); });
    
    while(!(odd.done() && even.done()))
    {
        std::cout << D1::ring << std::endl;
        std::cout << D2::ring << std::endl;
        L3::select(1, even, odd);
    }
    for(auto t: {&p1, &p2}) t->join();
}
