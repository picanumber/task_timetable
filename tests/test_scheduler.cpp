#include "doctest/doctest.h"
#include "scheduler.h"
#include "test_utils.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

using namespace std::chrono_literals;

TEST_CASE("Construction")
{
    // Default.
    CHECK_NOTHROW(ttt::CallScheduler plan);
    // Account for task execution time.
    CHECK_NOTHROW(ttt::CallScheduler plan(true, 2));
    CHECK_NOTHROW(ttt::CallScheduler plan(true, 5));
    CHECK_NOTHROW(ttt::CallScheduler plan(true, 500));
    // Do not account for task execution time.
    CHECK_NOTHROW(ttt::CallScheduler plan(false));
    CHECK_NOTHROW(ttt::CallScheduler plan(false, 2));
    CHECK_NOTHROW(ttt::CallScheduler plan(false, 5));
    CHECK_NOTHROW(ttt::CallScheduler plan(false, 500));
    // Without workers assigned.
    CHECK_THROWS_WITH_AS(ttt::CallScheduler plan(true, 0);
                         , ttt::detail::kErrorNoWorkersInScheduler,
                         std::runtime_error);
    CHECK_THROWS_WITH_AS(ttt::CallScheduler plan(false, 0);
                         , ttt::detail::kErrorNoWorkersInScheduler,
                         std::runtime_error);
}

TEST_CASE("Immediatelly cancelled tasks")
{
    const int reps = 100;
    std::atomic_size_t callCount{0};
    auto fun = [&callCount] {
        ++callCount;
        return ttt::Result::Finished;
    };

    {
        ttt::CallScheduler plan;
        for (int i(0); i < reps; ++i)
        {
            auto token = plan.add(fun, 1ms, false);
            // Destruction of token cancels the added task.
        }
        CHECK_MESSAGE(callCount < reps, "Cancellation failed");
    }

    {
        callCount = 0;
        ttt::CallScheduler plan;
        for (int i(0); i < reps; ++i)
        {
            auto token = plan.add(fun, 1ms, true);
            // Destruction of token cancels the added task.
        }
        CHECK_MESSAGE(callCount <= reps, "Cancellation failed");
    }

    {
        callCount = 0;
        ttt::CallScheduler plan(true, 2);
        for (int i(0); i < reps; ++i)
        {
            auto token = plan.add(fun, 1ms, false);
            // Destruction of token cancels the added task.
        }
        CHECK_MESSAGE(callCount < reps, "Cancellation failed");
    }

    {
        callCount = 0;
        ttt::CallScheduler plan(true, 2);
        for (int i(0); i < reps; ++i)
        {
            auto token = plan.add(fun, 1ms, true);
            // Destruction of token cancels the added task.
        }
        CHECK_MESSAGE(callCount <= reps, "Cancellation failed");
    }
}

TEST_CASE("Detached task tokens")
{
    const int reps = 100;
    std::atomic_size_t callCount{0};
    auto fun = [&callCount] {
        ++callCount;
        return ttt::Result::Finished;
    };

    {
        ttt::CallScheduler plan;
        auto start = test::now();
        for (int i(0); i < reps; ++i)
        {
            plan.add(fun, 1us, false).detach();
        }

        while (reps != callCount.load())
        {
            REQUIRE_MESSAGE(test::delta(start) < 1ms, "Tasks not executed");
            std::this_thread::yield();
        }
    }

    {
        callCount = 0;
        ttt::CallScheduler plan;
        auto start = test::now();
        for (int i(0); i < reps; ++i)
        {
            plan.add(fun, 1us, true).detach();
        }

        while (reps != callCount.load())
        {
            REQUIRE_MESSAGE(test::delta(start) < 1ms, "Tasks not executed");
            std::this_thread::yield();
        }
    }

    {
        callCount = 0;
        ttt::CallScheduler plan(true, 2);
        auto start = test::now();
        for (int i(0); i < reps; ++i)
        {
            plan.add(fun, 1us, false).detach();
        }

        while (reps != callCount.load())
        {
            REQUIRE_MESSAGE(test::delta(start) < 1ms, "Tasks not executed");
            std::this_thread::yield();
        }
    }

    {
        callCount = 0;
        ttt::CallScheduler plan(true, 2);
        auto start = test::now();
        for (int i(0); i < reps; ++i)
        {
            plan.add(fun, 1us, true).detach();
        }

        while (reps != callCount.load())
        {
            REQUIRE_MESSAGE(test::delta(start) < 1ms, "Tasks not executed");
            std::this_thread::yield();
        }
    }
}

TEST_CASE("Check token expiration")
{
    std::atomic_bool allowCall{false};
    std::atomic_size_t callCount{0};

    auto fun = [&] {
        while (!allowCall)
        {
            // Spin until the call is allowed.
        }
        ++callCount;
        return ttt::Result::Repeat;
    };

    ttt::CallScheduler plan;
    {
        auto tkn = plan.add(fun, 1us, true);
        std::this_thread::sleep_for(100us);

        CHECK_MESSAGE(0 == callCount.load(), "Call should not be allowed here");
        allowCall = true; // Allow the call to run, hence to be rescheduled,
                          // but immediatelly destroy the token.
    }
    const auto callCountAfterTokenDestruction = callCount.load();

    std::this_thread::sleep_for(100us);
    REQUIRE_MESSAGE(callCountAfterTokenDestruction == callCount.load(),
                    "No invocations allowed after token destruction");
}

TEST_CASE("Check repetition")
{
    const size_t reps{5};
    std::atomic_size_t callCount{0};

    auto fun = [&] {
        ++callCount;
        return callCount < reps ? ttt::Result::Repeat : ttt::Result::Finished;
    };

    ttt::CallScheduler plan;
    auto tkn = plan.add(fun, 10us, true);
    std::this_thread::sleep_for(500us);

    CHECK_MESSAGE(reps == callCount.load(), "Call should have finished");

    std::this_thread::sleep_for(500us);
    REQUIRE_MESSAGE(reps == callCount.load(),
                    "No furhter repetitions should happen");
}

TEST_CASE("Check granularity")
{
    // TODO(picanumber): Make sure the scheduler can provide results with
    // reasonable tolerance comared to the specified schedule.
}
