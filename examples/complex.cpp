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
#include <L3/static/disruptor.h>
#include <L3/static/consume.h>

#include <thread>
#include <iostream>

//
// We're going to set up the following topology.
//
// P1
//   \
//    D1 - C1
//   /  \    \
// P2     C2 - C3/P3 - D2 - C4
//
// Producers P1 and P2 feed into disruptor D1. Consumers C1 and C2 get
// directly from the ring. Consumer C3 follows C1 and C2 and passes
// the messages into the second disruptor D2. These are finally
// consumed by C4.
//
// This might be a complex topology but we're not going to do anything
// realistic with the messages. Still passing numbers around to make
// it easy to test no messages are lost and order is preserved.
//
using Msg = size_t;
//
// Size of ring buffers as power of two. I've chosen this number
// because 2^17 == 128K and 128K * sizeof(Msg) == 1MB.
//
constexpr size_t log2size = 17;
//
// First disruptor handles these messages. Third template parameter is
// an instance tag. It defaults to void. I'm using the Tag utility
// struct but you would most probably use a proper class to
// disambiguate this.
//
using D1 = L3::Disruptor<Msg, log2size, L3::Tag<1>>;
//
// We must define Get types for our consumers first. C1 and C2 follow
// the commit cursor in D1. This is the default. Need to supply
// explicit instance number to C2 but I've give them for both to make
// it explicit.
//
using Get1 = D1::Get<L3::Tag<1>>;
using Get2 = D1::Get<L3::Tag<2>>;
//
// C3 follows C1 and C2.
//
using Get3 = D1::Get<L3::Tag<3>, L3::Barrier<Get1, Get2>>;
//
// Define the Put type shared between P1 and P2. It follows C3. It
// uses a shared commit policy.
//
using Put1 = D1::Put<L3::Barrier<Get3>, L3::CommitPolicy::Shared>;
//
// Now define the second disruptor much like the first.
//
using D2 = L3::Disruptor<Msg, log2size, L3::Tag<2>>;
//
// It is single producer/single consumer which is what the default
// Put/Get templates do.
//
using Get4 = D2::Get<>;
using Put2 = D2::Put<>;
//
// That's it in terms of defining the disruptor. The rest of this
// file is code that uses it.
//
// Going to produce sequences of numbers. P1 odd, P2 even. We reserve
// 0 to as and end of stream message to shut things down.
//
constexpr Msg eos = 0;

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

//
// Consumers make sure sequence of messages is correct.
//
template<typename Get>
inline void
checkSequence()
{
    Msg oldOdd = 1;
    Msg oldEven = 0;

    L3::CheckEOS<Msg, eos> checkEOS(2);
    L3::consume<Get>(
        checkEOS,
        [&](Msg msg)
        {
            //
            // Don't check sequence for eos event.
            //
            if(msg == eos)
            {
                return;
            }
            Msg& old = msg & 0x1L ? oldOdd : oldEven;
            if(msg != old + 2)
            {
                std::cout << "FAIL: old: " << old << ", new: " << msg
                          << std::endl;
            }
            old = msg;
        });
}
//
// C3 acts as a bridge between D1 and D2.
//
inline void bridge()
{
    L3::CheckEOS<Msg, eos> checkEOS(2);
    L3::consume<Get3>(checkEOS, [](Msg msg){ Put2() = msg; });
}

int
main()
{
    //
    // Stick it all together.
    //
    Msg max = 10000000;
    std::thread p1([&]{ produce<Put1>(3, max); });
    std::thread p2([&]{ produce<Put1>(2, max); });
    std::thread c1(checkSequence<Get1>);
    std::thread c2(checkSequence<Get2>);
    std::thread c3(bridge);
    std::thread c4(checkSequence<Get4>);
    //
    // We love c++11.
    //
    for(auto t: {&p1, &p2, &c1, &c2, &c3, &c4})
    {
        t->join();
    }
}
    
