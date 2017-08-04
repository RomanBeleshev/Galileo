#include <Windows.h>
#include <FltUser.h>

#include "Common/Exception.h"
#include "Common/PrivilegeManager.h"

#include "InMemoryFileSystem.h"
#include "FileSystemStub.h"
#include "CommunicationPort.h"
#include "Operations.h"

#include <string>
#include <iostream>
#include <conio.h>

class FilterDriver
{
public:
	FilterDriver(std::wstring const& name, std::wstring const& drive) :
		m_name(name),
		m_drive(drive)
	{
		HRESULT result = ::FilterLoad(m_name.c_str());
		if (result != HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING))
		{
			HandleExplicitError(result);
		}
		result = ::FilterAttach(m_name.c_str(), m_drive.c_str(), NULL, 0, NULL);
		if (result != ERROR_FLT_INSTANCE_NAME_COLLISION)
		{
			HandleExplicitError(result);
		}
	}

	~FilterDriver()
	{
		HandleExplicitError(::FilterDetach(m_name.c_str(), m_drive.c_str(), NULL));
		HandleExplicitError(::FilterUnload(m_name.c_str()));
	}

private:
	std::wstring const m_name;
	std::wstring const m_drive;
};

char iBuffer[1024 * 1024];
char oBuffer[1024 * 1024];

int main(int, char*[])
{
	try
	{
		InMemoryFileSystem fileSystem;
		FileSystemStub().PopulateFileSystem(fileSystem);

		PrivilegeManager pm;
		pm.SetPrivilege(SE_LOAD_DRIVER_NAME);
	
		FilterDriver driver(L"GalileoFilter", L"R:\\");
		CommunicationPort port(L"\\GalileoCommunicationPort");

		while (!_kbhit())
		{
			port.GetMessage(iBuffer, sizeof(iBuffer));
			char const *iBody = CommunicationPort::GetMessageBody(iBuffer);
			int oSize = CommunicationPort::GetMaxReplyBodySize(iBuffer);
			if (sizeof(oBuffer) < oSize)
			{
				oSize = sizeof(oBuffer);
			}
			OperationsRoutine(fileSystem, iBody, 0, CommunicationPort::GetMessageReplyBody(oBuffer), oSize);
			port.ReplyMessage(oBuffer, oSize);
		}
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