#pragma once

#include <wtypes.h>

class CommunicationPort
{
public:
	CommunicationPort(LPCWSTR portName);
	~CommunicationPort();

	void GetMessage(PVOID buffer, DWORD size);
	void ReplyMessage(PVOID buffer, DWORD size);

	static char const* GetMessageBody(PVOID buffer);

	static char* GetMessageReplyBody(PVOID buffer);
	static ULONG GetMaxReplyBodySize(PVOID buffer);

private:
	static HANDLE Connect(LPCWSTR portName);

private:
	HANDLE m_port;
	ULONGLONG m_lastMessageId;
};

