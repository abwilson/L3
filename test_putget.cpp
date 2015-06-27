#include "cacheline.h"
#include "get.h"
#include "put.h"
#include "ring.h"
#include "sequence.h"
#include "disruptor.h"

#include <thread>
#include <iostream>

#ifndef L3_ITERATIONS
#    define L3_ITERATIONS 100000000
#endif 

constexpr size_t iterations {L3_ITERATIONS};

#ifndef L3_QSIZE
#    define L3_QSIZE 20
#endif

#if 0
void
dbg()
{
    std::cout << "commitCursor: " << D::commitCursor
              << ", writeCursor: " << writeCursor
              << ", readCursor: " << readCursor
              << ", value: " << D::ring[readCursor]
              << std::endl;
}
#endif

namespace test1to1
{
    using D = L3::Disruptor<size_t, L3_QSIZE, 1>;

    L3_CACHE_LINE L3::Sequence writeCursor{D::commitCursor + 1};
    L3_CACHE_LINE L3::Sequence readCursor{D::commitCursor + 1};

    using Get = L3::Get<readCursor,
                        D::Iterator,
                        L3::SequenceList<D::commitCursor>>;

    using Put = L3::Put<writeCursor,
                        D::commitCursor,
                        D::Iterator,
                        L3::SequenceList<readCursor>,
                        L3::CommitPolicy::Unique>;

    bool test1Thread()
    {
        Put() = 42;
        for(auto& m: Get())
        {
            if(m != 42)
            {
                return false;
            }
        }
        Put() = 101;
        Put() = 202;
        {
            Get g;
            D::Iterator b(g.begin());
            D::Iterator e(g.end());

            D::Msg m1 = *b++;
            D::Msg m2 = *b++;
            
            // std::cout << "m1: " << m1 << ", m2: " << m2 << std::endl;
            // std::cout << "b: " << b << ", e: " << e << std::endl;

            return m1 == 101 && m2 == 202 && b == e;
        }
    }

    bool
    test2Threads()
    {
        std::thread prod(
            [](){ for(D::Msg i = 1; i < iterations; ++i) Put() = i; });

        D::Msg previous = 0;
        for(D::Msg i = 1; i < iterations;)
        {
            for(auto msg: Get())
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
        prod.join();

        return true;
    }

    bool test()
    {
        bool result = true;
        result &= test1Thread();
        result &= test2Threads();
        return result;
    }
}

namespace test2to1
{
    using D = L3::Disruptor<size_t, L3_QSIZE, 1>;

    L3_CACHE_LINE L3::Sequence writeCursor{D::commitCursor + 1};
    L3_CACHE_LINE L3::Sequence readCursor{D::commitCursor + 1};

    using Get = L3::Get<readCursor,
                        D::Iterator,
                        L3::SequenceList<D::commitCursor>>;

    using Put = L3::Put<writeCursor,
                        D::commitCursor,
                        D::Iterator,
                        L3::SequenceList<readCursor>,
                        L3::CommitPolicy::Shared>;

    bool test3Threads()
    {
        std::atomic<bool> go { false };
        std::thread producer1(
            [&](){
                while(!go);
                for(D::Msg i = 3; i < iterations; i += 2)
                {
                    Put() = i;
                }
            });

        std::thread producer2(
            [&](){
                while(!go);
                for(D::Msg i = 2; i < iterations; i += 2)
                {
                    Put() = i;
                }
            });

        go = true;
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
                    return false;
                }
                old = msg;
            }
        }
        std::cout << "Cons done" << std::endl;
        producer1.join();
        producer2.join();
        return true;
    }
    bool test()
    {
        return test3Threads();
    }
}

namespace test1to2
{
    using D = L3::Disruptor<size_t, L3_QSIZE, 1>;

    L3_CACHE_LINE L3::Sequence writeCursor{D::commitCursor + 1};
    L3_CACHE_LINE L3::Sequence readCursor1{D::commitCursor + 1};
    L3_CACHE_LINE L3::Sequence readCursor2{D::commitCursor + 1};

    using Get1 = L3::Get<readCursor1,
                         D::Iterator,
                         L3::SequenceList<D::commitCursor>>;

    using Get2 = L3::Get<readCursor2,
                         D::Iterator,
                         L3::SequenceList<D::commitCursor>>;

    using Put = L3::Put<writeCursor,
                        D::commitCursor,
                        D::Iterator,
                        L3::SequenceList<readCursor1, readCursor2>,
                        L3::CommitPolicy::Unique>;

    template<typename Get>
    bool cons()
    {
        D::Msg previous = 0;
        for(D::Msg i = 1; i < iterations;)
        {
            for(auto msg: Get())
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
        return true;
    }

    bool
    test()
    {
        std::thread prod(
            [](){ for(D::Msg i = 1; i < iterations; ++i) Put() = i; });

        std::thread cons1(cons<Get1>);
        cons<Get2>();
        prod.join();
        cons1.join();

        return true;
    }
}

namespace test1to2to1
{
    using D = L3::Disruptor<size_t, L3_QSIZE, 1>;

    L3_CACHE_LINE L3::Sequence writeCursor{D::commitCursor + 1};
    L3_CACHE_LINE L3::Sequence readCursor1{D::commitCursor + 1};
    L3_CACHE_LINE L3::Sequence readCursor2{D::commitCursor + 1};
    L3_CACHE_LINE L3::Sequence readCursor3{readCursor1 + 1};
    
    using Get1 = L3::Get<readCursor1,
                         D::Iterator,
                         L3::SequenceList<D::commitCursor>>;

    using Get2 = L3::Get<readCursor2,
                         D::Iterator,
                         L3::SequenceList<D::commitCursor>>;

    using Get3 = L3::Get<readCursor3,
                         D::Iterator,
                         L3::SequenceList<readCursor1, readCursor2>>;

    using Put = L3::Put<writeCursor,
                        D::commitCursor,
                        D::Iterator,
                        L3::SequenceList<readCursor3>,
                        L3::CommitPolicy::Unique>;

    template<typename Get>
    bool cons()
    {
        D::Msg previous = 0;
        for(D::Msg i = 1; i < iterations;)
        {
            for(auto msg: Get())
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
        return true;
    }

    bool
    test()
    {
        std::thread prod(
            [](){ for(D::Msg i = 1; i < iterations; ++i) Put() = i; });

        std::thread cons1(cons<Get1>);
        std::thread cons2(cons<Get2>);
        cons<Get3>();
        prod.join();
        cons1.join();
        cons2.join();

        return true;
    }
}

int
main()
{
    bool status = true;
//    status &= test1to1::test();
    // status &= test1Thread();
    // status &= test2Threads();
//    status &= test2to1::test();
    status &= test1to2::test();
    
    return status ? 0 : 1;
}

