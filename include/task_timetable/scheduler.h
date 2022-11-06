// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "buffered_worker.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>

namespace ttt
{

/**
 * @brief Designates the result of a call++ task, i.e. whether it is to be
 * repeated or the execution was the last one.
 */
enum class Result : uint8_t
{
    Finished,
    Repeat
};

namespace detail
{

constexpr char kErrorNoWorkersInScheduler[] = "Scheduler has NO workers";

class CallTokenImpl;

struct Task
{
    std::function<Result()> work;
    std::shared_ptr<CallTokenImpl> pass;
    std::chrono::microseconds interval;
};

} // namespace detail

/**
 * @brief Controls the execution of a Callscheduler task.
 *
 * @details A token is 1-1 associated with a task that is added to the call
 * scheduler. Due to this association three things can happen:
 * 1. The token is alive    : Task is allowed to run.
 * 2. The token is destroyed: Tasks are prevented from running after token
 *                            destruction and are removed from the scheduler.
 * 3. The token is detached : Task is independent from the token state.
 */
class CallToken
{
    std::shared_ptr<detail::CallTokenImpl> _token;

  public:
    explicit CallToken(std::shared_ptr<detail::CallTokenImpl> token);

    CallToken(CallToken const &) = delete;
    CallToken &operator=(CallToken const &) = delete;
    CallToken(CallToken &&) = default;

    /**
     * @brief Cancels the associated task iff the token is not detached.
     */
    ~CallToken();

    /**
     * @brief Disassociate the token from the execution of the task.
     */
    void detach();
};

/**
 * @brief Central class of the task timetable library.
 *
 * @details Creates an itinerary for users to plan task execution on.
 * Processing is done in two thread groups:
 * - A single coordinator thread which picks "due to run" tasks.
 * - An executor thread pool where tasks actually run.
 * Decomposition in two parts is done so that scheduling is not slowed down by
 * task processing.
 * A scheduler drops all unfinished tasks upon destruction, since repeating
 * tasks would prevent destruction otherwise.
 */
class CallScheduler final
{
    using execution_time_point_t = std::chrono::steady_clock::time_point;
    using task_map_t = std::multimap<execution_time_point_t, detail::Task>;

    class TaskRunner
    {
        CallScheduler &_parent;
        task_map_t::node_type _node;

      public:
        TaskRunner(CallScheduler &parent, task_map_t::node_type &&node);
        void operator()();
    };

  public:
    /**
     * @brief Create a call scheduler.
     *
     * @param countIntervalOnTaskStart Tasks repeat every interval, calculate
     * the time point of next execution by:
     * - true  : Subtracting the task running time from the interval.
     * - false : Adding the interval when an execution has finished.
     * @param nExecutors Number of workers that execute tasks. Values beyond
     * hardware concurrency will be truncated.
     */
    explicit CallScheduler(bool countIntervalOnTaskStart = true,
                           unsigned nExecutors = 1);

    ~CallScheduler();

    /**
     * @brief Add a new task to the scheduler.
     *
     * @param call Task to be executed by the scheduler. The return value is a
     * Result enumerator denoting whether to repeat the task or drop it.
     * @param interval Timeout until repeating the execution of a task (if
     * applicable).
     * @param immediate If true the task is immediately scheduled for execution.
     *
     * @return Calltoken object controlling the lifetime of the added task.
     */
    [[nodiscard]] CallToken add(std::function<Result()> call,
                                std::chrono::microseconds interval,
                                bool immediate = false);

  private:
    // Collection of active tasks.
    task_map_t _tasks;
    // Worker responsible for running tasks.
    std::vector<BufferedWorker<TaskRunner>> _executors;
    // Worker responsible for coordinating tasks.
    struct
    {
        std::thread consumer;
        mutable std::mutex mtx;
        mutable std::condition_variable cv;
        std::atomic_bool stop{false};
    } _scheduler;

    std::size_t _currentExecutor = 0;
    bool _countOnTaskStart;

  private:
    void run();
};

} // namespace ttt
