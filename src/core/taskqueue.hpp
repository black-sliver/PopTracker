#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

namespace pop {

template<class T>
class TaskQueue {
public:
    static constexpr size_t NO_TASK = 0;

    size_t enqueue(std::function<T()>&& func)
    {
        std::lock_guard lock(_taskMutex);
        _lastTaskNumber++;
        if (_lastTaskNumber == NO_TASK)
            _lastTaskNumber++;
        {
            // TODO: reuse of taskNumber is very unlikely on 64bit. Skip this?
            std::lock_guard resultMutex(_resultMutex);
            while (_tasks.find(_lastTaskNumber) != _tasks.end() || _results.find(_lastTaskNumber) != _results.end()) {
                _lastTaskNumber++;
                if (_lastTaskNumber == NO_TASK)
                    _lastTaskNumber++;
            }
        }
        _tasks.emplace(_lastTaskNumber, std::move(func));
        if (_tasks.size() == 1) { // TODO: one fixed thread and wake it instead?
            if (_thread.joinable())
                _thread.join();
            _thread = std::thread(&TaskQueue::run, this);
        }
        return _lastTaskNumber;
    }

    std::optional<T> tryGet(const size_t taskNumber)
    {
        std::lock_guard lock(_resultMutex);
        const auto it = _results.find(taskNumber);
        if (it == _results.end()) {
            return {};
        }
        T res = std::move(it->second);
        _results.erase(it);
        return res;
    }

    T get(const size_t taskNumber)
    {
        std::unique_lock lock(_resultMutex);
        while (true) {
            const auto it = _results.find(taskNumber);
            if (it != _results.end()) {
                T res = std::move(it->second);
                _results.erase(it);
                return res;
            }
            _resultCondition.wait(lock);
        }
    }

    bool isDone(const size_t taskNumber)
    {
        std::lock_guard lock(_resultMutex);
        const auto it = _results.find(taskNumber);
        return it != _results.end();
    }

    void prioritize(const size_t taskNumber)
    {
        std::lock_guard lock(_priorityMutex);
        if (std::find(_priorityQueue.begin(), _priorityQueue.end(), taskNumber) != _priorityQueue.end())
            return;
        _priorityQueue.emplace_back(taskNumber);
    }

    bool cancel(const size_t taskNumber)
    {
        std::lock_guard lock(_taskMutex);
        if (_activeTaskNumber == taskNumber)
            return false; // already running
        auto it = _tasks.find(taskNumber);
        if (it == _tasks.end())
            return false; // already completed
        if (_tasks.size() == 1)
            return false; // about to start, could deadlock the thread restart logic
        _tasks.erase(it);
        return true;
    }

    ~TaskQueue()
    {
        // NOTE: this should run after dtor of all task owners, so _tasks should be empty.
        //       Uncollected/uncanceled tasks are potential memory leaks.
        std::unique_lock taskLock(_taskMutex);
        if (!_tasks.empty()) {
            // Getting here is a bug in the program. We assume that the owners of tasks are dead and try to shut down.
            fprintf(stderr, "WARNING: destroying task queue with %zu queued tasks!\n", _tasks.size());
            // Give it some time to finish since we don't know if cancelling tasks is safe.
            for (int i = 0; i < 100; ++i) {
                taskLock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                taskLock.lock();
                if (_tasks.empty()) {
                    break;
                }
            }
            // Try to cancel tasks that can be canceled, and finish the remaining ones.
            std::vector<size_t> taskNumbers;
            for (const auto& pair: _tasks) {
                taskNumbers.emplace_back(pair.first);
            }
            taskLock.unlock();
            for (auto& taskNumber: taskNumbers) {
                if (cancel(taskNumber))
                    taskNumber = NO_TASK;
            }
            for (const auto taskNumber: taskNumbers) {
                if (taskNumber != NO_TASK)
                    get(taskNumber);
            }
        } else {
            taskLock.unlock();
        }
        {
            std::lock_guard lock(_resultMutex);
            if (!_results.empty()) {
                fprintf(stderr, "WARNING: destroying task queue with %zu pending results!\n", _results.size());
            }
        }
        if (_thread.joinable())
            _thread.join();
    }

private:
    void run()
    {
        // NOTE: because of the thread restart logic, this lock is awkward
        std::unique_lock taskLock(_taskMutex);
        while (true) {
            size_t taskNumber = NO_TASK;
            std::function<T()> func;
            // find priority task to execute
            while (taskNumber == NO_TASK) {
                {
                    std::lock_guard lock(_priorityMutex);
                    if (_priorityQueue.empty())
                        break;
                    taskNumber = _priorityQueue.front();
                    _priorityQueue.erase(_priorityQueue.begin());
                }
                {
                    auto work = _tasks.find(taskNumber);
                    if (work != _tasks.end()) {
                        _activeTaskNumber = taskNumber;
                        func = std::move(work->second);
                        break;
                    }
                    taskNumber = NO_TASK; // try next
                }
            }
            // if no priority task was found, execute first task
            if (taskNumber == NO_TASK) {
                if (_tasks.empty())
                    break;
                auto work = _tasks.begin();
                taskNumber = work->first;
                _activeTaskNumber = taskNumber;
                func = std::move(work->second);
            }
            taskLock.unlock();
            // ReSharper disable once CppTooWideScope
            T res = func();
            {
                std::lock_guard lock(_resultMutex);
                _results.emplace(taskNumber, std::move(res));
                _resultCondition.notify_all();
            }
            taskLock.lock();
            _tasks.erase(taskNumber);
            _activeTaskNumber = NO_TASK;
        }
#ifdef SDL_INIT_EVERYTHING
        // if SDL is included, assume the tasks may have used SDL
        SDL_TLSCleanup();
#endif
    }

    size_t _lastTaskNumber = NO_TASK;
    size_t _activeTaskNumber = NO_TASK;
    std::mutex _taskMutex;
    std::map<size_t, std::function<T()>> _tasks;
    std::mutex _priorityMutex;
    std::vector<size_t> _priorityQueue;
    std::mutex _resultMutex;
    std::condition_variable _resultCondition;
    std::map<size_t, T> _results;
    std::thread _thread;
};

} // namespace pop
