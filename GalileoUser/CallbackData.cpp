#include "CallbackData.h"

#include "fltKernel.h"

struct CallbackData::ExtraParams
{
	ULONG InformationClass;
	ULONG OutputBufferLength;
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
	m_fileName(AdvancePointer<wchar_t>(m_extraParams, sizeof(ExtraParams) + 2))
{
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

wchar_t const* CallbackData::FileName() const
{
	return m_fileName;
}

int  CallbackData::InformationClass() const
{
	return m_extraParams->InformationClass;
}

int  CallbackData::OutputBufferLength() const
{
	return m_extraParams->OutputBufferLength;
}

void const*  CallbackData::InputBuffer() const
{
	return nullptr;
}

int  CallbackData::InputBufferLength() const
{
	return 0;
}

