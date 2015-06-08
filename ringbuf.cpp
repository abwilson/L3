
#include <iostream>
#include <atomic>
#include <thread>

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

#define CACHE_LINE alignas(cache_line_size)
//#define CACHE_LINE

namespace Disruptor
{
    static constexpr size_t cache_line_size = CACHE_LINE_SIZE;
    using Sequence = std::atomic_size_t;
//    using Sequence = size_t;
    
    struct alignas(cache_line_size) CacheLine
    {
        char padding[cache_line_size];
    };

    /**
     * A buffer of Ts indexed in a circular manner using
     *
     *   index % size.
     *
     * As an optimisation we constrain that size = 2^n. This allows
     * the remainder to be calculated by masking out the lower order
     * bits in the index.
     *
     * Since we are constraining size to powers of 2 we express this
     * parameter directly as a power.
     */
    template<typename T, char log2size>
    class Ring
    {
        static_assert(log2size < 32, "Unreasonable RingBuf size.");
        //
        // In the implementation of a disruptor there's a window
        // between a slot has been claimed and the cursor advanced
        // where the value must be copied into the ring buffer. If
        // this operation fails and the cursor is not advanced then
        // the whole system will block. To ensure the value copy
        // cannot fail constrain T to being a POD.
        //
        static_assert(std::is_pod<T>::value, "PODs only");        
    public:
        //
        // We want indexes to have a large range so we don't have to
        // worry about wrapping.
        //
        using Index = uint64_t;
        using value_type = T;
        static constexpr Index size = 1L << log2size;
        static constexpr Index mask = size - 1;

        Ring(): storage{} {}

        T& operator[](Index idx)
        {
            //
            // Circularise indexing by using idx mod size implemented
            // by masking off the high order bits.
            //
            return storage[idx & mask];
        }

    protected:
        T storage[size];
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
}

int main()
{
    using namespace Disruptor;
    using DR = Disruptor<size_t, 10>;

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
