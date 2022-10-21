// © 2022 Nikolaos Athanasiou, github.com/picanumber
#include "scheduler.h"
#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace ttt
{

namespace detail
{

class CallTokenImpl
{
    // Potential states of a token.
    static constexpr int kIdle = 0;
    static constexpr int kRunning = 1;
    static constexpr int kDead = 2;

    class StateReset
    {
        std::atomic_int &_state;

      public:
        explicit StateReset(std::atomic_int &state) : _state(state)
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
    [[nodiscard]] auto allow()
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
        int expected = kIdle;
        while (!_state.compare_exchange_strong(expected, kDead) &&
               kDead != expected)
        {
            expected = kIdle;
        }
    }

  private:
    std::atomic_int _state{kIdle};
};

} // namespace detail

CallToken::CallToken(std::shared_ptr<detail::CallTokenImpl> token)
    : _token(std::move(token))
{
}

CallToken::~CallToken()
{
    if (_token)
    {
        _token->cancel();
    }
}

void CallToken::detach()
{
    _token.reset();
}

CallScheduler::CallScheduler(bool countOnTaskStart, unsigned nExecutors)
    : _executors(std::min(nExecutors, std::thread::hardware_concurrency())),
      _countOnTaskStart(countOnTaskStart)
{
    if (0 == nExecutors)
    {
        throw std::runtime_error(detail::kErrorNoWorkersInScheduler);
    }

    _scheduler.consumer = std::thread(&CallScheduler::run, this);
}

CallScheduler::~CallScheduler()
{
    // Stop scheduling tasks on the executors.
    {
        std::lock_guard<std::mutex> lock(_scheduler.mtx);
        _scheduler.stop = true;
    }
    _scheduler.cv.notify_one();
    _scheduler.consumer.join();

    // Explicit so that access to destroyed tasks is prevented.
    _executors.clear();
}

CallToken CallScheduler::add(std::function<Result()> call,
                             std::chrono::microseconds interval, bool immediate)
{
    auto token{std::make_shared<detail::CallTokenImpl>()};

    detail::Task task{
        .work = std::move(call), .pass = token, .interval = interval};

    {
        std::lock_guard<std::mutex> lock(_scheduler.mtx);
        _tasks.emplace(immediate ? std::chrono::steady_clock::now()
                                 : std::chrono::steady_clock::now() + interval,
                       std::move(task));
    }
    _scheduler.cv.notify_one();

    return CallToken(token);
}

void CallScheduler::run()
{
    while (!_scheduler.stop)
    {
        std::unique_lock<std::mutex> lock(_scheduler.mtx);

        if (_tasks.empty())
        {
            _scheduler.cv.wait(
                lock, [this] { return _scheduler.stop || !_tasks.empty(); });

            if (_scheduler.stop)
            {
                break;
            }
        }
        else if (_scheduler.cv.wait_until(lock, _tasks.begin()->first,
                                          [this] { return !!_scheduler.stop; }))
        {
            break;
        }

        if (std::chrono::steady_clock::now() >= _tasks.begin()->first)
        {
            _executors[_currentExecutor++ % _executors.size()].add(
                TaskRunner(*this, _tasks.extract(_tasks.begin())));
        }
    }
}

CallScheduler::TaskRunner::TaskRunner(CallScheduler &parent,
                                      task_map_t::node_type &&node)
    : _parent(parent), _node(std::move(node))
{
}

void CallScheduler::TaskRunner::operator()()
{
    Result outcome{Result::Finished};
    auto &task = _node.mapped();

    if (auto reset = task.pass->allow())
    {
        outcome = task.work();
    }

    if (Result::Repeat == outcome)
    {
        _node.key() =
            (_parent._countOnTaskStart ? _node.key()
                                       : std::chrono::steady_clock::now()) +
            task.interval;

        {
            std::lock_guard<std::mutex> lock(_parent._scheduler.mtx);
            _parent._tasks.insert(std::move(_node));
        }
        _parent._scheduler.cv.notify_one();
    }
}

} // namespace ttt
