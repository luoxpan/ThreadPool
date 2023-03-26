#include "ThreadPool.h"

ThreadPool::ThreadPool(int init_size):
    m_init_threads_size(
        init_size>MAX_THREADS_NUM ? MAX_THREADS_NUM:init_size
        ),
    m_max_tasks_num(MAX_TASKS_NUM),
    m_max_threads_num(MAX_THREADS_NUM),
    m_mutex(),
    m_not_empty(),
    m_not_full(),
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
        m_not_empty.notify_all();//通知所有挂起线程
        m_not_full.notify_all();
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
    std::cout << "thread " << std::this_thread::get_id() << " start" << std::endl;
    while (m_is_running)
    {
        task_t task = take();
        if (task)
        {
            task();
        }
    }
    std::cout << "thread " << std::this_thread::get_id() << " exit." << std::endl;
}

void ThreadPool::add_task(const task_t& task)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_tasks.size() >= m_max_tasks_num && m_is_running) {
            m_not_full.wait(lock);
        }
        if (m_tasks.size() < m_max_tasks_num&&m_is_running) {
            m_tasks.push_back(task);
            m_not_empty.notify_one();
        }
    }
}

ThreadPool::task_t ThreadPool::take()
{
    task_t task;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        //这里的wait必须在while循环内部,不能是if
        while (m_tasks.empty() && m_is_running)
        {
            m_not_empty.wait(lock);
        }
        
        if (!m_tasks.empty() && m_is_running)
        {
            task = m_tasks.front();
            m_tasks.pop_front();
            m_not_full.notify_one();
        }
    }
    return task;
}
