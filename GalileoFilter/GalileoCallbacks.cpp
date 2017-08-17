#include "GalileoCallbacks.h"

#include "CommunicationPort.h"
#include "InplaceConstructible.h"
#include "FastMutex.h"
#include "Trace.h"

#pragma warning (push)
#pragma warning (disable: 4510)
#pragma warning (disable: 4512)
#pragma warning (disable: 4610)
#include <fltKernel.h>
#pragma warning (pop)


class GlobalData
{
public:
	GlobalData(PFLT_FILTER filter) :
		m_port(filter, L"\\GalileoCommunicationPort")
	{
	}

	~GlobalData()
	{
	}

	bool SendMessage(PVOID CONST iBuffer, ULONG iSize, PVOID oBuffer, ULONG& oSize)
	{
		return m_port.SendMessage(iBuffer, iSize, oBuffer, oSize);
	}

private:
	CommunicationPort m_port;
};

InplaceConstructible<GlobalData> globalData;

void Initialize(PFLT_FILTER filter)
{
	PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!Initialize\n"));

	globalData.Construct(filter);
}

void UnInitialize()
{
	globalData.Destruct();

	PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!UnInitialize\n"));
}

bool IsNameless(UCHAR majorCode)
{
	return majorCode == IRP_MJ_VOLUME_DISMOUNT || majorCode == IRP_MJ_VOLUME_MOUNT;
}

char iBuffer[1024 * 1024];
char oBuffer[1024 * 1024];

void CopyData(void const* from, void* to, ULONG size, ULONG& sizeToAdvance)
{
	::RtlCopyMemory(to, from, size);
	sizeToAdvance += size;
}

ULONG PtrDiff(void const* from, void const* to)
{
	return static_cast<ULONG>(reinterpret_cast<char const*>(to)-reinterpret_cast<char const*>(from));
}

struct ExtraParams
{
	ULONG Param1;
	ULONG OutputBufferLength;
};

template <typename ToType, typename FromType>
ToType const* AdvancePointer(FromType const* from, int size = sizeof(FromType))
{
	char const* pointer = reinterpret_cast<char const*>(from);
	return reinterpret_cast<const ToType*>(pointer + size);
}

FLT_PREOP_CALLBACK_STATUS SendMessage(PFLT_CALLBACK_DATA data, PFILE_OBJECT CONST fileObject)
{
	PVOID* buffer;
	PULONG length;
	LOCK_OPERATION desiredAccess;

	ULONG iSize = 0;
	CopyData(data->Iopb, iBuffer + iSize, PtrDiff(data->Iopb, &data->Iopb->TargetInstance), iSize);

	if (::FltDecodeParameters(data, NULL, &buffer, &length, &desiredAccess) != STATUS_SUCCESS)
	{
		buffer = NULL;
		length = NULL;
	}

	if (length != NULL && *length + 24 > sizeof(oBuffer))
	{
		PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!!!!!! Insufficient internal buffer!!!! len %d\n", *length));
		data->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
		return FLT_PREOP_COMPLETE;
	}

	ExtraParams extraParams = { 0, length == NULL ? 0 : *length };
	if (data->Iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL && data->Iopb->MinorFunction == IRP_MN_QUERY_DIRECTORY)
	{
		extraParams.Param1 = data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
	}
	else if (data->Iopb->MajorFunction == IRP_MJ_CREATE)
	{
		extraParams.Param1 = data->Iopb->Parameters.Create.Options;
	}
	else if (data->Iopb->MajorFunction == IRP_MJ_QUERY_INFORMATION)
	{
		extraParams.Param1 = data->Iopb->Parameters.QueryFileInformation.FileInformationClass;
	}

	CopyData(&extraParams, iBuffer + iSize, sizeof(ExtraParams), iSize);

	PCHAR const irpName = ::FltGetIrpName(data->Iopb->MajorFunction);
	PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!%s(Minor=%d, Param=%d, BufLen=%d, File=%wZ)\n", irpName,
		data->Iopb->MinorFunction, extraParams.Param1, extraParams.OutputBufferLength, fileObject->FileName));

	UNICODE_STRING const& path = fileObject->FileName;
	CopyData(&path.Length, iBuffer + iSize, sizeof(path.Length), iSize);
	CopyData(path.Buffer, iBuffer + iSize, path.Length * sizeof(WCHAR), iSize);
	iBuffer[iSize++] = 0;
	iBuffer[iSize++] = 0;

	ULONG oSize = sizeof(oBuffer);
	if (!globalData->SendMessage(iBuffer, iSize, oBuffer, oSize))
	{
		data->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
		return FLT_PREOP_COMPLETE;
	}

	PIO_STATUS_BLOCK statusBlock = reinterpret_cast<PIO_STATUS_BLOCK>(oBuffer);

	data->IoStatus = *statusBlock;

	ULONG const* callbackStatus = AdvancePointer<ULONG>(statusBlock);
	ULONG const* alignedBuffer = AdvancePointer<ULONG>(callbackStatus);
	char const* sourceBuffer = AdvancePointer<char>(alignedBuffer);
	ULONG const headerSize = PtrDiff(oBuffer, sourceBuffer);
	oSize -= headerSize;

	if (statusBlock->Status == STATUS_INSUFFICIENT_RESOURCES)
	{
		PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!%s - Not implemented\n", irpName));
	}
	else
	{
		PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!%s -> (Status=%x, Info=%ld, BufLen=%d)\n",
			irpName, statusBlock->Status, statusBlock->Information, oSize));
	}

	if (buffer != NULL && length != NULL)
	{
		if (oSize > *length)
		{
			PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter! Size mismatch!!!! hs/os/len %d/%d/%d\n", headerSize, oSize, *length));
			data->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
			return FLT_PREOP_COMPLETE;
		}
		::RtlCopyMemory(*buffer, sourceBuffer, *length);
	}

	return static_cast<FLT_PREOP_CALLBACK_STATUS>(*callbackStatus);
}

bool IsRoot(UNICODE_STRING const& path)
{
	return path.Length == 1 || path.Length == 2 && path.Buffer[1] == 0;
}

bool IsToPassBy(UCHAR majorCode, UNICODE_STRING const& path)
{
	if (IsRoot(path) &&
		(majorCode == IRP_MJ_CREATE || majorCode == IRP_MJ_CLEANUP || majorCode == IRP_MJ_CLOSE ||
		majorCode == IRP_MJ_QUERY_INFORMATION))
	{
		return true;
	}

	if (majorCode == IRP_MJ_QUERY_VOLUME_INFORMATION)
	{
		return true;
	}

	return false;
}

int PreOperation(PFLT_CALLBACK_DATA data, PCFLT_RELATED_OBJECTS const objects)
{
	UCHAR const majorCode = data->Iopb->MajorFunction;

	if (IsNameless(majorCode))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	UNICODE_STRING fileName = objects->FileObject->FileName;
	if (IsToPassBy(majorCode, fileName))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	static FastMutex mutex;
	Lock lock(mutex);

	return SendMessage(data, objects->FileObject);
}

void PostOperation(PFLT_CALLBACK_DATA)
{
}
