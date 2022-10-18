#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>

namespace ttt
{

namespace detail
{

constexpr char kErrorWorkerSize[] = "Woker cannot have a zero length buffer";

}

/**
 * @brief A worker thread encapsulation.
 *
 * @details Features:
 * - Doubly buffered production/consumption of task items.
 *
 * @tparam TaskType type of the unit of work.
 */
template <class TaskType> class BufferedWorker
{
  public:
    using work_item_t = TaskType;

    /**
     * @brief Constructor
     *
     * @param maxLen Per buffer max allowed task queue size. Older tasks are
     * replaced by new ones beyond this limit.
     * @param dropLefoverTasks Worker behavior when destruction happens with
     * non-empty task queues.
     */
    explicit BufferedWorker(std::size_t maxLen = 10'000,
                            bool dropLefoverTasks = true)
        : _front(&_buffers[0]), _back(&_buffers[1]), _maxLen(maxLen),
          _stop(false), _executeLeftoverTasks(!dropLefoverTasks)
    {
        if (0 == maxLen)
        {
            throw std::runtime_error(detail::kErrorWorkerSize);
        }
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
            std::lock_guard<std::mutex> lock(_mtx);

            if (_back->size() >= _maxLen)
            {
                _back->pop();
            }

            _back->emplace(std::move(work));
            _bell.notify_one();
        }

        return ret;
    }

    void kill()
    {
        if (!_stop)
        {
            {
                std::lock_guard<std::mutex> lock(_mtx);
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
            swapBuffers();
            processFrontBuffer();
            waitForDataOrStop();
        }

        if (_executeLeftoverTasks)
        {
            swapBuffers();
            processFrontBuffer();
        }
    }

    void swapBuffers()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        std::swap(_front, _back);
    }

    void processFrontBuffer()
    {
        while (!_front->empty() && (!_stop || _executeLeftoverTasks))
        {
            std::invoke(_front->front());
            _front->pop();
        }
    }

    void waitForDataOrStop()
    {
        std::unique_lock<std::mutex> lock(_mtx);
        _bell.wait(lock, [this] { return _stop || !_back->empty(); });
    }

  private:
    std::thread _worker;
    std::queue<work_item_t> _buffers[2];
    std::queue<work_item_t> *_front, *_back;
    mutable std::mutex _mtx;
    mutable std::condition_variable _bell;
    const std::size_t _maxLen;
    std::atomic_bool _stop;
    const std::atomic_bool _executeLeftoverTasks;
};

} // namespace ttt
