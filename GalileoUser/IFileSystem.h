#pragma once

#include <wtypes.h>

#include <memory>
#include <string>

struct FileInfo
{
	FileInfo(bool directory, std::wstring const& name, ULONG size, FILETIME time) :
		Directory(directory),
		Name(name),
		Size(size),
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

	virtual ~IFileSystem() {}
};
