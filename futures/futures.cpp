#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

int calculate_square(int x)
{
    std::cout << "Starting calculation for " << x << " in " << std::this_thread::get_id() << std::endl;

    std::random_device rd;
    std::uniform_int_distribution<> distr(100, 5000);

    std::this_thread::sleep_for(std::chrono::milliseconds(distr(rd)));

    if (x % 3 == 0)
        throw std::runtime_error("Error#3");

    return x * x;
}

void save_to_file(const std::string& filename)
{
    std::cout << "Saving to file: " << filename << std::endl;

    std::this_thread::sleep_for(3s);

    std::cout << "File saved: " << filename << std::endl;
}

void consume(std::shared_future<int> fsquare)
{
    std::cout << "Consuming in THD#" << std::this_thread::get_id() << " - " << fsquare.get() << std::endl;
}

int main()
{
    std::future<int> f1 = std::async(std::launch::async, &calculate_square, 13);
    std::future<int> f2 = std::async(std::launch::async, &calculate_square, 9);
    std::future<int> f3 = std::async(std::launch::deferred, &calculate_square, 23);
    std::future<void> fs = std::async(std::launch::async, &save_to_file, "data.txt");

    while (fs.wait_for(500ms) != std::future_status::ready)
    {
        std::cout << "... still waiting for the end..." << std::endl;
    }

    std::cout << "13 * 13 = " << f1.get() << std::endl;

    try
    {
        auto result = f2.get();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    std::cout << "23 * 23 = " << f3.get() << std::endl;

    ///////////////////////////////

    std::future<int> fsquare = std::async(std::launch::async, &calculate_square, 101);

    std::shared_future<int> shared_fsquare = fsquare.share();

    std::vector<std::thread> thds;
    for(int i = 0; i < 5; ++i)
        thds.emplace_back(&consume, shared_fsquare);

    for(auto& thd : thds)
    {
        thd.join();
    }
}
