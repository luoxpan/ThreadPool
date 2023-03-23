# ThreadPool

## 运行
`cmake .` 

`./out/build/linux-debug/ThreadPool`

## 原理
本版本为基础实现，创建线程池时给定线程池中线程数量，该值不会改变，线程数量固定，直到调用stop()方法，线程数量才会减少。

构造函数赋初值并启动start().
start()的主要功能是创建线程
``` 
ThreadPool::ThreadPool(int init_size):
    m_init_threads_size(init_size),
    m_mutex(),
    m_cond(),
    m_is_running(false)
{
    start();
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
```
这些创建出来的线程一直运行thread_loop().故名思意，这个函数是一个循环，
只要线程池m_is_running就一直循环去从任务队列里面取一个任务。取到可用的任务
，就执行这个任务。

注意:task的类型是`std::function<void()>` .一个无参无返回值的仿函数

`typedef std::function<void()> task_t` 
``` 
void ThreadPool::thread_loop()
{
    std::cout << "thread " << gettid() << " start" << std::endl;
    while (m_is_running)
    {
        //尝试去取一个任务来完成
        task_t task = take();
        if (task)
        {
            task();
        }
    }
    std::cout << "thread " << gettid() << " exit." << std::endl;
}
``` 
使用take()取任务。从任务队列里取任务，首先加锁，然后判断任务列表是否为空，以及
线程池是否还在运行，如果还在运行且任务队列为空就调用条件变量的wait()让线程阻塞。
因为任务队列里没有任务嘛，就让线程等着呗。

注意：wait()放在while循环里，而不能是if。假设用if，这里的wait()可能挂起了多个线程，
条件变量在其它地方调用notify_all()时，这些线程都会向下执行，导致数据竞争。

如果线程池还在运行且任务队列不为空，就取出一个任务，返回给调用take()的线程。

```
//实现1 正确 建议使用的实现
ThreadPool::task_t ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    //这里的wait必须在while循环内部,不能是if
    while (m_tasks.empty() && m_is_running)
    {
        m_cond.wait(lock);
    }
    task_t task;
    if (!m_tasks.empty() && m_is_running)
    {
        task = m_tasks.front();
        m_tasks.pop_front();
    }
    return task;
}
```
也可以把while中的m_is_running提出来。
```
//实现2 正确
ThreadPool::task_t ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    task_t task;
    if(!m_is_running){
        return task;
    }
    //这里的wait必须在while循环内部,不能是if
    while (m_tasks.empty() && m_is_running)
    {
        m_cond.wait(lock);
    }
    if (!m_tasks.empty() && m_is_running)
    {
        task = m_tasks.front();
        m_tasks.pop_front();
    }
    return task;
}
```
但不能把两个m_is_running都提取出来

注意：下面的实现是错误的！！！。

原因：看看stop()的实现，令m_is_running=false后调用notify_all()，
这里挂起的线程都会唤醒，如果是下面这种实现，明明用户叫停了，还会有一些线程
不听话的去取任务来做。
``` 
//实现3 错误
ThreadPool::task_t ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    task_t task;
    if (!m_is_running) {
        return task;//这个默认构造的task是无法使用的
    }
    //这里的wait必须在while循环内部,不能是if
    while (m_tasks.empty())
    {
        m_cond.wait(lock);
    }
    if (!m_tasks.empty())
    {
        task = m_tasks.front();
        m_tasks.pop_front();
    }
    return task;
}
```

线程池还需要两个public 函数接口，一个是add_task(),用户往任务列表里加入任务，
还有一个是stop()，用户可以主动停止线程池，当然，如果用户不主动停止，
线程池的析构函数里也会调用stop().

首先是add_task()的实现。这个基础版本没有限制最大队列数，所以实现很简单，
先上锁，然后将任务加入任务队列，通知因队列为空而等待的线程即可。
```
void ThreadPool::add_task(const task_t& task)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_tasks.push_back(task);
    m_cond.notify_one();
}
```

然后是stop()的实现。上锁，然后令m_is_running=false,通知那些被
阻塞在take()函数中的线程。这些线程被唤醒了，但由于m_is_running已经是
false了，所以他们在take()中取到了无效的task,再把这个无效的task返回到thread_loop()中，
，再thread_loop中，收到take()的无效task且已经中止运行了，线程退出。

```
void ThreadPool::stop()
{
    std::cout << "thread_pool stop. " << std::endl;
    {   //注意这个花括号是必不可少的，它用来限定unique_lock锁的范围
        std::unique_lock<std::mutex> lock(m_mutex);
        m_is_running = false;
        m_cond.notify_all();//通知所有挂起线程
    }
    //下面的实现和上面一样的。使用unique_lock更方便，但不要忽略花括号
    /* 
    m_mutex.lock();
    m_is_running = false;
    m_cond.notify_all();//通知所有挂起线程
    m_mutex.unlock();
    */
    for (threads_t::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
    {
        (*it)->join();
        delete* it;
    }
    m_threads.clear();
}
```

注意：当stop()被调用时，一些子线程可能还没有做完任务，也就是还没从thread_loop()的循环中
退出，因此，stop()中还要对每个线程调用join(),等待所有子线程退出。




