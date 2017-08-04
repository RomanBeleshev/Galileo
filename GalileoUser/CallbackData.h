#pragma once

typedef struct _FLT_IO_PARAMETER_BLOCK FLT_IO_PARAMETER_BLOCK;

class CallbackData
{
public:
	CallbackData(void const* buffer);
	~CallbackData();

	int MajorCode() const;
	int MinorCode() const;
	int OperationFlags() const;
	int InformationClass() const;
	wchar_t const* FileName() const;

	void const* InputBuffer() const;
	int InputBufferLength() const;

	int OutputBufferLength() const;

private:
	struct ExtraParams;

private:
	FLT_IO_PARAMETER_BLOCK const* m_ioParams;
	ExtraParams const* m_extraParams;
	wchar_t const* m_fileName;
};