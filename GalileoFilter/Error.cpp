#include "Error.h"
#include "Trace.h"

bool ApiCallSucceeded(NTSTATUS status, PCWSTR location)
{
	if (NT_SUCCESS(status))
	{
		return true;
	}
	else
	{
		PT_DBG_PRINT(PTDBG_TRACE_GALILEO, ("GalileoFilter!%ls failed with status=%08x\n", location, status));
		return false;
	}
}