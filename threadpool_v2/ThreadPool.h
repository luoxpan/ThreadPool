#pragma once
#include <iostream>
#include <thread>
#include <vector>
#include <deque>
#include <functional>
#include <iterator>
#include <cassert>
#include <mutex>
#include <future>
#include <condition_variable>
const int MAX_TASKS_NUM = 50;
const int MAX_THREADS_NUM = 7;
const int DEFAULT_THREADS_NUM=4;
class  ThreadPool{
public:
	//typedef std::function<void()> task_t;
	using task_t = std::function<void()>;

	ThreadPool(int init_size = DEFAULT_THREADS_NUM);
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	void stop();
	
	template<typename F,typename... Args>
	auto submit(F&& f, Args &&... args) -> std::future<decltype(f(args...))>
	{
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
		std::function<void()>task_func = [task_ptr]()
		{
			(*task_ptr)();
		};
		add_task(task_func);
		return task_ptr->get_future();
	}

private:

	bool is_running() { return m_is_running; }
	void start();

	void thread_loop();
	task_t take();
	void add_task(const task_t&);

	typedef std::vector<std::thread*> threads_t;
	typedef std::deque<task_t> tasks_t;

	int m_init_threads_size;
	int m_max_tasks_num;
	int m_max_threads_num;

	threads_t m_threads;
	tasks_t m_tasks;

	std::mutex m_mutex;
	std::condition_variable m_not_full;
	std::condition_variable m_not_empty;
	bool m_is_running;
};

