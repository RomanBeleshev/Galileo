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

ULONG FileAttributesFromFileInfo(FileInfo const& from)
{
	return from.Directory ? (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_NORMAL) : FILE_ATTRIBUTE_NORMAL;
}

template <typename FileInfoType>
void FromFileInfoGeneralized(FileInfo const& from, FileInfoType* to)
{
	FromFileTime(from.Time, to->CreationTime);
	FromFileTime(from.Time, to->LastAccessTime);
	FromFileTime(from.Time, to->LastWriteTime);
	FromFileTime(from.Time, to->ChangeTime);
	to->FileAttributes = FileAttributesFromFileInfo(from);
}


template <typename FileInfoType>
void SizesFromFileInfo(FileInfo const& from, FileInfoType* to)
{
	to->EndOfFile.QuadPart = from.Size;
	to->AllocationSize.QuadPart = Align<10>(from.Size);
}

template <typename FileInfoType>
void FromFileInfoGeneralized2(FileInfo const& from, FileInfoType* to)
{
	FromFileInfoGeneralized(from, to);
	SizesFromFileInfo(from, to);
}

template <typename FileInfoType>
void NameFromFileInfo(FileInfo const& from, FileInfoType* to)
{
	to->FileNameLength = ULONG(from.Name.size() * sizeof(WCHAR));
	std::wcsncpy(to->FileName, from.Name.c_str(), from.Name.size());
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

	FromFileInfoGeneralized2(from, to);

	to->NextEntryOffset = nextOffset;
	to->FileIndex = index;
	to->EaSize = 0;
	NameFromFileInfo(from, to);
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

int FromFileInfo(FileInfo const& from, ULONG currentOffset, int index, PFILE_BOTH_DIR_INFORMATION to)
{
	return FromFileInfoGeneralized(from, currentOffset, index, to);
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
	FileInfoType* nextRecord = buffer;
	while (list.GetNext(file))
	{
		nextRecord = buffer == NULL ? NULL : reinterpret_cast<FileInfoType*>(reinterpret_cast<char*>(buffer) + currentOffset);
		currentOffset += FromFileInfo(file, currentOffset, ++index, nextRecord);
	}

	if (nextRecord != NULL)
	{
		nextRecord->NextEntryOffset = 0;
	}

	return currentOffset;
}

template <typename FileInfoType>
NTSTATUS ConvertListGeneralized(IFileInfoIterator& list, FileInfoType* buffer, int& size)
{
	list.SetBookmark();
	if (ConvertListImpl(list, static_cast<FileInfoType*>(NULL)) > size)
	{
		size = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}

	list.RollbackToBookmark();
	size = ConvertListImpl(list, buffer);
	return size == 0 ? STATUS_NO_MORE_FILES : STATUS_SUCCESS;
}

NTSTATUS FileSystemConverter::ConvertList(IFileInfoIterator& list, PFILE_ID_BOTH_DIR_INFORMATION buffer, int& size)
{
	return ConvertListGeneralized(list, buffer, size);
}

NTSTATUS FileSystemConverter::ConvertList(IFileInfoIterator& list, PFILE_BOTH_DIR_INFORMATION buffer, int& size)
{
	return ConvertListGeneralized(list, buffer, size);
}

NTSTATUS FileSystemConverter::ConvertList(IFileInfoIterator& list, PFILE_FULL_DIR_INFORMATION buffer, int& size)
{
	return ConvertListGeneralized(list, buffer, size);
}

void FileSystemConverter::ConvertFileInfo(FileInfo const& from, PFILE_BASIC_INFORMATION to, int& size)
{
	FromFileInfoGeneralized(from, to);
	size = sizeof(FILE_BASIC_INFORMATION);
}

void FileSystemConverter::ConvertFileInfo(FileInfo const& from, PFILE_NETWORK_OPEN_INFORMATION to, int& size)
{
	FromFileInfoGeneralized2(from, to);
	size = sizeof(FILE_NETWORK_OPEN_INFORMATION);
}

void FileSystemConverter::ConvertFileInfo(FileInfo const& from, PFILE_STANDARD_INFORMATION to, int& size)
{
	SizesFromFileInfo(from, to);
	to->NumberOfLinks = 1;
	to->DeletePending = FALSE;
	to->Directory = from.Directory ? TRUE : FALSE;

	size = sizeof(FILE_STANDARD_INFORMATION);
}

void FileSystemConverter::ConvertFileInfo(FileInfo const& from, PFILE_ATTRIBUTE_TAG_INFORMATION to, int& size)
{
	to->FileAttributes = FileAttributesFromFileInfo(from);

	size = sizeof(PFILE_ATTRIBUTE_TAG_INFORMATION);
}

void FileSystemConverter::ConvertFileInfo(FileInfo const& from, PFILE_NAME_INFORMATION to, int& size)
{
	NameFromFileInfo(from, to);
	size = ULONG(sizeof(PFILE_NAME_INFORMATION) + from.Name.size() * sizeof(WCHAR));
}
