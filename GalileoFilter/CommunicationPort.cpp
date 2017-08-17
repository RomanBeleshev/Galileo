#include "CommunicationPort.h"
#include "Error.h"
#include "Trace.h"

#pragma warning (push)
#pragma warning (disable: 4510)
#pragma warning (disable: 4512)
#pragma warning (disable: 4610)
#include <fltKernel.h>
#pragma warning (pop)

NTSTATUS UserConnect(_In_ PFLT_PORT ClientPort, _In_ PVOID ServerPortCookie,
	_In_reads_bytes_(SizeOfContext) PVOID ConnectionContext, _In_ ULONG SizeOfContext,
	_Flt_ConnectionCookie_Outptr_ PVOID *ConnectionCookie)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);

	*ConnectionCookie = ServerPortCookie;

	CommunicationPort *port = reinterpret_cast<CommunicationPort *>(ServerPortCookie);
	port->UserConnected(ClientPort);

	return STATUS_SUCCESS;
}

VOID UserDisconnect(_In_opt_ PVOID ConnectionCookie)
{
	PAGED_CODE();

	CommunicationPort *port = reinterpret_cast<CommunicationPort *>(ConnectionCookie);
	port->UserDisconnected();
}

NTSTATUS UserMessage(_In_ PVOID ConnectionCookie, _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
	_In_ ULONG InputBufferSize, _Out_writes_bytes_to_opt_(OutputBufferSize, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferSize, _Out_ PULONG ReturnOutputBufferLength)
{
	ConnectionCookie, InputBuffer, InputBufferSize, OutputBuffer, OutputBufferSize, ReturnOutputBufferLength;
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ConnectionCookie);

	CommunicationPort *port = reinterpret_cast<CommunicationPort *>(ConnectionCookie);
	port->UserMessage();

	return STATUS_SUCCESS;
}

PFLT_PORT CreateCommunicationPort(PFLT_FILTER filter, PCWSTR portName, PVOID cookie)
{
	PSECURITY_DESCRIPTOR sd;
	if (!ApiCallSucceeded(::FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS), L"CreateCommunicationPort"))
	{
		return nullptr;
	}


	if (!ApiCallSucceeded(::RtlSetDaclSecurityDescriptor(sd, TRUE, NULL, FALSE), L"CreateCommunicationPort"))
	{
		::FltFreeSecurityDescriptor(sd);
		return nullptr;
	}

	UNICODE_STRING uniString;
	::RtlInitUnicodeString(&uniString, portName);

	OBJECT_ATTRIBUTES oa;
	InitializeObjectAttributes(&oa, &uniString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);

	PFLT_PORT result;
	if (!ApiCallSucceeded(::FltCreateCommunicationPort(filter, &result, &oa, cookie,
		UserConnect, UserDisconnect, UserMessage, 1), L"CreateCommunicationPort"))
	{
		result = nullptr;
	}

	::FltFreeSecurityDescriptor(sd);

	return result;
}

CommunicationPort::CommunicationPort(PFLT_FILTER filter, PCWSTR portName) :
	m_filter(filter),
	m_serverPort(CreateCommunicationPort(filter, portName, this)),
	m_clientPort(nullptr)
{
}


CommunicationPort::~CommunicationPort()
{
	if (m_serverPort != nullptr)
	{
		::FltCloseCommunicationPort(m_serverPort);
		m_serverPort = nullptr;
	}
}

void CommunicationPort::UserConnected(PFLT_PORT clientPort)
{
	PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!UserConnected\n"));

	if (m_clientPort == nullptr)
	{
		m_clientPort = clientPort;
	}
	else
	{
		PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!UserConnected: Only single connection allowed. Closing incoming connection.\n"));
		::FltCloseClientPort(m_filter, &clientPort);
	}
}

void CommunicationPort::UserDisconnected()
{
	PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!UserDisconnected\n"));

	::FltCloseClientPort(m_filter, &m_clientPort);
	m_clientPort = nullptr;
}

void CommunicationPort::UserMessage()
{
	PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!UserMessage\n"));
}

bool CommunicationPort::SendMessage(PVOID CONST iBuffer, ULONG iSize, PVOID oBuffer, ULONG& oSize)
{
	if (m_clientPort == nullptr)
	{
		PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!SendMessage failed - client not connected.\n"));
		return false;
	}

	LARGE_INTEGER timeout;
	timeout.QuadPart = -10 * 10000;

	NTSTATUS result = ::FltSendMessage(m_filter, &m_clientPort, iBuffer, iSize, oBuffer, &oSize, NULL/*&timeout*/);
	if (result == STATUS_TIMEOUT)
	{
		PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!SendMessage timed out.\n"));
		return false;
	}
	else if (ApiCallSucceeded(result, L"CreateCommunicationPort"))
	{
	}

	return true;
}
