#pragma once

#include "fltKernel.h"

struct FileInfo;
class IFileInfoIterator;

class FileSystemConverter
{
public:
	static NTSTATUS ConvertListTo(IFileInfoIterator& list, PFILE_ID_BOTH_DIR_INFORMATION buffer, int& size);
	static NTSTATUS ConvertListTo(IFileInfoIterator& list, PFILE_FULL_DIR_INFORMATION buffer, int& size);
};