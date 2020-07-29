#pragma once
 
#include <vector>
#include <thread>
#include <memory>
#include <future>
#include <functional>
#include <type_traits>
#include <queue>
#include <cassert>
#include <mutex>


class threadpool
{
private:
    template <typename T>
    class workqueue
    {
    private:
        std::mutex              d_mutex;
        std::condition_variable d_condition;
        std::queue<T>           d_queue;
    public:
        void push(T const& value) {
            {
                std::unique_lock<std::mutex> lock(this->d_mutex);
                d_queue.push(value);
            }
            this->d_condition.notify_one();
        }
        T pop() {
            std::unique_lock<std::mutex> lock(this->d_mutex);
            this->d_condition.wait(lock, [=, this]{ return !this->d_queue.empty(); });
            T rc(std::move(this->d_queue.front()));
            this->d_queue.pop();
            return rc;
        }
    };
    std::vector<std::thread> m_workers;
    workqueue<std::function<void(void)>> m_work;
public:
    threadpool(unsigned numworkers = std::thread::hardware_concurrency())
    {
        for(unsigned i = 0; i < numworkers; i++)
        {
            m_workers.push_back(std::thread([this]()
            {
                std::function<void(void)> work;
                while(true)
                {
                    work = m_work.pop();
                    if(work != nullptr)
                    {
                        work();
                    }
                    else
                    {
                        m_work.push(nullptr);
                        break;
                    }
                    
                }
            }));
        }
    }
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using ret = typename std::result_of<F(Args...)>::type;
        //This Is magick. 
        auto task = std::make_shared<std::packaged_task<ret()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<ret> res = task->get_future();
        m_work.push([task]()
        {
            (*task)();
        });
        return res;
    }
    ~threadpool()
    {
        m_work.push(nullptr);
        for(auto &t : m_workers)
        {
            t.join();
        }
    }
};