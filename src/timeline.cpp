// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "timeline.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

#if __cpp_concepts
#include <concepts>
#endif

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

#if __cpp_concepts
template <class T>
concept string_like = std::convertible_to<std::decay_t<T>, std::string>;
#endif

std::string stich(char delimiter, std::string const &arg,
#if __cpp_concepts
                  string_like
#endif
                  auto &&...args)
{
    std::string ret;
    ret.reserve((arg.size() + ... + std::size(args)));
    ((ret += arg), ..., (ret += delimiter, ret += args));

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
        return stich(ttt::kElementFieldsDelimiter, ttt::kTimerElement, _name,
                     std::to_string(_resolution.count()),
                     std::to_string(_duration.count()),
                     std::to_string(_remaining.count()),
                     (_repeating ? "1" : "0"));
    }

    // Returns whether it can tick again.
    bool tick()
    {
        bool ret = true;

        if (0 == _remaining.count())
        {
            if (_repeating)
            {
                reset();
            }
            else
            {
                throw std::runtime_error(kInvalidTimerCountdown);
            }
        }
        else
        {
            _remaining -= _resolution;
        }

        if (_remaining.count() == 0)
        {
            ret = _repeating;
        }

        return ret;
    }

    void reset()
    {
        _remaining = _duration;
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
