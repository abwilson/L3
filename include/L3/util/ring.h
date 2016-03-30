#ifndef RING_H
#define RING_H

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
        static_assert(log2size > 0, "Minimun ring size is 2");        
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

        const T* begin() const { return std::begin(_storage); }
        const T* end() const { return std::end(_storage); }

        template<typename I>
        struct IteratorT: std::iterator<std::random_access_iterator_tag, T>
        {
            IteratorT(Ring* r, Index i): _ring(r), _index(i) {}

            T& operator*() const { return (*_ring)[_index]; }
            T* operator->() const { return &(*_ring)[_index]; }

            I& operator++()
            {
                ++_index;
                return *static_cast<I*>(this);
            }

            I operator++(int)
            {
                I result{*this};
                ++_index;
                return result;
            }
            operator Index() const { return _index; }

        private:
            Ring* const _ring;
            Index _index;
        };
        
        struct Iterator: IteratorT<Iterator>
        {
            Iterator(Ring* r, Index i): IteratorT<Iterator>(r, i) {}
        };
        //
        // An index typed on the ring instance it came from.
        //
#if 1
        template<Ring& r>
        struct StaticIterator: std::iterator<std::random_access_iterator_tag, T>
        {
            using Ring = Ring;
            StaticIterator(Index i): _index(i) {}
            static constexpr Ring& _ring = r;
            T& operator*()  const { return _ring[_index]; }
            T* operator->() const { return &_ring[_index]; }

            StaticIterator& operator++()
            {
                ++_index;
                return *this;
            }

            StaticIterator operator++(int)
            {
                StaticIterator result{*this};
                ++_index;
                return result;
            }
            operator Index() const { return _index; }

        private:
            Index _index;
        };
#else
        template<Ring& r>
        struct StaticIterator: IteratorT<StaticIterator<r>>
        {
//            using Ring = Ring;
            StaticIterator(Index i): IteratorT<StaticIterator<r>>(&r, i) {}
        };
#endif

    protected:
        T _storage[size];
    };

    template<typename OS, typename T, size_t log2size>
    OS& operator<<(OS& os, const Ring<T, log2size>& ring)
    {
        using Ring = Ring<T, log2size>;
        std::ostream_iterator<typename Ring::value_type> oi(os, ", ");
        std::copy(ring.begin(), ring.end(), oi);
        return os;
    }
}

#endif
