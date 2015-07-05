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

This is comparing performance two threads feeding a single consumer
implemented as either a shared put or as two separate disruptors with
a selector combining the streams. This version does the latter. Time for 

100,000,000 iterations

real	0m5.734s
user	0m10.688s
sys	0m0.048s

This is about 1/3 the time of the shared put version.

*/
#include <L3/disruptor/disruptor.h>
#include <L3/disruptor/selector.h>
#include <L3/disruptor/consume.h>

#include <iostream>
#include <thread>

using Msg = size_t;

//
// Note the shared put version uses 2^17 size ring. To compare like
// for like since this test has two rings each is size 2^16.
//
using D1 = L3::Disruptor<Msg, 16, 1>;
using Put1 = D1::Put<>;

using D2 = L3::Disruptor<Msg, 16, 2>;
using Put2 = D2::Put<>;

const Msg eos{0};

struct Handler
{
    static Msg oldOdd;
    static Msg oldEven;
    static Msg eosRemaining;

    void operator()(Msg& m)
    {
        Msg& old = m & 0x1L ? oldOdd : oldEven;
        if(m == eos)
        {
            --eosRemaining;
            return;
        }
        if(m != old + 2)
        {
            std::cout << "old: " << old << ", new: " << m
                      << std::endl;
        }
        old = m;
    }
};

Msg Handler::oldOdd {1};
Msg Handler::oldEven{2};
Msg Handler::eosRemaining{2};

using S1 = L3::Selector<D1::Get<>, Handler,
                        D2::Get<>, Handler>;

template<class Put>
struct Producer
{
    Msg begin;
    Msg end;
    void operator()()
    {
        for(Msg i = begin; i < end; i += 2) Put() = i;
        Put() = eos;
    }
};

int
main()
{
    Msg iterations{100000000};

    std::thread p1(Producer<Put1>{3, iterations});
    std::thread p2(Producer<Put2>{4, iterations});

    while(Handler::eosRemaining)
    {
        S1::select();
    }

    std::cout << "Done" << std::endl;
    p1.join();
    p2.join();
    return 0;
}
