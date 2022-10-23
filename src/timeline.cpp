// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "timeline.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#if 0
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

template <class Rep, class Period>
std::string to_string(std::chrono::duration<Rep, Period> const &d)
{
    return std::to_string(d.count());
}

std::chrono::milliseconds millis_from(std::string const &s)
{
    return std::chrono::milliseconds(std::stoul(s));
}

#if 0
template <class T>
concept string_like = std::convertible_to<std::decay_t<T>, std::string>;

std::string stich(char delimiter, std::string const &arg,
                  string_like auto &&...args)
#else
template <class... Args>
auto stich(char delimiter, std::string const &arg, Args &&...args)
    -> std::enable_if_t<
        (std::is_convertible_v<std::decay_t<Args>, std::string> && ...),
        std::string>
#endif
{
    std::string ret;
    ret.reserve((arg.size() + ... + std::size(args)));
    ((ret += arg), ..., (ret += delimiter, ret += args));

    return ret;
}

class TimerState
{
    std::string _name;
    const std::chrono::milliseconds _resolution, _duration;
    std::chrono::milliseconds _remaining;
    const bool _repeating;

    TimerState(std::vector<std::string> const &args)
        : TimerState(args.at(1), millis_from(args.at(2)),
                     millis_from(args.at(3)), args.at(4) == "1" ? true : false)
    {
    }

  public:
    TimerState(std::string const &state)
        : TimerState(split(state, ttt::kElementFieldsDelimiter))
    {
    }
    TimerState(std::string const &name, std::chrono::milliseconds resolution,
               std::chrono::milliseconds duration, bool repeating)
        : _name(name), _resolution(resolution), _duration(duration),
          _remaining(duration), _repeating(repeating)
    {
    }

    std::string toString() const
    {
        return stich(ttt::kElementFieldsDelimiter, ttt::kTimerElement, _name,
                     to_string(_resolution), to_string(_duration),
                     to_string(_remaining), (_repeating ? "1" : "0"));
    }

    // Remove a "resolution" from remaining. Returns whether it can tick again.
    bool tick()
    {
        bool ret = true;

        if (0 == _remaining.count())
        {
            throw std::runtime_error(kInvalidTimerCountdown);
        }
        else
        {
            _remaining -= _resolution;
        }

        if (0 == _remaining.count())
        {
            if (_repeating)
            {
                reset(); // Repeating clocks re-start from "duration".
            }
            ret = _repeating;
        }

        return ret;
    }

    void reset()
    {
        _remaining = _duration;
    }
};

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
