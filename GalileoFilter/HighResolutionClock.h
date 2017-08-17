#pragma once

#include <ntifs.h>

class HighResolutionClock
{
public:
	HighResolutionClock() :
		m_startClock(GetClock())
	{
	}

	static LONGLONG GetFrequency()
	{
		static LONGLONG frequency = GetFrequencyOnce();
		return frequency;
	}

	LONGLONG TicksElapsed() const
	{
		return GetClock() - m_startClock;
	}

	LONGLONG MicroElapsed() const
	{
		return (GetClock() - m_startClock) * 1000 * 1000 / GetFrequency();
	}

private:
	static LONGLONG GetFrequencyOnce()
	{
		LARGE_INTEGER frequency;
		::KeQueryPerformanceCounter(&frequency);
		return frequency.QuadPart;
	}

	static LONGLONG GetClock()
	{
		return ::KeQueryPerformanceCounter(NULL).QuadPart;
	}

private:
	LONGLONG m_startClock;
};
