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

class CallTokenImpl;

struct Task
{
    std::function<Result()> work;
    std::shared_ptr<CallTokenImpl> pass;
    std::chrono::microseconds interval;
};

} // namespace detail

class CallToken
{
    std::shared_ptr<detail::CallTokenImpl> _token;

  public:
    explicit CallToken(std::shared_ptr<detail::CallTokenImpl> token);

    CallToken(CallToken const &) = delete;
    CallToken &operator=(CallToken const &) = delete;
    CallToken(CallToken &&) = default;

    ~CallToken();
    void detach();
};

class CallScheduler
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
    explicit CallScheduler(bool countIntervalOnTaskStart = true);
    ~CallScheduler();

    [[nodiscard]] CallToken add(std::function<Result()> call,
                                std::chrono::microseconds interval,
                                bool immediate = false);

  private:
    // Collection of active tasks.
    task_map_t _tasks;
    // Worker responsible for running tasks.
    BufferedWorker<TaskRunner> _executor;
    // Worker responsible for coordinating tasks.
    struct
    {
        std::thread consumer;
        mutable std::mutex mtx;
        mutable std::condition_variable cv;
        std::atomic_bool stop{false};
    } _scheduler;

    // Explanation.
    bool _countOnTaskStart;

  private:
    void run();
};

} // namespace ttt
