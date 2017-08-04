#include "Operations.h"
#include "CallbackData.h"
#include "FileSystemConverter.h"
#include "IFileSystem.h"

#include "fltKernel.h"

#include <stdexcept>

bool IsRoot(wchar_t const* path)
{
	return path[0] == 0 || path[1] == 0;
}

bool IsToPassBy(int majorCode, wchar_t const* path)
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

template <typename DirectoryInformation>
NTSTATUS OnDirectoryControl(IFileInfoIterator& list, CallbackData const& callbackData, DirectoryInformation* buffer, int& size)
{
	ULONG const operationFlags = callbackData.OperationFlags();
	if ((operationFlags & SL_RESTART_SCAN) || (operationFlags & SL_RETURN_SINGLE_ENTRY))
	{
		return FileSystemConverter::ConvertListTo(list, static_cast<DirectoryInformation *>(buffer), size);
	}
	else
	{
		return STATUS_NO_MORE_FILES;
	}
}

NTSTATUS OnDirectoryControl(FileInfoIteratorPtr list, CallbackData const& callbackData, void* oBuffer, int& oSize)
{
	if (callbackData.MinorCode() != IRP_MN_QUERY_DIRECTORY)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	int const informationClass = callbackData.InformationClass();
	if (informationClass == FileFullDirectoryInformation)
	{
		return OnDirectoryControl(*list, callbackData, PFILE_FULL_DIR_INFORMATION(oBuffer), oSize);
	}

	if (informationClass == FileIdBothDirectoryInformation)
	{
		return OnDirectoryControl(*list, callbackData, PFILE_ID_BOTH_DIR_INFORMATION(oBuffer), oSize);
	}

	return STATUS_INSUFFICIENT_RESOURCES;
}

void FillSatatus(FLT_PREOP_CALLBACK_STATUS callbackStatus, NTSTATUS operationStatus, ULONG_PTR information, PVOID buffer)
{
	PIO_STATUS_BLOCK statusBlock = static_cast<PIO_STATUS_BLOCK>(buffer);
	statusBlock->CallbackStatus = callbackStatus;
	statusBlock->Status = operationStatus;
	statusBlock->Information = information;
}

void OperationsRoutine(IFileSystem& fileSystem, char const* iBuffer, int iSize, char* oBuffer, int& oSize)
{
	CallbackData const callbackData(iBuffer);
	int const majorCode = callbackData.MajorCode();

	wchar_t const* fileName = callbackData.FileName();
	if (IsToPassBy(majorCode, fileName))
	{
		FillSatatus(FLT_PREOP_SUCCESS_NO_CALLBACK, STATUS_SUCCESS, 0, oBuffer);
		return;
	}

	if (!IsRoot(fileName) && !fileSystem.FindByPath(fileName, nullptr))
	{
		FillSatatus(FLT_PREOP_COMPLETE, STATUS_NOT_FOUND, 0, oBuffer);
		return;
	}

	int const outputBufferLength = callbackData.OutputBufferLength();
	if (outputBufferLength + static_cast<int>(sizeof(IO_STATUS_BLOCK)) > oSize)
	{
		throw std::length_error("User-space output buffer too small");
	}
	else
	{
		oSize = outputBufferLength;
	}

	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
	ULONG_PTR information = 0;
	void* callBackOutputBuffer = static_cast<void *>(oBuffer + sizeof(IO_STATUS_BLOCK));
	if (majorCode == IRP_MJ_DIRECTORY_CONTROL)
	{
		status = OnDirectoryControl(fileSystem.ListDirectory(fileName), callbackData, callBackOutputBuffer, oSize);
		information = oSize;
	}
	else
	{
		oSize = 0;
	}

	oSize += sizeof(IO_STATUS_BLOCK);
	FillSatatus(FLT_PREOP_COMPLETE, status, information, oBuffer);
}