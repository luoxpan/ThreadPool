#include "ThreadPool.h"

ThreadPool::ThreadPool(int init_size):
    m_init_threads_size(init_size),
    m_mutex(),
    m_cond(),
    m_is_running(false)
{
    start();
}

ThreadPool::~ThreadPool()
{
    if (m_is_running)
    {
        stop();
    }
}

void ThreadPool::start()
{
    assert(m_threads.empty());
    m_is_running = true;
    m_threads.reserve(m_init_threads_size);
    for (int i = 0; i < m_init_threads_size; ++i)
    {
        m_threads.push_back(new std::thread(std::bind(&ThreadPool::thread_loop, this)));
    }

}

void ThreadPool::stop()
{
    std::cout << "thread_pool stop. " << std::endl;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_is_running = false;
        m_cond.notify_all();//通知所有挂起线程
    }

    for (threads_t::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
    {
        (*it)->join();
        delete* it;
    }
    m_threads.clear();
}


void ThreadPool::thread_loop()
{
    std::cout << "thread " << gettid() << " start" << std::endl;
    while (m_is_running)
    {
        task_t task = take();
        if (task)
        {
            task();
        }
    }
    std::cout << "thread " << gettid() << " exit." << std::endl;
}

void ThreadPool::add_task(const task_t& task)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_tasks.push_back(task);
    m_cond.notify_one();
}

ThreadPool::task_t ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    //这里的wait必须在while循环内部,不能是if
    while (m_tasks.empty() && m_is_running)
    {
        m_cond.wait(lock);
    }
    task_t task;
    auto size = m_tasks.size();
    if (!m_tasks.empty() && m_is_running)
    {
        task = m_tasks.front();
        m_tasks.pop_front();
    }
    return task;
}
