# L3 - Low Latency Lib
C++ tools for building low latency systems. Includes a very fast and light implementation of an [LMAX disruptor] (https://lmax-exchange.github.io/disruptor). Code is written in c++11, makes use of `std::atomic` and therefore requires a suitable compiler. This is a header only library. Built and tested with Apple LLVM 5.1 on OS 10.10.3.

Simple example usage:
```C++
#include <L3/disruptor/disruptor.h>

#include <thread>
#include <iostream>

//
// Lets say we're going to use size_t as our message.
//
using Msg = size_t;
//
// Declare alias for a disruptor for this type of message and a ring
// buffer of size 2^19 = 512K messages.
//
using D = L3::Disruptor<Msg, 19>;
//
// Alias Put and Get types using defaults.
//
using Get = D::Get<>;
using Put = D::Put<>;
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
        p = 1;
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
    Put() = 2;
    //
    // To get we do something similar but gets are batched so we need
    // to read in a loop.
    //
    for(Msg m: Get())
    {
        std::cout << "got: " << m << std::endl;
    }
    //
    // We can send messages between two threads like so.
    //
    // 10 Million messages takes less than a second on my old macbook
    // pro.
    //
    const size_t loops = 10000000; 
    //
    // Producer just writes sequencial integers.
    //
    std::thread producer([]{ for(size_t i = 1; i < loops; ++i) Put() = i; });
    //
    // Consumer checks that these are read in the same order.
    //
    Msg previous = 0;
    //
    // Should get the same number of messages we put.
    //
    for(size_t i = 1; i < loops;)
    {
        for(Msg m: Get())
        {
            if(m != previous + 1)
            {
                //
                // Will abort since producer not joined.
                //
                return -1;
            }
            previous = m;
            ++i;
        }
    }
    producer.join();
    return 0;
}
```
See examples directory for how more complex topologies are supported.
