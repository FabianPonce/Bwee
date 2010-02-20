#ifndef _UTIL_H
#define _UTIL_H

#include "StdAfx.h"

class BweeGlobalStopEvent : public Singleton<BweeGlobalStopEvent>
{
	uint32 m_stopCount;
	Mutex m_mutex;

public:
	BweeGlobalStopEvent() : m_stopCount(0), Singleton() {}

	void mark()
	{
		Guard g(m_mutex);
		m_stopCount++;
	}

	void unmark()
	{
		Guard g(m_mutex);
		m_stopCount--;
	}

	bool marked()
	{
		Guard g(m_mutex);
		return (m_stopCount > 0);
	}
};

class Timer
{
private:
	uint32 m_usec;
	uint32 m_interval;
	uint32 m_period;
public:
	Timer(uint32 interval)
	{
		m_interval = interval;
		m_period = 0;
	}
	
	void mark()
	{
		m_usec = getMSTime();
	}

	bool met()
	{
		bool r = m_period <= m_usec;
		if(r)
			m_period = m_usec + m_interval;

		return r;
	}

};

#endif