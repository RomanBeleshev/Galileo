#include "Common/Exception.h"
#include "Common/WinAPi.h"

#include "Windows.h"
#include "Imagehlp.h"

#include <iostream>
#include <set>
#include <vector>
#include <cstring>
#include <string>

typedef std::vector<std::wstring> StringArray;
typedef std::set<std::wstring> StringSet;

class RegistryKey
{
public:
	RegistryKey(HKEY parent, LPCTSTR name) :
		m_hkey(Open(parent, name))
	{
	}

	RegistryKey(RegistryKey const& parent, LPCTSTR name) :
		m_hkey(Open(parent.m_hkey, name))
	{
	}

	~RegistryKey()
	{
		::RegCloseKey(m_hkey);
	}

	typedef StringArray KeysCollection;
	KeysCollection EnumerateKeys() const
	{
		KeysCollection keys;
		for (int i = 0;; ++i)
		{
			TCHAR buffer[1024];
			LONG result = ::RegEnumKey(m_hkey, i, buffer, 1024);
			if (result == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			HandleExplicitError(result);
			keys.push_back(std::wstring(buffer));
		}
		return keys;
	}

	bool GetString(LPCTSTR valueName, std::wstring& value) const
	{
		TCHAR buffer[1024];
		DWORD size = sizeof(buffer);
		LONG result = ::RegGetValue(m_hkey, NULL, valueName, RRF_RT_REG_SZ, NULL, buffer, &size);
		if (result == ERROR_FILE_NOT_FOUND)
		{
			return false;
		}
		HandleExplicitError(result);
		value = std::wstring(buffer);
		return true;
	}

	bool GetInt(LPCTSTR valueName, int& value) const
	{
		DWORD buffer;
		DWORD size = sizeof(buffer);
		LONG result = ::RegGetValue(m_hkey, NULL, valueName, RRF_RT_REG_DWORD, NULL, &buffer, &size);
		if (result == ERROR_FILE_NOT_FOUND)
		{
			return false;
		}
		HandleExplicitError(result);
		value = buffer;
		return true;
	}

private:
	static HKEY Open(HKEY parent, LPCTSTR name)
	{
		HKEY result;
		HandleExplicitError(::RegOpenKey(parent, name, &result));
		return result;
	}

private:
	HKEY m_hkey;
};

void CopyFile(std::wstring const& from, std::wstring const& to)
{
	HandleImplicitError(::CopyFile(from.c_str(), to.c_str(), FALSE));
	std::wcout << from << " - OK" << std::endl;
}

void CopyFileNoThrow(std::wstring const& from, std::wstring const& to)
{
	try
	{
		CopyFile(from, to);
	}
	catch (Exception const& e)
	{
		std::wcout << from << " - Error: " << e.Message() << std::endl;
	}
}

PIMAGE_SECTION_HEADER GetEnclosingSectionHeader(DWORD rva, PIMAGE_NT_HEADERS pNTHeader)
{
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
	unsigned i;

	for (i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++)
	{
		// This 3 line idiocy is because Watcom's linker actually sets the
		// Misc.VirtualSize field to 0.  (!!! - Retards....!!!)
		DWORD size = section->Misc.VirtualSize;
		if (0 == size)
			size = section->SizeOfRawData;

		// Is the RVA within this section?
		if ((rva >= section->VirtualAddress) &&
			(rva < (section->VirtualAddress + size)))
			return section;
	}

	return 0;
}

void const* GetPtrFromRVA(DWORD rva, PIMAGE_NT_HEADERS pNTHeader, PBYTE imageBase)
{
	PIMAGE_SECTION_HEADER pSectionHdr;
	INT delta;

	pSectionHdr = GetEnclosingSectionHeader(rva, pNTHeader);
	if (!pSectionHdr)
		return 0;

	delta = (INT)(pSectionHdr->VirtualAddress - pSectionHdr->PointerToRawData);
	return static_cast<void const *>(imageBase + rva - delta);
}

StringArray GetDriverDependencies(std::wstring const& path, StringSet& skippedDependencies)
{
	PLOADED_IMAGE image = ::ImageLoad(FromWideString(path).c_str(), NULL);
	if (image == NULL)
	{
		return StringArray();
	}

	StringArray result;
	if (image->FileHeader->OptionalHeader.NumberOfRvaAndSizes >= 2)
	{
		PIMAGE_IMPORT_DESCRIPTOR importDesc =
			(PIMAGE_IMPORT_DESCRIPTOR)GetPtrFromRVA(
			image->FileHeader->OptionalHeader.DataDirectory[1].VirtualAddress,
			image->FileHeader, image->MappedAddress);
		if (importDesc != NULL) while(1)
		{
			// See if we've reached an empty IMAGE_IMPORT_DESCRIPTOR
			if ((importDesc->TimeDateStamp == 0) && (importDesc->Name == 0))
				break;

			char const* importName = static_cast<char const*>(GetPtrFromRVA(importDesc->Name,
				image->FileHeader,
				image->MappedAddress));
			importDesc++;

			if (std::strstr(importName, ".sys") != nullptr || std::strstr(importName, ".SYS") != nullptr)
			{ 
				result.push_back(ToWideString(importName).c_str());
			}
			else
			{
				skippedDependencies.insert(ToWideString(importName).c_str());
			}
		}
	}
	ImageUnload(image);
	return result;
}

void CopyBootDrivers()
{
	RegistryKey const systemKey(HKEY_LOCAL_MACHINE, L"SYSTEM");
	RegistryKey const currentControlSetKey(systemKey, L"CurrentControlSet");
	RegistryKey const servicesKey(currentControlSetKey, L"Services");

	StringSet pendingDrivers;

	RegistryKey::KeysCollection const services = servicesKey.EnumerateKeys();
	for (RegistryKey::KeysCollection::const_iterator it = services.begin(); it != services.end(); ++it)
	{
		RegistryKey const serviceKey(servicesKey, it->c_str());

		int startType;
		if (!serviceKey.GetInt(L"Start", startType) || startType != SERVICE_BOOT_START)
		{
			if (startType == 1)
			{
				continue;
			}
			continue;
		}

		std::wstring imagePath;
		if (!serviceKey.GetString(L"ImagePath", imagePath))
		{
			continue;
		}

		::CharLower(&imagePath[0]);
		pendingDrivers.insert(L"c:\\windows\\" + imagePath);
	}

	// FS driver has to be explicitly added
	pendingDrivers.insert(L"c:\\windows\\system32\\drivers\\ntfs.sys");

	StringSet bootDrivers;
	StringSet skippedDependencies;
	while (!pendingDrivers.empty())
	{
		std::wstring const& front = *pendingDrivers.begin();
		bootDrivers.insert(front);

		StringArray deps = GetDriverDependencies(front, skippedDependencies);
		for (StringArray::iterator it = deps.begin(); it != deps.end(); ++it)
		{
			::CharLower(&(*it)[0]);

			std::wstring const path = L"c:\\windows\\system32\\drivers\\" + *it;
			if (bootDrivers.find(path) == bootDrivers.end())
			{
				pendingDrivers.insert(path);
			}
		}

		pendingDrivers.erase(front);
	}

	for (StringSet::const_iterator it = bootDrivers.begin(); it != bootDrivers.end(); ++it)
	{
		std::wstring const& from = *it;
		std::wstring to = from;
		to[0] = L'r';
		CopyFileNoThrow(from, to);
	}
}

void CopyKernelAndDependencies()
{
	static const LPCTSTR files[] =
	{ 
		// kernel and deps
		L"Windows\\System32\\ntoskrnl.exe",
		L"Windows\\System32\\hal.dll",
		L"Windows\\System32\\kdcom.dll",
		L"Windows\\System32\\pshed.dll",
		L"Windows\\System32\\ci.dll",
		L"Windows\\System32\\clfs.sys",				// Windows 7
		L"Windows\\System32\\drivers\\clfs.sys",

		// NLS and fonts
		L"Windows\\System32\\l_intl.nls",
		L"Windows\\System32\\c_1252.nls",
		L"Windows\\System32\\c_437.nls",
		L"Windows\\Fonts\\vgaoem.fon"
	};


	for (int i = 0; i < sizeof(files) / sizeof(files[0]); ++i)
	{
		std::wstring const from = std::wstring(L"C:\\") + files[i];
		std::wstring const to = std::wstring(L"R:\\") + files[i];

		CopyFileNoThrow(from, to);
	}
}

int main(int argc, char* argv[])
{
	try
	{
		::CreateDirectory(L"R:\\Windows", NULL);
		::CreateDirectory(L"R:\\Windows\\Fonts", NULL);
		::CreateDirectory(L"R:\\Windows\\System32", NULL);
		::CreateDirectory(L"R:\\Windows\\System32\\drivers", NULL);

		CopyKernelAndDependencies();
		CopyBootDrivers();
	}
	catch (Exception const& e)
	{
		std::wcout << "Failure: " << e.Message() << std::endl;
	}
	catch (std::exception const& e)
	{
		std::wcout << "Failure: " << e.what() << std::endl;
	}

	return 0;
}