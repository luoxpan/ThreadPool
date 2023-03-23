# ThreadPool

## ����
`cmake .` 

`./out/build/linux-debug/ThreadPool`

## ԭ��
���汾Ϊ����ʵ�֣������̳߳�ʱ�����̳߳����߳���������ֵ����ı䣬�߳������̶���ֱ������stop()�������߳������Ż���١�

���캯������ֵ������start().
start()����Ҫ�����Ǵ����߳�
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
��Щ�����������߳�һֱ����thread_loop().����˼�⣬���������һ��ѭ����
ֻҪ�̳߳�m_is_running��һֱѭ��ȥ�������������ȡһ������ȡ�����õ�����
����ִ���������

ע��:task��������`std::function<void()>` .һ���޲��޷���ֵ�ķº���

`typedef std::function<void()> task_t` 
``` 
void ThreadPool::thread_loop()
{
    std::cout << "thread " << gettid() << " start" << std::endl;
    while (m_is_running)
    {
        //����ȥȡһ�����������
        task_t task = take();
        if (task)
        {
            task();
        }
    }
    std::cout << "thread " << gettid() << " exit." << std::endl;
}
``` 
ʹ��take()ȡ���񡣴����������ȡ�������ȼ�����Ȼ���ж������б��Ƿ�Ϊ�գ��Լ�
�̳߳��Ƿ������У���������������������Ϊ�վ͵�������������wait()���߳�������
��Ϊ���������û������������̵߳����¡�

ע�⣺wait()����whileѭ�����������if��������if�������wait()���ܹ����˶���̣߳�
���������������ط�����notify_all()ʱ����Щ�̶߳�������ִ�У��������ݾ�����

����̳߳ػ���������������в�Ϊ�գ���ȡ��һ�����񣬷��ظ�����take()���̡߳�

```
//ʵ��1 ��ȷ ����ʹ�õ�ʵ��
ThreadPool::task_t ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    //�����wait������whileѭ���ڲ�,������if
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
Ҳ���԰�while�е�m_is_running�������
```
//ʵ��2 ��ȷ
ThreadPool::task_t ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    task_t task;
    if(!m_is_running){
        return task;
    }
    //�����wait������whileѭ���ڲ�,������if
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
�����ܰ�����m_is_running����ȡ����

ע�⣺�����ʵ���Ǵ���ģ�������

ԭ�򣺿���stop()��ʵ�֣���m_is_running=false�����notify_all()��
���������̶߳��ỽ�ѣ��������������ʵ�֣������û���ͣ�ˣ�������һЩ�߳�
��������ȥȡ����������
``` 
//ʵ��3 ����
ThreadPool::task_t ThreadPool::take()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    task_t task;
    if (!m_is_running) {
        return task;//���Ĭ�Ϲ����task���޷�ʹ�õ�
    }
    //�����wait������whileѭ���ڲ�,������if
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

�̳߳ػ���Ҫ����public �����ӿڣ�һ����add_task(),�û��������б����������
����һ����stop()���û���������ֹͣ�̳߳أ���Ȼ������û�������ֹͣ��
�̳߳ص�����������Ҳ�����stop().

������add_task()��ʵ�֡���������汾û��������������������ʵ�ֺܼ򵥣�
��������Ȼ���������������У�֪ͨ�����Ϊ�ն��ȴ����̼߳��ɡ�
```
void ThreadPool::add_task(const task_t& task)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_tasks.push_back(task);
    m_cond.notify_one();
}
```

Ȼ����stop()��ʵ�֡�������Ȼ����m_is_running=false,֪ͨ��Щ��
������take()�����е��̡߳���Щ�̱߳������ˣ�������m_is_running�Ѿ���
false�ˣ�����������take()��ȡ������Ч��task,�ٰ������Ч��task���ص�thread_loop()�У�
����thread_loop�У��յ�take()����Чtask���Ѿ���ֹ�����ˣ��߳��˳���

```
void ThreadPool::stop()
{
    std::cout << "thread_pool stop. " << std::endl;
    {   //ע������������Ǳز����ٵģ��������޶�unique_lock���ķ�Χ
        std::unique_lock<std::mutex> lock(m_mutex);
        m_is_running = false;
        m_cond.notify_all();//֪ͨ���й����߳�
    }
    //�����ʵ�ֺ�����һ���ġ�ʹ��unique_lock�����㣬����Ҫ���Ի�����
    /* 
    m_mutex.lock();
    m_is_running = false;
    m_cond.notify_all();//֪ͨ���й����߳�
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

ע�⣺��stop()������ʱ��һЩ���߳̿��ܻ�û����������Ҳ���ǻ�û��thread_loop()��ѭ����
�˳�����ˣ�stop()�л�Ҫ��ÿ���̵߳���join(),�ȴ��������߳��˳���




