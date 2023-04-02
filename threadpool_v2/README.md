# ThreadPool 实现2
在基础版本上进行升级
## 基础修改
* 设置默认线程数为`DEFAULT_THREADS_NUM`;
* 新增任务列表最大长度`m_max_tasks_num`
* 使用`m_not_empty,m_not_full`两个条件变量实现
* 新增最大线程个数`m_max_threads_num;MAX_THREADS_NUM;`
* `using task_t=std::function<void()>`;替代原来的typedef，无他，用下C++11的新语法而已
* 新增`ThreadPool(ThreadPool &&) = delete;ThreadPool &operator=(ThreadPool &&) = delete;`
	* 禁止移动构造和移动赋值构造

## 核心修改
原来的实现只支持`void()`类的任务。现在通过使用可变参数模板实现可以提交任意任务。
* 注意原来的`add_task()`函数不需要更改，但不再向用户开放，即设为private
* 用户通过`submit()`向线程池提交任意参数个数的函数
* 线程池中的`submit()`函数将任意参数个数的函数包装成`std::function<void()>`类型，然后调用`add_task()`;
``` C++
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
```
* `decltype()`是自动推导表达式的类型。`decltype(f(args...))`就是推导函数f的返回值

* `std::bind()`是将函数与参数绑定起来
```
double func(double a,double b){
	return a+b;
}

//std::bind(func,2,3)简单理解为是下面这样的一个函数
double bind_func(){
	a=2;b=3;
	return a+b;
}
//看，我们把一个有两个参数的函数封装成了无参函数。
```
* `std::future`是`std::pckaged_task`运行结果保存，这个对象中保存了运行结果，

	用`get_future()`得到future对象，future的`get()`方法可以获取函数return 的结果
``` C++
	template<typename F,typename... Args>
	auto submit(F&& f, Args &&... args) -> std::future<decltype(f(args...))>
	{
		/*将 double ff(a,b) 包装成 double func();
		* decltype(f(args...))表示自动推导返回值，我们这里返回的就是double
		*/
		std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
		/* 这个func不能直接作为task，因为我们的task是void()类型的
		* 先把这个函数func封装成packaged_task，packaged_task类似与function,
		* 是对可执行对象的封装，且它支持异步执行
		* 再生成packaged_task的shared_ptr指针task_ptr
		*/
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
		/*
		* 利用这个指针和lambda函数，返回一个std::function<void()>对象
		* 这个对象可以作为add_task()的参数了
		*/
		std::function<void()>task_func = [task_ptr]()
		{
			(*task_ptr)();
		};
		add_task(task_func);
		/*返回future对象，这个对象中保存这函数计算结果*/
		return task_ptr->get_future();
	}
```