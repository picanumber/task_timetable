#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace ttt
{

/**
 * @brief A worker thread encapsulation.
 *
 * @details Features:
 * - Doubly buffered production/consumption of task items.
 * - Cancellable schedule (back buffer).
 *
 * @tparam TaskType type of the unit of work.
 */
template <class TaskType> class BufferedWorker
{
  public:
    using work_item_t = TaskType;

    explicit BufferedWorker(std::size_t maxLen = 10'000)
        : _front(&_buffers[0]), _back(&_buffers[1]), _maxLen(maxLen),
          _stop(false)
    {
        _worker = std::thread(&BufferedWorker::consume, this);
    }

    ~BufferedWorker()
    {
        kill();
    }

    bool add(work_item_t work)
    {
        bool ret = false;

        if (!_stop)
        {
            ret = true;
            std::lock_guard lock(_mtx);

            if (_back->size() >= _maxLen)
            {
                _back->pop();
            }

            _back->emplace(std::move(work));
            _bell.notify_one();
        }

        return ret;
    }

    void cancelScheduled()
    {
        std::lock_guard lock(_mtx);

        std::queue<work_item_t> empty{};
        _back->swap(empty);
    }

    void kill()
    {
        if (!_stop)
        {
            {
                std::lock_guard lock(_mtx);
                _stop = true;
                _bell.notify_one();
            }

            _worker.join();
        }
    }

  private:
    void consume()
    {
        while (!_stop)
        {
            {
                std::lock_guard lock(_mtx);
                std::swap(_front, _back);
            }

            while (!(_stop || _front->empty()))
            {
                std::invoke(_front->front());
                _front->pop();
            }

            {
                std::unique_lock lock(_mtx);
                _bell.wait(lock, [this] { return _stop || !_back->empty(); });
            }
        }
    }

  private:
    std::thread _worker;
    std::queue<work_item_t> _buffers[2];
    std::queue<work_item_t> *_front, *_back;
    mutable std::mutex _mtx;
    mutable std::condition_variable _bell;
    const std::size_t _maxLen;
    std::atomic_bool _stop;
};

} // namespace ttt
