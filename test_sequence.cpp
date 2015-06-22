#include "sequence.h"
#include "cacheline.h"

#include <iostream>

using namespace L3;

L3_CACHE_LINE Sequence s1{0};

SequenceList<s1> s1l;

L3_CACHE_LINE Sequence s2{0};
SequenceList<s1, s2> s2l;

constexpr size_t size = 4;
L3_CACHE_LINE Sequence sArray[size];
SequenceArray<size, sArray> sal;

// struct Seq2: public Sequence {};

// Seq2 seq2;

// SequenceList<seq2> seq1l;

int
main()
{
    s1l.lessThanEqual(0);
    s2l.lessThanEqual(0);
    sal.lessThanEqual(0);
//    seq1l.lessThanEqual(0);

    s1l.least();
}
