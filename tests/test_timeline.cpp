// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "doctest/doctest.h"
#include "task_timetable/scheduler.h"
#include "task_timetable/timeline.h"
#include "test_utils.h"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

using ttt::Timeline;
using ttt::TimerState;

using namespace std::chrono_literals;

static void DummyTimerAction(TimerState const &)
{
}

TEST_CASE("Construction")
{
    std::vector<std::string> entityStrings{
        "timer:t4:100:500:500:1:1",
        "timer:t3:100:500:500:0:1",
        "timer:t1:1000:10000:10000:1:1",
        "timer:t2:1000:10000:10000:0:1",
    };

    CHECK_NOTHROW(Timeline schedule);
    CHECK_NOTHROW(Timeline schedule({entityStrings.at(0)}, DummyTimerAction));
    CHECK_NOTHROW(Timeline schedule({entityStrings.at(1)}, DummyTimerAction));
    CHECK_NOTHROW(Timeline schedule({entityStrings.at(2)}, DummyTimerAction));

    Timeline demoTimeline(entityStrings, {});
    auto serialized = demoTimeline.serialize(true, true, true);
    CHECK_MESSAGE(entityStrings.size() == serialized.size(),
                  "Entities improperly serialized");
    for (auto const &ent : serialized)
    {
        CHECK_MESSAGE(entityStrings.end() != std::find(entityStrings.begin(),
                                                       entityStrings.end(),
                                                       ent),
                      (ent + ": Entity improperly serialized"));
    }

    CHECK_THROWS_AS(Timeline schedule(
                        {"junk:string:that:does:not:designate:timeline:entry"},
                        DummyTimerAction);
                    , std::runtime_error);
    CHECK_THROWS_AS(Timeline schedule({""}, DummyTimerAction);, std::exception);
}

TEST_CASE("Expiring timer")
{
    Timeline schedule;

    std::atomic_size_t callCount{0};
    auto timerAction = [&callCount](TimerState const &) { ++callCount; };

    REQUIRE_MESSAGE(
        schedule.timerAdd("t1", 10ms, 100ms, false, timerAction, false),
        "Unable to add timer");

    auto start = test::now();
    while (callCount < 10)
    {
        if (test::delta(start) > 5s)
        {
            FAILED_REQUIREMENT("Timer not ticking in tempo");
        }
    }

    REQUIRE_MESSAGE(callCount == 10, "Wrong number of iterations");
    REQUIRE_MESSAGE(callCount == 10, "Further calls should be impossible");
    REQUIRE_MESSAGE(callCount == 10, "Further calls should be impossible");
}

TEST_CASE("Repeating timer")
{
    Timeline schedule;

    std::atomic_size_t callCount{0};
    auto timerAction = [&callCount](TimerState const &) { ++callCount; };

    REQUIRE_MESSAGE(
        schedule.timerAdd("t1", 10ms, 100ms, true, timerAction, false),
        "Unable to add timer");

    auto start = test::now();
    while (callCount < 11)
    {
        if (test::delta(start) > 5s)
        {
            FAILED_REQUIREMENT("Timer not ticking in tempo");
        }
    }

    REQUIRE_MESSAGE(callCount >= 11, "Wrong number of iterations");
    REQUIRE_MESSAGE(callCount >= 11, "Further calls should be possible");
    REQUIRE_MESSAGE(callCount >= 11, "Further calls should be possible");
}

TEST_CASE("Two timers")
{
    Timeline schedule;

    std::atomic_size_t sum{0}, c1{0}, c2{0};

    auto t1 = [&sum, &c1](TimerState const &) {
        ++sum;
        ++c1;
        return ttt::Result::Repeat;
    };
    auto t2 = [&sum, &c2](TimerState const &) {
        ++sum;
        ++c2;
        return ttt::Result::Repeat;
    };

    REQUIRE_MESSAGE(schedule.timerAdd("t1", 10ms, 100ms, false, t1, false),
                    "Unable to add timer 1");
    REQUIRE_MESSAGE(schedule.timerAdd("t2", 10ms, 100ms, false, t2, false),
                    "Unable to add timer 2");

    auto start = test::now();
    while (sum < 20)
    {
        if (test::delta(start) > 5s)
        {
            FAILED_REQUIREMENT("Timer not ticking in tempo");
        }
    }

    REQUIRE_MESSAGE(sum == 20, "Wrong number of iterations");

    REQUIRE_MESSAGE(c1 == 10, "Wrong number of iterations");
    REQUIRE_MESSAGE(c2 == 10, "Wrong number of iterations");

    REQUIRE_MESSAGE(sum == 20, "Further calls should be impossible");
}

TEST_CASE("Timer remove")
{
    Timeline schedule;

    std::atomic_size_t callCount{0};
    auto timerAction = [&callCount](TimerState const &) { ++callCount; };

    const std::string timerName("t1");
    REQUIRE_MESSAGE(
        schedule.timerAdd(timerName, 500ms, 10s, true, timerAction, true),
        "Unable to add timer");

    auto start = test::now();
    while (0 == callCount)
    {
        if (test::delta(start) > 100ms)
        {
            FAILED_REQUIREMENT("Timer should tick on addition");
        }
    }
    REQUIRE_MESSAGE(callCount.load() == 1,
                    "Period does not justify extra tick");

    REQUIRE_MESSAGE(schedule.timerRemove(timerName), "Timer removal error");

    auto state = schedule.serialize(true, false, false);
    REQUIRE_MESSAGE(state.empty(), "No elements should exist");

    while (1 == callCount)
    {
        if (test::delta(start) > 500ms)
        {
            return; // Verified the timer did not tick again.
        }
    }
    FAILED_REQUIREMENT("Timer should not tick again");
}

TEST_CASE("Timer reset")
{
    Timeline schedule;

    std::mutex mtx;
    std::atomic_size_t callCount{0};
    std::atomic_bool resetCalled{false};

    auto timerAction = [&mtx, &callCount, &resetCalled](TimerState const &s) {
        std::lock_guard<std::mutex> criticalSection(mtx);

        if (0 == callCount)
        {
            REQUIRE_MESSAGE(
                s.remaining.load().count() == s.duration.count(),
                "Timers that run on addition start with remaining == duration");
        }

        if (resetCalled)
        {
            REQUIRE_MESSAGE(s.remaining.load().count() == s.duration.count(),
                            "State reset unsuccessful");
            resetCalled = false; // Notify that the assertion was executed.
        }
        ++callCount;
    };

    const std::string timerName("t1");
    REQUIRE_MESSAGE(
        schedule.timerAdd(timerName, 10ms, 500ms, true, timerAction, true),
        "Error adding timer");

    auto start = test::now();
    while (0 == callCount)
    {
        if (test::delta(start) > 100ms)
        {
            FAILED_REQUIREMENT("Timer should tick on addition");
        }
    }

    {
        std::lock_guard<std::mutex> criticalSection(mtx);
        REQUIRE(schedule.timerReset(timerName));
        resetCalled = true;
    }

    while (resetCalled)
    {
        if (test::delta(start) > 100ms)
        {
            FAILED_REQUIREMENT("Timer should tick on resetting");
        }
    }
    // resetCalled = false, meaning the action completed successfully.
}

TEST_CASE("Timer: Stop-Resume")
{
    Timeline schedule;

    std::atomic_size_t callCount{0};
    std::atomic_bool stopCalled{false};

    auto timerAction = [&callCount, &stopCalled](TimerState const &s) {
        if (0 == callCount)
        {
            REQUIRE_MESSAGE(s.remaining.load().count() ==
                                s.duration.count() - s.resolution.count(),
                            "Timers that don't run on addition start with "
                            "remaining == duration-resolution");
        }

        if (stopCalled)
        {
            REQUIRE_MESSAGE(s.remaining.load().count() ==
                                s.duration.count() - s.resolution.count(),
                            "Resuming after stop, steps down from duration");
            stopCalled = false; // Notify that the assertion was executed.
        }
        ++callCount;
    };

    const std::string timerName("t1");
    REQUIRE_MESSAGE(
        schedule.timerAdd(timerName, 10ms, 500ms, true, timerAction, false),
        "Error adding timer");

    auto start = test::now();
    while (0 == callCount)
    {
        if (test::delta(start) > 100ms)
        {
            FAILED_REQUIREMENT("Timer should tick after ~10ms");
        }
    }

    REQUIRE(schedule.timerStop(timerName));
    stopCalled = true; // Ok because timer should not run anymore.
    REQUIRE_MESSAGE(schedule.serialize(true, false, false).size() == 1,
                    "Stopped timers are serializable");
    REQUIRE(schedule.timerResume(timerName));

    start = test::now();
    while (stopCalled)
    {
        if (test::delta(start) > 100ms)
        {
            FAILED_REQUIREMENT("Timer should tick ~10ms after resuming");
        }
    }
    // resetCalled = false, meaning the action completed successfully.
}

TEST_CASE("Timer: Pause-Resume")
{
    Timeline schedule;

    std::atomic_size_t callCount{0};
    std::atomic_bool pauseCalled{false};

    auto timerAction = [&callCount, remainingMsWhenPaused = 0L,
                        &pauseCalled](TimerState const &s) mutable {
        if (0 == callCount)
        {
            REQUIRE_MESSAGE(s.remaining.load().count() ==
                                s.duration.count() - s.resolution.count(),
                            "Timers that don't run on addition start with "
                            "remaining == duration-resolution");
        }

        if (pauseCalled)
        {
            REQUIRE_MESSAGE(s.remaining.load().count() ==
                                remainingMsWhenPaused - s.resolution.count(),
                            "Resuming after stop, steps down from duration");
            pauseCalled = false; // Notify that the assertion was executed.
        }

        remainingMsWhenPaused = s.remaining.load().count();
        ++callCount;
    };

    const std::string timerName("t1");
    REQUIRE_MESSAGE(
        schedule.timerAdd(timerName, 10ms, 50s, true, timerAction, false),
        "Error adding timer");

    auto start = test::now();
    while (callCount < 3)
    {
        if (test::delta(start) > 500ms)
        {
            FAILED_REQUIREMENT("Timer should tick every ~10ms");
        }
    }

    REQUIRE(schedule.timerPause(timerName));
    pauseCalled = true; // Ok because timer should not run anymore.
    REQUIRE_MESSAGE(schedule.serialize(true, false, false).size() == 1,
                    "Paused timers are serializable");
    REQUIRE(schedule.timerResume(timerName));

    start = test::now();
    while (pauseCalled)
    {
        if (test::delta(start) > 100ms)
        {
            FAILED_REQUIREMENT("Timer should tick ~10ms after resuming");
        }
    }
    // resetCalled = false, meaning the action completed successfully.
}
