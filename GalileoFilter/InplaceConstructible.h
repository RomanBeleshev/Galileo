#pragma once

void* operator new  (unsigned long long /*count*/, void* ptr)
{
	return ptr;
}

void operator delete (void *)
{
}

template <typename T>
class InplaceConstructible
{
public:
	void Construct()
	{
		m_instance = new(m_memory)T();
	}

	template <typename P1>
	void Construct(P1 const& p1)
	{
		m_instance = new(m_memory)T(p1);
	}

	void Destruct()
	{
		m_instance->~T();
		m_instance = nullptr;
	}

	T* operator ->()
	{
		return m_instance;
	}

private:
	char m_memory[sizeof(T)];
	T* m_instance;
};