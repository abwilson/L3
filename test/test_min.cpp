#include <L3/util/scopedtimer.h>

#include <algorithm>
#include <iostream>
#include <random>

using namespace std::chrono;

template<typename T>
inline const T& fastMinRef(const T& lhs, const T& rhs)
{
    const T* const tmp[] = { &lhs, &rhs };
    return *tmp[lhs < rhs];
}

template<typename T>
inline T fastMin(const T lhs, const T rhs)
{
    const T tmp[] = { lhs, rhs };
    return tmp[lhs < rhs];
}

template<typename T>
inline T fastMin2(T&& lhs, T&& rhs)
{
    constexpr T one = 1;
    constexpr T signbit = (one << 7) << (sizeof(T) - 1) * 8;
    auto diff = lhs - rhs;
    auto minusDiff = -diff;
    constexpr auto shiftBack = sizeof(T) * 8 - 1;

    return lhs * (diff >> shiftBack) + rhs * (minusDiff >> shiftBack);
}

inline float branchFloat(bool b) { return b ? 1.3f : 2.6f; }

inline float indexFloat(bool b)
{
    constexpr float vals[] = { 2.6f, 1.3f };
    return vals[b];
}

inline int branchInt(bool b){ return b ? 1 : 3; }

inline int indexInt(bool b)
{
    constexpr float vals[] = { 3, 1 };
    return vals[b];
}

template<typename T>
inline T fastArrayMin(const T (&vals)[2]) { return vals[vals[0] < vals[1]]; }

template<typename T, size_t n>
inline T fastArrayMin(const T (&vals)[n])
{
    
}

constexpr size_t dataSize = 1 << 24;
int data[dataSize];
int data2[dataSize];
float result1[dataSize >> 1];
float result2[dataSize >> 1];

int resultInt1[dataSize >> 1];
int resultInt2[dataSize >> 1];

template<typename T, size_t n>
void randomiseBlock(T (&data)[n])
{
    std::ranlux24_base rd;
    std::uniform_int_distribution<T> dist;

    for(auto& i: data) i = dist(rd);
}

template<typename Fun>
inline L3::ScopedTimer<>::duration runCase(Fun& fun)
{
    L3::ScopedTimer<>::duration elapsedTime{0};
    L3::ScopedTimer<> timer(elapsedTime);
    fun();
    return elapsedTime;
}

template<typename Ratio = std::milli>
class RunTimer
{
public:
    
    using Duration = duration<double, std::milli>;

    Duration aTime{0};
    Duration bTime{0};

    template<typename A, typename B>
    void run(A&& a, B&& b, size_t repeats)
    {
        for(size_t i = 0; i < repeats; ++i)
        {
            aTime += runCase(a);
            bTime += runCase(b);
        }
        aTime /= repeats;
        bTime /= repeats;
    }
};

template<typename Data, typename Result, typename Fun>
struct DataRunner
{
    Data data;
    Result result;
    Fun fun;

    DataRunner(Fun&& f):
        fun(f)
    {
        randomiseBlock(data);
    }
    
    void operator()()
    {
        auto* r = result;
        for(const auto* i = std::begin(data); i != std::end(data);)
        {
            *r++ = fun(i);
        }
    }
};

template<typename Data, typename Result, typename Fun>
inline DataRunner<Data, Result, Fun>
makeDataRunner(Fun&& fun)
{
    return DataRunner<Data, Result, Fun>(std::move(fun));
}

auto branchRunner = makeDataRunner<int[dataSize], float[dataSize]>(
    [](const int*& i)
    {
        int l = *i++;
        return branchFloat(l < *i++);
    });

auto indexRunner = makeDataRunner<int[dataSize], float[dataSize]>(
    [](const int*& i)
    {
        int l = *i++;
        return indexFloat(l < *i++);
    });

auto branchRunnerInt = makeDataRunner<int[dataSize], int[dataSize]>(
    [](const int*& i)
    {
        int l = *i++;
        return branchInt(l < *i++);
    });

auto indexRunnerInt = makeDataRunner<int[dataSize], int[dataSize]>(
    [](const int*& i)
    {
        int l = *i++;
        return indexInt(l < *i++);
    });

auto fastArrayRunnerInt = makeDataRunner<int[dataSize], int[dataSize]>(
    [](const int*& i)
    {
        int result = fastArrayMin(reinterpret_cast<const int (&)[2]>(*i));
        i += 2;
        return result;
    });

auto stdMinRunnerInt = makeDataRunner<int[dataSize], int[dataSize]>(
    [](const int*& i){
        int tmp = *i++;
        return std::min(tmp, *i++);
    });

auto fastArrayRunner = makeDataRunner<float[dataSize], float[dataSize]>(
    [](const float*& i)
    {
        float result = fastArrayMin(reinterpret_cast<const float (&)[2]>(*i));
        i += 2;
        return result;
    });

auto stdMinRunner = makeDataRunner<float[dataSize], float[dataSize]>(
    [](const float*& i){
        float tmp = *i++;
        return std::min(tmp, *i++);
    });

int
main()
{
    std::atomic<int> i;
    using clock = std::chrono::steady_clock;
    clock::time_point start(clock::now());
    i.store(10);
    clock::time_point end(clock::now());

    L3::ScopedTimer<>::duration tickSize = end - start;

    std::cout << "Minimum measurable time: "
              << duration_cast<nanoseconds>(tickSize).count()
              << "ns"
              << std::endl;
    
    L3::ScopedTimer<>::duration elapsedTime{0};
    {
        L3::ScopedTimer<> timer(elapsedTime);
        randomiseBlock(data);
        randomiseBlock(data2);
    }
    std::cout << "Time: init: "
              << duration_cast<microseconds>(elapsedTime).count()
              << "us"
              << std::endl;

    RunTimer<> runner;
    int repeats = 100;
    runner.run(stdMinRunner, fastArrayRunner, repeats);
    std::cout << "std::min v fastArray" << std::endl;
    std::cout << "average branch: " << runner.aTime.count() << "ms"
              << std::endl;
    std::cout << "average index: " << runner.bTime.count() << "ms"
              << std::endl;
    
    runner.run(branchRunner, indexRunner, repeats);
    std::cout << "branch v index" << std::endl;
    std::cout << "average branch: " << runner.aTime.count() << "ms"
              << std::endl;
    std::cout << "average index: " << runner.bTime.count() << "ms"
              << std::endl;

    runner.run(branchRunnerInt, indexRunnerInt, repeats);
    std::cout << "branchInt v indexInt" << std::endl;
    std::cout << "average branch: " << runner.aTime.count() << "ms"
              << std::endl;
    std::cout << "average index: " << runner.bTime.count() << "ms"
              << std::endl;
    
    runner.run(
        []()
        { 
            float* r = result2;
            for(const int* i = std::begin(data); i != std::end(data);)
            {
                int tmp = *i++;
                *r++ = branchFloat(tmp < *++i);
            }
        },
        [](){
            float* r = result1;
            for(const int* i = std::begin(data2); i != std::end(data2);)
            {
                int tmp = *i++;
                *r++ = indexFloat(tmp < *++i);
            }
        },
        repeats);

    std::cout << "branchFloat v indexFloat" << std::endl;
    std::cout << "average branch: " << runner.aTime.count() << "ms" << std::endl;
    std::cout << "average index: " << runner.bTime.count() << "ms" << std::endl;

    runner.run(
        [](){
            int* r = resultInt1;
            for(const int* i = std::begin(data2); i != std::end(data2);)
            {
                int tmp = *i++;
                *r++ = indexInt(tmp < *++i);
            }
        },
        []()
        { 
            int* r = resultInt2;
            for(const int* i = std::begin(data); i != std::end(data);)
            {
                int tmp = *i++;
                *r++ = branchInt(tmp < *++i);
            }
        },
        repeats);
        
    std::cout << "indexInt v branchInt" << std::endl;
    std::cout << "average index: " << runner.aTime.count() << "ms"
              << std::endl;
    std::cout << "average branch: " << runner.bTime.count() << "ms"
              << std::endl;
}
