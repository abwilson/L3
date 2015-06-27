#ifndef CONSUMER_H
#define CONSUMER_H

#include "disruptor.h"
#include "sequence.h"

namespace L3
{
    template<typename C, C&... tail>
    struct ConsumerList;

    template<typename Dstor, Dstor& dstor>
    class ConsumerT
    {
        template<typename C, C&... tail>
        friend struct ConsumerList;

        template<typename C, size_t size, C (&consumers)[size]>
        friend struct ConsumerArray;
        
        L3_CACHE_LINE Sequence _tail{Dstor::size + 1};

//        constexpr ConsumerT(){}

        template<typename SpinProbe=NoOp>
        Index claim(SpinProbe spinProbe=SpinProbe())
        {
            //
            // Eventually slot will be used to calculate the address
            // of the next item read from the ring. It will therefore
            // form a dependency chain with this item so consume
            // memory order is sufficient.
            //
            Index slot = _tail.load(std::memory_order_acquire);
            while(slot > dstor._cursor.load(std::memory_order_acquire))
            {
                spinProbe();
            }
            return slot;
        }
      
        void commit()
        {
            _tail.fetch_add(1, std::memory_order_release);
        }
        
    public:
        using value_type = typename Dstor::value_type;

        template<typename ClaimProbe=NoOp>
        class Get
        {
            ConsumerT& _c;
            Index _slot;
            
        public:
            using value_type = typename Dstor::value_type;
            
            Get(ConsumerT& c): _c(c), _slot(c.claim(ClaimProbe())) {}
            ~Get() { _c.commit(); }

            operator const value_type&() const { return dstor[_slot]; }
        };            

        value_type get()
        {
            return Get<>(*this);
        }
    };

    template<typename C, C&... tail>
    struct ConsumerList
    {
        static bool lessThanEqual(Index i) { return false; }
    };
    
    template<typename C, C& c, C&... tail>
    struct ConsumerList<C, c, tail...>: ConsumerList<C, tail...>
    {
        static bool lessThanEqual(Index i)
        {
            return c._tail.load(std::memory_order_acquire) <= i ||
                ConsumerList<C, tail...>::lessThanEqual(i);
        }
    };

    template<typename C, size_t size, C (&consumers)[size]>
    struct ConsumerArray
    {
        static bool lessThanEqual(Index idx)
        {
            for(auto& i: consumers)
            {
                if(i._tail.load(std::memory_order_acquire) <= idx)
                {
                    return true;
                }
            }
            return false;
        }
    };
}

#endif
