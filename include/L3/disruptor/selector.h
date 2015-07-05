#ifndef SELECTOR_H
#define SELECTOR_H

namespace L3
{
    template<typename...>
    struct Selector
    {
        static void select() {}
    };

    template<typename Get, typename F, typename... Tail>
    struct Selector<Get, F, Tail...>: Selector<Tail...>
    {
        static void select()
        {
            F f;
            for(auto& m: Get(Get::noBlock))
            {
                f(m);
            }
            Selector<Tail...>::select();            
        }
    };
#if 0
    template<typename...> void select(...);

    template<typename Get, typename F, typename... Tail>
    void select(F& f, ...)
    {
        for(auto& m: Get(Get::noBlock))
        {
            f(m);
        }
        select<Tail...>::select();
    }
#endif
}

#endif
