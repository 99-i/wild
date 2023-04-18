#pragma once
#include <vector>
#include <thread>

struct thread_pool
{
	void start();
private:
	void run_thread();
	std::vector<std::thread> threads;
};