// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "timeline.h"
#include "scheduler.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#if 0
#include <concepts>
#endif

namespace
{

constexpr char kNotIMplemented[] = "Feature not implemented";
constexpr char kInvalidTimerCountdown[] = "Timers cannot tick beyond zero";
constexpr char kInvalidElementType[] = "Type not oneof timer-pulse-alarm";

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

class TimerState : public ttt::TimelineEntity
{
    std::string _name;
    const std::chrono::milliseconds _resolution, _duration;
    std::chrono::milliseconds _remaining;
    const bool _repeating;

  public:
    TimerState(std::string const &state)
        : TimerState(split(state, ttt::kElementFieldsDelimiter))
    {
    }
    TimerState(std::vector<std::string> const &args)
        : TimerState(args.at(1), millis_from(args.at(2)),
                     millis_from(args.at(3)), args.at(5) == "1" ? true : false)
    {
    }
    TimerState(std::string const &name, std::chrono::milliseconds resolution,
               std::chrono::milliseconds duration, bool repeating)
        : _name(name), _resolution(resolution), _duration(duration),
          _remaining(duration), _repeating(repeating)
    {
    }

    std::string toString() const override
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

    auto resolution() const
    {
        return _resolution;
    }
};

struct TimerEntry
{
    std::shared_ptr<TimerState> state;
    std::optional<ttt::CallToken> token;

    explicit TimerEntry(std::vector<std::string> const &stateFields)
        : state(std::make_shared<TimerState>(stateFields))
    {
    }
};

} // namespace

namespace ttt
{

class TimelineImpl
{
    std::mutex _mtx;
    ttt::CallScheduler _schedule;
    std::map<std::string, TimerEntry> _timers;

  public:
    TimelineImpl() = default;

    TimelineImpl(std::vector<std::string> const &elements,
                 std::function<void(ttt::TimelineEntity const &)> call)
    {
        for (auto const &el : elements)
        {
            auto fields = split(el, kElementFieldsDelimiter);
            auto const &entityType = fields.at(0);

            if (entityType == kTimerElement)
            {
                if (auto [it, ok] = _timers.try_emplace(fields.at(1), fields);
                    true == ok)
                {
                    auto callback = [observer = std::weak_ptr(it->second.state),
                                     action = call] {
                        auto ret = ttt::Result::Repeat;
                        if (auto ptr = observer.lock())
                        {
                            if (!ptr->tick())
                            {
                                ret = ttt::Result::Finished;
                            }
                            action(dynamic_cast<ttt::TimelineEntity &>(*ptr));
                        }
                        return ret;
                    };
                    it->second.token.emplace(
                        _schedule.add(std::move(callback),
                                      it->second.state->resolution(), true));
                }
            }
            else if (entityType == kPulseElement)
            {
                throw std::runtime_error(kNotIMplemented);
            }
            else if (entityType == kAlarmElement)
            {
                throw std::runtime_error(kNotIMplemented);
            }
            else
            {
                throw std::runtime_error(kInvalidElementType);
            }
        }
    }
};

} // namespace ttt

namespace ttt
{

std::string TimerString([[maybe_unused]] std::string const &name,
                        [[maybe_unused]] std::chrono::milliseconds resolution,
                        [[maybe_unused]] std::chrono::milliseconds duration,
                        [[maybe_unused]] std::chrono::milliseconds remaining,
                        [[maybe_unused]] bool repeating)
{

    return {};
}

TimelineEntity::~TimelineEntity() = default;

Timeline::Timeline() : _impl(std::make_unique<TimelineImpl>())
{
}

Timeline::Timeline(std::vector<std::string> const &elements,
                   std::function<void(TimelineEntity const &)> call)
    : _impl(std::make_unique<TimelineImpl>(elements, std::move(call)))
{
}

Timeline::~Timeline() = default;

} // namespace ttt
