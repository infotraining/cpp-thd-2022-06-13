#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace std::literals; // dzięki temu nie będziemy wiedzieć jawnie co jest wołene z tego namespac'a ;) // odszczekuje

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

class BackgroundWork
{
    const int id_;
    const std::string text_;

public:
    BackgroundWork(int id, std::string text)
        : id_ {id}
        , text_ {std::move(text)}
    {
    }

    void operator()(std::chrono::milliseconds delay) const
    {
        std::cout << "BW#" << id_ << " has started..." << std::endl;

        for (const auto& c : text_)
        {
            std::cout << "BW#" << id_ << ": " << c << std::endl;

            std::this_thread::sleep_for(delay);
        }

        std::cout << "BW#" << id_ << " is finished..." << std::endl;
    }
};

std::thread::id main_thread_id;

void update_counter(int& counter)
{
    if (std::this_thread::get_id() == main_thread_id)
        std::cout << "update counter is running in main_thread" << std::endl;
    else
        for (int i = 0; i < 1'000'000; ++i)
            ++counter;
}

int main()
{
    main_thread_id = std::this_thread::get_id();

    std::cout << "Main thread starts..." << std::endl;
    const auto text = "Hello Threads"s;

    std::thread thd_empty;
    std::cout << "thd_empty id = " << thd_empty.get_id() << "\n";
    if (thd_empty.joinable())
        thd_empty.join();

    std::thread thd_1 {&background_work, 1, std::cref(text), 500ms};
    int id = 2;
    std::thread thd_2 {[id] { background_work(id, "Multithreading", 250ms); }};
    BackgroundWork bw(3, "C++20");
    std::jthread thd_3 {std::ref(bw), 100ms};
    std::thread thd_5 {&background_work, 665, "DEAMON", 1s};
    thd_5.detach();

    int counter {};
    std::thread thd_counter_1 {update_counter, std::ref(counter)};
    thd_counter_1.join();

    std::thread thd_counter_2 {[&counter] { update_counter(counter); }};
    thd_counter_2.join();

    std::cout << "counter: " << counter << "\n";

    if (thd_1.joinable())
        thd_1.join();

    thd_2.join();
    std::cout << "thd_2 id = " << thd_2.get_id() << std::endl;

    thd_2 = std::thread {background_work, 4, "new thd", 100ms};
    thd_2.join();

    std::cout << "Main thread ends..." << std::endl;
}
