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
a selector combining the streams. This version does the former. Time for 

100,000,000 iterations

real	0m15.527s
user	0m29.391s
sys	0m0.113s

This is about 3x the time of the selector version.

*/
#include <L3/disruptor/disruptor.h>
#include <L3/disruptor/consume.h>

#include <iostream>
#include <thread>

using Msg = size_t;
using D = L3::Disruptor<Msg, 17>;
using Get = D::Get<>;
using Put = D::Put<L3::Barrier<Get>, L3::CommitPolicy::Shared>;

int
main()
{
    struct Producer
    {
        Msg begin;
        Msg end;
        void operator()() { for(Msg i = begin; i < end; i += 2) Put() = i; }
    };

    Msg iterations{100000000};

    std::thread p1(Producer{3, iterations});
    std::thread p2(Producer{2, iterations});

    D::Msg oldOdd = 1;
    D::Msg oldEven = 0;
    for(size_t i = 2; i < iterations;)
    {
        for(auto msg: Get())
        {
            ++i;
            D::Msg& old = msg & 0x1L ? oldOdd : oldEven;
            if(msg != old + 2)
            {
                std::cout << "old: " << old << ", new: " << msg
                          << std::endl;
                return -1;
            }
            old = msg;
        }
    }
    std::cout << "Done" << std::endl;
    p1.join();
    p2.join();
    return 0;
}


