#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>

namespace cls
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

class CallToken
{
    // Potential states of a token.
    static constexpr int kIdle = 0;
    static constexpr int kRunning = 1;
    static constexpr int kDead = 2;

    class StateReset
    {
        std::atomic_int &_state;

      public:
        StateReset(std::atomic_int &state) : _state(state)
        {
        }

        StateReset(StateReset const &) = delete;
        StateReset &operator=(StateReset const &) = delete;

        ~StateReset()
        {
            _state = kIdle;
        }
    };

  public:
    ~CallToken()
    {
        cancel();
    }

    auto allow()
    {
        int expected = kIdle;
        std::unique_ptr<StateReset> ret;

        if (_state.compare_exchange_strong(expected, kRunning))
        {
            ret.reset(new StateReset(_state));
        }

        return ret;
    }

    void cancel()
    {
        if (_monitoring)
        {
            int expected = kIdle;
            while (!_state.compare_exchange_strong(expected, kDead) &&
                   kDead != expected)
            {
                expected = kIdle;
            }
            detach();
        }
    }

    void detach()
    {
        _monitoring = false;
    }

  private:
    std::atomic_int _state{kIdle};
    bool _monitoring{true};
};

namespace detail
{

struct Task
{
    std::function<Result()> work;
    std::weak_ptr<CallToken> pass;
    std::chrono::microseconds interval;
    std::chrono::steady_clock::time_point creationPoint;
};

} // namespace detail

class CallScheduler
{
    using execution_time_point_t = std::chrono::steady_clock::time_point;

    std::multimap<execution_time_point_t, detail::Task> _tasks;
    struct
    {
        std::thread consumer;
        mutable std::mutex mtx;
        mutable std::condition_variable cv;
        std::atomic_bool stop{false};
    } _scheduler;

  public:
    CallScheduler()
    {
        _scheduler.consumer = std::thread(&CallScheduler::run, this);
    }

    ~CallScheduler()
    {
        kill();
    }

    std::shared_ptr<CallToken> add(std::function<Result()> call,
                                   std::chrono::microseconds interval)
    {
        auto ret{std::make_shared<CallToken>()};

        detail::Task task{.work = std::move(call),
                          .pass = ret,
                          .interval = interval,
                          .creationPoint = std::chrono::steady_clock::now()};

        {
            std::lock_guard lock(_scheduler.mtx);
            _tasks.emplace(task.creationPoint + interval, std::move(task));
        }
        _scheduler.cv.notify_one();

        return ret;
    }

  private:
    void run()
    {
        while (!_scheduler.stop)
        {
            std::unique_lock lock(_scheduler.mtx);

            if (_tasks.empty())
            {
                _scheduler.cv.wait(lock, [this] {
                    return _scheduler.stop || !_tasks.empty();
                });

                if (_scheduler.stop)
                {
                    break;
                }
            }
            else if (_scheduler.cv.wait_until(
                         lock, _tasks.begin()->first,
                         [this] { return !!_scheduler.stop; }))
            {
                break;
            }

            if (!_tasks.empty() &&
                std::chrono::steady_clock::now() >= _tasks.begin()->first)
            {
                // TODO: A queue should be filled in the cv predicate.
                // and running of the function should happen in a different
                // process.
                auto nh = _tasks.extract(_tasks.begin());

                auto &task = nh.mapped();
                if (auto token = task.pass.lock())
                {
                    if (auto reset = token->allow())
                    {
                        task.work();
                        // TODO: Check result and if needed push the node back
                        // to the _tasks
                    }
                }
            }
        }
    }

    void kill()
    {
        {
            std::lock_guard lock(_scheduler.mtx);
            _scheduler.stop = true;
        }
        _scheduler.cv.notify_one();
        _scheduler.consumer.join();
    }
};

} // namespace cls
