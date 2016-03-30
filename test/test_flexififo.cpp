/*
The MIT License (MIT)

Copyright (c) 2015 Norman Wilson - Volcano Consultancy Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <L3/util/flexififo.h>
#include <L3/util/ring.h>

#include <iostream>

using namespace L3;

using Ring4 = Ring<Index, 2>;

FlexiFifo<Ring4, UniqueHead, UniqueTail> fifo1To1;
FlexiFifo<Ring4, UniqueHead, SharedTail> fifo1ToN;
FlexiFifo<Ring4, SharedHead, UniqueTail> fifoNTo1;
FlexiFifo<Ring4, SharedHead, SharedTail> fifoNToN;

using FifoNToN = FlexiFifo<Ring4, SharedHead, SharedTail>;

int
main()
{
    SharedConsumer<FifoNToN> consumer(&fifoNToN);
    SharedConsumer<FifoNToN> consumer2(&fifoNToN);
    std::cout << "sizeof: " << sizeof(consumer) << std::endl;

    fifoNToN.commitHead(fifoNToN.claimHead());
    fifoNToN.commitHead(fifoNToN.claimHead());

    consumer.claim();
    consumer2.claim();
    consumer2.commit();
    consumer.commit();
}


