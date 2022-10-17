#include "buffered_worker.h"
#include "doctest/doctest.h"
#include "test_utils.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

TEST_CASE("Construction")
{
    CHECK_NOTHROW(ttt::BufferedWorker<std::function<void()>> worker);
    CHECK_NOTHROW(ttt::BufferedWorker<std::function<void()>> worker(1));
    CHECK_NOTHROW(ttt::BufferedWorker<std::function<void()>> worker(100));
    CHECK_NOTHROW(ttt::BufferedWorker<std::function<void()>> worker(1'000));

    CHECK_THROWS_WITH_AS(ttt::BufferedWorker<std::function<void()>> worker(0);
                         , ttt::kErrorWorkerSize, std::runtime_error);
}

TEST_CASE("Execute All added tasks")
{
    using task_t = std::function<void()>;

    ttt::BufferedWorker<task_t> worker;

    const int repetitions{200};
    std::atomic_int totalCalls{0};
    task_t incr = [&totalCalls] { totalCalls += 1; };

    for (int i(0); i < repetitions; ++i)
    {
        worker.add(incr);
    }

    auto start = test::now();
    while (repetitions != totalCalls.load())
    {
        REQUIRE_MESSAGE(test::delta(start).count() < 10, "Tasks not executed");
        std::this_thread::yield();
    }
}

TEST_CASE("Execute All added tasks - Kill before destroy")
{
    using task_t = std::function<void()>;

    ttt::BufferedWorker<task_t> worker;

    const int repetitions{200};
    std::atomic_int totalCalls{0};
    task_t incr = [&totalCalls] { totalCalls += 1; };

    for (int i(0); i < repetitions; ++i)
    {
        worker.add(incr);
    }

    auto start = test::now();
    while (repetitions != totalCalls.load())
    {
        REQUIRE_MESSAGE(test::delta(start).count() < 10, "Tasks not executed");
        std::this_thread::yield();
    }

    CHECK_NOTHROW(worker.kill());
}

TEST_CASE("Execute tasks until worker destruction")
{
    using task_t = std::function<void()>;

    const int repetitions{100};
    std::atomic_int totalCalls{0};
    task_t incr = [&totalCalls] {
        std::this_thread::sleep_for(test::k10us);
        totalCalls += 1;
    };

    {
        ttt::BufferedWorker<task_t> worker;
        for (int i(0); i < repetitions; ++i)
        {
            worker.add(incr);
        }
    }

    REQUIRE_MESSAGE(totalCalls < repetitions,
                    "Worker should have dropped tasks");
}

TEST_CASE("Execute tasks until destruction of non-dropping worker")
{
    using task_t = std::function<void()>;

    const int repetitions{100};
    std::atomic_int totalCalls{0};
    task_t incr = [&totalCalls] {
        std::this_thread::sleep_for(test::k10us);
        totalCalls += 1;
    };

    {
        ttt::BufferedWorker<task_t> worker(1'000, false);
        // Worker is not allowed to drop tasks    ^^^^^
        for (int i(0); i < repetitions; ++i)
        {
            worker.add(incr);
        }
    }

    REQUIRE_MESSAGE(totalCalls == repetitions,
                    "Worker is not allowed to drop tasks");
}

TEST_CASE("Execute tasks until worker destruction - Kill before destroy")
{
    using task_t = std::function<void()>;

    ttt::BufferedWorker<task_t> worker;

    const int repetitions{200};
    std::atomic_int totalCalls{0};
    task_t incr = [&totalCalls] { totalCalls += 1; };

    for (int i(0); i < repetitions; ++i)
    {
        worker.add(incr);
    }

    REQUIRE_MESSAGE(totalCalls <= repetitions, "Irregular task execution");
    CHECK_NOTHROW(worker.kill());
}

TEST_CASE("Execute no task - Kill before add")
{
    using task_t = std::function<void()>;

    ttt::BufferedWorker<task_t> worker;

    const int repetitions{200};
    std::atomic_int totalCalls{0};
    task_t incr = [&totalCalls] { totalCalls += 1; };

    REQUIRE_NOTHROW(worker.kill());
    for (int i(0); i < repetitions; ++i)
    {
        REQUIRE_MESSAGE(false == worker.add(incr),
                        "Dead worker accepted a task");
    }

    std::this_thread::yield();
    REQUIRE_MESSAGE(0 == totalCalls.load(), "Task executed on dead worker");
}
