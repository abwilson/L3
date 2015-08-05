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

#ifndef L3_ITERATIONS
#    define L3_ITERATIONS 10000000
#endif 

constexpr size_t iterations {L3_ITERATIONS};

#ifndef L3_QSIZE
#    define L3_QSIZE 20
#endif

template<typename D>
void dbg()
{
    std::cout << "ring: ";
    std::ostream_iterator<typename D::Msg> oi(std::cout, ", ");
    std::copy(D::ring.begin(), D::ring.end(), oi);
    std::cout << std::endl;

    std::cout << "cursor: " << D::cursor << std::endl;
}

struct ThrowSpin
{
    void operator()() const
    {
        throw std::runtime_error("shouldn't spin");
    }
};

namespace testSpins
{
    using D = L3::Disruptor<size_t, 1, L3::Tag<100>>;

    using Get = D::Get<L3::Tag<0>, L3::Barrier<D>, ThrowSpin>;
    using Put = D::Put<L3::Barrier<Get>,
                       L3::CommitPolicy::Unique,
                       ThrowSpin,
                       ThrowSpin>;
    void dbg1()
    {
        dbg<D>();
        std::cout << "writeCursor: " << Put::cursor
                  << ", readCursor: " << Get::cursor
                  << std::endl;
    }

    bool test()
    {
        Put() = 42;
        Put() = 43;
        
        try
        {
            Put() = 44;

//            std::cout << "put spin fail" << std::endl;
            return false;
        }
        catch(const std::exception& ex)
        {
//            std::cout << ex.what() << std::endl;
        }
        auto i = Get().begin();
        if(*i == 42 && *++i != 43)
        {
            return false;
        }
        try
        {
            auto i = Get();
//            std::cout << "get spin fail" << std::endl;
            return false;
        }
        catch(const std::exception& ex)
        {
//            std::cout << ex.what() << std::endl;
        }
        return true;
    }
}

namespace testSpins1to1to1
{
    using D = L3::Disruptor<size_t, 1, L3::Tag<150>>;

    using Get1 = D::Get<L3::Tag<0>, L3::Barrier<D>, ThrowSpin>;
    using Get2 = D::Get<L3::Tag<1>, L3::Barrier<Get1>, ThrowSpin>;

    using Put = D::Put<L3::Barrier<Get2>,
                       L3::CommitPolicy::Unique,
                       ThrowSpin,
                       ThrowSpin>;
    void dbg()
    {
// dbg<D>();
        std::cout << "writeCursor: " << Put::cursor
                  << ", readCursor1: " << Get1::cursor
                  << ", readCursor2: " << Get2::cursor
                  << std::endl;
    }

    bool test()
    {
        Put() = 42;
        Put() = 43;
//        dbg();
        try
        {
            Put() = 44;

//            std::cout << "put spin fail" << std::endl;
            return false;
        }
        catch(const std::exception& ex)
        {
//            std::cout << ex.what() << std::endl;
        }
//        dbg();
        auto i = Get1().begin();
        if(*i == 42 && *++i != 43)
        {
            return false;
        }
        try
        {
            auto i = Get1();
//            std::cout << "get spin fail" << std::endl;
            return false;
        }
        catch(const std::exception& ex)
        {
//            std::cout << ex.what() << std::endl;
        }
        try
        {
            Put() = 44;

//            std::cout << "put spin fail" << std::endl;
            return false;
        }
        catch(const std::exception& ex)
        {
//            std::cout << ex.what() << std::endl;
        }
        i = Get2().begin();
        if(*i == 42 && *++i != 43)
        {
            return false;
        }
//        dbg();
        try
        {
            auto i = Get2();
//            std::cout << "get spin fail" << std::endl;
            return false;
        }
        catch(const std::exception& ex)
        {
//            std::cout << ex.what() << std::endl;
        }
        Put() = 44;
        return true;
    }
}

namespace test1to1
{
    using D = L3::Disruptor<size_t, L3_QSIZE, L3::Tag<200>>;
    //
    // Using defaults.
    //
    using Get = D::Get<>;
    using Put = D::Put<>;

    void dbg1()
    {
//        dbg<D>();
//        std::cout << "writeCursor: " << Put::cursor
                  // << ", readCursor: " << readCursor
                  // << std::endl;
    }
    
    bool test1Thread()
    {
        Put() = 42;
//        dbg1();
        for(auto& m: Get())
        {
//            std::cout << "m: " << m << std::endl;
//            dbg1();
            if(m != 42)
            {
                return false;
            }
        }
//        dbg1();
        Put() = 101;
//        dbg1();
        Put() = 202;
//        dbg1();
        {
            Get g;
//            dbg1();
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
//                    dbg<D>();
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
    using D = L3::Disruptor<size_t, L3_QSIZE, L3::Tag<300>>;

    using Get = D::Get<>;

    using Put = D::Put<L3::Barrier<Get>, L3::CommitPolicy::Shared>;

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
//                dbg<D>();
                
                ++i;
                D::Msg& old = msg & 0x1L ? oldOdd : oldEven;
                if(msg != old + 2)
                {
                    // std::cout << "old: " << old << ", new: " << msg
                    //           << std::endl;
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

template<typename D, typename Get>
bool consume(size_t instance, bool& status)
{
    typename D::Msg previous = 0;
    for(typename D::Msg i = 1; i < iterations;)
    {
        for(auto msg: Get())
        {
            // std::cout << "NOT FAIL: instance: " << instance
            //           << ", previous: " << previous
            //           << ", msg: " << msg
            //           << ", i: " << i
            //           << std::endl;
            
            if(msg != previous + 1)
            {
                std::cout << "FAIL: previous: " << previous
                          << ", msg: " << msg
                          << std::endl;
                return status = false;
            }
            previous = msg;
            i++;
        }
    }
    return status = true;
}

namespace test1to2
{
    using D = L3::Disruptor<size_t, L3_QSIZE, L3::Tag<400>>;

    using Get1 = D::Get<L3::Tag<0>>;
    using Get2 = D::Get<L3::Tag<1>>;
    using Put = D::Put<L3::Barrier<Get1, Get2>, L3::CommitPolicy::Unique>;

    bool
    test()
    {
        std::thread prod(
            [](){ for(D::Msg i = 1; i < iterations; ++i) Put() = i; });

        bool stat1;
        std::thread cons1([&]{consume<D, Get1>(1, stat1);});
        bool stat2;
        consume<D, Get2>(2, stat2);

        prod.join();
        cons1.join();

        return stat1 && stat2;
    }
}


namespace test1to2to1
{
    using D = L3::Disruptor<size_t, L3_QSIZE, L3::Tag<500>>;

    using Get1 = D::Get<L3::Tag<0>, L3::Barrier<D>>;
    using Get2 = D::Get<L3::Tag<1>, L3::Barrier<D>>;
    using Get3 = D::Get<L3::Tag<2>, L3::Barrier<Get1, Get2>>;

    using Put = D::Put<L3::Barrier<Get3>, L3::CommitPolicy::Unique>;

    void dbg()
    {
        // dbg<D>();

        std::cout << "readCursor1: " << Get1::cursor << std::endl;
        std::cout << "readCursor2: " << Get2::cursor << std::endl;
        std::cout << "readCursor3: " << Get3::cursor << std::endl;
    }

    bool
    test1()
    {
//        dbg();
        Put() = 42;
//        dbg();
//        Put() = 44;
//        dbg();

        D::Msg m1 = *Get1().begin();
//        dbg();
        D::Msg m2 = *Get2().begin();
//        dbg();
        D::Msg m3 = *Get3().begin();
//        dbg();

        Put() = 43;
        m1 = *Get1().begin();
        m2 = *Get2().begin();
        m3 = *Get3().begin();

//        dbg();
        Put() = 44;
        m1 = *Get1().begin();
        m2 = *Get2().begin();
        m3 = *Get3().begin();
//        dbg();
        
        std::cout << "m1: " << m1
                  << ", m2: " << m2
                  << ", m3: " << m3
                  << std::endl;

        return m1 == 44 && m2 == 44 && m3 ==44;
    }

    bool
    test2()
    {
        std::thread prod(
            [](){ for(D::Msg i = 1; i < iterations; ++i) Put() = i; });

        bool status1;
        bool status2;
        bool status3;
        std::thread consumer1([&]{consume<D, Get1>(1, status1);});
        std::thread consumer2([&]{consume<D, Get2>(2, status2);});
        consume<D, Get3>(3, status3);
        prod.join();
        consumer1.join();
        consumer2.join();

        return true;
    }
}

int
main()
{
    bool status = true;
    // status &= testSpins::test();
    // std::cerr << "testSpins::test: " << status << std::endl;

    // status &= testSpins1to1to1::test();
    // std::cerr << "testSpins1to1to1::test: " << status << std::endl;

    status &= test1to1::test();
    std::cerr << "test1to1::test: " << status  << std::endl;

    status &= test2to1::test();
    std::cerr << "test2to1::test: " << status << std::endl;

    status &= test1to2::test();
    std::cerr << "test1to2::test: " << status << std::endl;

    status &= test1to2to1::test1();
    std::cerr << "test1to2to1::test1: " << status << std::endl;

    status &= test1to2to1::test2();
    std::cerr << "test1to2to1::test2: " << status << std::endl;

    return status ? 0 : 1;
}

