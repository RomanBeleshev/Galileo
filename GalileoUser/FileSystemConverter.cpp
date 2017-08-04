#include "FileSystemConverter.h"
#include "IFileSystem.h"

#include <cwchar>

#pragma warning (disable: 4996)

template <ULONG bits>
ULONG Align(ULONG alignWhat)
{
	ULONG const mask = (1 << bits) - 1;
	return (alignWhat + mask) & ~mask;
}

void FromFileTime(FILETIME const& from, LARGE_INTEGER& to)
{
	to.HighPart = from.dwHighDateTime;
	to.LowPart = from.dwLowDateTime;
}

template <typename FileInfoType>
ULONG FromFileInfoGeneralized(FileInfo const& from, ULONG currentOffset, int index, FileInfoType* to)
{
	ULONG const size = ULONG(sizeof(FileInfoType) + from.Name.size() * sizeof(WCHAR));
	ULONG const nextOffset = Align<3>(currentOffset + size);
	if (to == NULL)
	{
		return nextOffset;
	}

	to->NextEntryOffset = nextOffset;
	to->FileIndex = index;
	FromFileTime(from.Time, to->CreationTime);
	FromFileTime(from.Time, to->LastAccessTime);
	FromFileTime(from.Time, to->LastWriteTime);
	FromFileTime(from.Time, to->ChangeTime);
	to->EndOfFile.QuadPart = from.Size;
	to->AllocationSize.QuadPart = Align<10>(from.Size);
	to->FileAttributes = from.Directory ? (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL) : FILE_ATTRIBUTE_NORMAL;
	to->FileNameLength = ULONG(from.Name.size() * sizeof(WCHAR));
	to->EaSize = 0;
	std::wcsncpy(to->FileName, from.Name.c_str(), from.Name.size());
	return nextOffset;
}

int FromFileInfo(FileInfo const& from, ULONG currentOffset, int index, PFILE_ID_BOTH_DIR_INFORMATION to)
{
	if (to == NULL)
	{
		return FromFileInfoGeneralized(from, currentOffset, index, to);
	}
	else
	{
		//ULONG const length = from.NameLength < 12 ? from.NameLength : 12;
		//to->ShortNameLength = static_cast<CCHAR>(length * sizeof(WCHAR));
		//std::wcsncpy(to->ShortName, from.Name, length);
		to->FileId.QuadPart = index;
		return FromFileInfoGeneralized(from, currentOffset, index, to);
	}
}

int FromFileInfo(FileInfo const& from, ULONG currentOffset, int index, PFILE_FULL_DIR_INFORMATION to)
{
	return FromFileInfoGeneralized(from, currentOffset, index, to);
}

template <typename FileInfoType>
int ConvertListImpl(IFileInfoIterator& list, FileInfoType* buffer)
{
	int currentOffset = 0;
	int filesCount = 0;
	FileInfo file;
	int index = 0;
	FileInfoType* nextRecord = NULL;
	while (list.GetNext(file))
	{
		nextRecord = buffer == NULL ? NULL : reinterpret_cast<FileInfoType*>(reinterpret_cast<char*>(buffer) + currentOffset);
		currentOffset = FromFileInfo(file, currentOffset, ++index, nextRecord);
	}

	if (nextRecord != NULL)
	{
		nextRecord->NextEntryOffset = 0;
	}

	return currentOffset;
}

template <typename FileInfoType>
NTSTATUS ConvertListToGeneralized(IFileInfoIterator& list, FileInfoType* buffer, int& size)
{
	list.SetBookmark();
	if (ConvertListImpl(list, static_cast<FileInfoType*>(NULL)) > size)
	{
		size = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}

	list.RollbackToBookmark();
	size = ConvertListImpl(list, buffer);
	return STATUS_SUCCESS;
}

NTSTATUS FileSystemConverter::ConvertListTo(IFileInfoIterator& list, PFILE_ID_BOTH_DIR_INFORMATION buffer, int& size)
{
	return ConvertListToGeneralized(list, buffer, size);
}

NTSTATUS FileSystemConverter::ConvertListTo(IFileInfoIterator& list, PFILE_FULL_DIR_INFORMATION buffer, int& size)
{
	return ConvertListToGeneralized(list, buffer, size);
}

