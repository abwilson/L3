#include "ring.h"
#include "allocator.h"

#include <iostream>
#include <atomic>
#include <thread>

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

#define CACHE_LINE alignas(cache_line_size)
//#define CACHE_LINE

namespace L3 // Low Latency Library
{
    static constexpr size_t cache_line_size = CACHE_LINE_SIZE;
    using Sequence = std::atomic_size_t;
//    using Sequence = size_t;
    
    struct alignas(cache_line_size) CacheLine
    {
        char padding[cache_line_size];
    };

    size_t putSpinCount = 0;
    size_t getSpinCount = 0;
    size_t cursorSpinCount = 0;

    template<typename T, char log2size>
    class Disruptor
    {
    public:
        typedef T value_type;

        Disruptor(): head{1}, tail{1}, cursor{0}, buf{} {}

        class Put
        {
        public:
            Put(Disruptor& x): disruptor(x), slot(nextSlot()) {}
            Put(Disruptor& x, const T& rhs): Put(x) { (*this) = rhs; }
        
            ~Put()
            {
                while(disruptor.cursor.load(std::memory_order_acquire) < slot - 1)
                {
                    cursorSpinCount++;
                    ;//                    std::atomic_thread_fence(std::
                }
                    ; // Spin.
                //
                // No need to CAS. Only we could have been waiting for
                // this cursor value.
                //
                disruptor.cursor.fetch_add(1, std::memory_order_release);
            }

            T& operator=(const T& rhs) const
            {
                return disruptor.buf[slot] = rhs;
            }

            operator T&() const { return disruptor.buf[slot]; }
        protected:
            Disruptor& disruptor;
            size_t slot;

            size_t nextSlot()
            {
                while(disruptor.tail.load(std::memory_order_acquire) >
                      disruptor.head.load(std::memory_order_acquire))
                {
                    putSpinCount++;
                }
                return disruptor.head.fetch_add(1, std::memory_order_release);
            }
        };

        class Get
        {
        public:
            Get(Disruptor& x): disruptor(x), slot(nextSlot()) {}

            operator const T&() const { return disruptor.buf[slot]; }

        protected:
            Disruptor& disruptor;
            size_t slot;

            size_t nextSlot()
            {
                while(disruptor.tail.load(std::memory_order_acquire) >
                      disruptor.cursor.load(std::memory_order_acquire))
                {
                    getSpinCount++;
                }
                return disruptor.tail.fetch_add(1, std::memory_order_release);
            }
        };

    protected:
        CACHE_LINE Sequence head;
        CACHE_LINE Sequence tail;
        CACHE_LINE Sequence cursor;
        CACHE_LINE Ring<T, log2size> buf;
    };

    using IntRing = Ring<int, 10>;
    IntRing r;
    using Idx = IntRing::Idx<r>;
    Idx i {0};

    struct Foo { int x; int y; };

    Allocator<Foo, 10> a;
}

int main()
{
    using namespace L3;
    using DR = Disruptor<size_t, 10>;

    a.allocate(1);
    


    DR buf;

    std::cout << "sizeof(buf) = " << sizeof(buf) << std::endl;
    {
        DR::Put(buf, 42);
    }
    std::cout << DR::Get(buf) << std::endl;

    int result = 0;
    const size_t last = 50000000; // 50 Million.

    std::thread consumer(
        [&](){
            size_t previous = 0;
            for(size_t i = 1; i < last; ++i)
            {
                size_t msg = DR::Get(buf);
                result += msg - previous - 1;
                previous = msg;
            }
        });

    std::thread producer(
        [&](){
            for(size_t i = 1; i < last; i++)
            {
                DR::Put(buf, i);
            }
        });

    producer.join();
    consumer.join();

    std::cout << "result: " << result << std::endl;

    std::cout << "putSpinCount = " << putSpinCount << std::endl;
    std::cout << "getSpinCount = " << getSpinCount << std::endl; 
    std::cout << "cursorSpinCount = " << cursorSpinCount << std::endl;
}
