/************
 * mkdir build_wsl
 * cd build_wsl
 * cmake ..
 * make
 **************/

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <future>

using namespace std;

int main()
{
    const long N = 100'000'000;

    {
        //////////////////////////////////////////////////////////////////////////////
        // single thread

        cout << "Pi calculation started! Single thread..." << endl;
        const auto start = chrono::high_resolution_clock::now();

        long hits = 0;

        std::mt19937_64 rand_engine {std::hash<std::thread::id> {}(std::this_thread::get_id())};
        std::uniform_real_distribution<double> rand_distr {0.0, 1.0};

        for (long n = 0; n < N; ++n)
        {
            double x = rand_distr(rand_engine);
            double y = rand_distr(rand_engine);
            if (x * x + y * y < 1)
                hits++;
        }

        const double pi = static_cast<double>(hits) / N * 4;

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }

    //////////////////////////////////////////////////////////////////////////////

    std::cout << "###################################################################################" << std::endl;

    //////////////////////////////////////////////////////////////////////////////
    // multithreading
    {
        cout << "Pi calculation started! Multithreading..." << endl;
        const auto start = chrono::high_resolution_clock::now();

        const size_t thread_count = std::max(1u, std::thread::hardware_concurrency());

        const auto N_per_thread = N / thread_count;

        std::vector<std::thread> threads(thread_count);
        std::vector<long> partial_hits(thread_count);

        auto run = [](long count, long& hits)
        {
            std::mt19937_64 rand_engine {std::hash<std::thread::id> {}(std::this_thread::get_id())};
            std::uniform_real_distribution<double> rand_distr {0.0, 1.0};

            for (long n = 0; n < count; ++n)
            {
                double x = rand_distr(rand_engine);
                double y = rand_distr(rand_engine);
                if (x * x + y * y < 1)
                    hits++;
            }
        };

        for (int i = 0; i < threads.size(); ++i)
        {
            threads[i] = std::thread(run, N_per_thread, std::ref(partial_hits[i]));
        }

        for (auto& thd : threads)
            thd.join();

        const long hits = std::accumulate(partial_hits.begin(), partial_hits.end(), 0L);

        const double pi = static_cast<double>(hits) / N * 4;

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }
    //////////////////////////////////////////////////////////////////////////////

    std::cout << "###################################################################################" << std::endl;

    //////////////////////////////////////////////////////////////////////////////
    // multithreading
    {
        cout << "Pi calculation started! Multithreading with local hits counter..." << endl;
        const auto start = chrono::high_resolution_clock::now();

        const size_t thread_count = std::max(1u, std::thread::hardware_concurrency());

        const auto N_per_thread = N / thread_count;

        std::vector<std::thread> threads(thread_count);
        std::vector<long> partial_hits(thread_count);

        auto run = [](long count, long& hits)
        {
            long localHits = 0;

            std::mt19937_64 rand_engine {std::hash<std::thread::id> {}(std::this_thread::get_id())};
            std::uniform_real_distribution<double> rand_distr {0.0, 1.0};

            for (long n = 0; n < count; ++n)
            {
                double x = rand_distr(rand_engine);
                double y = rand_distr(rand_engine);
                if (x * x + y * y < 1)
                    localHits++;
            }

            hits += localHits;
        };

        for (int i = 0; i < threads.size(); ++i)
        {
            threads[i] = std::thread(run, N_per_thread, std::ref(partial_hits[i]));
        }

        for (auto& thd : threads)
            thd.join();

        const long hits = std::accumulate(partial_hits.begin(), partial_hits.end(), 0L);

        const double pi = static_cast<double>(hits) / N * 4;

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }
    //////////////////////////////////////////////////////////////////////////////

    std::cout << "###################################################################################" << std::endl;

    //////////////////////////////////////////////////////////////////////////////
    // multithreading with atomic
    {
        cout << "Pi calculation started! Multithreading with atomic hits..." << endl;
        const auto start = chrono::high_resolution_clock::now();

        const size_t thread_count = std::max(1u, std::thread::hardware_concurrency());

        const auto N_per_thread = N / thread_count;

        std::atomic<long> hits {};

        auto run = [&hits](long count)
        {
            std::mt19937_64 rand_engine {std::hash<std::thread::id> {}(std::this_thread::get_id())};
            std::uniform_real_distribution<double> rand_distr {0.0, 1.0};

            for (long n = 0; n < count; ++n)
            {
                double x = rand_distr(rand_engine);
                double y = rand_distr(rand_engine);
                if (x * x + y * y < 1)
                {
                    hits++;
                    // hits.fetch_add(1, std::memory_order_relaxed);
                }
            }
        };

        std::vector<std::thread> threads(thread_count);

        for (auto& thd : threads)
        {
            thd = std::thread(run, N_per_thread);
        }

        for (auto& thd : threads)
            thd.join();

        const double pi = static_cast<double>(hits) / N * 4;

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }
    //////////////////////////////////////////////////////////////////////////////

    std::cout << "###################################################################################" << std::endl;

    //////////////////////////////////////////////////////////////////////////////
    // multithreading with futures
    {
        cout << "Pi calculation started! Multithreading with atomic hits..." << endl;
        const auto start = chrono::high_resolution_clock::now();

        const size_t thread_count = std::max(1u, std::thread::hardware_concurrency());

        const auto N_per_thread = N / thread_count;

        auto count_hits = [](long count)
        {
            long hits{};
            std::mt19937_64 rand_engine {std::hash<std::thread::id> {}(std::this_thread::get_id())};
            std::uniform_real_distribution<double> rand_distr {0.0, 1.0};

            for (long n = 0; n < count; ++n)
            {
                double x = rand_distr(rand_engine);
                double y = rand_distr(rand_engine);
                if (x * x + y * y < 1)
                {
                    hits++;                    
                }
            }

            return hits;
        };

        std::vector<std::future<long>> partial_hits(thread_count);

        for (auto& ph : partial_hits)
        {
            ph = std::async(std::launch::async, count_hits, N_per_thread);
        }

        long hits{};
        for(auto& ph : partial_hits)
        {
            hits += ph.get();
        }
        
        //long hits = std::accumulate(partial_hits.begin(), partial_hits.end(), 0L, [](long total_hits, std::future<long>& fh) { return total_hits + fh.get(); });

        const double pi = static_cast<double>(hits) / N * 4;

        const auto end = chrono::high_resolution_clock::now();
        const auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

        cout << "Pi = " << pi << endl;
        cout << "Elapsed = " << elapsed_time << "ms" << endl;
    }
    //////////////////////////////////////////////////////////////////////////////
}
