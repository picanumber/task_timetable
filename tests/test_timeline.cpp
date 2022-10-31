// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "doctest/doctest.h"
#include "scheduler.h"
#include "test_utils.h"
#include "timeline.h"

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
        "timer:t4:100:500:500:1",
        "timer:t3:100:500:500:0",
        "timer:t1:1000:10000:10000:1",
        "timer:t2:1000:10000:10000:0",
    };

    CHECK_NOTHROW(Timeline schedule);
    CHECK_NOTHROW(Timeline schedule({entityStrings.at(0)}, DummyTimerAction));
    CHECK_NOTHROW(Timeline schedule({entityStrings.at(1)}, DummyTimerAction));
    CHECK_NOTHROW(Timeline schedule({entityStrings.at(2)}, DummyTimerAction));

    Timeline demoTimeline(entityStrings, {});
    auto serialized = demoTimeline.serialize();
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

TEST_CASE("Single timer")
{
    Timeline schedule;

    std::atomic_size_t callCount{0};
    auto timerAction = [&callCount](TimerState const &) {
        ++callCount;
        return ttt::Result::Repeat;
    };

    REQUIRE_MESSAGE(schedule.timerAdd("t1", 10ms, 100ms, 0, timerAction),
                    "Unable to add timer");

    auto start = test::now();
    while (callCount < 10)
    {
        CHECK_MESSAGE(test::delta(start) < 5s, "Timer not ticking in tempo");
    }

    REQUIRE_MESSAGE(callCount == 10, "Wrong number of iterations");
    REQUIRE_MESSAGE(callCount == 10, "Wrong number of iterations");
    REQUIRE_MESSAGE(callCount == 10, "Wrong number of iterations");
}

TEST_CASE("ttgff timer")
{
    Timeline schedule;

    std::atomic_size_t callCount{0};
    auto timerAction = [&callCount](TimerState const &) {
        ++callCount;
        return ttt::Result::Repeat;
    };

    REQUIRE_MESSAGE(schedule.timerAdd("t1", 10ms, 100ms, 0, timerAction),
                    "Unable to add timer");

    auto start = test::now();
    while (callCount < 10)
    {
        CHECK_MESSAGE(test::delta(start) < 5s, "Timer not ticking in tempo");
    }

    REQUIRE_MESSAGE(callCount == 10, "Wrong number of iterations");
    REQUIRE_MESSAGE(callCount == 10, "Wrong number of iterations");
    REQUIRE_MESSAGE(callCount == 10, "Wrong number of iterations");
}
