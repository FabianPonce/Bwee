#include "StdAfx.h"

initialiseSingleton(BweeGlobalStopEvent);
initialiseSingleton(ThreadPool);

ThreadPoolPool::ThreadPoolPool() : IRunnable()
{
	Thread th(this);
	th.start();
}

ThreadPoolPool::~ThreadPoolPool()
{
	m_tasks.clear();
}

void ThreadPoolPool::ExecuteTask(ThreadContext* pRunnable)
{
	Guard g(m_lock);
	m_tasks.push_back(pRunnable);
}

void ThreadPoolPool::AbortTask(ThreadContext* pRunnable)
{
	for(list<ThreadContext*>::iterator itr = m_tasks.begin(); itr != m_tasks.end(); ++itr)
	{
		if( (*itr) == pRunnable )
		{
			m_tasks.erase(itr);
			break;
		}
	}
}

void ThreadPoolPool::Update()
{
	while(1)
	{
		m_lock.Acquire();
		for(list<ThreadContext*>::iterator itr = m_tasks.begin(); itr != m_tasks.end(); )
		{
			list<ThreadContext*>::iterator itrOld = itr;
			++itr;
			(*itrOld)->Update(); // in case of AbortEvent mid-execution
		}
		m_lock.Release();
		Sleep(5);
	}
}

ThreadPool::ThreadPool()
{
	// spawn pools
	for(size_t i = 0; i < THREAD_POOL_SIZE; ++i)
		m_pools[i] = new ThreadPoolPool();
}

ThreadPool::~ThreadPool()
{
	// destroy pools
	for(size_t i = 0; i < THREAD_POOL_SIZE; ++i)
		delete m_pools[i];
};

void ThreadPool::ExecuteTask(ThreadContext* pRunnable)
{
	// search for empty/unused pools
	for(size_t i = 0; i < THREAD_POOL_SIZE; ++i)
	{
		if( m_pools[i]->Pooled() == 0 )
		{
			m_pools[i]->ExecuteTask(pRunnable);
			return;
		}
	}

	// use the least loaded pool.
	size_t leastLoad = 0;
	size_t leastLoadId = 0;
	for(size_t i = 0; i < THREAD_POOL_SIZE; ++i)
	{
		if( m_pools[i]->Pooled() < leastLoad )
		{
			leastLoad = m_pools[i]->Pooled();
			leastLoadId = i;
		}
	}

	m_pools[leastLoadId]->ExecuteTask(pRunnable);
}

void ThreadPool::AbortTask(ThreadContext* pRunnable)
{
	for(size_t i = 0; i < THREAD_POOL_SIZE; ++i)
		m_pools[i]->AbortTask(pRunnable);
}

void ThreadPool::PrintStatus()
{
	Log.Notice("ThreadPool", "Status Check");
	Log.Notice("ThreadPool", "-----------------------------------------------------");
	Log.Notice("ThreadPool", "Pool Size: %u", THREAD_POOL_SIZE);
	for(size_t i = 0; i < THREAD_POOL_SIZE; ++i)
		Log.Notice("ThreadPool", "Pool %u Size: %u", i, m_pools[i]->Pooled());
}