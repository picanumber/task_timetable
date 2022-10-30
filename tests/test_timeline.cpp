// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "doctest/doctest.h"
#include "test_utils.h"
#include "timeline.h"

#include <stdexcept>
#include <string>
#include <vector>

using ttt::Timeline;
using ttt::TimerState;

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

    CHECK_THROWS_AS(
        ttt::Timeline schedule(
            {"junk:string:that:does:not:designate:timeline:entry"}, {});
        , std::runtime_error);
}
