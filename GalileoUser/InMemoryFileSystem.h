#pragma once

#include "IFileSystem.h"

#include <string>
#include <vector>
#include <map>

class InMemoryFileSystem : public IFileSystem
{
public:
	InMemoryFileSystem();

private:
	virtual bool FindByPath(PCWCHAR path, FileInfo* info) const;
	virtual FileInfoIteratorPtr ListDirectory(PCWCHAR path) const;
	virtual void CreateNewElement(PCWCHAR parentPath, FileInfo const& info);

	virtual int ReadContent(PCWCHAR filePath, void* buffer, Offset offset, int size) const;
	virtual void WriteContent(PCWCHAR filePath, void const* buffer, Offset offset, int size);

	bool FindByLowerPath(std::wstring const& lowerPath, FileInfo* info) const;
	FileInfo& FindByLowerPath(std::wstring const& lowerPath);
	FileInfo& FindByPath(PCWCHAR path);

private:
	typedef std::map<std::wstring, FileInfo> Elements;
	Elements m_elements;

	typedef std::vector<char> Content;
	typedef std::map<std::wstring, Content> Contents;
	Contents m_contents;

	class ElementsIterator;

private:
	Content const& FindContentByPath(PCWCHAR filePath) const;
	Content& FindContentByPath(PCWCHAR filePath);
};

void DecomposePath(std::wstring const& path, std::wstring& parent, std::wstring& name);