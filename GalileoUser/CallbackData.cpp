#include "CallbackData.h"

#include "fltKernel.h"

struct CallbackData::ExtraParams
{
	union
	{
		ULONG LongParam;
		LONGLONG LongLongParam;
	};

	ULONG OutputBufferLength;
	PVOID FileObject;
};

template <typename ToType, typename FromType>
ToType const* AdvancePointer(FromType const* from, int size = sizeof(FromType))
{
	char const* pointer = reinterpret_cast<char const*>(from);
	return reinterpret_cast<const ToType*>(pointer + size);
}

CallbackData::CallbackData(void const* buffer) :
	m_ioParams(reinterpret_cast<FLT_IO_PARAMETER_BLOCK const*>(buffer)),
	m_extraParams(AdvancePointer<ExtraParams>(m_ioParams)),
	m_fileNameBuffer(AdvancePointer<wchar_t>(m_extraParams, sizeof(ExtraParams) + 2)),
	m_fileName(m_fileNameBuffer)
{
	if (m_fileName.size() > 1 && m_fileName.back() == L'\\')
	{
		m_fileName.resize(m_fileName.size() - 1);
	}
}

CallbackData::~CallbackData()
{
}

int CallbackData::MajorCode() const
{
	return m_ioParams->MajorFunction;
}

int CallbackData::MinorCode() const
{
	return m_ioParams->MinorFunction;
}

int CallbackData::OperationFlags() const
{
	return m_ioParams->OperationFlags;
}

std::wstring const& CallbackData::FileName() const
{
	return m_fileName;
}

int CallbackData::InformationClass() const
{
	return m_extraParams->LongParam;
}

int CallbackData::FileInformationClass() const
{
	return m_extraParams->LongParam;
}

int CallbackData::Disposition() const
{
	return (m_extraParams->LongParam >> 24) & 0x000000ff;
}

int CallbackData::CreateOptions() const
{
	return m_extraParams->LongParam;
}

std::int64_t CallbackData::Offset() const
{
	return m_extraParams->LongLongParam;
}

int CallbackData::OutputBufferLength() const
{
	return m_extraParams->OutputBufferLength;
}

void const* CallbackData::InputBuffer() const
{
	return nullptr;
}

int CallbackData::InputBufferLength() const
{
	return 0;
}

void const* CallbackData::FileObject() const
{
	return m_extraParams->FileObject;
}
