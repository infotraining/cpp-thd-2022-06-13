#ifndef JOINING_THREAD_HPP
#define JOINING_THREAD_HPP

#include <thread>

namespace ext
{
    template <typename T1, typename T2>
    constexpr bool is_similar_v = std::is_same<std::decay_t<T1>, std::decay_t<T2>>::value;

    class joining_thread
    {
    public:
        joining_thread() = default;

        template<typename TFunction, typename... TArgs, typename = std::enable_if_t<!is_similar_v<joining_thread, TFunction>>>
        joining_thread(TFunction&& f, TArgs&&... args) : thd_{std::forward<TFunction>(f), std::forward<TArgs>(args)...}
        {}

        joining_thread(const joining_thread&) = delete;
        joining_thread& operator=(const joining_thread&) = delete;
        joining_thread(joining_thread&&) = default;
        joining_thread& operator=(joining_thread&&) = default;

        void join()
        {
            thd_.join();
        }

        void detach()
        {
            thd_.detach();
        }

        bool joinable() const 
        {
            return thd_.joinable();
        }

        std::thread::id get_id() const
        {
            return thd_.get_id();
        }

        std::thread::native_handle_type native_handle()
        {
            return thd_.native_handle();
        }

        ~joining_thread() noexcept
        {
            if (thd_.joinable())
                thd_.join();
        }
    private:
        std::thread thd_;
    };
}


#endif