#ifndef CONSUME_H
#define CONSUME_H

namespace L3
{
    //
    // General purpose consumer loop.
    //
    template<typename Get, typename F, typename EOS>
    inline void
    consume(EOS& checkEOS, const F& f)
    {
        for(;;)
        {
            for(auto msg: Get())
            {
                f(msg);

                if(checkEOS(msg))
                {
                    return;
                }
            }
        }
    }

    template<typename Msg, Msg eos>
    struct CheckEOS
    {
        CheckEOS(size_t n): _remaining(n) {}

        bool operator()(Msg m) { return m == eos && !--_remaining; }
        size_t _remaining;
    };
}

#endif
