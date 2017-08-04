#pragma once

#include <ntifs.h>

typedef struct _FLT_FILTER *PFLT_FILTER;
typedef struct _FLT_PORT *PFLT_PORT;

class CommunicationPort
{
public:
	CommunicationPort(PFLT_FILTER filter, PCWSTR portName);
	~CommunicationPort();

	bool SendMessage(PVOID CONST iBuffer, ULONG iSize, PVOID oBuffer, ULONG& oSize);

	// Callbacks
	void UserConnected(PFLT_PORT clientPort);
	void UserDisconnected();
	void UserMessage();

private:
	PFLT_FILTER m_filter;
	PFLT_PORT m_serverPort;
	PFLT_PORT m_clientPort;
};

