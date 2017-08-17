#include "CommunicationPort.h"

#include "Common/Exception.h"

#include <Fltuser.h>

CommunicationPort::CommunicationPort(LPCWSTR portName) :
	m_port(Connect(portName)),
	m_lastMessageId(0)
{
}

CommunicationPort::~CommunicationPort()
{
	::CloseHandle(m_port);
}

HANDLE CommunicationPort::Connect(LPCWSTR portName)
{
	HANDLE result;
	HandleExplicitError(::FilterConnectCommunicationPort(portName, 0, NULL, 0, NULL, &result));
	return result;
}

void CommunicationPort::GetMessage(PVOID buffer, DWORD size)
{
	PFILTER_MESSAGE_HEADER header = static_cast<PFILTER_MESSAGE_HEADER>(buffer);
	HandleExplicitError(::FilterGetMessage(m_port, header, size, NULL));
	m_lastMessageId = header->MessageId;
}

void CommunicationPort::ReplyMessage(PVOID buffer, DWORD size)
{
	PFILTER_REPLY_HEADER header = static_cast<PFILTER_REPLY_HEADER>(buffer);
	header->Status = 0;
	header->MessageId = m_lastMessageId;
	HandleExplicitError(::FilterReplyMessage(m_port, header, size + sizeof(FILTER_REPLY_HEADER)));
}

char const* CommunicationPort::GetMessageBody(PVOID buffer)
{
	return static_cast<char *>(buffer) + sizeof(FILTER_MESSAGE_HEADER);
}

char* CommunicationPort::GetMessageReplyBody(PVOID buffer)
{
	return static_cast<char *>(buffer)+sizeof(FILTER_REPLY_HEADER);
}

ULONG CommunicationPort::GetMaxReplyBodySize(PVOID buffer)
{
	PFILTER_MESSAGE_HEADER header = static_cast<PFILTER_MESSAGE_HEADER>(buffer);
	return header->ReplyLength;
}