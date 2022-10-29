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

/**
 * @brief Aggregate of values making up the state of a timer.
 */
struct TimerState
{
    const std::string name;
    const std::chrono::milliseconds resolution;
    const std::chrono::milliseconds duration;
    std::chrono::milliseconds remaining;
    const bool repeating;
};

// Split a state string into key and value.
std::pair<std::string, std::string> KeyValueFrom(std::string const &stateStr);

// Stich a key-value pair into a state string.
std::string StateStringFrom(std::string const &key, std::string const &value);

class Timeline final
{
    std::unique_ptr<TimelineImpl> _impl;

  public:
    Timeline();
    /**
     * @brief Construct a timeline out of serialized information.
     *
     * @param elements All entities as state strings.
     * @param timersEvent Callback that applies to timer events.
     */
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

    /**
     * @brief Add a timer to the timeline.
     *
     * @param name Timer description.
     * @param resolution Interval between timer invocations.
     * @param duration Total execution time for the timer.
     * @param repeating Whether to count from the top when reaching zero.
     * @param onTick Callback to execute on invocation of the timer.
     *
     * @return Whether the timer was added.
     */
    bool timerAdd(std::string const &name, std::chrono::milliseconds resolution,
                  std::chrono::milliseconds duration, bool repeating,
                  std::function<void(TimerState const &)> onTick);
    /**
     * @brief Remove the specified timer.
     *
     * @param name Name of the timer to remove.
     *
     * @return Whether the timer was removed.
     */
    bool timerRemove(std::string const &name);
    /**
     * @brief Reset the remaining time.
     *
     * @param name Name of the timer to reset.
     *
     * @return Whether the timer was reset.
     */
    bool timerReset(std::string const &name);
    /**
     * @brief Stop the specified timer, i.e. stop ticking and reset the timer
     * entity state.
     *
     * @param name Name of the timer to stop.
     *
     * @return Whether the timer was stopped.
     */
    bool timerStop(std::string const &name);
    /**
     * @brief Pause the specified timer, i.e. pause ticking and keep state as
     * is.
     *
     * @param name Name of the timer to pause.
     *
     * @return Whether the timer was paused.
     */
    bool timerPause(std::string const &name);
    /**
     * @brief Force a timer to start ticking again.
     *
     * @param name Name of the timer to resume.
     *
     * @return Whether timer event emission was resumed.
     */
    bool timerResume(std::string const &name);

    // Pulse
    // Alarm
};

} // namespace ttt
