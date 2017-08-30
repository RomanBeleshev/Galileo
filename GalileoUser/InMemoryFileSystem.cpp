#include "InMemoryFileSystem.h"

#include <algorithm>
#include <stdexcept>
#include <string>

InMemoryFileSystem::InMemoryFileSystem() :
	m_elements(),
	m_contents()
{
	m_elements.insert(std::make_pair(L"\\", FileInfo()));
}

void ToLower(std::wstring& s)
{
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

bool InMemoryFileSystem::FindByPath(PCWCHAR path, FileInfo* info) const
{
	std::wstring lowerPath(path);
	ToLower(lowerPath);
	return FindByLowerPath(lowerPath, info);
}

FileInfo& InMemoryFileSystem::FindByLowerPath(std::wstring const& lowerPath)
{
	auto it = m_elements.find(lowerPath);
	if (it == m_elements.end())
	{
		throw std::runtime_error("Element not found");
	}

	return it->second;
}

FileInfo& InMemoryFileSystem::FindByPath(PCWCHAR path)
{
	std::wstring lowerPath(path);
	ToLower(lowerPath);
	return FindByLowerPath(lowerPath);
}

bool InMemoryFileSystem::FindByLowerPath(std::wstring const& lowerPath, FileInfo* info) const
{
	auto const it = m_elements.find(lowerPath);
	if (it == m_elements.end())
	{
		return false;
	}

	if (info != nullptr)
	{
		*info = it->second;
	}

	return true;
}

class InMemoryFileSystem::ElementsIterator : public IFileInfoIterator
{
public:
	ElementsIterator(Elements::const_iterator first, Elements::const_iterator last, int level) :
		m_first(first),
		m_last(last),
		m_bookmark(first),
		m_level(level)
	{
	}

private:
	virtual bool GetNext(FileInfo& value)
	{
		while (m_first != m_last && m_first->second.Level >= m_level)
		{
			if (m_first->second.Level == m_level)
			{
				value = m_first->second;
				++m_first;
				return true;
			}

			++m_first;
		}

		return false;
	}

	virtual void SetBookmark()
	{
		m_bookmark = m_first;
	}

	virtual void RollbackToBookmark()
	{
		m_first = m_bookmark;
	}

private:
	Elements::const_iterator m_first;
	Elements::const_iterator const m_last;
	Elements::const_iterator m_bookmark;
	int const m_level;
};

FileInfoIteratorPtr InMemoryFileSystem::ListDirectory(PCWCHAR path) const
{
	std::wstring lowerPath(path);
	ToLower(lowerPath);

	auto it = m_elements.lower_bound(lowerPath);
	if (it == m_elements.end() || it->first != lowerPath)
	{
		throw std::runtime_error("Directory not found");
	}

	int const level = it->second.Level + 1;
	++it;

	return FileInfoIteratorPtr(new ElementsIterator(it, m_elements.end(), level));
}

void DecomposePath(std::wstring const& path, std::wstring& parent, std::wstring& name)
{
	auto const slashPosition = path.find_last_of(L'\\');
	if (slashPosition == std::wstring::npos)
	{
		throw std::logic_error("Path should contain at least one slash");
	}

	parent = path.substr(0, slashPosition);
	name = path.substr(slashPosition + 1);
}

void InMemoryFileSystem::CreateNewElement(PCWCHAR parentPath, FileInfo const& info)
{
	FileInfo parent;
	std::wstring lowerParentPath(parentPath);

	if (lowerParentPath.size() > 0)
	{
		ToLower(lowerParentPath);

		if (!FindByLowerPath(lowerParentPath, &parent))
		{
			throw std::runtime_error("Parent not found");
		}
	}

	FileInfo toInsert(info);
	ToLower(toInsert.Name);
	toInsert.Level = parent.Level + 1;

	m_elements.insert(std::make_pair(lowerParentPath + L'\\' + toInsert.Name, toInsert));
}

#undef min

int InMemoryFileSystem::ReadContent(PCWCHAR filePath, void* buffer, Offset offset, int size) const
{
	Content const& content = FindContentByPath(filePath);
	if (offset > Offset(content.size()))
	{
		return 0;
	}

	int const sizeRead = std::min(size, int(content.size() - offset));
	std::memcpy(buffer, &content.front() + offset, sizeRead);
	return sizeRead;
}

void InMemoryFileSystem::WriteContent(PCWCHAR filePath, void const* buffer, Offset offset, int size)
{
	Content& content = FindContentByPath(filePath);
	if (offset + size > Offset(content.size()))
	{
		content.resize(offset + size);
	}

	std::memcpy(&content.front() + offset, buffer, size);

	FileInfo& file = FindByPath(filePath);
	if (content.size() > file.Size)
	{
		file.Size = ULONG(content.size());
	}
}

InMemoryFileSystem::Content const& InMemoryFileSystem::FindContentByPath(PCWCHAR filePath) const
{
	std::wstring lowerPath(filePath);
	ToLower(lowerPath);

	auto const it = m_contents.find(lowerPath);
	if (it == m_contents.end())
	{
		throw std::runtime_error("File content not found");
	}

	return it->second;
}

InMemoryFileSystem::Content& InMemoryFileSystem::FindContentByPath(PCWCHAR filePath)
{
	std::wstring lowerPath(filePath);
	ToLower(lowerPath);

	auto it = m_contents.find(lowerPath);
	if (it == m_contents.end())
	{
		it = m_contents.insert(std::make_pair(lowerPath, Content())).first;
	}

	return it->second;
}
