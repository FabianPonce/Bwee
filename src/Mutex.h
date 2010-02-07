/*
* Aspire Hearthstone
* Copyright (C) 2008 AspireDev <http://www.aspiredev.org/>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef _THREADING_MUTEX_H
#define _THREADING_MUTEX_H
#include "StdAfx.h"

class Mutex
{
public:
	friend class Condition;

	/** Initializes a mutex class, with InitializeCriticalSection / pthread_mutex_init
	*/
	Mutex();

	/** Deletes the associated critical section / mutex
	*/
	~Mutex();

	/** Acquires this mutex. If it cannot be acquired immediately, it will block.
	*/
	BWEE_INLINE void Acquire()
	{
#ifndef WIN32
		pthread_mutex_lock(&mutex);
#else
		EnterCriticalSection(&cs);
#endif
	}

	/** Releases this mutex. No error checking performed
	*/
	BWEE_INLINE void Release()
	{
#ifndef WIN32
		pthread_mutex_unlock(&mutex);
#else
		LeaveCriticalSection(&cs);
#endif
	}

	/** Attempts to acquire this mutex. If it cannot be acquired (held by another thread)
	* it will return false.
	* @return false if cannot be acquired, true if it was acquired.
	*/
	BWEE_INLINE bool AttemptAcquire()
	{
#ifndef WIN32
		return (pthread_mutex_trylock(&mutex) == 0);
#else
		return (TryEnterCriticalSection(&cs) == TRUE ? true : false);
#endif
	}

protected:
#ifdef WIN32
	/** Critical section used for system calls
	*/
	CRITICAL_SECTION cs;

#else
	/** Static mutex attribute
	*/
	static bool attr_initalized;
	static pthread_mutexattr_t attr;

	/** pthread struct used in system calls
	*/
	pthread_mutex_t mutex;
#endif
};

#ifdef WIN32

class FastMutex
{
#pragma pack(push,8)
	volatile long m_lock;
#pragma pack(pop)
	DWORD m_recursiveCount;

public:
	BWEE_INLINE FastMutex() : m_lock(0),m_recursiveCount(0) {}
	BWEE_INLINE ~FastMutex() {}

	BWEE_INLINE void Acquire()
	{
		DWORD thread_id = GetCurrentThreadId(), owner;
		if(thread_id == (DWORD)m_lock)
		{
			++m_recursiveCount;
			return;
		}

		for(;;)
		{
			owner = InterlockedCompareExchange(&m_lock, thread_id, 0);
			if(owner == 0)
				break;

			Sleep(0);
		}

		++m_recursiveCount;
	}

	BWEE_INLINE bool AttemptAcquire()
	{
		DWORD thread_id = GetCurrentThreadId();
		if(thread_id == (DWORD)m_lock)
		{
			++m_recursiveCount;
			return true;
		}

		DWORD owner = InterlockedCompareExchange(&m_lock, thread_id, 0);
		if(owner == 0)
		{
			++m_recursiveCount;
			return true;
		}

		return false;
	}

	BWEE_INLINE void Release()
	{
		if((--m_recursiveCount) == 0)
			InterlockedExchange(&m_lock, 0);
	}
};
#else
#define FastMutex Mutex
#endif

class Guard
{
private:
	Mutex& m_mutex;
public:
	Guard(Mutex& pMutex) : m_mutex(pMutex)
	{
		m_mutex.Acquire();
	}

	~Guard()
	{
		m_mutex.Release();
	}
};

#endif

