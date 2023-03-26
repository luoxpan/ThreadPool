#include <iostream>
#include <chrono>
#include <condition_variable>
#include "ThreadPool.h"

int testFunc1(int a,int b)
{
	for (int i = 1; i < 4; ++i)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::cout << "testFunc() [1_" << a << "] at thread [ " << std::this_thread::get_id() << "] output" << std::endl;
	}
	return a + b;
}

void testFunc2()
{
	for (int i = 1; i < 4; ++i) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		std::cout << "testFunc() [2_" << i << "] at thread [ " << std::this_thread::get_id() << "] output" << std::endl;
	}
}


int main()
{
	ThreadPool thread_pool;

	for (int i = 0; i < 5; i++) {
		auto future1=thread_pool.submit(testFunc1, i, i+1);
		auto future2 = thread_pool.submit(testFunc2);
		auto res = future1.get();
		std::cout << res << std::endl;
	}
	getchar();
	return 0;
}