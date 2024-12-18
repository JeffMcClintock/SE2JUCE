#pragma once
#include <assert.h>
#include "./my_input_stream.h"
#include "./TypeToEDatatype.h"
#include "./modules/se_sdk3/mp_api.h"
#include "IPluginGui.h"
#include "RawConversions.h"
#include "./modules/tinyXml2/tinyxml2.h"
#include "SerializationHelper_XML.h"

// Specialized Extra Data policy classes

struct MetaData_none // standard variable, no extra data
{
	static MetaData_none global_metadata_none; // saves creating many instances when needed as member
	void SerialiseMetaData( my_output_stream& ) {}
	void SerialiseMetaData( my_input_stream& ) {}
	virtual int32_t GetDatatype( ParameterFieldType field, int* returnValue);
	void SetValueRaw( ParameterFieldType field, const void* data, int size );
	void GetValueRaw2( ParameterFieldType field, const void** data, int* size );
	int getMetaValue()
	{
		return 0;
	} // dummy function

	// fillers for values NOT convertible to normalised
	bool ValueFromNormalised(float /*p_normalised*/, MpBlob& /*returnValue*/, bool /*applyDawAjustment*/)
	{
		return false;
	} // filler
	bool ValueFromNormalised(float /*p_normalised*/, int& /*returnValue*/, bool /*applyDawAjustment*/)
	{
		return false;
	} // filler
	bool ValueFromNormalised(float p_normalised, bool& returnValue, bool /*applyDawAjustment*/)
	{
		returnValue = p_normalised >= 0.5f;
		return true;
	} // filler

	bool ValueFromNormalised(float /*p_normalised*/, std::wstring& returnValue, bool /*applyDawAjustment*/)
	{
		returnValue.clear();
		return false;
	} // filler

	float NormalisedFromValue(int /*p_value*/)
	{
		return 0.0f;
	}
	float NormalisedFromValue(bool value);
	float NormalisedFromValue(const MpBlob& /*value*/)
	{
		return 0.0f;
	}
	float NormalisedFromValue(const std::wstring& /*p_value*/)
	{
		return 0.0f;
	}

	// New. every metadata same members.
	void setRangeMinimum( int /*minimum*/ ) {}
	void setRangeMaximum( int /*maximum*/ ) {}
	void setTextMetadata( const std::wstring& /*text*/ ) {}
	int getRangeMinimum( void )
	{
		return 0;
	}
	int getRangeMaximum( void )
	{
		return 0;
	}
	std::wstring getTextMetadata( void )
	{
		return L"";
	}

	void parse(const std::wstring& /*val*/)
	{}

	void Export(tinyxml2::XMLElement* /*parameter_xml*/, ExportFormatType /*targetType*/)
	{
	}
	void Import(tinyxml2::XMLElement* /*parameter_xml*/, ExportFormatType /*targetType*/)
	{
	}
};

struct MetaData_enum // enum, provides list-of-values
{
	MetaData_enum() {}
	MetaData_enum( const wchar_t* p_enum_list ) : m_enum_list(p_enum_list) {} // optial contructor with initialiser
	virtual std::wstring getEnumList()
	{
		return m_enum_list;
	}
	virtual std::wstring getMetaValue();
	void setEnumList(const wchar_t* enumList);
	//	void setEnumList(const std::wstring &p_enum_list);
	virtual void setEnumList(const std::wstring& p_enum_list)
	{
		if( m_enum_list != p_enum_list ) // avoid unnesc messages
		{
			m_enum_list = p_enum_list;
			OnMetaDataChanged();
		}
	}
	virtual void OnMetaDataChanged() {}
	void SerialiseMetaData( my_output_stream& p_stream )
	{
		p_stream << m_enum_list;
	}
	void SerialiseMetaData( my_input_stream& p_stream )
	{
		p_stream >> m_enum_list;
	}
	bool ValueFromNormalised(float p_normalised, int& returnValue, bool applyDawAjustment);
	float NormalisedFromValue(int p_value);
	virtual int32_t GetDatatype( ParameterFieldType field, int* returnValue);
	void SetValueRaw( ParameterFieldType field, const void* data, int size );
	void GetValueRaw2( ParameterFieldType field, const void** data, int* size );

	// New. every metadata same members.
	void setRangeMinimum( int /*minimum*/ ) {}
	void setRangeMaximum( int /*maximum*/ ) {}
	void setTextMetadata( const std::wstring& text )
	{
		m_enum_list=text;
	}
	int getRangeMinimum( void );
	int getRangeMaximum( void );
	std::wstring getTextMetadata( void )
	{
		return m_enum_list;
	}
	void parse(const std::wstring& val)
	{
		m_enum_list = val;
	}
	void Export(tinyxml2::XMLElement* parameter_xml, ExportFormatType /*targetType*/)
	{
		if(!m_enum_list.empty())
		{
			parameter_xml->SetAttribute("MetaData", WStringToUtf8(m_enum_list).c_str());
		}
	}
	void Import(tinyxml2::XMLElement* parameter_xml, ExportFormatType /*targetType*/)
	{
		const char* s = "";
		parameter_xml->QueryStringAttribute("MetaData", &s);
		m_enum_list = Utf8ToWstring(s);
	}

private:
	std::wstring m_enum_list;
};

struct MetaData_filename // text, provides file optional extension
{
	std::wstring getFileExt()
	{
		return m_file_ext;
	}
	virtual std::wstring getMetaValue();
	//	void setFileExt(const std::wstring &p_file_ext);
	void setFileExt(const std::wstring& p_file_ext)
	{
		m_file_ext = p_file_ext;
		OnMetaDataChanged();
	}
	bool is_filename()
	{
		return !m_file_ext.empty();
	}
	virtual void OnMetaDataChanged() {}
	void SerialiseMetaData( my_output_stream& p_stream )
	{
		p_stream << m_file_ext;
	}
	void SerialiseMetaData( my_input_stream& p_stream )
	{
		p_stream >> m_file_ext;
	}
	virtual int32_t GetDatatype( ParameterFieldType field, int* returnValue);
	void SetValueRaw( ParameterFieldType field, const void* data, int size );
	void GetValueRaw2( ParameterFieldType field, const void** data, int* size );
	bool ValueFromNormalised(float /*p_normalised*/, std::wstring& /*returnValue*/, bool /*applyDawAjustment*/)
	{
		return false;
	} // filler

	float NormalisedFromValue(const std::wstring& /*p_value*/)
	{
		return 0.0f;
	}

	// New. every metadata same members.
	void setRangeMinimum( int /*minimum*/ ) {}
	void setRangeMaximum( int /*maximum*/ ) {}
	void setTextMetadata( const std::wstring& text )
	{
		m_file_ext=text;
	}

	int getRangeMinimum( void )
	{
		return 0;
	}
	int getRangeMaximum( void )
	{
		return 0;
	}

	std::wstring getTextMetadata( void )
	{
		return m_file_ext;
	}
	void parse(const std::wstring& val)
	{
		m_file_ext = val;
	}
	void Export(tinyxml2::XMLElement* parameter_xml, ExportFormatType /*targetType*/)
	{
		if(!m_file_ext.empty())
		{
			parameter_xml->SetAttribute("MetaData", WStringToUtf8(m_file_ext).c_str());
		}
	}
	void Import(tinyxml2::XMLElement* parameter_xml, ExportFormatType /*targetType*/)
	{
		const char* s = "";
		parameter_xml->QueryStringAttribute("MetaData", &s);
		m_file_ext = Utf8ToWstring(s);
	}

private:
	std::wstring m_file_ext;
};

template <typename T>
struct MetaData_ranged // provides hi/lo range
{
	// workarround user patch pins not inititialising gmetadata !!!
	MetaData_ranged() : m_hi( (T) 10), m_lo( (T) 0) {}

	void setRangeHi(T p_value)
	{
		m_hi = p_value;
	}
	void setRangeLo(T p_value)
	{
		m_lo = p_value;
	}
	T getRangeLo()
	{
		return m_lo;
	}
	T getRangeHi()
	{
		return m_hi;
	}
	virtual void OnMetaDataChanged() {}
	void SerialiseMetaData( my_output_stream& p_stream )
	{
		p_stream << m_lo;
		p_stream << m_hi;
	}
	void SerialiseMetaData( my_input_stream& p_stream )
	{
		p_stream >> m_lo;
		p_stream >> m_hi;
	}

	float adjustNormalisedForDaw(float p_normalised, bool applyDawAjustment)
	{
		if(!applyDawAjustment || getRangeLo() < getRangeHi())
		{
			return p_normalised;
		}

		return 1.0f - p_normalised;
	}

	bool ValueFromNormalised(float p_normalised, T& returnValue, bool applyDawAjustment)
	{
		const auto normalised = adjustNormalisedForDaw(p_normalised, applyDawAjustment);

		returnValue = (T) ((float)getRangeLo() + normalised * ( (float)getRangeHi() - (float)getRangeLo() ));
		return true;
	}
	float NormalisedFromValue(T p_value)
	{
		float normalised = (float)(p_value - getRangeLo()) / (float) ( getRangeHi() - getRangeLo());

		if( normalised > 1.f )
			normalised = 1.f;
		else
		{
			if( !( normalised >= 0.f ) ) // reverse logic catches NANs
				normalised = 0.f;
		}

		return normalised;
	}

	virtual int32_t GetDatatype( ParameterFieldType /*field*/, int* returnValue)
	{
		*returnValue = (int) TypeToEDatatype< T >::enum_value;
		return gmpi::MP_OK;
	}

	void SetValueRaw( ParameterFieldType field, const void* data, int size )
	{
		switch(field)
		{
		case FT_RANGE_HI:
			setRangeHi( RawToValue<T>(data, size) );
			break;

		case FT_RANGE_LO:
			setRangeLo( RawToValue<T>(data, size) );
			break;

		default:
			assert(false);
		};
	}
	
	void GetValueRaw2( ParameterFieldType field, const void** data, int* size)
	{
		*size = sizeof(T);

		switch (field)
		{
		case FT_RANGE_HI:
			*data = (void*)&m_hi; //RawData(getRangeHi());
			break;

		case FT_RANGE_LO:
			*data = (void*) &m_lo; // RawData(getRangeLo());
			break;

		default:
			assert(false);
		};
	}

	// New. every metadata same members.
	void setRangeMinimum( T minimum )
	{
		m_lo = minimum;
	}
	void setRangeMaximum( T maximum )
	{
		m_hi = maximum;
	}
	void setTextMetadata( const std::wstring& /*text*/ ) {}
	T getRangeMinimum( void )
	{
		return m_lo;
	}
	T getRangeMaximum( void )
	{
		return m_hi;
	}
	std::wstring getTextMetadata( void )
	{
		return L"";
	}
	void parse(const std::wstring& val)
	{
        std::wstring s = val;

		// e.g. s=",,10,0" => min=0 max= 0
		std::vector<std::wstring> words;
		while (s.size() > 0)
		{
			size_t i = s.find(L',');
			if (i == std::string::npos)
			{
				words.push_back(s);
				break;
			}
			else
			{
				std::wstring s2 = s.substr(0, i);
				size_t remainder = s.size() - i - 1;
				s = Right(s, remainder);
				s = TrimLeft(s);
				words.push_back(s2);
			}
		}
		if (words.size() == 4)
		{
			MyTypeTraits<T>::parse(words[2].c_str(), m_hi);
			MyTypeTraits<T>::parse(words[3].c_str(), m_lo);
		}
	}
	void Export(tinyxml2::XMLElement* parameter_xml, ExportFormatType /*targetType*/)
	{
		if(m_lo != 0)
		{
			parameter_xml->SetAttribute("RangeMinimum", m_lo);
		}
		if(m_hi != 10)
		{
			parameter_xml->SetAttribute("RangeMaximum", m_hi);
		}
	}

	void Import(tinyxml2::XMLElement* parameter_xml, ExportFormatType /*targetType*/)
	{
		m_lo = 0;
		m_hi = 10;
		parameter_xml->QueryAttribute("RangeMinimum", &m_lo);
		parameter_xml->QueryAttribute("RangeMaximum", &m_hi);
	}

private:
	T m_hi;
	T m_lo;
};

typedef MetaData_ranged<float> MetaData_float;

struct MetaData_int : public MetaData_ranged<int32_t>
{
	bool ValueFromNormalised(float p_normalised, int32_t& returnValue, bool applyDawAjustment)
	{
		const auto normalised = adjustNormalisedForDaw(p_normalised, applyDawAjustment);

		returnValue = getRangeLo() + static_cast<int32_t>(std::round(normalised * static_cast<float>(getRangeHi() - getRangeLo())));

		return true;
	}
};
