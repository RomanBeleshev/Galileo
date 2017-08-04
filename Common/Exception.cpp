#include "Exception.h"

Exception::Exception(LONG errorCode) :
	m_message(FormatMessage(errorCode)) 
{
}

std::wstring const& Exception::Message() const
{
	return m_message;
}

std::wstring Exception::FormatMessage(LONG errorCode)
{
	TCHAR buffer[1024];
	::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, NULL, buffer, 1024, NULL);
	return std::wstring(buffer);
}

void HandleExplicitError(LONG errorCode)
{
	if (errorCode != ERROR_SUCCESS)
	{
		throw Exception(errorCode);
	}
}

void HandleImplicitError(BOOL result)
{
	if (result == FALSE)
	{
		throw Exception(::GetLastError());
	}
}
