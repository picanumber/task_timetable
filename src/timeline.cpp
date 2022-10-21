// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "timeline.h"
#include <chrono>
#include <cstdlib>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>

namespace
{

constexpr char kInvalidTimerCountdown[] = "Timers cannot tick beyond zero";

std::vector<std::string> split(std::string const &input, char delim)
{
    thread_local std::stringstream ss;
    ss.str(input);

    std::vector<std::string> ret;

    std::string item;
    while (std::getline(ss, item, delim))
    {
        ret.emplace_back(std::move(item));
    }

    return ret;
}

class TimerState
{
    std::string _name;
    std::chrono::milliseconds _resolution, _duration;
    std::chrono::milliseconds _remaining;
    bool _repeating;

  public:
    TimerState(std::string const &state)
    {
        auto fields = split(state, ttt::kElementFieldsDelimiter);
    }
    TimerState(std::string const &name, std::size_t resolutionMs,
               std::size_t durationMs)
        : _name(name), _resolution(std::chrono::milliseconds(resolutionMs)),
          _duration(std::chrono::milliseconds(durationMs)),
          _remaining(_duration)
    {
        // TODO(picanumber): _repeating!!
    }

    std::string toString() const
    {
        std::string ret(ttt::kTimerElement);

        ret += ttt::kElementFieldsDelimiter;
        ret += _name;
        ret += ttt::kElementFieldsDelimiter;
        ret += std::to_string(_resolution.count());
        ret += ttt::kElementFieldsDelimiter;
        ret += std::to_string(_duration.count());
        ret += ttt::kElementFieldsDelimiter;
        ret += std::to_string(_remaining.count());
        ret += ttt::kElementFieldsDelimiter;
        ret += _repeating ? "1" : "0";

        return ret;
    }

    // Returns whether it can tick again.
    bool tick()
    {
        bool ret = false;

        if (_remaining.count())
        {
            throw std::runtime_error(kInvalidTimerCountdown);
        }

        return ret;
    }
};

#if 0
auto operator<=>(TimerState const &lhs, TimerState const &rhs)
{
    return lhs.name() <=> rhs.name();
}
#endif

} // namespace

namespace ttt
{

class TimelineImpl
{
    std::set<TimerState> _timers;

  public:
    TimelineImpl()
    {
    }

    TimelineImpl(std::vector<TimelineElement> const &elements)
    {
        (void)elements;
    }
};

} // namespace ttt

namespace ttt
{

Timeline::Timeline() : _impl(std::make_unique<TimelineImpl>())
{
}

Timeline::Timeline(std::vector<TimelineElement> const &elements)
    : _impl(std::make_unique<TimelineImpl>(elements))
{
}

Timeline::~Timeline() = default;

} // namespace ttt
