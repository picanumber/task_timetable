// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "scheduler.h"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace ttt
{

class TimelineImpl;

constexpr char kElementFieldsDelimiter = ':';

constexpr char kTimerElement[] = "timer";
constexpr char kPulseElement[] = "pulse";
constexpr char kAlarmElement[] = "alarm";

constexpr short kElementFieldSz = std::extent<decltype(kTimerElement)>::value;

std::string TimerString(std::string const &name,
                        std::chrono::milliseconds resolution,
                        std::chrono::milliseconds duration,
                        std::chrono::milliseconds remaining, bool repeating);

struct TimelineEntity
{
    virtual ~TimelineEntity();
    virtual std::string toString() const = 0;
};

class Timeline final
{
    std::unique_ptr<TimelineImpl> _impl;

  public:
    Timeline();
    explicit Timeline(std::vector<std::string> const &elements,
                      std::function<void(TimelineEntity const &)> call);
    ~Timeline();

    // General
    std::vector<std::string> serialize() const;
    bool add(std::string const &element);

    // Timer
    bool addTimer(std::string const &name, std::size_t durationMs,
                  std::size_t resolutionMs, bool repeating);
    bool removeTimer(std::string const &name);
    bool resetTimer(std::string const &name);
    bool stopTimer(std::string const &name);
    bool pauseTimer(std::string const &name);
    bool resumeTimer(std::string const &name);

    // Pulse
    // Alarm
};

} // namespace ttt
