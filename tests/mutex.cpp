#include <gtest/gtest.h>

import std;
import crab_cpp;

using namespace crab_cpp;

// The owning mutex itself cannot be copied or moved, while the guard can only
// be moved.
static_assert(!std::is_copy_constructible_v<Mutex<int>>);
static_assert(!std::is_move_constructible_v<Mutex<int>>);
static_assert(!std::is_copy_assignable_v<Mutex<int>>);
static_assert(!std::is_move_assignable_v<Mutex<int>>);

static_assert(!std::is_copy_constructible_v<MutexGuard<int>>);
static_assert(!std::is_copy_assignable_v<MutexGuard<int>>);
static_assert(std::is_move_constructible_v<MutexGuard<int>>);
static_assert(std::is_move_assignable_v<MutexGuard<int>>);

TEST(MutexTest, LockWhenFree)
{
    Mutex<int> m{ 0 };

    {
        auto guard = m.lock();
        EXPECT_EQ(*guard, 0);

        *guard = 42;
    }

    EXPECT_EQ(m.get_mut(), 42);
}

TEST(MutexTest, LockOnConstMutex)
{
    // Interior mutability: a const mutex can still be locked and mutated.
    const Mutex<int> m{ 5 };

    {
        auto guard = m.lock();
        EXPECT_EQ(*guard, 5);

        *guard = 6;
    }

    auto guard = m.lock();
    EXPECT_EQ(*guard, 6);
}

TEST(MutexTest, TryLockWhenFree)
{
    Mutex<int> m{ 1 };

    {
        auto maybe = m.try_lock();
        ASSERT_TRUE(maybe.is_some());

        *maybe.unwrap() = 2;
    }

    EXPECT_EQ(m.get_mut(), 2);
}

TEST(MutexTest, TryLockOnConstMutexWhenFree)
{
    const Mutex<int> m{ 7 };

    auto maybe = m.try_lock();
    ASSERT_TRUE(maybe.is_some());
    EXPECT_EQ(*maybe.unwrap(), 7);
}

TEST(MutexTest, TryLockWhenHeldByOtherThread)
{
    Mutex<int> m{ 0 };

    // The main thread holds the lock for the whole worker's lifetime.
    auto guard = m.lock();

    std::atomic<bool> acquired{ false };
    std::thread worker{ [&]
    {
        auto maybe = m.try_lock();
        acquired = maybe.is_some();
    } };
    worker.join();

    // try_lock from another thread must fail while the lock is held.
    EXPECT_FALSE(acquired);
}

TEST(MutexTest, TryLockSucceedsAfterOtherThreadReleases)
{
    Mutex<int> m{ 0 };

    std::atomic<bool> worker_holds{ false };
    std::atomic<bool> release_worker{ false };
    std::atomic<bool> locked_while_held{ false };

    std::thread worker{ [&]
    {
        auto guard = m.lock();
        worker_holds = true;

        // Hold the lock until the main thread signals it may release.
        while (!release_worker)
        {
            std::this_thread::yield();
        }
    } };

    while (!worker_holds)
    {
        std::this_thread::yield();
    }

    // Held by the worker: try_lock must report "would block".
    locked_while_held = m.try_lock().is_none();
    EXPECT_TRUE(locked_while_held);

    release_worker = true;
    worker.join();

    // Released by the worker: try_lock must now succeed.
    auto maybe = m.try_lock();
    EXPECT_TRUE(maybe.is_some());
}

TEST(MutexTest, LockBlocksUntilReleased)
{
    Mutex<int> m{ 0 };

    std::atomic<bool> worker_started{ false };
    std::atomic<bool> worker_acquired{ false };

    std::thread worker;
    {
        // The main thread holds the lock, so the worker's lock() must block.
        auto guard = m.lock();

        worker = std::thread{ [&]
        {
            worker_started = true;

            auto inner = m.lock();
            worker_acquired = true;
        } };

        while (!worker_started)
        {
            std::this_thread::yield();
        }

        // The worker cannot have acquired the lock while it is held.
        EXPECT_FALSE(worker_acquired);
    } // The main thread releases the lock here.

    worker.join();

    EXPECT_TRUE(worker_acquired);
}

TEST(MutexTest, TryLockUnderContention)
{
    Mutex<int> counter{ 0 };

    constexpr int thread_count = 4;
    constexpr int iterations = 20000;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (int i = 0; i < thread_count; ++i)
    {
        workers.emplace_back([&]
        {
            for (int j = 0; j < iterations; ++j)
            {
                while (true)
                {
                    auto maybe = counter.try_lock();

                    if (maybe.is_some())
                    {
                        *maybe.unwrap() += 1;
                        break;
                    }

                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    EXPECT_EQ(counter.get_mut(), thread_count * iterations);
}

TEST(MutexTest, LockUnderContention)
{
    Mutex<int> counter{ 0 };

    constexpr int thread_count = 4;
    constexpr int iterations = 20000;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (int i = 0; i < thread_count; ++i)
    {
        workers.emplace_back([&]
        {
            for (int j = 0; j < iterations; ++j)
            {
                auto guard = counter.lock();
                *guard += 1;
            }
        });
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    EXPECT_EQ(counter.get_mut(), thread_count * iterations);
}

TEST(MutexTest, LockReleasedOnException)
{
    Mutex<std::vector<int>> data{ { 1, 2, 3 } };

    try
    {
        auto guard = data.lock();
        guard->push_back(4);

        throw std::runtime_error{ "boom" };
    }
    catch (const std::runtime_error&)
    {
    }

    // If the lock had leaked, this would deadlock.
    auto guard = data.lock();
    EXPECT_EQ(guard->size(), 4);
}

TEST(MutexTest, GuardMoveTransfersLock)
{
    Mutex<int> m{ 1 };

    {
        auto first = m.lock();
        auto second = std::move(first);

        *second = 2;
    }

    // The guard must unlock exactly once: re-locking would deadlock on a leak.
    auto guard = m.lock();
    EXPECT_EQ(*guard, 2);
}

TEST(MutexTest, GuardMoveAssignmentTransfersLock)
{
    Mutex<int> a{ 1 };
    Mutex<int> b{ 2 };

    {
        auto first = a.lock();
        auto second = b.lock();

        // Move assignment releases b (the target's old lock) and takes over a.
        second = std::move(first);
        *second = 3;

        // b must be free again right after the assignment.
        auto guard_b = b.try_lock();
        EXPECT_TRUE(guard_b.is_some());
    }

    // Both mutexes are free a second time: exactly one unlock each.
    auto guard_a = a.lock();
    auto guard_b = b.lock();
    EXPECT_EQ(*guard_a, 3);
    EXPECT_EQ(*guard_b, 2);
}

TEST(MutexTest, GetMut)
{
    Mutex<int> m{ 3 };

    m.get_mut() = 4;
    EXPECT_EQ(m.get_mut(), 4);
}

TEST(MutexTest, IntoInner)
{
    Mutex<std::string> m{ "hello" };

    auto value = std::move(m).into_inner();
    EXPECT_EQ(value, "hello");
}
