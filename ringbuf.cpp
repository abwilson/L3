#include "ring.h"
//#include "allocator.h"

#include <array>
#include <atomic>
#include <iostream>
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
    
    size_t putSpinCount = 0;
    size_t getSpinCount = 0;
    size_t cursorSpinCount = 0;

    template<typename T, char log2size>
    class Disruptor: private Ring<T, log2size>
    {
        using Base = Ring<T, log2size>;
    public:
        using Base::size;
        typedef T value_type;
        typedef typename Base::Index Index;

        Disruptor():
            Base(),
            //
            // The next put position.
            //
            _head{size + 1},
            //
            // The next get.
            //
            _tail{size + 1},
            //
            // The latest commited put.
            //
            _cursor{size}
            //
            // To prevent _head catching up with _tail we must ensure
            //
            //     _head - size < _tail
            //
            // To make this check we have to subtract size from _head,
            // but Index is unsigned. So start all the counters off at
            // size so we know this difference will always be
            // positive.
            //
        {}

        friend class Put;
        class Put
        {
        public:
            Put(Disruptor& d): _dr(d), _slot(nextSlot()) {}
            ~Put() { commit(); }

            T& operator=(const T& rhs) const { return _dr[_slot] = rhs; }
            operator T&() const              { return _dr[_slot]; }
        protected:
            Disruptor& _dr;
            size_t _slot;

            size_t nextSlot()
            {
                //
                // Spin while we have caught up with consumer.
                //
                Index hd = _dr._head.load(std::memory_order_relaxed);
                Index wrapAt = hd - size;
                while(_dr._tail.load(std::memory_order_acquire) <= wrapAt)
                {
                    putSpinCount++;
                }
                return _dr._head.fetch_add(1, std::memory_order_release);
            }

            void commit()
            {
                while(_dr._cursor.load(std::memory_order_acquire) < _slot - 1)
                {
                    cursorSpinCount++;
                }
                //
                // No need to CAS. Only we could have been waiting for
                // this cursor value.
                //
                _dr._cursor.fetch_add(1, std::memory_order_release);
            }
        };

        void put(const T& rhs)
        {
            Put(*this) = rhs;
        }

        friend class Get;
        class Get
        {
        public:
            Get(Disruptor& d): _dr(d), _slot(nextSlot()) {}
            ~Get() { commit(); }

            operator const T&() const { return _dr[_slot]; }

        protected:
            Disruptor& _dr;
            size_t     _slot;

            Index nextSlot()
            {
                size_t slot = _dr._tail.load(std::memory_order_acquire);
                while(slot > _dr._cursor.load(std::memory_order_acquire))
                {
                    getSpinCount++;
                }
                return slot;
            }

            void commit()
            {
                _dr._tail.fetch_add(1, std::memory_order_release);
            }
        };

        T get()
        {
            return Get(*this);
        }
            
    protected:
        CACHE_LINE Sequence _head;
        CACHE_LINE Sequence _tail;
        CACHE_LINE Sequence _cursor;
    };
}

constexpr size_t iterations = 100000000; // 100 Million.
std::array<size_t, iterations> msgs;

using namespace L3;
using DR = Disruptor<size_t, 10>;
DR buf;

int main()
{
    int result = 0;

//    std::atomic<bool> go { false };

    std::thread producer(
        [&](){
//            while(!go);
            for(size_t i = 1; i < iterations; i++)
            {
                {
#if 0
                    DR::Put p(buf);
                    p = i;
#else
                    buf.put(i);
#endif
                }
            }
            std::cerr << "Prod done" << std::endl;
        });

    std::thread consumer(
        [&](){
//            while(!go);
            size_t previous = buf.get();
            for(size_t i = 1; i < iterations - 1; ++i)
            {
                size_t msg = buf.get();
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

    std::cout << "putSpinCount = " << putSpinCount << std::endl;
    std::cout << "getSpinCount = " << getSpinCount << std::endl; 
    std::cout << "cursorSpinCount = " << cursorSpinCount << std::endl;

    size_t previous = 0;
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
    //     std::ostream_iterator<size_t>(std::cout, "\n"));
    return status ? 0 : 1;
}
