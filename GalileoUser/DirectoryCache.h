#pragma once

#include <map>
#include <string>
#include <memory>
#include <stdexcept>

class IFileInfoIterator;
typedef std::unique_ptr<IFileInfoIterator> FileInfoIteratorPtr;

template <typename Key>
class DirectoryCache
{
public:
	void Add(Key key, std::wstring const& name, FileInfoIteratorPtr list)
	{
		auto found = m_cache.find(key);
		if (found != m_cache.end())
		{
			throw std::logic_error("DirectoryCache: Trying to add duplicate key");
		}
		m_cache.insert(std::make_pair(key, Item(name, std::move(list))));
	}

	void Remove(Key key)
	{
		auto found = m_cache.find(key);
		if (found == m_cache.end())
		{
			throw std::logic_error("DirectoryCache: Trying to remove non-existing key");
		}

		m_cache.erase(found);
	}

	bool Exists(Key key, std::wstring const& name, bool& differentName) const
	{
		auto found = m_cache.find(key);
		if (found == m_cache.end())
		{
			return false;
		}
		differentName = found->second.Name != name;
		return true;
	}

	IFileInfoIterator& Get(Key key, std::wstring const& name) const
	{
		auto const found = m_cache.find(key);
		if (found == m_cache.end())
		{
			throw std::logic_error("DirectoryCache: Trying to get non-existing key");
		}
		if (found->second.Name != name)
		{
			throw std::logic_error("DirectoryCache: Trying to get key with different name");
		}

		return *found->second.List;
	}

private:
	struct Item
	{
		Item(std::wstring const& name, FileInfoIteratorPtr list) :
			Name(name),
			List(std::move(list))
		{
		}

		Item(Item&& item) :
			Name(item.Name),
			List(std::move(item.List))
		{
		}

		~Item()
		{
		}

		std::wstring Name;
		FileInfoIteratorPtr List;
	};

	typedef std::map<typename Key, Item> Cache;
	Cache m_cache;
};
