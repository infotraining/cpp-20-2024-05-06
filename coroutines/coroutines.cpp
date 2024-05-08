#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <coroutine>
#include <thread>
#include <syncstream>

using namespace std::literals;

class TaskResumer
{
public:
    struct promise_type
    {
        int value_;

        TaskResumer get_return_object()
        {
            return TaskResumer{std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        
        auto initial_suspend() -> std::suspend_always
        {
            return {};
        }
        
        auto final_suspend() noexcept -> std::suspend_always
        {
            return {};
        }

        void return_value(auto expr)
        {
            value_ = expr;
        }

        void unhandled_exception()
        {
            std::terminate();
        }
    };
public:
    TaskResumer(std::coroutine_handle<promise_type> coro_hndl) : coro_hndl_{coro_hndl}
    {}

    TaskResumer(const TaskResumer&) = delete;
    TaskResumer& operator=(const TaskResumer&) = delete;
    TaskResumer(TaskResumer&&) = delete;
    TaskResumer& operator=(TaskResumer&&) = delete;

    ~TaskResumer() 
    {
        if (coro_hndl_)
            coro_hndl_.destroy();
    }

    bool resume() const
    {
         if (!coro_hndl_ || coro_hndl_.done())
            return false;

        coro_hndl_.resume(); // resuming suspended coroutine

        return !coro_hndl_.done();
    }

    int get_value() 
    {
        return coro_hndl_.promise().value_;
    }

private:
    std::coroutine_handle<promise_type> coro_hndl_;
};

TaskResumer foo(int max)
{
    std::string str = "HELLO";
    std::cout << "..coro(START, " << max << ")\n";

    for (int value = 1; value <= max; ++value)
    {
        std::cout << "..coro(" << value << ", " << max << ")\n";
        ///////////////////////////////////////////////////////
        co_await std::suspend_always{}; // SUSPENSION POINT
    }

    std::cout << "..coro(END, " << max << ")\n";

    co_return 42;
}

TEST_CASE("first coroutine")
{
    TaskResumer task = foo(5);

    std::cout << "--------------------\n";
    
    while(task.resume())
    {
        std::cout << "foo() suspended...\n";
    }

    std::cout << "--------------------\n";

    std::cout << "foo() done\n";

    int value = task.get_value();

    std::cout << "value: " << value << "\n";
}

inline auto sync_out(std::ostream& out = std::cout)
{
    return std::osyncstream{out};
}

struct FireAndForget
{
    struct promise_type
    {
        FireAndForget get_return_object()
        {
            return {};
        }

        std::suspend_never initial_suspend() const noexcept
        {
            sync_out() << "...Initial suspension point...\n";
            return {};
        }

        std::suspend_never final_suspend() const noexcept
        {
            sync_out() << "...Final suspension point...\n";
            return {};
        }

        void unhandled_exception() { std::terminate(); }

        void return_void() const noexcept
        {
            sync_out() << "...Exiting the coroutine...\n";
        }
    };
};

auto resume_on_new_thread()
{
    // Awaiter
    struct ResumeOnNewThreadAwaiter : std::suspend_always
    {
        void await_suspend(std::coroutine_handle<> coroutine_hndl)
        {
            std::thread([coroutine_hndl] { coroutine_hndl.resume(); }).detach();
        }

        std::thread::id await_resume() const noexcept
        {
            return std::this_thread::get_id();
        }
    };

    return ResumeOnNewThreadAwaiter{};
}

FireAndForget coro_on_many_threads(int id)
{
    const int max_step = 3;
    int step = 1;
    sync_out() << "Coro#" << id << " - Part#" << step << "/" << max_step << " - started on THD#" << std::this_thread::get_id() << "\n";

    std::thread::id thd_id = co_await resume_on_new_thread();

    ++step;
    sync_out() << "Coro#" << id << " - Part#" << step << "/" << max_step << " - continues on THD#" << std::this_thread::get_id() << "\n";
    assert(thd_id == std::this_thread::get_id());

    thd_id = co_await resume_on_new_thread(); /////////////////////////////////// context switch

    ++step;
    sync_out() << "Coro#" << id << " - Part#" << step << "/" << max_step << " - ends on THD#" << std::this_thread::get_id() << "\n";
    assert(thd_id == std::this_thread::get_id());
}

TEST_CASE("resume part of the function on the new thread")
{
    using namespace std::literals;

    FireAndForget tsk1 = coro_on_many_threads(1);

    std::this_thread::sleep_for(5s);
}