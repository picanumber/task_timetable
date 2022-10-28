// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "scheduler.h"

#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ttt
{

class TimelineImpl;

struct TimerState
{
    const std::string name;
    const std::chrono::milliseconds resolution;
    const std::chrono::milliseconds duration;
    std::chrono::milliseconds remaining;
    const bool repeating;
};

std::pair<std::string, std::string> KeyValueFrom(std::string const &stateStr);
std::string StateStringFrom(std::string const &key, std::string const &value);

class Timeline final
{
    std::unique_ptr<TimelineImpl> _impl;

  public:
    Timeline();
    explicit Timeline(std::vector<std::string> const &elements,
                      std::function<void(TimerState const &)> timersEvent);
    ~Timeline();

    /**
     * @brief String representation of the state of all entities.
     *
     * @return Collection of strings containing all state, apart from the
     * attached callbacks.
     */
    std::vector<std::string> serialize() const;

    // Timer
    bool addTimer(std::string const &name, std::size_t durationMs,
                  std::size_t resolutionMs, bool repeating,
                  std::function<void(TimerState const &)> onTick);
    bool removeTimer(std::string const &name);
    bool resetTimer(std::string const &name);
    bool stopTimer(std::string const &name);
    bool pauseTimer(std::string const &name);
    bool resumeTimer(std::string const &name);

    // Pulse
    // Alarm
};

} // namespace ttt
