#include "ThreadPool.h"
#include <iostream>


ThreadPool::ThreadPool(uint32_t thd_count)
{
    for(int i = 0; i < thd_count; i++)
    {

        threads.push_back(std::thread([this]{
            while(true)
            {
                std::function<void()> job;
                {
                    // Wrap this in its own block so the lock releases automatically at end of scope (doing it explicitly anyway though)
                    //lock mutex and wait until job in queue or destroy thread is called
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    wait_cv.wait(lock, [this]() {
                        return !job_queue.empty() || destroy_thread;
                    });
        
                    // Return if there are no more jobs to do
                    if(destroy_thread && job_queue.empty())
                    {
                        lock.unlock();
                        return;
                    }
        
                    //once awake dequeue job (need to do it like this because of c++ move semantics)
                    job = std::move(job_queue.front());
                    job_queue.pop();
        
                    lock.unlock();
                }
        
                job();
            }
        }));
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        destroy_thread = true;
        lock.unlock();
    }

    wait_cv.notify_all();


    for(int i = 0; i < threads.size(); i++)
    {
        try
        {
            threads[i].join();
        } 
        catch(const std::system_error& e)
        {
            std::cout << e.what() << std::endl;
        }
    }

}

void ThreadPool::enqueue(std::function<void()> job)
{

    std::unique_lock<std::mutex> lock(queue_mutex);
    job_queue.push(job);    // Might need to std::move this

    wait_cv.notify_one();
    lock.unlock();

}