#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals;

namespace BusyWaitWithMutex
{
    class Data
    {
        std::vector<int> data_;
        bool is_data_ready_ {false};
        std::mutex mtx_is_data_ready_;

    public:
        void produce()
        {
            std::cout << "Preparing data..." << std::endl;
            data_.resize(1000);
            std::random_device rd;
            std::mt19937_64 rnd_engine {rd()};
            std::uniform_int_distribution<int> rnd_distr(0, 100);
            std::generate(data_.begin(), data_.end(), [&]
                { return rnd_distr(rnd_engine); });
            std::this_thread::sleep_for(2s);
            std::cout << "Data ready..." << std::endl;

            {
                std::lock_guard<std::mutex> lk {mtx_is_data_ready_};
                is_data_ready_ = true;
            }
        }

        void consume(int id)
        {

            while (true)
            {
                std::lock_guard<std::mutex> lk {mtx_is_data_ready_};
                if (is_data_ready_)
                    break;
            }

            int sum = std::accumulate(data_.begin(), data_.end(), 0);
            std::cout << "Consumer#" << id << " - sum: " << sum << std::endl;
        }
    };
}

namespace BusyWaitWithAtomic
{
    class Data
    {
        std::vector<int> data_;
        std::atomic<bool> is_data_ready_ {false};

    public:
        void produce()
        {                        
            std::cout << "Preparing data..." << std::endl;
            data_.resize(1000);
            std::random_device rd;
            std::mt19937_64 rnd_engine {rd()};
            std::uniform_int_distribution<int> rnd_distr(0, 100);
            std::generate(data_.begin(), data_.end(), [&]
                { return rnd_distr(rnd_engine); });
            std::this_thread::sleep_for(2s);
            std::cout << "Data ready..." << std::endl;

            // XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
            is_data_ready_.store(true, std::memory_order::memory_order_release);
        }

        void consume(int id)
        {
            assert(is_data_ready_.is_lock_free());
            static_assert(is_data_ready_.is_always_lock_free);

            // ###########################################
            while (!is_data_ready_.load(std::memory_order_acquire))
            {
            }

            int sum = std::accumulate(data_.begin(), data_.end(), 0);
            std::cout << "Consumer#" << id << " - sum: " << sum << std::endl;
        }
    };
}

int main()
{
    using namespace BusyWaitWithMutex;

    std::cout << "Main thread starts..." << std::endl;
    const std::string text = "Hello Threads";

    {
        std::shared_ptr<Data> data = std::make_shared<Data>();

        std::thread thd_producer {[data]
            { data->produce(); }};
        std::thread thd_consumer_1 {[data]
            { data->consume(1); }};
        std::thread thd_consumer_2 {[data]
            { data->consume(2); }};

        data.reset();

        thd_producer.join();
        thd_consumer_1.join();
        thd_consumer_2.join();
    }

    std::cout << "Main thread ends..." << std::endl;
}
