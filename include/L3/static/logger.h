#ifndef LOGGER_H
#define LOGGER_H
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

#include <L3/static/disruptor.h>

#include <iostream>
#include <sstream>
#include <thread>

namespace L3 // Low Latency Library
{
    class Logger
    {
        using Buffer = Disruptor<std::string, 14, Logger>;
        using Put = Buffer::Put<Barrier<Buffer::Get<>>, CommitPolicy::Shared>;

    public:
        Logger(): os() {}
        ~Logger()
        {
            Put() = std::move(os.str());
        }

        template<typename T>
        friend std::ostream& operator<<(Logger&& l, const T& rhs)
        {
            l.os << rhs;
            return l.os;
        }

        struct Writer
        {
            Writer(std::ostream* os = &std::cout): m_os(os), m_running(true) {}

            void operator()() const
            {
                std::ostream_iterator<std::string> oi(*m_os, "\n");
                
                while(m_running)
                {
                    Buffer::Get<> g;
                    std::copy(g.begin(), g.end(), oi);
                }
            }
            std::thread start()
            {
                return std::thread([=]{ (*this)(); });
            }
            void stop() { Logger() << "stoping logger"; m_running = false; }
        private:
            std::ostream* const m_os;
            std::atomic<bool> m_running;
        };
    protected:
        std::ostringstream os;
    };
}
                    
#endif
