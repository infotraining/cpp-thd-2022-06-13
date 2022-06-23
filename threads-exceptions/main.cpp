#include "joining_thread.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

template <typename T>
struct ThreadResult
{
    T value;
    std::exception_ptr eptr;

    T get()
    {
        if (eptr)
            std::rethrow_exception(eptr);

        return std::move(value);
    }
};

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay, ThreadResult<char>& result)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    try
    {
        result.value = text.at(6);
    }
    catch (...)
    {
        result.eptr = std::current_exception();
        return;
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    ThreadResult<char> result;

    ext::joining_thread thd_1 {background_work, 1, text, 100ms, std::ref(result)};
    thd_1.join();

    try
    {
        std::cout << result.get() << " returned from thread" << std::endl;
    }
    catch (const std::out_of_range& e)
    {
        std::cout << "Logging " << e.what() << std::endl;
    }

    std::cout << "Main thread ends..." << std::endl;
}
