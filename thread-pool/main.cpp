#include "thread_safe_queue.hpp"

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <future>
#include <random>

using namespace std::literals;

using Task = std::function<void()>;
//using Task = Folly::function<void()>;

namespace ver_1_0
{
    class ThreadPool
    {
    public:
        ThreadPool(size_t size)
            : threads_(size)
        {
            for (auto& thread : threads_)
                thread = std::thread([this]
                    { run(); });
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
                tasks_.push(STOP);

            for (auto& thread : threads_)
                thread.join();
        }

        void submit(Task task)
        {
            if (!task)
                throw std::invalid_argument("Empty task not supported");

            tasks_.push(task);
        }

    private:
        std::vector<std::thread> threads_;
        ThreadSafeQueue<Task> tasks_;
        const Task STOP;

        void run()
        {
            while (true)
            {
                Task task;
                tasks_.pop(task);

                if (!task)
                    break;

                task();
            }
        }
    };
}

namespace ver_2_0
{
    class ThreadPool
    {
    public:
        ThreadPool(size_t size)
            : threads_(size)
        {
            for (auto& thread : threads_)
                thread = std::thread([this]
                    { run(); });
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
                tasks_.push([this]
                    { stop_ = true; });

            for (auto& thread : threads_)
                thread.join();
        }


        template <typename F>
        auto submit(F&& f) -> std::future<decltype(f())>
        {
            using ResultT = decltype(f());
            auto pt = std::make_shared<std::packaged_task<ResultT()>>(std::forward<F>(f));
            std::future<ResultT> f_result = pt->get_future();
            tasks_.push([pt] { (*pt)(); });
            return f_result;
        }

    private:
        std::vector<std::thread> threads_;
        ThreadSafeQueue<Task> tasks_;
        std::atomic<bool> stop_ {false};

        void run()
        {
            while (!stop_)
            {
                Task task;
                tasks_.pop(task);
                task();
            }
        }
    };
}

void background_work(size_t id, const std::string& text, std::chrono::milliseconds delay)
{
    std::cout << "bw#" << id << " has started..." << std::endl;

    for (const auto& c : text)
    {
        std::cout << "bw#" << id << ": " << c << std::endl;

        std::this_thread::sleep_for(delay);
    }

    std::cout << "bw#" << id << " is finished..." << std::endl;
}

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

int main()
{
    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    ver_2_0::ThreadPool thd_pool(10);

    for (int i = 1; i <= 30; ++i)
    {
        thd_pool.submit([=]
            { background_work(i, text, 250ms); });
    }

    std::vector<std::pair<int, std::future<int>>> f_squares;

    for(int i = 1; i < 100; ++i)
    {
        f_squares.emplace_back(i, thd_pool.submit([i] { return calculate_square(i); }));
    }

    for(auto& fs : f_squares)
    {
        std::cout << fs.first << " - ";
        try
        {
            int result = fs.second.get();
            std::cout << result << "\n";
        }
        catch(const std::exception& e)
        {
            std::cout << e.what() << '\n';
        }        
    }

    std::cout << "Main thread ends..." << std::endl;
}
