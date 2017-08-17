#pragma once

#include "IFileSystem.h"

#include <string>
#include <map>

class InMemoryFileSystem : public IFileSystem
{
public:
	InMemoryFileSystem();

private:
	virtual bool FindByPath(PCWCHAR path, FileInfo* info) const;
	virtual FileInfoIteratorPtr ListDirectory(PCWCHAR path) const;
	virtual void CreateNewElement(PCWCHAR parentPath, FileInfo const& info);

	bool FindByLowerPath(std::wstring const& lowerPath, FileInfo* info) const;

private:
	typedef std::map<std::wstring, FileInfo> Elements;
	Elements m_elements;

	class ElementsIterator;
};

void DecomposePath(std::wstring const& path, std::wstring& parent, std::wstring& name);