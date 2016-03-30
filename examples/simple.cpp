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
#include <L3/util/scopedtimer.h>

#include <thread>
#include <iostream>
#include <iomanip>
#include <cmath>

#ifndef RING_SIZE
#define RING_SIZE (10)
#endif

using namespace std::chrono;

using Clock = steady_clock;

//
// Our test message.
//
struct Msg
{
    //
    // Write a sequence number to assert that we don't miss or reorder
    // messages. NB we're just doing this for test purposes. The
    // disruptor framework ensures this.
    //
    size_t sequence;
    //
    // We'll also pass a timestamp so we can measure latency.
    //
    Clock::time_point stamp;
};

//
// To gather stats.
//
struct Stat
{
    size_t sequence;
    Clock::time_point start;
    Clock::time_point end;
    Clock::duration latency;
};
//
// Declare alias for a disruptor for this type of message and a ring
// buffer of size 2^19 = 512K messages.
//
using D = L3::Disruptor<Msg, RING_SIZE>;
//
// Alias Put and Get types using defaults.
//
using Get = D::Get<>;
using Put = D::Put<>;

using secs = duration<double>;

//
//
const size_t loops = 1 * std::mega::num;
//
// Store stats here.
//
Stat stats[loops];


template<size_t size>
void
processStats(Stat (&stats)[size], double time)
{
    Clock::duration min{Clock::duration::max()};
    Clock::duration max{0};
    Clock::duration sum{0};

    size_t count = 0;
    for(auto& s: stats)
    {
        //
        // Skip first ring worth of messages as these may have
        // been waiting on the queue for a while.
        //
        if(s.sequence < D::size)
        {
            continue;
        }
        count++;
        s.latency = s.end - s.start;
        if(s.latency < min)
        {
            min = s.latency;
        }
        if(s.latency > max)
        {
            max = s.latency;
        }
        sum += s.latency;
    }
    secs mean = duration_cast<secs>(sum) / count;

//    std::cout << "ringMsgs ringSize throughput messages mean min max std" << std::endl;

    std::cout << (1 << RING_SIZE)
              << " " << sizeof(D::Ring)
              << " " << (loops / time) / std::mega::num
              << " " << size
              << " " << duration_cast<nanoseconds>(mean).count()
              << " " << duration_cast<nanoseconds>(min).count()
              << " " << duration_cast<nanoseconds>(max).count();

    secs deviationSum{0};
    for(auto& s: stats)
    {
        secs tmp = duration_cast<secs>(s.latency - mean);
        deviationSum += secs(tmp.count() * tmp.count());
    }
    secs stdDeviation = secs(std::sqrt(deviationSum.count() / size));
    std::cout << " "
              << duration_cast<nanoseconds>(stdDeviation).count()
              << std::endl;
}


//
// We've now created a very basic disruptor with a single producer and
// consumer. We can use it like this...
//
int
main()
{
    //
    // To do a put we make an instance of the Put class we've just
    // defined.
    //
    {
        //
        // Creating a put claims a slot in the ring.
        //
        Put p;
        //
        // To write a value use assignment.
        //
        p = Msg{1, Clock::now()};
        //
        // When p goes out of scope the put is commited and published
        // to any consumers. It's important to note that once you have
        // started a put it will always be commited. RAII takes care
        // of that.
        //
    }
    //
    // Since unnamed temporaries have the lifetime of the statement
    // they are created in we can shorten the above to:
    //
    Put() = Msg{2, Clock::now()};
    //
    // To get we do something similar but gets are batched so we need
    // to read in a loop.
    //
    for(Msg m: Get())
    {
//        std::cout << "got: " << m << std::endl;
    }
    //
    // Get is committed when the temporary goes out of scope.
    //
    
    //
    // We can send messages between two threads like so.
    //
    // We're going to send sequential integers. That way we can
    // confirm that we didn't loose any messages and they all came in
    // the right order. This is how we consume them.
    //
    std::thread consumer(
        [&]{
            //
            // Consumer checks that these are read in the same order.
            //
            size_t previous = 0;
            
            for(auto s = std::begin(stats); s != std::end(stats);)
            {
                for(Msg m: Get())
                {
                    if(m.sequence != previous + 1)
                    {
                        //
                        // Oops - something went wrong.
                        //
                        throw std::runtime_error("Bad sequence");
                    }
                    previous = m.sequence;
                    *s++ = {m.sequence, m.stamp, Clock::now()};
                }
            }
        });
    //
    // Measure the time.
    //
    L3::ScopedTimer<>::duration elapsedTime{0};
    {
        L3::ScopedTimer<> timer(elapsedTime);
        //
        // Producer just writes sequencial integers. Start it in the
        // current thread.
        //
        for(size_t i = 1; i < loops + 1; ++i) Put() = Msg{i, Clock::now()};
        //
        // Should get the same number of messages we put so this join
        // should work.
        //
        consumer.join();
    }
    processStats(stats, duration_cast<secs>(elapsedTime).count());

    return 0;
}
