#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <gtest/gtest.h>
#include "../../src/core/taskqueue.hpp"

TEST(TaskQueue, Cancel) {
    pop::TaskQueue<int> queue;
    std::mutex mutex;
    std::unique_lock lock(mutex);
    std::condition_variable cvStarted;
    std::condition_variable cvDone;
    const size_t first = queue.enqueue([&mutex, &cvStarted, &cvDone] {
        std::unique_lock firstLock(mutex);
        cvStarted.notify_all();
        EXPECT_EQ(cvDone.wait_for(firstLock, std::chrono::seconds(1)), std::cv_status::no_timeout);
        return 1;
    });
    EXPECT_FALSE(queue.cancel(first)); // about to start
    // ^ This is an implementation detail, but covers an additional branch.
    // Note: after the second enqueue, cancel(first) may succeed
    const size_t second = queue.enqueue([] {
        return 2;
    });
    ASSERT_EQ(cvStarted.wait_for(lock, std::chrono::seconds(1)), std::cv_status::no_timeout);
    EXPECT_FALSE(queue.cancel(first)); // already running
    EXPECT_TRUE(queue.cancel(second)); // ok, not running
    EXPECT_FALSE(queue.cancel(second)); // already canceled
    cvDone.notify_all();
    lock.unlock();
    queue.get(first);
    EXPECT_FALSE(queue.cancel(first)); // already finished
}

TEST(TaskQueue, TryGet) {
    pop::TaskQueue<int> queue;
    std::mutex mutex;
    std::unique_lock lock(mutex);
    std::condition_variable cvStarted;
    std::condition_variable cvDone;
    const size_t first = queue.enqueue([&mutex, &cvStarted, &cvDone] {
        std::unique_lock firstLock(mutex);
        cvStarted.notify_all();
        EXPECT_EQ(cvDone.wait_for(firstLock, std::chrono::seconds(1)), std::cv_status::no_timeout);
        return 1;
    });
    ASSERT_EQ(cvStarted.wait_for(lock, std::chrono::seconds(1)), std::cv_status::no_timeout);
    EXPECT_FALSE(queue.tryGet(first).has_value()); // not done
    EXPECT_FALSE(queue.tryGet(first + 1).has_value()); // doesn't exist
    // ^ This is an implementation detail.
    cvDone.notify_all();
    lock.unlock();
    for (size_t i = 0; i < 100; i++) {
        auto res = queue.tryGet(first);
        if (res.has_value()) {
            EXPECT_EQ(res.value(), 1);
            return; // done
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    GTEST_FAIL() << "Didn't finish";
}

TEST(TaskQueue, IsDone) {
    pop::TaskQueue<int> queue;
    std::mutex mutex;
    std::unique_lock lock(mutex);
    std::condition_variable cvStarted;
    std::condition_variable cvDone;
    const size_t first = queue.enqueue([&mutex, &cvStarted, &cvDone] {
        std::unique_lock firstLock(mutex);
        cvStarted.notify_all();
        EXPECT_EQ(cvDone.wait_for(firstLock, std::chrono::seconds(1)), std::cv_status::no_timeout);
        return 1;
    });
    EXPECT_FALSE(queue.isDone(first)); // not started
    EXPECT_FALSE(queue.isDone(first+1 + 1)); // doesn't exist
    // ^ This is an implementation detail.
    const size_t second = queue.enqueue([] {
        return 2;
    });
    ASSERT_EQ(cvStarted.wait_for(lock, std::chrono::seconds(1)), std::cv_status::no_timeout);
    cvDone.notify_all();
    lock.unlock();
    // waiting for second guarantees first is done
    EXPECT_EQ(queue.get(second), 2);
    EXPECT_TRUE(queue.isDone(first)); // done
    EXPECT_FALSE(queue.isDone(second)); // already gotten
    EXPECT_EQ(queue.get(first), 1);
}

// TODO: also try to test for races somehow

TEST(TaskQueue, Delayed) {
    pop::TaskQueue<int> queue;
    const size_t first = queue.enqueue([] {
        return 1;
    });
    // wait for task to finish ...
    EXPECT_EQ(queue.get(first), 1);
    // ... then immediately start another one
    const size_t second = queue.enqueue([] {
        return 2;
    });
    // wait for task to finish ...
    EXPECT_EQ(queue.get(second), 2);
    // 10ms should hopefully make sure the worker thread stops if it ever stops
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // ... and then run another one
    const size_t third = queue.enqueue([] {
        return 3;
    });
    EXPECT_EQ(queue.get(third), 3);
}

TEST(TaskQueue, Prioritize) {
    pop::TaskQueue<int> queue;
    std::vector<int> order;
    std::mutex mutex;
    std::unique_lock lock(mutex);
    std::condition_variable cvStarted;
    std::condition_variable cvDone;
    std::vector<size_t> taskNumbers;
    taskNumbers.push_back(queue.enqueue([&mutex, &cvStarted, &cvDone, &order] {
        std::unique_lock firstLock(mutex);
        cvStarted.notify_all();
        EXPECT_EQ(cvDone.wait_for(firstLock, std::chrono::seconds(1)), std::cv_status::no_timeout);
        order.emplace_back(1);
        return 1;
    }));
    for (size_t i = 2; i <= 5; i++) {
        taskNumbers.push_back(queue.enqueue([&mutex, i, &order] {
            std::lock_guard nthLock(mutex);
            order.emplace_back(i);
            return i;
        }));
    }
    // wait for task to start
    ASSERT_EQ(cvStarted.wait_for(lock, std::chrono::seconds(1)), std::cv_status::no_timeout);
    // prioritize task 5 and task 3
    queue.prioritize(taskNumbers[4]);
    queue.prioritize(taskNumbers[2]);
    // let the tasks run
    cvDone.notify_all();
    lock.unlock();
    // wait for all tasks to finish
    for (const auto taskNumber : taskNumbers) {
        queue.get(taskNumber);
    }
    // order tasks finished in should be 1, 5, 3, 2, 4
    ASSERT_EQ(order.size(), 5u);
    const std::vector<int> expectedOrder{1, 5, 3, 2, 4};
    EXPECT_EQ(order, expectedOrder);
}

TEST(TaskQueue, AutoCancel) {
    {
        pop::TaskQueue<int> queue;
        queue.enqueue([] {
            return 1;
        });
        queue.enqueue([] {
            return 2;
        });
    } // queue goes out of scope here and runs deconstructor
}
