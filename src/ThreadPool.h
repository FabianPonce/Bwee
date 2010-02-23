#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include "StdAfx.h"

#define THREAD_POOL_SIZE 4

struct ThreadContext
{
	virtual void Update() {}
};

class ThreadPoolPool : public IRunnable
{
private:
	list<ThreadContext*> m_tasks;
	Mutex m_lock;

public:
	ThreadPoolPool();
	~ThreadPoolPool();

	void ExecuteTask(ThreadContext* pRunnable);
	void AbortTask(ThreadContext* pRunnable);
	size_t Pooled() { return m_tasks.size(); }
	void Update();
};

class ThreadPool : public Singleton<ThreadPool>
{
private:
	ThreadPoolPool* m_pools[THREAD_POOL_SIZE];

public:
	ThreadPool();
	~ThreadPool();

	void ExecuteTask(ThreadContext* pRunnable);
	void AbortTask(ThreadContext* pRunnable);
	void PrintStatus();
};

#endif