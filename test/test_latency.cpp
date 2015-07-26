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

#include <thread>
#include <iostream>
#include <chrono>

using namespace std::chrono;

struct Msg
{
    //L3_CACHE_LINE
    size_t sequence;
    steady_clock::time_point timestamp;
};

using D = L3::Disruptor<Msg, 19>;
using Get = D::Get<>;
using Put = D::Put<>;

int
main()
{
    //
    // We can send messages between two threads like so.
    //
    // 10 Million messages takes less than a second on my old macbook
    // pro.
    //
    const size_t loops = 100 * 1000 * 1000; 
    //
    // Producer just writes sequencial integers.
    //
    std::thread producer(
        []{
            for(size_t i = 1; i < loops; ++i)
            {
                Put() = Msg{i, steady_clock::now()};
            }
        });
    //
    // Consumer checks that these are read in the same order.
    //
    size_t previous{0};
    uint64_t totalLatency{0};
    //
    // Should get the same number of messages we put.
    //
    for(size_t i = 1; i < loops;)
    {
        for(Msg m: Get())
        {
            if(m.sequence != previous + 1)
            {
                //
                // Oops - something went wrong. Return here will abort
                // since producer not joined.
                //
                return -1;
            }
            steady_clock::time_point end{steady_clock::now()};
            
            totalLatency += duration_cast<nanoseconds>(
                end - m.timestamp).count();

            previous = m.sequence;
            ++i;
        }
    }
    producer.join();
    std::cout << "total latency: " << (totalLatency / 1000000) << "ms"
              << std::endl;

    std::cout << "average latency: " << (totalLatency / loops) << "ns"
              << std::endl;
    
    return 0;
}
