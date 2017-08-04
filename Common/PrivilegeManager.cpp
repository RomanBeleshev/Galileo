#include "PrivilegeManager.h"

#include "Exception.h"

PrivilegeManager::PrivilegeManager() :
	m_hToken(OpenProcessToken())
{
}

PrivilegeManager::~PrivilegeManager()
{
	::CloseHandle(m_hToken);
}

void PrivilegeManager::SetPrivilege(LPCTSTR szPrivilege)
{
	LUID luid;
	HandleImplicitError(::LookupPrivilegeValue(NULL, szPrivilege, &luid));

	TOKEN_PRIVILEGES NewState;
	NewState.PrivilegeCount = 1;
	NewState.Privileges[0].Luid = luid;
	NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	HandleImplicitError(::AdjustTokenPrivileges(m_hToken, FALSE, &NewState, sizeof(NewState), NULL, NULL));
}

HANDLE PrivilegeManager::OpenProcessToken()
{
	HANDLE hToken = INVALID_HANDLE_VALUE;
	HandleImplicitError(::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken));
	return hToken;
}