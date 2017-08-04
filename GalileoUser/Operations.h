#pragma once

class IFileSystem;
void OperationsRoutine(IFileSystem& fileSystem, char const* iBuffer, int iSize, char* oBuffer, int& oSize);
