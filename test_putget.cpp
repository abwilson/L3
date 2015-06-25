#include "cacheline.h"
#include "get.h"
#include "put.h"
#include "ring.h"
#include "sequence.h"

#include <thread>
#include <iostream>

//using namespace L3;

#ifndef L3_ITERATIONS
// 100 Million.
#define L3_ITERATIONS (100000000) 
#endif 

constexpr size_t iterations {L3_ITERATIONS};

#ifndef L3_QSIZE
#define L3_QSIZE 19 /* 1/2 MB */
#endif

constexpr size_t qSize {L3_QSIZE};

using Msg = size_t;

using Ring = L3::Ring<Msg, qSize>;
L3_CACHE_LINE Ring ring;
using Iterator = Ring::Iterator<ring>;

L3_CACHE_LINE L3::Sequence commitCursor{Ring::size - 1};
L3_CACHE_LINE L3::Sequence writeCursor{Ring::size};
L3_CACHE_LINE L3::Sequence readCursor{Ring::size};

using Get = L3::Get<readCursor, Iterator, L3::SequenceList<commitCursor>>;

using Put = L3::Put<
    writeCursor, commitCursor,
    Iterator,
    L3::SequenceList<readCursor>,
    L3::CommitPolicy::Unique>;

void
dbg()
{
    // std::cout << "Ring::size: " << Ring::size << std::endl;
    // std::cout << "qSize: " << qSize << std::endl;
    
    std::cout << "commitCursor: " << commitCursor
              << ", writeCursor: " << writeCursor
              << ", readCursor: " << readCursor
              << ", value: " << ring[readCursor]
              << std::endl;
}

bool test1Thread()
{
//    Put()(ring) = 42;
    Put() = 42;
    for(auto& m: Get())
    {
        std::cout << "m: " << m << std::endl;
        if(m != 42)
        {
            return false;
        }
    }
    Put() = 101;
    Put() = 202;
    for(auto& m: Get())
    {
        std::cout << "m: " << m << std::endl;
    }
    return true;
}

bool
test2Threads()
{
    std::thread prod1(
        [](){ for(Msg i = 1; i < iterations; ++i) Put() = i; });

    Msg previous = 0;
    for(Msg i = 1; i < iterations;)
    {
        for(auto& msg: Get())
        {
            i++;
            if(msg != previous + 1)
            {
                std::cout << "previous: " << previous
                          << ", msg: " << msg
                          << std::endl;
                return false;
            }
            previous = msg;
        }
    }
    prod1.join();

    return true;
}

int
main()
{
    bool status = true;
    status &= test1Thread();
    status &= test2Threads();
    return status ? 0 : 1;
}

