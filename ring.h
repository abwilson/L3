#ifndef RING_H
#define RING_H

#include <atomic>
#include <cstdint>
#include <type_traits>
#include <iterator>

namespace L3 // Low Latency Library
{
    //
    // With a 64 bit index we could use values at a rate of 4GHz
    // for 136 years before worrying about wrapping.
    //
    using Index = uint64_t;
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
     * parameter directly as n where size = 2^n. 
     *
     * Items stored in a ring can be addressed via an index. Since the
     * index wraps back round it can address a bounded buffer but also
     * be monotonically increasing. This property can be used in to
     * solve the ABA problem in algorithms involving CAS.
     */
    template<typename T, size_t log2size>
    class Ring
    {
        static_assert(log2size < 32, "Unreasonable RingBuf size.");
    public:
        using value_type = T;
        static constexpr Index size = 1L << log2size;

        Ring(): _storage{} {}

        T& operator[](Index idx)
        {
            //
            // Circularise indexing by using (idx mod size)
            // implemented by masking off the high order bits.
            //
            static constexpr Index _mask = size - 1;
            return _storage[idx & _mask];
        }
        //
        // An index typed on the ring instance it came from.
        //
        template<Ring& r>
        struct Iterator: std::iterator<std::random_access_iterator_tag, T>
        {
            using Ring = Ring;
            Iterator(Index i): _index(i) {}
            static constexpr Ring& _ring = r;
            T& operator*()  const { return _ring[_index]; }
            T* operator->() const { return &_ring[_index]; }

            Iterator& operator++()
            {
                ++_index;
                return *this;
            }

            operator Index() const { return _index; }

        private:
            Index _index;
        };

    protected:
        T _storage[size];
    };
}

#endif
