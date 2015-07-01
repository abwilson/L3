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
    // Get is committed when the temporary goes out of scope.
    //
    
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
                // Oops - something went wrong. Return here will abort
                // since producer not joined.
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
