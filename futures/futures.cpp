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

template <typename F>
auto spawn_task(F&& f)
{
    using ResultT = decltype(f());
    std::packaged_task<ResultT()> pt(std::forward<F>(f));

    std::future<ResultT> fresult = pt.get_future();

    std::thread thd{std::move(pt)};
    thd.detach();

    return fresult;
}

class Calculator
{
    std::promise<int> promise_;
public:
    std::future<int> get_future()
    {
        return promise_.get_future();
    }

    void calculate(int n)
    {
        try
        {
            int result = calculate_square(n);
            promise_.set_value(result);
        }
        catch(...)
        {
            promise_.set_exception(std::current_exception());
        }        
    }
};

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

    std::future<int> f_deffered = std::async(std::launch::deferred, calculate_square, 13);

    assert(f_deffered.wait_for(0ns) == std::future_status::deferred);

    auto result = f_deffered.get();

    std::cout << "\n#############################################" << std::endl;

    // auto fs1 = std::async(std::launch::async, save_to_file, "data1.txt");
    // auto fs2 = std::async(std::launch::async, save_to_file, "data2.txt");
    // auto fs3 = std::async(std::launch::async, save_to_file, "data3.txt");
    // auto fs4 = std::async(std::launch::async, save_to_file, "data4.txt");

    spawn_task([] { save_to_file("data1.txt"); });
    spawn_task([] { save_to_file("data2.txt"); });
    spawn_task([] { save_to_file("data3.txt"); });
    auto ft = spawn_task([] { save_to_file("data4.txt"); });

    ft.wait();

    std::cout << "\n#############################################" << std::endl;
    
    // PACKAGED TASK

    std::packaged_task<int()> pt([] { return calculate_square(13); });
    std::future<int> f_result = pt.get_future();

    std::thread thd_runner{std::move(pt)};

    std::cout << "Result: " << f_result.get() << std::endl;

    thd_runner.join();

    Calculator calc;

    f_result = calc.get_future();

    spawn_task([&] { calc.calculate(101); });

    std::cout << "Calculator returns: " << f_result.get() << std::endl;
}
