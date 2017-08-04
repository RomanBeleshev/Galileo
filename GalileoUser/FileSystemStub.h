#pragma once

class IFileSystem;

class FileSystemStub
{
public:
	void PopulateFileSystem(IFileSystem& fileSystem);

private:
	void TestFileSystem(IFileSystem& fileSystem);
};
