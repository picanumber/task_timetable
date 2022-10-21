// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "doctest/doctest.h"
#include "scheduler.h"
#include "test_utils.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

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

#ifdef NDEBUG // Release mode specific since realistic timings are required.
TEST_CASE("Detached task tokens")
{
    const auto tol = 5ms;
    const int reps = 10;
    std::atomic_size_t callCount{0};
    auto fun = [&callCount] {
        ++callCount;
        return ttt::Result::Finished;
    };

    ttt::CallScheduler plan;
    {
        auto start = test::now();
        for (int i(0); i < reps; ++i)
        {
            plan.add(fun, 1us, false).detach();
        }

        while (reps != callCount.load())
        {
            WARN_MESSAGE(test::delta(start) < tol, "Tasks not executed");
        }
    }

    {
        callCount = 0;
        auto start = test::now();
        for (int i(0); i < reps; ++i)
        {
            plan.add(fun, 1us, true).detach();
        }

        while (reps != callCount.load())
        {
            WARN_MESSAGE(test::delta(start) < tol, "Tasks not executed");
        }
    }

    ttt::CallScheduler plan2(true, 2);
    {
        callCount = 0;
        auto start = test::now();
        for (int i(0); i < reps; ++i)
        {
            plan2.add(fun, 1us, false).detach();
        }

        while (reps != callCount.load())
        {
            WARN_MESSAGE(test::delta(start) < tol, "Tasks not executed");
        }
    }

    {
        callCount = 0;
        auto start = test::now();
        for (int i(0); i < reps; ++i)
        {
            plan2.add(fun, 1us, true).detach();
        }

        while (reps != callCount.load())
        {
            WARN_MESSAGE(test::delta(start) < tol, "Tasks not executed");
        }
    }
}
#endif

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

// This check merely checks correctness of task repetition. Intervals are
// purposely blown-up since it runs on sanitizer mode as well
static void CheckRepetition(std::string const &prefix, bool compensate,
                            unsigned nWorkers)
{
    ttt::CallScheduler plan(compensate, nWorkers);
    const size_t reps{5};
    std::atomic_size_t callCount{0};

    auto fun = [&] {
        ++callCount;
        return callCount < reps ? ttt::Result::Repeat : ttt::Result::Finished;
    };

    auto tkn = plan.add(fun, 10us, true);
    std::this_thread::sleep_for(50ms);

    CHECK_MESSAGE(reps == callCount.load(),
                  (prefix + "Calls should have finished"));

    std::this_thread::sleep_for(10ms);
    CHECK_MESSAGE(reps == callCount.load(),
                  (prefix + "No further repetitions should happen"));
}

TEST_CASE("Check repetition")
{
    CheckRepetition("plan1: ", true, 1);
    CheckRepetition("plan2: ", false, 1);
    CheckRepetition("plan3: ", true, 2);
    CheckRepetition("plan4: ", false, 2);
    CheckRepetition("plan5: ", true, 10);
    CheckRepetition("plan6: ", false, 10);
}

#ifdef NDEBUG // Release mode specific since realistic timings are required.
TEST_CASE("Check granularity")
{
    auto tol = 150us; // 150 microseconds is the accepted TOTAL drift time. By
                      // TOTAL we mean that this inconsistency is not added (or
                      // multiplied) but taken as the upper limit of cummulative
                      // error for schedulers that account for task execution
                      // time in the calculation of intervals.
    std::atomic_bool finished{false};
    const std::size_t callReps = 100;
    std::vector<std::chrono::steady_clock::time_point> callTimes;
    callTimes.reserve(callReps);

    auto marker = [&, cur = std::size_t(0)]() mutable {
        callTimes.emplace_back(std::chrono::steady_clock::now());

        ttt::Result ret = ttt::Result::Repeat;
        if (callReps == ++cur)
        {
            finished = true;
            ret = ttt::Result::Finished;
        }

        return ret;
    };

    ttt::CallScheduler plan;
    const auto start = test::now();
    plan.add(marker, 10ms, false).detach();

    while (!finished)
    {
        WARN_MESSAGE(test::delta<std::chrono::microseconds>(start).count() <=
                         callReps * (10'000us).count() + tol.count(),
                     "Scheduled tasks did not complete in time");
    }

    REQUIRE_MESSAGE(callTimes.size() == callReps, "Invalid call count");

    for (std::size_t i = 0; i < callReps; ++i)
    {
        WARN_MESSAGE(test::delta<std::chrono::microseconds>(start, callTimes[i])
                             .count() <=
                         (i + 1) * (10'000us).count() + (2 * tol).count(),
                     "Intermediate time point exceeds tolerance");
    }
}
#endif
