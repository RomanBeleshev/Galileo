#pragma once

#include <ntifs.h>

class FastMutex
{
public:
	FastMutex();

	void Lock();
	void Unlock();

private:
	FAST_MUTEX m_mutex;
};

class Lock
{
public:
	explicit Lock(FastMutex& mutex) :
		m_mutex(mutex)
	{
		m_mutex.Lock();
	}

	~Lock()
	{
		m_mutex.Unlock();
	}

	Lock& operator=(const Lock&) = delete;

private:
	FastMutex& m_mutex;
};