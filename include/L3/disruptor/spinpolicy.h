#ifndef SPINPOLICY_H
#define SPINPOLICY_H

#include <thread>

namespace L3
{
    namespace SpinPolicy
    {
        struct Yield
        {
            void operator()() const
            {
                std::this_thread::yield();
            }
        };
    }
}

#endif
