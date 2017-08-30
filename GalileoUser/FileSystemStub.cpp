#include "FileSystemStub.h"
#include "DirectoryCache.h"
#include "IFileSystem.h"

#include <stdexcept>

int const NumberOfGalileos = 777;
char GalileoItself[] = "GALILEO!!!";
int const SizeOfGalileo = sizeof(GalileoItself) - 1;

void FileSystemStub::PopulateFileSystem(IFileSystem& fileSystem)
{
	FILETIME time;
	::GetSystemTimeAsFileTime(&time);

	fileSystem.CreateNewElement(L"", FileInfo(true, L"Galileo Directory", time));
	fileSystem.CreateNewElement(L"", FileInfo(false, L"File", time));

	fileSystem.CreateNewElement(L"", FileInfo(true, L"1", time));

	fileSystem.CreateNewElement(L"", FileInfo(true, L"2", time));
	fileSystem.CreateNewElement(L"\\1", FileInfo(true, L"1", time));
	fileSystem.CreateNewElement(L"\\1", FileInfo(true, L"2", time));
	fileSystem.CreateNewElement(L"", FileInfo(true, L"2", time));

	for (int i = 0; i < NumberOfGalileos; ++i)
	{
		fileSystem.WriteContent(L"\\File", GalileoItself, SizeOfGalileo * i, SizeOfGalileo);
	}

	TestFileSystem(fileSystem);
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

#define RequireThrow(code) { try { code; throw std::logic_error("Filesystem exception required");} catch (std::exception const&) {} }

void FileSystemStub::TestFileSystem(IFileSystem& fileSystem)
{
	Require(fileSystem.FindByPath(L"\\galileo directory", nullptr));
	Require(fileSystem.FindByPath(L"\\FILE", nullptr));

	FileInfoIteratorPtr list = fileSystem.ListDirectory(L"\\1");
	RequireNextElement(*list, L"1");
	RequireNextElement(*list, L"2");
	RequireEnd(*list);

	list = fileSystem.ListDirectory(L"\\");
	RequireNextElement(*list, L"1");
	RequireNextElement(*list, L"2");
	RequireNextElement(*list, L"file");
	RequireNextElement(*list, L"galileo directory");
	RequireEnd(*list);

	char buffer[SizeOfGalileo];
	for (int i = 0; i < NumberOfGalileos; ++i)
	{
		Require(fileSystem.ReadContent(L"\\FiLe", buffer, SizeOfGalileo * i, SizeOfGalileo) == SizeOfGalileo);
		Require(std::memcmp(buffer, GalileoItself, SizeOfGalileo) == 0);
	}

	DirectoryCache<int> cache;
	bool dummy;
	Require(!cache.Exists(1, L"dir", dummy));
	RequireThrow(cache.Get(1, L"dir"));
	cache.Add(1, L"dir", fileSystem.ListDirectory(L"\\"));
	Require(cache.Exists(1, L"dir", dummy));
	RequireThrow(cache.Add(1, L"dir2", nullptr));
	RequireNextElement(cache.Get(1, L"dir"), L"1");
	RequireThrow(cache.Get(1, L"dir2"));
	RequireThrow(cache.Get(2, L"dir"));
	cache.Remove(1);
	RequireThrow(cache.Get(1, L"dir"));
}