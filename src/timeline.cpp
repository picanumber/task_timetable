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

constexpr char kElementFieldsDelimiter = ':';

constexpr char kTimerElement[] = "timer";
constexpr char kPulseElement[] = "pulse";
constexpr char kAlarmElement[] = "alarm";

constexpr short kElementFieldSz = std::extent<decltype(kTimerElement)>::value;

constexpr char kNotIMplemented[] = "Feature not implemented";
constexpr char kInvalidTimerCountdown[] = "Timers cannot tick beyond zero";
constexpr char kInvalidElementType[] = "Type not one of timer-pulse-alarm";
constexpr char kNonCallableEntity[] = "No action associated with the entity";

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

class TimerEntity
{
    ttt::TimerState _state;
    std::function<void(ttt::TimerState const &)> _onTick;

  public:
    TimerEntity(std::string const &state)
        : TimerEntity(split(state, kElementFieldsDelimiter))
    {
    }
    TimerEntity(std::vector<std::string> const &args)
        : TimerEntity(args.at(1), millis_from(args.at(2)),
                      millis_from(args.at(3)), args.at(5) == "1" ? true : false)
    {
    }
    TimerEntity(std::string const &name, std::chrono::milliseconds resolution,
                std::chrono::milliseconds duration, bool repeating)
        : _state{name, resolution, duration, duration, repeating}
    {
    }

    void setAction(std::function<void(ttt::TimerState const &)> action)
    {
        _onTick = std::move(action);
    }

    void operator()()
    {
        if (_onTick)
        {
            _onTick(_state);
        }
        else
        {
            throw std::runtime_error(kNonCallableEntity);
        }
    }

    std::string toString() const
    {
        return stich(kElementFieldsDelimiter, kTimerElement, _state.name,
                     to_string(_state.resolution), to_string(_state.duration),
                     to_string(_state.remaining),
                     (_state.repeating ? "1" : "0"));
    }

    // Remove a "resolution" from remaining. Returns whether it can tick again.
    bool tick()
    {
        bool ret = true;

        if (0 == _state.remaining.count())
        {
            throw std::runtime_error(kInvalidTimerCountdown);
        }
        else
        {
            _state.remaining -= _state.resolution;
        }

        if (0 == _state.remaining.count())
        {
            if (_state.repeating)
            {
                reset(); // Repeating clocks re-start from "duration".
            }
            ret = _state.repeating;
        }

        return ret;
    }

    void reset()
    {
        _state.remaining = _state.duration;
    }

    ttt::TimerState const &state() const
    {
        return _state;
    }
};

struct TimerEntry
{
    std::shared_ptr<TimerEntity> entity;
    std::optional<ttt::CallToken> token;

    template <class... Args>
    explicit TimerEntry(Args &&...args)
        : entity(std::make_shared<TimerEntity>(std::forward<Args>(args)...))
    {
    }
};

} // namespace

namespace ttt
{

class TimelineImpl
{
    mutable std::mutex _mtx;
    std::map<std::string, TimerEntry> _timers;
    ttt::CallScheduler _schedule;

  public:
    TimelineImpl() = default;

    TimelineImpl(std::vector<std::string> const &elements,
                 std::function<void(ttt::TimerState const &)> timersEvent)
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
                    it->second.entity->setAction(timersEvent);
                    it->second.token.emplace(scheduleTimer(it->second.entity));
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

    std::vector<std::string> serialize() const
    {
        std::lock_guard<std::mutex> lock(_mtx);

        std::vector<std::string> ret;
        ret.reserve(_timers.size() /* + _pulses.size() + _alarms.size() */);

        for (auto const &[name, timerEntry] : _timers)
        {
            ret.push_back(timerEntry.entity->toString());
        }

        // TODO: Same for pulses and alarms

        return ret;
    }

    bool addTimer(std::string const &name, std::chrono::milliseconds resolution,
                  std::chrono::milliseconds duration, bool repeating,
                  std::function<void(TimerState const &)> onTick)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        auto [it, ok] =
            _timers.try_emplace(name, name, resolution, duration, repeating);

        if (ok)
        {
            it->second.entity->setAction(std::move(onTick));
            it->second.token.emplace(scheduleTimer(it->second.entity));
        }

        return ok;
    }

    bool removeTimer(std::string const &name)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _timers.erase(name);
    }

    bool resetTimer(std::string const &name)
    {
        bool ret = false;
        std::lock_guard<std::mutex> lock(_mtx);

        if (auto it = _timers.find(name); _timers.end() != it)
        {
            it->second.token.reset();   // Cancel timer ticking.
            it->second.entity->reset(); // Reset timer state.

            // Reschedule the timer entity.
            it->second.token.emplace(scheduleTimer(it->second.entity));

            ret = true;
        }

        return ret;
    }

    bool stopTimer(std::string const &name, bool resetState)
    {
        bool ret = false;
        std::lock_guard<std::mutex> lock(_mtx);

        if (auto it = _timers.find(name); _timers.end() != it)
        {
            it->second.token.reset(); // Cancel timer ticking.
            if (resetState)
            {
                it->second.entity->reset();
            }

            ret = true;
        }

        return ret;
    }

    bool resumeTimer(std::string const &name)
    {
        bool ret = false;
        std::lock_guard<std::mutex> lock(_mtx);

        if (auto it = _timers.find(name);
            _timers.end() != it and it->second.token.has_value() == false)
        {
            // Reschedule the timer entity.
            it->second.token.emplace(scheduleTimer(it->second.entity));

            ret = true;
        }

        return ret;
    }

  private:
    [[nodiscard]] ttt::CallToken scheduleTimer(
        std::shared_ptr<TimerEntity> &ent)
    {
        auto callback = [observer = std::weak_ptr(ent)] {
            auto ret = ttt::Result::Repeat;
            if (auto ptr = observer.lock())
            {
                if (!ptr->tick())
                {
                    ret = ttt::Result::Finished;
                }
                (*ptr)();
            }
            return ret;
        };

        return _schedule.add(std::move(callback), ent->state().resolution,
                             false);
    }
};

} // namespace ttt

namespace ttt
{

std::pair<std::string, std::string> KeyValueFrom(std::string const &stateStr)
{
    std::pair<std::string, std::string> ret;

    if (auto splitPos =
            stateStr.find(kElementFieldsDelimiter, kElementFieldSz + 1);
        splitPos != std::string::npos)
    {
        ret.first = stateStr.substr(0, splitPos);
        ret.second = stateStr.substr(splitPos + 1);
    }

    return ret;
}

std::string StateStringFrom(std::string const &key, std::string const &value)
{
    return stich(kElementFieldsDelimiter, key, value);
}

Timeline::Timeline() : _impl(std::make_unique<TimelineImpl>())
{
}

Timeline::Timeline(std::vector<std::string> const &elements,
                   std::function<void(TimerState const &)> timersEvent)
    : _impl(std::make_unique<TimelineImpl>(elements, std::move(timersEvent)))
{
}

Timeline::~Timeline() = default;

std::vector<std::string> Timeline::serialize() const
{
    return _impl->serialize();
}

bool Timeline::timerAdd(std::string const &name,
                        std::chrono::milliseconds resolution,
                        std::chrono::milliseconds duration, bool repeating,
                        std::function<void(TimerState const &)> onTick)
{
    return _impl->addTimer(name, resolution, duration, repeating,
                           std::move(onTick));
}

bool Timeline::timerRemove(std::string const &name)
{
    return _impl->removeTimer(name);
}

bool Timeline::timerReset(std::string const &name)
{
    return _impl->resetTimer(name);
}

bool Timeline::timerStop(std::string const &name)
{
    return _impl->stopTimer(name, true);
}

bool Timeline::timerPause(std::string const &name)
{
    return _impl->stopTimer(name, false);
}

bool Timeline::timerResume(std::string const &name)
{
    return _impl->resumeTimer(name);
}

} // namespace ttt
