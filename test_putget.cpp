#include "cacheline.h"
#include "get.h"
#include "put.h"
#include "ring.h"
#include "sequence.h"
#include "disruptor.h"

#include <thread>
#include <iostream>

#ifndef L3_ITERATIONS
#    define L3_ITERATIONS 100000000 //000000
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

template<typename D>
void dbg()
{
    std::cout << "ring: ";
    std::ostream_iterator<typename D::Msg> oi(std::cout, ", ");
    std::copy(D::ring.begin(), D::ring.end(), oi);
    std::cout << std::endl;

    std::cout << "commitCursor: " << D::commitCursor << std::endl;
    std::cout << "writeCursor: " << D::writeCursor << std::endl;
}

struct ThrowSpin
{
//    void operator()() const { }
    void operator()() const
    {
        throw std::runtime_error("shouldn't spin");
    }
};

namespace testSpins
{
    using D = L3::Disruptor<size_t, 1, 100>;

    L3_CACHE_LINE L3::Sequence readCursor{D::Ring::size};

    using Get = D::Get<readCursor, L3::Barrier<D::commitCursor>, ThrowSpin>;

    using Put = D::Put<L3::Barrier<readCursor>,
                       L3::CommitPolicy::Unique,
                       ThrowSpin,
                       ThrowSpin>;
    void dbg1()
    {
        dbg<D>();
        std::cout << "writeCursor: " << D::writeCursor
                  << ", readCursor: " << readCursor
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
    using D = L3::Disruptor<size_t, 1, 150>;

    L3_CACHE_LINE L3::Sequence readCursor1{D::Ring::size};
    L3_CACHE_LINE L3::Sequence readCursor2{D::Ring::size};

    using Get1 = D::Get<readCursor1, L3::Barrier<D::commitCursor>, ThrowSpin>;
    using Get2 = D::Get<readCursor2, L3::Barrier<readCursor1>, ThrowSpin>;

    using Put = D::Put<L3::Barrier<readCursor2>,
                       L3::CommitPolicy::Unique,
                       ThrowSpin,
                       ThrowSpin>;
    void dbg()
    {
// dbg<D>();
        std::cout << "writeCursor: " << D::writeCursor
                  << ", readCursor1: " << readCursor1
                  << ", readCursor2: " << readCursor2
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
    using D = L3::Disruptor<size_t, L3_QSIZE, 200>;

    L3_CACHE_LINE L3::Sequence readCursor{D::Ring::size};

    using Get = D::Get<readCursor, L3::Barrier<D::commitCursor>>;
    using Put = D::Put<L3::Barrier<readCursor>, L3::CommitPolicy::Unique>;

    void dbg1()
    {
//        dbg<D>();
//        std::cout << "writeCursor: " << D::writeCursor
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
    using D = L3::Disruptor<size_t, L3_QSIZE, 300>;

    L3_CACHE_LINE L3::Sequence readCursor{D::Ring::size};

    using Get = D::Get<readCursor, L3::Barrier<D::commitCursor>>;

    using Put = D::Put<L3::Barrier<readCursor>, L3::CommitPolicy::Shared>;

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
//                std::cout << "old: " << old << ", new: " << msg << std::endl;
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

template<typename D, typename Get>
bool consume(bool& status)
{
    typename D::Msg previous = 0;
    for(typename D::Msg i = 1; i < iterations;)
    {
        for(auto msg: Get())
        {
            i++;
            if(msg != previous + 1)
            {
                std::cout << "FAIL: previous: " << previous
                          << ", msg: " << msg
                          << std::endl;
                return status = false;
            }
            previous = msg;
        }
    }
    return status = true;
}

namespace test1to2
{
    using D = L3::Disruptor<size_t, L3_QSIZE, 400>;

    L3_CACHE_LINE L3::Sequence readCursor1{D::Ring::size};
    L3_CACHE_LINE L3::Sequence readCursor2{D::Ring::size};

    using Get1 = D::Get<readCursor1, L3::Barrier<D::commitCursor>>;
    using Get2 = D::Get<readCursor2, L3::Barrier<D::commitCursor>>;

    using Put = D::Put<L3::Barrier<readCursor1, readCursor2>,
                       L3::CommitPolicy::Unique>;

    bool
    test()
    {
        std::thread prod(
            [](){ for(D::Msg i = 1; i < iterations; ++i) Put() = i; });

        bool stat1;
        std::thread cons1([&]{consume<D, Get1>(stat1);});
        bool stat2;
        consume<D, Get2>(stat2);

        prod.join();
        cons1.join();

        return stat1 && stat2;
    }
}


namespace test1to2to1
{
    using D = L3::Disruptor<size_t, L3_QSIZE, 500>;

    L3_CACHE_LINE L3::Sequence readCursor1{D::Ring::size};
    L3_CACHE_LINE L3::Sequence readCursor2{D::Ring::size};
    L3_CACHE_LINE L3::Sequence readCursor3{D::Ring::size};
    
    using Get1 = D::Get<readCursor1, L3::Barrier<D::commitCursor>>;
    using Get2 = D::Get<readCursor2, L3::Barrier<D::commitCursor>>;
    using Get3 = D::Get<readCursor3, L3::Barrier<readCursor1, readCursor2>>;

    using Put = D::Put<L3::Barrier<readCursor3>, L3::CommitPolicy::Unique>;

    void dbg()
    {
        // dbg<D>();

        std::cout << "readCursor1: " << readCursor1 << std::endl;
        std::cout << "readCursor2: " << readCursor2 << std::endl;
        std::cout << "readCursor3: " << readCursor3 << std::endl;
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
        std::thread consumer1([&]{consume<D, Get1>(status1);});
        std::thread consumer2([&]{consume<D, Get2>(status2);});
        consume<D, Get3>(status3);
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

