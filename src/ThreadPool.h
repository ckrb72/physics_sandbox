#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

class ThreadPool
{
    private:
        std::vector<std::thread> threads;
        std::mutex queue_mutex;
        std::queue<std::function<void()>> job_queue;
        std::condition_variable wait_cv;

        bool destroy_thread = false;

    public:
        ThreadPool(uint32_t thd_count);
        ~ThreadPool();
        void enqueue(std::function<void()> job);
};