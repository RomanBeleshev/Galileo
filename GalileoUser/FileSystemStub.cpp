#include "FileSystemStub.h"

#include "IFileSystem.h"

#include <stdexcept>

void FileSystemStub::PopulateFileSystem(IFileSystem& fileSystem)
{
	FILETIME time;
	::GetSystemTimeAsFileTime(&time);

	fileSystem.CreateNewElement(L"", FileInfo(true, L"Galileo Directory", 0, time));
	fileSystem.CreateNewElement(L"", FileInfo(false, L"Galileo File", 1234567, time));

	fileSystem.CreateNewElement(L"", FileInfo(true, L"1", 0, time));
	/*
	fileSystem.CreateNewElement(L"", FileInfo(true, L"2", 0, time));
	fileSystem.CreateNewElement(L"\\1", FileInfo(true, L"1", 0, time));
	fileSystem.CreateNewElement(L"\\1", FileInfo(true, L"2", 0, time));
	fileSystem.CreateNewElement(L"", FileInfo(true, L"2", 0, time));

	TestFileSystem(fileSystem);*/
}

void Require(bool condition)
{
	if (!condition)
	{
		throw std::logic_error("Filesystem works improperly");
	}
}

template <typename T1, typename T2>
void RequireEqual(T1 const& v1, T2 const& v2)
{
	Require(v1 == v2);
}

void RequireNextElement(IFileInfoIterator& it, PCWCHAR name)
{
	FileInfo info;
	Require(it.GetNext(info));
	RequireEqual(info.Name, name);
}

void RequireEnd(IFileInfoIterator& it)
{
	FileInfo info;
	Require(!it.GetNext(info));
}

void FileSystemStub::TestFileSystem(IFileSystem& fileSystem)
{
	Require(fileSystem.FindByPath(L"\\galileo directory", nullptr));
	Require(fileSystem.FindByPath(L"\\GALILEO FILE", nullptr));

	FileInfoIteratorPtr list = fileSystem.ListDirectory(L"\\1");
	RequireNextElement(*list, L"1");
	RequireNextElement(*list, L"2");
	RequireEnd(*list);

	list = fileSystem.ListDirectory(L"\\");
	RequireNextElement(*list, L"1");
	RequireNextElement(*list, L"2");
	RequireNextElement(*list, L"galileo directory");
	RequireNextElement(*list, L"galileo file");
	RequireEnd(*list);
}