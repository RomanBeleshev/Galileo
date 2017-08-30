#pragma once

#include <wtypes.h>

#include <memory>
#include <string>
#include <cstdint>

struct FileInfo
{
	FileInfo(bool directory, std::wstring const& name, FILETIME time) :
		Directory(directory),
		Name(name),
		Size(0),
		Time(time),
		Level(0)
	{
	}

	FileInfo() :
		Directory(true),
		Name(L""),
		Size(0),
		Time(),
		Level(0)
	{
	}

	bool Directory;
	std::wstring Name;
	ULONG Size;
	FILETIME Time;
	int Level;
};

class IFileInfoIterator
{
public:
	virtual bool GetNext(FileInfo& value) = 0;

	virtual void SetBookmark() = 0;
	virtual void RollbackToBookmark() = 0;

	virtual ~IFileInfoIterator() {}
};

typedef std::unique_ptr<IFileInfoIterator> FileInfoIteratorPtr;

class IFileSystem
{
public:
	virtual bool FindByPath(PCWCHAR path, FileInfo* info) const = 0;
	virtual FileInfoIteratorPtr ListDirectory(PCWCHAR path) const = 0;

	virtual void CreateNewElement(PCWCHAR parentPath, FileInfo const& info) = 0;

	typedef std::int64_t Offset;
	virtual int ReadContent(PCWCHAR filePath, void* buffer, Offset offset, int size) const = 0;
	virtual void WriteContent(PCWCHAR filePath, void const* buffer, Offset offset, int size) = 0;

	virtual ~IFileSystem() {}
};
