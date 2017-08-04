#pragma once

#include <Windows.h>

class PrivilegeManager
{
public:
	PrivilegeManager();
	~PrivilegeManager();

	void SetPrivilege(LPCTSTR szPrivilege);

private:
	static HANDLE OpenProcessToken();

private:
	HANDLE m_hToken;
};
