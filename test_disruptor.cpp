#include "disruptor.h"

#include <array>
#include <iostream>
#include <thread>

using namespace L3;

L3_CACHE_LINE size_t putSpinCount = 0;
L3_CACHE_LINE size_t getSpinCount = 0;
L3_CACHE_LINE size_t cursorSpinCount = 0;

template<size_t& counter>
struct Counter
{
    void operator()() { ++counter; }
};

using PutSpinCounter = Counter<putSpinCount>;
using GettSpinCounter = Counter<getSpinCount>;
using CursorSpinCounter = Counter<cursorSpinCount>;

typedef size_t Msg;

using DR = Disruptor<
    Ring<Msg, 18>,
    PutSpinCounter,
    GettSpinCounter,
    CursorSpinCounter>;
DR buf;

constexpr size_t iterations = 100000000; // 100 Million.

std::array<Msg, iterations> msgs;

bool testSingleProducerSingleConsumer()
{
    int result = 0;

//    std::atomic<bool> go { false };

    std::thread producer(
        [&](){
//            while(!go);
            for(Msg i = 1; i < iterations; i++)
            {
                buf.put(i);
            }
            std::cerr << "Prod done" << std::endl;
        });

    std::thread consumer(
        [&](){
//            while(!go);
            Msg previous = buf.get();
            for(size_t i = 1; i < iterations - 1; ++i)
            {
                Msg msg = buf.get();
                msgs[i] = msg;
                long diff = (msg - previous) - 1;
                result += (diff * diff);
                previous = msg;
            }
            std::cerr << "Cons done" << std::endl;
        });

//    go = true;

    producer.join();
    consumer.join();

    std::cout << "result: " << result << std::endl;
//    std::cout << "buf size: " << DR::size

    std::cout << "putSpinCount    = " << putSpinCount << std::endl;
    std::cout << "getSpinCount    = " << getSpinCount << std::endl; 
    std::cout << "cursorSpinCount = " << cursorSpinCount << std::endl;

    Msg previous = 0;
    bool status = true;
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
    // std::copy(
    //     msgs.begin(),
    //     msgs.end(),
    //     std::ostream_iterator<Msg>(std::cout, "\n"));
    
    return status;
}

bool testTwoProducerSingleConsumer()
{
    int result = 0;

    std::atomic<bool> go { false };

    std::thread producer1(
        [&](){
            while(!go);
            for(Msg i = 3; i < iterations; i += 2)
            {
                buf.put(i);
            }
            std::cerr << "Prod 1 done" << std::endl;
        });

    std::thread producer2(
        [&](){
            while(!go);
            for(Msg i = 2; i < iterations; i += 2)
            {
                buf.put(i);
            }
            std::cerr << "Prod 2 done" << std::endl;
        });
    

    auto oddOrEven = [](const Msg& m, Msg& odd, Msg& even) -> Msg&
    {
        return m & 0x1L ? odd : even;
    };
    
    std::thread consumer(
        [&](){
            while(!go);
            Msg oldOdd = 1;
            Msg oldEven = 0;
            for(size_t i = 1; i < iterations - 5; ++i)
            {
                Msg msg = buf.get();

                Msg& old = msg & 0x1L ? oldOdd : oldEven;

                long diff = (msg - old) - 2;
                result += (diff * diff);
                old = msg;
                msgs[i] = msg;                
            }
            std::cerr << "Cons done" << std::endl;
        });

    go = true;

    producer1.join();
    producer2.join();
    consumer.join();

    std::cout << "result: " << result << std::endl;
//    std::cout << "buf size: " << DR::size

    std::cout << "putSpinCount    = " << putSpinCount << std::endl;
    std::cout << "getSpinCount    = " << getSpinCount << std::endl; 
    std::cout << "cursorSpinCount = " << cursorSpinCount << std::endl;

    // Msg previous = 0;
    bool status = result == 0;
    // for(auto i: msgs)
    // {
    //     if(i == 0)
    //     {
    //         continue;
    //     }
    //     if(previous >= i)
    //     {
    //         std::cout << "Wrong at: " << i << std::endl;
    //         status = false;
    //     }
    //     previous = i;
    // }
    // std::copy(
    //     msgs.begin(),
    //     msgs.end(),
    //     std::ostream_iterator<Msg>(std::cout, "\n"));
    
    return status;
}



int main()
{
    bool status = true;
    status = testSingleProducerSingleConsumer();
    status &= testTwoProducerSingleConsumer() && status;
    
    return status ? 0 : 1;
}
