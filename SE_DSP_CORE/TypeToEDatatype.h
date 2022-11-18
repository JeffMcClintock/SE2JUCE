#pragma once
#include "./modules/se_sdk2/se_datatypes.h"

template<class U> struct SeDatatypeTraits
{
	enum { result = DT_NONE };
};

template<>
struct SeDatatypeTraits<float>
{
	enum { result = DT_FLOAT };
};

template<>
struct SeDatatypeTraits<std::wstring>
{
	enum { result = DT_TEXT };
};

template<>
struct SeDatatypeTraits<bool>
{
	enum { result = DT_BOOL };
};

template<>
struct SeDatatypeTraits<int>
{
	enum { result = DT_INT };
};

template<>
struct SeDatatypeTraits<MpBlob>
{
	enum { result = DT_BLOB };
};

template<>
struct SeDatatypeTraits<short>
{
	enum { result = DT_ENUM };
};

template<>
struct SeDatatypeTraits<double>
{
	enum { result = DT_DOUBLE };
};

// converts a type (float) to a plug datatype enum (DT_FLOAT)
template <typename T> class TypeToEDatatype
{

public:
	static const EPlugDataType enum_value = (EPlugDataType) SeDatatypeTraits<T>::result;
};
