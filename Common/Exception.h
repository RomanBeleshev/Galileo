#pragma once

#include <Windows.h>

#include <string>

class Exception
{
public:
	Exception(LONG errorCode);
	std::wstring const& Message() const;

private:
	static std::wstring FormatMessage(LONG errorCode);

private:
	std::wstring m_message;
};

void HandleExplicitError(LONG errorCode);
void HandleImplicitError(BOOL result);
