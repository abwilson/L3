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

#if 1
    template<typename G, typename F>
    struct Handler //: public std::function<void, const Get&>
    {
        F f;
        using Get = G;
    };

    template<typename G, typename F>
    inline Handler<G, F> makeHandler(const F& f)
    {
        return Handler<G, F>{f};
    }
#endif
    
    template<typename... Tail> void select(size_t, Tail&...);

    template<typename Handler>
    inline void select(size_t batchSize, Handler& h)
    {
        using Get = typename Handler::Get;
        for(auto& m: Get(batchSize, Get::noBlock))
        {
            h(m);
        }
    }
    
    template<typename Handler, typename... Tail>
    inline void select(size_t batchSize, Handler& h, Tail&... tail)
    {
        select(batchSize, h);
        select(batchSize, tail...);
    }
}

#endif
