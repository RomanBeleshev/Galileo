#include "Common/Exception.h"
#include "Common/PrivilegeManager.h"
#include "Common/WinAPi.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <map>

WCHAR const DriveFrom = L'B';
WCHAR const DriveTo = L'E';

struct StartupLogRecord
{
	std::string ProcessName;
	std::string Operation;
	std::string Path;
	std::string Result;
	std::string Detail;
	std::string EventClass;
};

typedef std::map<std::string, int> Distribution;

struct FileOperation
{
	enum class OperationType
	{
		Read,
		Write,
		Irrelevant,
		Other
	};

	FileOperation(std::string const& operationString, std::string const& resultString) :
		Operation(OperationFromString(operationString)),
		Success(ResultFromString(resultString))
	{
	}

	OperationType OperationFromString(std::string const& operationString)
	{
		if (operationString == "ReadFile" || operationString.find("Query") == 0)
		{
			return OperationType::Read;
		}
		if (operationString == "WriteFile" || operationString.find("Set") == 0)
		{
			return OperationType::Write;
		}

		if (operationString == "CreateFile" || 
			operationString.find("IRP") != std::string::npos || 
			operationString.find("FASTIO") != std::string::npos)
		{
			return OperationType::Irrelevant;
		}

		return OperationType::Other;
	}

	bool ResultFromString(std::string const& resultString)
	{
		if (resultString == "SUCCESS" || resultString == "NO MORE FILES" || resultString == "END OF FILE")
		{
			return true;
		}

		return false;
	}

	OperationType Operation;
	bool Success;
};

struct FileStatistics
{
	FileStatistics() :
		WriteOnly(false),
		ReadOnly(false),
		ReadWrite(false),
		Other(false)
	{
	}

	void ProcessOperation(std::string const& operationString, std::string const& resultString)
	{
		FileOperation const operation(operationString, resultString);

		// skip irrelevant operations
		if (operation.Operation == FileOperation::OperationType::Irrelevant)
		{
			return;
		}

		// skip failed operations
		if (!operation.Success)
		{
			return;
		}

		if (Operations.empty())
		{
			WriteOnly = operation.Operation == FileOperation::OperationType::Write;
			ReadOnly = operation.Operation == FileOperation::OperationType::Read;
			ReadWrite = WriteOnly || ReadOnly;
			Other = operation.Operation == FileOperation::OperationType::Other;
		}
		else
		{
			if (operation.Operation == FileOperation::OperationType::Write)
			{
				ReadWrite = true;
				ReadOnly = false;
			}
			else if (operation.Operation == FileOperation::OperationType::Read)
			{
				ReadWrite = true;
				WriteOnly = false;
			}
			else if (operation.Operation == FileOperation::OperationType::Other)
			{
				Other = true;
			}
		}

		if (Operations.empty() || Operations.back().Operation != operation.Operation)
		{
			Operations.push_back(operation);
			if (OperationsString.find(operationString) == std::string::npos)
			{
				OperationsString += operationString + " ";
			}
		}
	}

	bool WriteOnly;
	bool ReadOnly;
	bool ReadWrite;
	bool Other;

	std::vector<FileOperation> Operations;
	std::string OperationsString;
};
struct StartupStatistics
{
	void ProcessRecord(StartupLogRecord const& record)
	{
		// Process disk C: only
		if (record.Path.size() < 4 || record.Path[0] != 'c' && record.Path[0] != 'C')
		{
			return;
		}

		// Skip system objects
		if (record.Path.find("C:\\$") != std::string::npos || record.Path.find("C:\\Extend\\") != std::string::npos)
		{
			SkippedSystemObjects.insert(record.Path);
			return;
		}

		// Skip requests with masks
		if (record.Path.find_first_of("*?") != std::string::npos)
		{
			SkippedMasks.insert(record.Path);
			return;
		}

		// Skip pagefile
		if (record.Path.find("pagefile.sys") != std::string::npos)
		{
			return;
		}

		// Do not overwrite C:\Boot\BCD
		if (record.Path.find(":\\Boot\\") != std::string::npos)
		{
			return;
		}


		Processes[record.ProcessName]++;
		Operations[record.Operation]++;
		Results[record.Result]++;
		EventClasses[record.EventClass]++;

		FilesStatistics[record.Path].ProcessOperation(record.Operation, record.Result);
	}

	Distribution Processes;
	Distribution Operations;
	Distribution Results;
	Distribution EventClasses;

	std::set<std::string> SkippedSystemObjects;
	std::set<std::string> SkippedMasks;

	typedef std::map<std::string, FileStatistics> StatisticsByFiles;
	StatisticsByFiles FilesStatistics;
};

class StringTokenizer
{
public:
	StringTokenizer(char const* source) : m_context(source)
	{
	}

	bool NextToken(std::string& result)
	{
		char const* start = std::strchr(m_context, '\"');
		if (start == nullptr)
		{
			return false;
		}

		m_context = start + 1;

		char const* end = std::strchr(m_context, '\"');
		if (end == nullptr)
		{
			return false;
		}

		result.assign(m_context, end - m_context);
		m_context = end + 1;
		return true;
	}

private:
	char const* m_context;
};

void ParseStartupLog(char const* filePath, StartupStatistics& statistics)
{
	std::ifstream logStream;
//	logStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	logStream.open(filePath);

	int ProcessNameIndex = -1;
	int OperationIndex = -1;
	int PathIndex = -1;
	int ResultIndex = -1;
	int DetailIndex = -1;
	int EventClassIndex = -1;

	std::string line;
	std::getline(logStream, line);
	StringTokenizer header(std::strchr(&line[0], '\"'));
	std::string token;
	for (int i = 0; header.NextToken(token); ++i)
	{
		if (token == "Process Name")
		{
			ProcessNameIndex = i;
		}
		else if (token == "Operation")
		{
			OperationIndex = i;
		}
		else if (token == "Path")
		{
			PathIndex = i;
		}
		else if (token == "Result")
		{
			ResultIndex = i;
		}
		else if (token == "Detail")
		{
			DetailIndex = i;
		}
		else if (token == "Event Class")
		{
			EventClassIndex = i;
		}
	}

	if (ProcessNameIndex == -1 || OperationIndex == -1 || PathIndex == -1 || ResultIndex == -1 || DetailIndex == -1 || EventClassIndex == -1)
	{
		throw std::invalid_argument("Invalid input file format. Mandatory column(s) missing.");
	}

	while (logStream.good())
	{
		std::getline(logStream, line);
		if (line.empty() || line.find("\"\"\"\"") != std::string::npos)
		{
			continue;
		}

		StartupLogRecord record;
		std::string eventClass;

		StringTokenizer header(&line[0]);
		std::string token;
		for (int i = 0; header.NextToken(token); ++i)
		{
			if (i == ProcessNameIndex)
			{
				record.ProcessName = token;
			}
			else if (i == OperationIndex)
			{
				record.Operation = token;
			}
			else if (i == PathIndex)
			{
				record.Path = token;
			}
			else if (i == ResultIndex)
			{
				record.Result = token;
			}
			else if (i == DetailIndex)
			{
				record.Detail = token;
			}
			else if (i == EventClassIndex)
			{
				record.EventClass = token;
			}
		}

		if (record.EventClass != "File System")
		{
			continue;
		}

		statistics.ProcessRecord(record);
	}
}

void OutputDistribution(Distribution const& distr)
{
	for (Distribution::const_iterator it = distr.begin(); it != distr.end(); ++it)
	{
		std::cout << it->first << "\t" << it->second << std::endl;
	}
}

void OutputFileOperations(StartupStatistics::StatisticsByFiles const& statisticsByFiles)
{
	std::set<std::string> Write;
	std::set<std::string> Read;
	std::set<std::string> ReadWrite;
	std::set<std::string> Other;
	std::set<std::string> Operations;

	for (auto const it : statisticsByFiles)
	{
		if (it.second.WriteOnly)
		{
			Write.insert(it.first);
		}
		if (it.second.ReadOnly)
		{
			Read.insert(it.first);
		}
		if (!it.second.WriteOnly && !it.second.ReadOnly && it.second.ReadWrite)
		{
			ReadWrite.insert(it.first);
		}
		if (!it.second.ReadWrite && it.second.Other)
		{
			Other.insert(it.first);
		}
		Operations.insert(it.second.OperationsString);
	}

}

void OutputStatistics(StartupStatistics const& statistics)
{
	/*
	std::cout << "Operations:" << std::endl;
	OutputDistribution(statistics.Operations);

	std::cout << std::endl << "Results:" << std::endl;
	OutputDistribution(statistics.Results);
	*/

	OutputFileOperations(statistics.FilesStatistics);
}

bool HasAttribute(DWORD attributes, DWORD attribute)
{
	return (attributes & attribute) != 0;
}

#pragma pack(push, 1)

typedef struct _REPARSE_DATA_BUFFER {
	ULONG  ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG  Flags;
			WCHAR  PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR  PathBuffer[1];
		} MountPointReparseBuffer;
		struct {
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
	};
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#pragma pack(pop)

bool PathExists(std::wstring const& path)
{
	return ::GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::wstring GetParentPath(std::wstring const& path)
{
	std::size_t const lastSlash = path.find_last_of(L"\\");
	if (lastSlash == std::string::npos)
	{
		throw std::invalid_argument("No slash in the parent path.");
	}

	return path.substr(0, lastSlash);
}

void CloneDirectoryRecursively(std::wstring const& path, std::wstring const& cloneFrom)
{
	// CreateDirectoryEx alsoautomatically creates reparse point (but a wrong one)
//	while (::CreateDirectoryEx(cloneFrom.c_str(), path.c_str(), NULL) == FALSE)
	while (::CreateDirectory(path.c_str(), NULL) == FALSE)
	{
		if (::GetLastError() == ERROR_PATH_NOT_FOUND)
		{
			CloneDirectoryRecursively(GetParentPath(path), GetParentPath(cloneFrom));
		}
		else
		{
			HandleImplicitError(FALSE);
		}
	}
}

void ChangeDriveLetter(LPTSTR letter)
{
	if (*letter == L'C' || *letter == L'c')
	{
		*letter = DriveTo;
	}
	else
	{
		*letter = DriveTo;
	}
}

typedef std::unique_ptr<void, BOOL(__stdcall*)(HANDLE)> HandleGuard;

void CopyReparsePoint(std::wstring const& from, std::wstring const& to)
{
	HandleGuard fromReparsePoint(::CreateFile(from.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, 0), ::CloseHandle);
	HandleImplicitError(fromReparsePoint.get() != INVALID_HANDLE_VALUE);

	CHAR buffer[1024];
	DWORD reparsePointSize;
	HandleImplicitError(::DeviceIoControl(fromReparsePoint.get(),
		FSCTL_GET_REPARSE_POINT,     // dwIoControlCode
		NULL,                        // lpInBuffer
		0,                           // nInBufferSize
		buffer,						// output buffer
		sizeof(buffer),				// size of output buffer
		&reparsePointSize,   // number of bytes returned
		NULL  // OVERLAPPED structure
		));

	fromReparsePoint.reset();

	PREPARSE_DATA_BUFFER reparseBuffer = reinterpret_cast<PREPARSE_DATA_BUFFER>(buffer);

	LPTSTR printName = reparseBuffer->SymbolicLinkReparseBuffer.PathBuffer + reparseBuffer->SymbolicLinkReparseBuffer.PrintNameOffset / 2;
	printName = reparseBuffer->MountPointReparseBuffer.PathBuffer + reparseBuffer->MountPointReparseBuffer.PrintNameOffset / 2;
//	ChangeDriveLetter(printName);

	LPTSTR substituteName = reparseBuffer->SymbolicLinkReparseBuffer.PathBuffer + reparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset / 2;
	substituteName = reparseBuffer->MountPointReparseBuffer.PathBuffer + reparseBuffer->MountPointReparseBuffer.SubstituteNameOffset / 2;
//	ChangeDriveLetter(substituteName + 4);

	if (PathExists(to))
	{
		return;
	}

	CloneDirectoryRecursively(to, from);
	HandleGuard toReparsePoint(::CreateFile(to.c_str(), GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0), ::CloseHandle);
	HandleImplicitError(toReparsePoint.get() != INVALID_HANDLE_VALUE);

	HandleImplicitError(::DeviceIoControl(toReparsePoint.get(),              // handle to file or directory
		FSCTL_SET_REPARSE_POINT,       // dwIoControlCode
		buffer,           // input buffer 
		reparsePointSize,         // size of input buffer
		NULL,                          // lpOutBuffer
		0,                             // nOutBufferSize
		NULL,                          // lpBytesReturned
		NULL));
}

void CopyFile(std::wstring const& from, std::wstring const& to)
{
	SHFILEOPSTRUCT s = { 0 };
	s.hwnd = NULL;
	s.wFunc = FO_COPY;
	s.fFlags = FOF_SILENT | FOF_NORECURSION | FOF_NOCONFIRMATION;
	s.pTo = to.c_str();
	s.pFrom = from.c_str();

	int result = SHFileOperation(&s);
	if (result != 0)
	{
		std::ostringstream errorText;
		errorText << "SHFileOperation error: 0x" << std::hex << result;
		throw std::invalid_argument(errorText.str());
	}
}

void CopyFileSystemObject(std::string const& path)
{
	std::wstring from = ToWideString(path);
	std::wstring to = ToWideString(path);

	from += L'\0';
	to += L'\0';

	from[0] = DriveFrom;
	to[0] = DriveTo;

	DWORD const attributes = ::GetFileAttributes(from.c_str());
	HandleImplicitError(attributes != INVALID_FILE_ATTRIBUTES);

	bool const isDirectory = HasAttribute(attributes, FILE_ATTRIBUTE_DIRECTORY);
	bool const isReparsePoint = HasAttribute(attributes, FILE_ATTRIBUTE_REPARSE_POINT);

	if (isReparsePoint)
	{
		if (!isDirectory)
		{
			int g = 0;
		}
		CopyReparsePoint(from, to);
	}
	else if (!isDirectory)
	{
		CopyFile(from, to);
	}
}

void CopyStartupFiles(StartupStatistics::StatisticsByFiles const& statisticsByFiles)
{
	std::set<std::string> failedFiles;
	std::set<std::string> writeOnlyFiles;

	for (auto const it : statisticsByFiles)
	{
		if (it.second.Operations.empty())
		{
			continue;
		}

		if (it.second.WriteOnly)
		{
			writeOnlyFiles.insert(it.first);
//			continue;
		}

		try
		{
			std::cout << it.first;
			CopyFileSystemObject(it.first);
			std::cout << " - OK" << std::endl;
		}
		catch (Exception const& e)
		{
			failedFiles.insert(it.first);
			std::wcout << " - Failure: " << e.Message() << std::endl;
		}
		catch (std::exception const& e)
		{
			failedFiles.insert(it.first);
			std::wcout << " - Failure: " << e.what() << std::endl;
		}
	}

	std::cout << "Files failed to copy:" << std::endl;
	for (auto const it : failedFiles)
	{
		std::cout << it << std::endl;
	}
}

void SetPrivileges()
{
	PrivilegeManager pm;
	pm.SetPrivilege(SE_BACKUP_NAME);
	pm.SetPrivilege(SE_RESTORE_NAME);

	pm.SetPrivilege(SE_TCB_NAME);
	pm.SetPrivilege(SE_ASSIGNPRIMARYTOKEN_NAME);
	pm.SetPrivilege(SE_INCREASE_QUOTA_NAME);

	pm.SetPrivilege(SE_SECURITY_NAME);
	pm.SetPrivilege(SE_TAKE_OWNERSHIP_NAME);
	pm.SetPrivilege(TEXT("SeCreateSymbolicLinkPrivilege"));
}

int main(int argc, char* argv[])
{
	try
	{
		//BOOL r = ::CreateDirectoryEx(L"B:\\ProgramData\\Application Data", L"E:\\ProgramData\\Application Data", FALSE);
		//r = ::CreateDirectoryEx(L"B:\\ProgramData", L"E:\\ProgramData", FALSE);
		//r = ::CreateDirectoryEx(L"B:\\ProgramData\\Application Data", L"E:\\ProgramData\\Application Data", FALSE);

		SetPrivileges();

		StartupStatistics statistics;
		ParseStartupLog("B:\\startup.csv", statistics);
		CopyStartupFiles(statistics.FilesStatistics);
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