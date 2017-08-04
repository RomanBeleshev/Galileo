#include "WinApi.h"

#include "Exception.h"

#include <Windows.h>

std::string FromWideString(std::wstring const& s)
{
	CHAR buffer[1024];
	HandleImplicitError(::WideCharToMultiByte(CP_ACP, 0, s.c_str(), -1, buffer, 1024, NULL, FALSE));
	return std::string(buffer);
}

std::wstring ToWideString(std::string const& s)
{
	TCHAR buffer[1024];
	HandleImplicitError(::MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, buffer, 1024));
	return std::wstring(buffer);
}
