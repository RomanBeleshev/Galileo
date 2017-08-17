#pragma once

#include "fltKernel.h"

struct FileInfo;
class IFileInfoIterator;

class FileSystemConverter
{
public:
	static NTSTATUS ConvertList(IFileInfoIterator& list, PFILE_ID_BOTH_DIR_INFORMATION buffer, int& size);
	static NTSTATUS ConvertList(IFileInfoIterator& list, PFILE_BOTH_DIR_INFORMATION buffer, int& size);
	static NTSTATUS ConvertList(IFileInfoIterator& list, PFILE_FULL_DIR_INFORMATION buffer, int& size);

	static void ConvertFileInfo(FileInfo const& from, PFILE_BASIC_INFORMATION to, int& size);
	static void ConvertFileInfo(FileInfo const& from, PFILE_NETWORK_OPEN_INFORMATION to, int& size);
	static void ConvertFileInfo(FileInfo const& from, PFILE_STANDARD_INFORMATION to, int& size);
	static void ConvertFileInfo(FileInfo const& from, PFILE_ATTRIBUTE_TAG_INFORMATION to, int& size);
	static void ConvertFileInfo(FileInfo const& from, PFILE_NAME_INFORMATION to, int& size);
};