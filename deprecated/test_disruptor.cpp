#include "consumer.h"
#include "disruptor.h"
#include "producer.h"
#include "sequence.h"
#include "util.h"

#include <array>
#include <iostream>
#include <thread>

using namespace L3;
typedef size_t Msg;

#ifndef L3_ITERATIONS
// 100 Million.
#define L3_ITERATIONS (100000000) 
#endif 

constexpr size_t iterations {L3_ITERATIONS};

#ifndef L3_QSIZE
#define L3_QSIZE 19
#endif

constexpr size_t qSize {L3_QSIZE};

template<typename Type=ProducerType::Single, unsigned consumerInstances=1>
struct TestCase
{
    using Disruptor = DisruptorT<Msg, qSize>;
    L3_CACHE_LINE static Disruptor disruptor;

//    static SequenceList<disruptor._cursor> sl;

    L3_CACHE_LINE static size_t cursorSpinCount;
    L3_CACHE_LINE static size_t putSpinCount;
    L3_CACHE_LINE static size_t getSpinCount;

    using CursorSpinCounter = Counter<cursorSpinCount>;
    using PutSpinCounter = Counter<putSpinCount>;
    using GetSpinCounter = Counter<getSpinCount>;

    using Consumer = ConsumerT<Disruptor, disruptor>;

    static Consumer consumers[consumerInstances];

    using CArray = ConsumerArray<Consumer, consumerInstances, consumers>;

    using Producer = ProducerT<
        Disruptor, disruptor,
        CArray,
        Type>;
    static Producer producer;

    using PUT = typename Producer::template Put<PutSpinCounter,
                                                CursorSpinCounter>;
    
    using GET = typename Consumer::template Get<GetSpinCounter>;
};

template<typename Type, unsigned consumerInstances>
L3_CACHE_LINE typename TestCase<Type, consumerInstances>::Disruptor
TestCase<Type, consumerInstances>::disruptor;

template<typename Type, unsigned consumerInstances>
L3_CACHE_LINE size_t TestCase<Type, consumerInstances>::cursorSpinCount;

template<typename Type, unsigned consumerInstances>

L3_CACHE_LINE size_t TestCase<Type, consumerInstances>::putSpinCount;

template<typename Type, unsigned consumerInstances>
L3_CACHE_LINE size_t TestCase<Type, consumerInstances>::getSpinCount;

template<typename Type, unsigned consumerInstances>
typename TestCase<Type, consumerInstances>::Consumer
TestCase<Type, consumerInstances>::consumers[consumerInstances];

template<typename Type, unsigned consumerInstances>
typename TestCase<Type, consumerInstances>::Producer
TestCase<Type, consumerInstances>::producer;


namespace test1to1
{
    using TC = TestCase<ProducerType::Single, 1>;

    std::array<Msg, iterations> msgs;

    bool run()
    {
        int result = 0;

//    std::atomic<bool> go { false };

        {
            TC::PUT p(TC::producer);
            p = 42;
        }
        {
            TC::GET g(TC::consumers[0]);
            Msg m = g;
            std::cout << "Got: " << m << std::endl;
        }
        {
            TC::PUT p(TC::producer);
            p = 48;
        }
        {
            TC::GET g(TC::consumers[0]);
            Msg m = g;
            std::cout << "Got: " << m << std::endl;
        }

        std::thread prod1(
            [&](){
//            while(!go);
                for(Msg i = 1; i < iterations; i++)
                {
                    TC::PUT p(TC::producer);
                    p = i;
                }
                std::cerr << "Prod done" << std::endl;
            });

        auto consume = [&](){
//            while(!go);
            Msg previous;
            {
                TC::GET g(TC::consumers[0]);
                previous = g;
            }
            msgs[0] = previous;
            for(size_t i = 1; i < iterations - 1; ++i)
            {
                Msg msg;
                {
                    TC::GET g(TC::consumers[0]);
                    msg = g;
                }
                msgs[i] = msg;
                long diff = (msg - previous) - 1;
                result += (diff * diff);
                previous = msg;
            }
            std::cerr << "Cons done" << std::endl;
        };
//        std::thread consumer(consume);
        consume();

//    go = true;

        prod1.join();
//        consumer.join();

        std::cout << "result: " << result << std::endl;

        std::cout << "putSpinCount    = " << TC::putSpinCount << std::endl;
        std::cout << "getSpinCount    = " << TC::getSpinCount << std::endl; 
        std::cout << "cursorSpinCount = " << TC::cursorSpinCount << std::endl;

        Msg previous = 0;
        bool status = result == 0;
        for(auto i: msgs)
        {
            if(i == 0)
            {
                continue;
            }
            if(previous >= i)
            {
                std::cout << "Wrong at: " << i << std::endl;
                status = false;
            }
            previous = i;
        }
        // std::cout << "Dequeued." << std::endl;
        // std::copy(
        //     msgs.begin(),
        //     msgs.end(),
        //     std::ostream_iterator<Msg>(std::cout, "\n"));

        // std::cout << "Ring." << std::endl;
        // for(int i = 0; i < L3_QSIZE; i++)
        // {
        //     std::cout << TC::disruptor[i] << std::endl;
        // }
        return status;
    }
}

namespace test2to1
{
    using TC = TestCase<ProducerType::Multi, 1>;
    std::array<Msg, iterations> msgs;
    
    bool run()
    {
    
        int result = 0;

        std::atomic<bool> go { false };

        std::thread producer1(
            [&](){
                while(!go);
                for(Msg i = 3; i < iterations; i += 2)
                {
                    TC::PUT p(TC::producer);
                    p = i;
                }
                std::cerr << "Prod 1 done" << std::endl;
            });

        std::thread producer2(
            [&](){
                while(!go);
                for(Msg i = 2; i < iterations; i += 2)
                {
                    TC::PUT p(TC::producer);
                    p = i;
                }
                std::cerr << "Prod 2 done" << std::endl;
            });

        auto consume = [&](){
            Msg oldOdd = 1;
            Msg oldEven = 0;
            for(size_t i = 1; i < iterations - 5; ++i)
            {
                Msg msg;
                {
                    TC::GET g(TC::consumers[0]);
                    msg = g;
                }
                Msg& old = msg & 0x1L ? oldOdd : oldEven;

                long diff = (msg - old) - 2;
                result += (diff * diff);
                old = msg;
                msgs[i] = msg;                
            }
            std::cerr << "Cons done" << std::endl;
        };

        go = true;
        consume();
        producer1.join();
        producer2.join();

        std::cout << "result: " << result << std::endl;
        std::cout << "putSpinCount    = " << TC::putSpinCount << std::endl;
        std::cout << "getSpinCount    = " << TC::getSpinCount << std::endl; 
        std::cout << "cursorSpinCount = " << TC::cursorSpinCount << std::endl;

        // Msg previous = 0;
        bool status = result == 0;
    
        return status;
    }
}

namespace test1to2
{
    using TC = TestCase<ProducerType::Single, 2>;

    bool run()
    {
        std::atomic<int> result{0};
        std::atomic<bool> go { false };

        auto produce = [&](){
            while(!go);
            for(Msg i = 1; i < iterations; i++)
            {
                TC::PUT p(TC::producer);
                p = i;
            }
            std::cerr << "Prod 1 done" << std::endl;
        };

        auto consume = [&](TC::Consumer& cons){
            Msg previous = 0;
            for(size_t i = 1; i < iterations; ++i)
            {
                Msg msg;
                {
                    TC::GET g(cons);
                    msg = g;
                }
                long diff = (msg - previous) - 1;
                result.fetch_add(diff * diff);
                previous = msg;
            }
            std::cerr << "Cons done" << std::endl;
        };

        std::thread cons1([&](){ consume(TC::consumers[0]); });
        std::thread cons2([&](){ consume(TC::consumers[1]); });

        go = true;
        produce();
        cons1.join();
        cons2.join();

        std::cout << "result: " << result << std::endl;
        std::cout << "putSpinCount    = " << TC::putSpinCount << std::endl;
        std::cout << "getSpinCount    = " << TC::getSpinCount << std::endl; 
        std::cout << "cursorSpinCount = " << TC::cursorSpinCount << std::endl;

        // Msg previous = 0;
        bool status = result == 0;
    
        return status;
    }
}

int main()
{
    bool status = true;
    status &= test1to1::run();
    status &= test2to1::run();
    status &= test1to2::run();
    
    return status ? 0 : 1;
}
