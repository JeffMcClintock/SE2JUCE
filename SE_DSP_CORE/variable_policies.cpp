#include "pch.h"
#include "variable_policies.h"
#include "conversion.h"
#include "modules/se_sdk3/it_enum_list.h"
#include "IPluginGui.h"
#include "RawConversions.h"

int32_t MetaData_none::GetDatatype( ParameterFieldType field, int* returnValue)
{
	*returnValue = 0;
	return gmpi::MP_FAIL;
}

bool MetaData_enum::ValueFromNormalised(float p_normalised, int& returnValue, bool applyDawAjustment)
{
	returnValue = normalised_to_enum( getEnumList(), p_normalised );
	return true;
}
void MetaData_enum::setEnumList(const wchar_t* enumList)
{
	setEnumList(std::wstring(enumList));
}

float MetaData_enum::NormalisedFromValue(int p_value)
{
	return enum_to_normalised(std::wstring(getEnumList().c_str()),p_value);
	/*
		int hi,lo;
		get _enum_range(getEnumList(),lo,hi);

		return (float) ( p_value - lo ) / (hi - lo);
	*/
}

void MetaData_none::GetValueRaw2( ParameterFieldType field, const void** data, int* size )
{
	*data = 0;
	*size = 0;
}

void MetaData_none::SetValueRaw( ParameterFieldType field, const void* data, int size )
{
	assert(false);
}

float MetaData_none::NormalisedFromValue(bool value)
{
	if(value)
		return 1.f;
	else
		return 0.f;
}

int32_t MetaData_enum::GetDatatype( ParameterFieldType field, int* returnValue)
{
	switch(field)
	{
	case FT_ENUM_LIST:
	{
		*returnValue = (int) TypeToEDatatype<std::wstring>::enum_value;;
	}
	break;

	default:
		assert(false);
		return gmpi::MP_FAIL;
	};

	return gmpi::MP_OK;
}

void MetaData_enum::GetValueRaw2( ParameterFieldType field, const void** data, int* size )
{
	switch(field)
	{
	case FT_ENUM_LIST:
	{
		*size = (int) (sizeof(wchar_t) * m_enum_list.size());
		*data = RawData3( m_enum_list );
	}
	break;

	default:
		*data = 0;
		*size = 0;
		assert(false);
	};
}

int MetaData_enum::getRangeMinimum( void )
{
	return get_enum_range_lo( std::wstring( m_enum_list.c_str() ) );
}

int MetaData_enum::getRangeMaximum( void )
{
	return get_enum_range_hi( std::wstring( m_enum_list.c_str() ) );
}


void MetaData_enum::SetValueRaw( ParameterFieldType field, const void* data, int size )
{
	switch(field)
	{
	case FT_ENUM_LIST:
		setEnumList( RawToValue<std::wstring>(data, size) );
		break;

	default:
		assert(false);
	};
}

/*
class RawValuePtr
{
	void *data;
	int size;
}
*/
int32_t MetaData_filename::GetDatatype( ParameterFieldType field, int* returnValue)
{
	*returnValue = (int) TypeToEDatatype<std::wstring>::enum_value;
	return gmpi::MP_OK;
}

std::wstring MetaData_filename::getMetaValue()
{
	return std::wstring(m_file_ext.c_str());
};
std::wstring MetaData_enum::getMetaValue()
{
	return std::wstring(m_enum_list.c_str());
};

void MetaData_filename::GetValueRaw2( ParameterFieldType field, const void** data, int* size )
{
	switch(field)
	{
	case FT_FILE_EXTENSION:
	{
		*size = (int) (sizeof(wchar_t) * m_file_ext.size());
		*data = RawData3( m_file_ext );
	}
	break;

	default:
		*data = 0;
		*size = 0;
		assert(false);
	};
}

void MetaData_filename::SetValueRaw( ParameterFieldType field, const void* data, int size )
{
	switch(field)
	{
	case FT_FILE_EXTENSION:
		setFileExt( RawToValue<std::wstring>(data, size) );
		break;

	default:
		assert(false);
	};
}

