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
const Msg eos{0};
constexpr size_t log2size = 2;

using D1 = L3::Disruptor<Msg, log2size, 1>;
using D2 = L3::Disruptor<Msg, log2size, 2>;
using DCtrl = L3::Disruptor<Msg, log2size, 3>;

template<size_t tag>
struct Handler
{
    static
    //std::atomic<
        Msg
        //        >
    old;
    void operator()(Msg& m)
    {
        if(old + 2 != m)
        {
            std::cout << "FAIL: tag: " << tag
                      << ", msg: " << m
                      << ", old: " << old
                      << std::endl;
        }
        old = m;
    }
};

template<size_t tag>
//std::atomic<
    Msg
//    >
Handler<tag>::old{tag};

using S1 = L3::Selector<D1::Get<>, Handler<1>,
                        D2::Get<>, Handler<2>>;
//                        DCtrl::Get<>, Controller>;

const Msg loops{10000000};

template<typename Put>
inline void
produce(Msg first, Msg last)
{
    for(Msg i = first; i < last; i += 2)
    {
        Put() = i;
    }
    Put() = eos;
}

int
main()
{
    // D1::Put<>() = 20;
    // D2::Put<>() = 30;
    // S1::select();
    // std::cout << "selected 20, 30" << std::endl;

    // S1::select();
    // std::cout << "selected nowt" << std::endl;

    // D1::Put<>() = 10;
    // S1::select();
    // std::cout << "selected 10" << std::endl;

    // S1::select();
    // std::cout << "selected nowt" << std::endl;

    std::thread p1([]{ produce<D1::Put<>>(3, loops); });
    std::thread p2([]{ produce<D2::Put<>>(4, loops); });
    
    while(Handler<1>::old != eos ||
          Handler<2>::old != eos)
    {
        S1::select();
    }

    for(auto t: {&p1, &p2}) t->join();
}
