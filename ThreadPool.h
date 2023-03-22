#pragma once
#include <iostream>
#include <thread>
#include <vector>
#include <deque>
#include <functional>
#include <iterator>
#include <cassert>
#include <mutex>
#include <condition_variable>

class  ThreadPool{
public:
	typedef std::function<void()> task_t;

	ThreadPool(int init_size = 3);
	~ThreadPool();

	void stop();
	void add_task(const task_t&);

private:
	ThreadPool(const ThreadPool&)=delete;
	const ThreadPool& operator=(const ThreadPool&)=delete;

	bool is_running() { return m_is_running; }
	void start();

	void thread_loop();
	task_t take();

	typedef std::vector<std::thread*> threads_t;
	typedef std::deque<task_t> tasks_t;

	int m_init_threads_size;

	threads_t m_threads;
	tasks_t m_tasks;

	std::mutex m_mutex;
	std::condition_variable m_cond;
	bool m_is_running;
};