/**
 * @file test_threadpool.cpp
 * @brief Unit tests for ThreadPool shutdown behavior (Phase 3.2)
 * 
 * Tests cover:
 * - Safe initialization (members init before workers start)
 * - Drain-on-shutdown (100 queued tasks all execute)
 * - Join all threads properly
 * - Exception handling
 */

#include <gtest/gtest.h>
#include "thread_pool.hpp"
#include <atomic>
#include <chrono>
#include <thread>

// Test fixture for ThreadPool tests
class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset counters before each test
        task_counter.store(0);
    }
    
    std::atomic<int> task_counter{0};
};

// Test basic task execution
TEST_F(ThreadPoolTest, BasicTaskExecution) {
    ThreadPool<std::function<void()>> pool(2, 10, 50);  // 2 worker threads, 10 task queue, priority 50
    
    pool.addTask([this]() {
        task_counter++;
    });
    
    // Give time for task to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(task_counter.load(), 1);
}

// Test multiple tasks
TEST_F(ThreadPoolTest, MultipleTasks_10) {
    ThreadPool<std::function<void()>> pool(4, 20, 50);  // 4 worker threads, 20 task queue, priority 50
    
    for (int i = 0; i < 10; ++i) {
        pool.addTask([this]() {
            task_counter++;
        });
    }
    
    // Give time for all tasks to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(task_counter.load(), 10);
}

// Phase 3.2 acceptance test: 100 queued tasks drain before shutdown
TEST_F(ThreadPoolTest, Acceptance_DrainOnShutdown_100Tasks) {
    {
        ThreadPool<std::function<void()>> pool(4, 150, 50);  // 4 worker threads, 150 task queue (for 100 tasks), priority 50
        
        // Queue 100 tasks
        for (int i = 0; i < 100; ++i) {
            pool.addTask([this]() {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                task_counter++;
            });
        }
        
        // Destructor will be called here, which should:
        // 1. Set stop flag
        // 2. Drain queue (execute remaining tasks)
        // 3. Join all threads
    }
    
    // After ThreadPool destruction, all 100 tasks should have executed
    EXPECT_EQ(task_counter.load(), 100);
}

// Test thread count
TEST_F(ThreadPoolTest, CorrectThreadCount) {
    ThreadPool<std::function<void()>> pool(8, 32, 50);  // 8 worker threads, 32 task queue, priority 50
    
    // Verify by checking that 8 tasks can run concurrently
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};
    
    for (int i = 0; i < 16; ++i) {
        pool.addTask([&concurrent_count, &max_concurrent]() {
            int current = ++concurrent_count;
            
            // Update max if needed
            int expected = max_concurrent.load();
            while (current > expected && 
                   !max_concurrent.compare_exchange_weak(expected, current)) {
                expected = max_concurrent.load();
            }
            
            // Hold for a bit to ensure concurrency
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            --concurrent_count;
        });
    }
    
    // Give time for tasks to run
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Max concurrent should be close to 8 (thread pool size)
    EXPECT_GE(max_concurrent.load(), 7);  // Allow some scheduling variance
    EXPECT_LE(max_concurrent.load(), 8);
}

// Test task ordering (FIFO queue behavior)
TEST_F(ThreadPoolTest, TaskOrdering_FIFO) {
    ThreadPool<std::function<void()>> pool(1, 20, 50);  // Single thread for deterministic ordering, 20 task queue, priority 50
    
    std::vector<int> execution_order;
    std::mutex order_mutex;
    
    // Queue 10 tasks in order
    for (int i = 0; i < 10; ++i) {
        pool.addTask([i, &execution_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(i);
        });
    }
    
    // Give time for all tasks to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Verify FIFO order
    ASSERT_EQ(execution_order.size(), 10);
    for (size_t i = 0; i < execution_order.size(); ++i) {
        EXPECT_EQ(execution_order[i], static_cast<int>(i));
    }
}

// Test exception in task doesn't crash pool
TEST_F(ThreadPoolTest, ExceptionInTask_NoCrash) {
    ThreadPool<std::function<void()>> pool(2, 10, 50);  // 2 worker threads, 10 task queue, priority 50
    
    // Add task that throws
    pool.addTask([]() {
        throw std::runtime_error("Test exception");
    });
    
    // Add normal task after exception
    pool.addTask([this]() {
        task_counter++;
    });
    
    // Give time for both tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Normal task should still execute
    EXPECT_EQ(task_counter.load(), 1);
}

// Test rapid queue/dequeue
TEST_F(ThreadPoolTest, RapidQueueing) {
    ThreadPool<std::function<void()>> pool(4, 1100, 50);  // 4 worker threads, 1100 task queue (for 1000 tasks), priority 50
    
    // Rapidly queue 1000 small tasks
    for (int i = 0; i < 1000; ++i) {
        pool.addTask([this]() {
            task_counter++;
        });
    }
    
    // Give time for all tasks
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    EXPECT_EQ(task_counter.load(), 1000);
}
