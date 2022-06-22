#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include "joining_thread.hpp"

using namespace std::literals;

void may_throw()
{
    throw std::runtime_error("UPS...");
}

template <typename T>
struct SynchronizedValue
{
    T value{};
    std::mutex mtx;
};

using SyncCounter = SynchronizedValue<uint64_t>;

void run(SyncCounter& counter)
{
    for (uint64_t i = 0; i < 1'000'000ULL; ++i)
    {
        std::lock_guard<std::mutex> lk{counter.mtx}; // SC begins
        ++counter.value;    
        //may_throw();
    } // SC ends
}


int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    SyncCounter counter;

    {
        ext::joining_thread thd1 {run, std::ref(counter)};
        ext::joining_thread thd2 {[&counter]
            { run(counter); }};
    } // implicit join

    std::cout << "Counter: " << counter.value << std::endl;

    std::cout << "Main thread ends..." << std::endl;
}
