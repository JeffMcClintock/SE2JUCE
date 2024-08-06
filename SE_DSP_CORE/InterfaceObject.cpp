#include "pch.h"
#include <assert.h>
#include <typeinfo>
#include "UPlug.h"
#include "conversion.h"
#include "./plug_description.h"
#include "./IPluginGui.h"
#include "./HostControls.h"
#include "./USampBlock.h"
#include "./UMidiBuffer2.h"

#if defined( SE_EDIT_SUPPORT )
#include "./Plug_decorator_autoduplicate.h"
#include "./plug_decorator_default.h"
#include "./Plug_decorator_namable.h"
#include "./ccontainer.h"
#include "tinyxml/tinyxml.h"
#endif


using namespace std;

IMPLEMENT_SERIAL( InterfaceObject, CObject, 1 )

SafeInterfaceObjectArray::~SafeInterfaceObjectArray()
{
	// delete contained objects
	for( auto it = begin(); it != end() ; ++it )
	{
		delete *it;
	}
}

InterfaceObject::InterfaceObject( void* addr, const wchar_t* p_name, EDirection p_direction, EPlugDataType p_datatype, const wchar_t* def_val, const wchar_t* defid, int flags, const wchar_t* p_comment, float** p_sample_ptr ) :
	Direction( p_direction ),
	Name( p_name ),
	Flags(flags),
	DefaultVal( def_val ),
	subtype(defid)
	,sample_ptr(p_sample_ptr)
	,address(addr)
	,datatype(p_datatype)
	,m_id(-1)
	,parameterFieldId_(FT_VALUE)
	,parameterId_(-1)
	,hostConnect_(HC_NONE)
#if defined( SE_EDIT_SUPPORT )
	,comment(p_comment)
#endif
{
	assert( (intptr_t) sample_ptr != 0x01010101 );

	if( GetDirection() == DR_IN && GetDatatype() == DT_FSAMPLE && (GetFlags() & (IO_LINEAR_INPUT|IO_POLYPHONIC_ACTIVE)) == 0 )
	{
		//assert(false); // don't forget to set you plugs to
		//		_RPT1(_CRT_WARN, "]Auto set poly active: %s. [FORGOTTEN TO SET FLAG??]\n",Name );
		SetFlags(GetFlags() | IO_POLYPHONIC_ACTIVE );
	}

	//	#ifdef _DEBUG
	//		assert( p_datatype == GetDatatype() );
	//		if(p_direction == DR _PARAMETER && CheckDirection(&p_typeinfo) != DR_IN)
	//			assert(false); // parameter should be a pointer (they are inputs)
	//		checkpointer(&p_typeinfo); // not checking allows use after connected
	//	#endif
	/*
	if( !p_comment.empty() )
	{
		if( wcscmp(L" {FT_RANGE_HI}", p_comment) == 0 )
		{
			parameterFieldId_ = FT_RANGE_HI;
		}
		if( wcscmp(L" {FT_RANGE_LO}", p_comment) == 0 )
		{
			parameterFieldId_ = FT_RANGE_LO;
		}
	}*/
	// Hack to support host-connect on SDK2
	if( (Flags & IO_HOST_CONTROL) != 0 )
	{
		hostConnect_ = StringToHostControl(Name);
	}
	if ((Flags & (IO_PATCH_STORE | IO_UI_DUAL_FLAG)) != 0)
	{
		parameterId_ = 0; // default for older modules.
	}
}; // does the datatype enum not agree with the variable?

// SDK3 version
InterfaceObject::InterfaceObject( int p_id, pin_description2& p_plugs_info ) :
	Direction( p_plugs_info.direction )
	,Name( p_plugs_info.name.c_str() )
	,Flags(p_plugs_info.flags)
	,DefaultVal( p_plugs_info.default_value.c_str() )
	,subtype( p_plugs_info.meta_data.c_str() )
	,datatype(p_plugs_info.datatype)
	,classname(p_plugs_info.classname)
	,sample_ptr(0)
	,address(0)
	,m_id(p_id)
	,parameterFieldId_(FT_VALUE)
	,parameterId_(-1)
	,hostConnect_( StringToHostControl(p_plugs_info.hostConnect) )
{
	if( GetDirection() == DR_IN && GetDatatype() == DT_FSAMPLE && (GetFlags() & (IO_LINEAR_INPUT|IO_POLYPHONIC_ACTIVE)) == 0 )
	{
		//assert(false); // don't forget to set you plugs to
		//		_RPT1(_CRT_WARN, "]Auto set poly active: %s. [FORGOTTEN TO SET FLAG??]\n",Name );
		SetFlags(GetFlags() | IO_POLYPHONIC_ACTIVE );
	}

	//	if( !p_plugs_info.notes.empty() ) //&& p_plugs_info.notes.at(0) == L'{' )
	{
		// hacky way for old internal modules to access parameterFieldId_.
		assert( wcscmp(L"{FT_SHORT_NAME}", p_plugs_info.notes.c_str()) != 0 );
		assert( wcscmp(L"{FT_ENUM_LIST}", p_plugs_info.notes.c_str()) != 0 );
		/* never used???
			if( wcscmp(L"{FT_SHORT_NAME}", p_plugs_info.notes.c_str()) == 0 )
			{
				parameterFieldId_ = FT_SHORT_NAME;
			}
			if( wcscmp(L"{FT_ENUM_LIST}", p_plugs_info.notes.c_str()) == 0 )
			{
				parameterFieldId_ = FT_ENUM_LIST;
			}
		*/
	}
}

InterfaceObject::InterfaceObject( int p_id, pin_description& p_plugs_info ) :
	Direction( p_plugs_info.direction )
	,Name( p_plugs_info.name )
	,Flags(p_plugs_info.flags)
	,DefaultVal( p_plugs_info.default_value )
	,subtype( p_plugs_info.meta_data )
	,datatype(p_plugs_info.datatype)
	,sample_ptr(0)
	,address(0)
	,m_id(p_id)
	,parameterFieldId_(FT_VALUE)
	,parameterId_(-1)
	,hostConnect_(HC_NONE)
{
	if( GetDirection() == DR_IN && GetDatatype() == DT_FSAMPLE && (GetFlags() & (IO_LINEAR_INPUT|IO_POLYPHONIC_ACTIVE)) == 0 )
	{
		//assert(false); // don't forget to set you plugs to
		//		_RPT1(_CRT_WARN, "]Auto set poly active: %s. [FORGOTTEN TO SET FLAG??]\n",Name );
		SetFlags(GetFlags() | IO_POLYPHONIC_ACTIVE );
	}

	if( p_plugs_info.notes && *(p_plugs_info.notes) == '{' )
	{
		// hacky way for old internal modules to access parameterFieldId_.
		if( wcscmp(L"{FT_SHORT_NAME}", p_plugs_info.notes) == 0 )
		{
			parameterFieldId_ = FT_SHORT_NAME;
		}

		if( wcscmp(L"{FT_ENUM_LIST}", p_plugs_info.notes) == 0 )
		{
			parameterFieldId_ = FT_ENUM_LIST;
		}
	}
}

#if defined( SE_EDIT_SUPPORT )
bool InterfaceObject::GetVisible(IPlug* self)
{
	if( self->GetNumConnections() > 0 )
		return true;

	bool obsolete = self->DisableIfNotConnected() || (GetFlags() & (IO_PARAMETER_SCREEN_ONLY) ) != 0;
	return !self->isMinimised() && !obsolete;
}

// ref also : Plug_decorator_default::ExportXml
class TiXmlElement* InterfaceObject::ExportXml(IPlug* self)
{
	// Settable outputs MUST set 'Default' attribute, else SE fails to create a buffer for the pin in ug_base::Setup
	if (isSettableOutput(0) && self->GetDefault() == DefaultVal)
	{
		auto plugElement = new TiXmlElement("Plug");
		plugElement->SetAttribute("Default", WStringToUtf8(DefaultVal));
		return plugElement;
	}

	// No need to inform DSP of default values (it knows them already).
	return 0;
}

#endif

EDirection InterfaceObject::GetDirection()
{
	return Direction;
}

const std::wstring& InterfaceObject::GetName()
{
	return Name;
}

const std::wstring& InterfaceObject::GetDefaultVal()
{
	return DefaultVal;
}
#if defined( SE_EDIT_SUPPORT )

sRange InterfaceObject::GetDefaultRange(IPlug* self)
{
	return sRange(subtype);
}

std::wstring InterfaceObject::getUnitsLabel(IPlug* self)
{
	if( self->getDatatype() == DT_FSAMPLE )
	{
		return L"Volts";
	}

	return L"";
}

std::wstring InterfaceObject::getFileExt(IPlug* self)
{
	if( self->is_filename() )	// default enum list doubles as file extension for data type 'file'
	{
		return self->getDefaultEnumList();
	}

	return {};
}

bool InterfaceObject::UsesAutoEnumList(IPlug* self)
{
	if( self->getDatatype() == DT_ENUM )
	{
		if( self->isUiPlug() )
		{
			return (GetFlags() & IO_AUTO_ENUM ) != 0;
		}
		else
		{
			return getDefaultEnumList(0) == L"{AUTO}";
		}
	}

	return false;
}

bool InterfaceObject::SetsOwnValue(IPlug* self)
{
	// input plugs with no connections need to set their own value
	// output plugs in 'fixed values' ug also set their own value
	return ( self->GetDirection() == DR_IN && !self->HasActiveConnections() ) || self->isDualUiPlug() || self->isSettableOutput();
}
#endif

const std::wstring& InterfaceObject::GetEnumList()
{
	return subtype;
}

bool InterfaceObject::GetPPActiveFlag()
{
	assert( (Flags & IO_POLYPHONIC_ACTIVE ) == 0 || (Flags & IO_LINEAR_INPUT) == 0 ); // check both flags arn't set (mutually exclusive)
	return (Flags & IO_LINEAR_INPUT) == 0 ; // if not linear input, assume polyphonic active.
}

void* InterfaceObject::GetVarAddr()
{
	return address;
}

// For debug purpose, infer direction from datatypr
// *datatype
EDirection InterfaceObject::CheckDirection(const type_info* dtype)
{
	if( *dtype == typeid(float*))
		return DR_IN;

	if( *dtype == typeid(float))
		return DR_OUT;

	if( *dtype == typeid(double*))
		return DR_IN;

	if( *dtype == typeid(double))
		return DR_OUT;

	//	if( *dtype == typeid(short*))
	//		return DR_IN;
	if( *dtype == typeid(short))
		return DR_OUT;

	if( *dtype == typeid(std::wstring*))
		return DR_IN;

	if( *dtype == typeid(std::wstring))
		return DR_OUT;

	//	if( *dtype == typeid(UMidiBuffer*))
	//		return DR_IN;
	//	if( *dtype == typeid(UMidiBuffer))
	//		return DR_OUT;
	if( *dtype == typeid(bool))
		return DR_OUT;

	if( *dtype == typeid(bool*))
		return DR_IN;

	// Input Array version (used by ADDER)
	//	if( *dtype == typeid(InputArray_Sample))
	//		return DR_IN;
	assert(false); //add datatype here (checkdirection)
	return DR_OUT;
}

#if defined( SE_EDIT_SUPPORT )
void InterfaceObject::SetDefault(IPlug* self, const std::wstring& val)
{
	if( val != GetDefault(self) )
	{
		auto pd = new Plug_decorator_default();
		self->AddDecorator(pd);
		pd->SetDefault(self, val);
	}
};

void InterfaceObject::SetDefaultQuiet(IPlug* self, const std::wstring& val)
{
	if( val != GetDefault(self) )
	{
		IPlugDescriptionDecorator* pd = self->AddDecorator( new Plug_decorator_default() );
		pd->SetDefaultQuiet(self, val);
	}
};

void InterfaceObject::setName(IPlug* self, const std::wstring& p_name)
{
	if( p_name != GetName() )
	{
		IPlugDescriptionDecorator* pd = self->AddDecorator( new Plug_decorator_namable() );
		pd->setName(self, p_name);
	}
};

void InterfaceObject::setPlugDescID( IPlug* self, int id )		// autoduplicate plugs only (SDK3).
{
	assert( self->autoDuplicate() );

	if( id != m_id )
	{
		self->AddDecorator( new Plug_decorator_autoduplicate( id ) );
	}
}

void InterfaceObject::Serialize( CArchive& ar )
{
	CObject::Serialize( ar );

	if( ar.IsStoring() )
	{
		ar << (int) Direction;
		//		ar << comment; // not really needed
		ar << Name;
		ar << DefaultVal;
		ar << subtype;
		ar << Flags;
		ar << (int) datatype;
		//	void *address;
		ar << m_id;
		ar << parameterId_;
		ar << parameterFieldId_;
		ar << hostConnect_;
	}
	else
	{
		int temp;
		ar >> temp;
		Direction = (EDirection) temp;
		ar >> Name;
		ar >> DefaultVal;
		ar >> subtype;
		ar >> Flags;
		ar >> temp;
		datatype = (EPlugDataType) temp;
		//	void *address;
		ar >> m_id;
		ar >> parameterId_;
		ar >> parameterFieldId_;
		{
			int t;
			ar >> t;
			hostConnect_ = (HostControls) t;
		}
		address = 0;
		sample_ptr = 0;
	}
}

void InterfaceObject::Export(class Json::Value& pins_json, ExportFormatType targetType)
{
	//TiXmlElement* pinXml = new TiXmlElement("Pin");
	//DspXml->LinkEndChild(pinXml);
	Json::Value pin_json(Json::objectValue);

	//pinXml->SetAttribute("name", WStringToUtf8(GetName()));
	//pinXml->SetAttribute("datatype", XmlStringFromDatatype(GetDatatype()));
	pin_json["name"] = WStringToUtf8(GetName());
	pin_json["type"] = XmlStringFromDatatype(GetDatatype());

	if( GetDatatype() == DT_FSAMPLE )
	{
		//pinXml->SetAttribute("rate", "audio");
//		pin_json["rate"] = "audio";
		pin_json["type"] = "audio"; // combined "float"/"rate=audio" -> just "audio".
	}

	char *direction = 0; // or "in" (default)
	if( GetDirection() == DR_OUT )
	{
		direction = "out";
	}
	else
	{
		wstring default = GetDefault(0);
		if( !default.empty() )
		{
			bool needsDefault = true;
			// In SDK3 Audio data defaults are literal (not volts).
			if( GetDatatype() == DT_FSAMPLE || GetDatatype() == DT_FLOAT || GetDatatype() == DT_DOUBLE )
			{
				float d = StringToFloat(default); // won't cope with large 64bit doubles.

				if( GetDatatype() == DT_FSAMPLE )
				{
					d *= 0.1f; // Volts to actual value.
				}

				default = FloatToString(d);

				needsDefault = d != 0.0f;
			}
			if( GetDatatype() == DT_ENUM || GetDatatype() == DT_BOOL || GetDatatype() == DT_INT || GetDatatype() == DT_INT64 )
			{
				int d = StringToInt(default); // won't cope with large 64bit ints.
				default = IntToString(d);

				needsDefault = d != 0;
			}

			if( needsDefault )
			{
				//pinXml->SetAttribute("default", WStringToUtf8(default));
				pin_json["default"] = WStringToUtf8(default);
			}
		}
	}

	if( direction )
	{
		//pinXml->SetAttribute("direction", direction);
		pin_json["direction"] = direction;
	}

	if( DisableIfNotConnected(0) )
	{
		//pinXml->SetAttribute("private", "true");
		pin_json["private"] = "true";
	}
	if( isRenamable(0) )
	{
		//pinXml->SetAttribute("autoRename", "true");
		pin_json["autoRename"] = "true";
	}
	if( is_filename(0) )
	{
		//pinXml->SetAttribute("isFilename", "true");
		pin_json["isFilename"] = "true";
	}
	if( ( GetFlags() & IO_LINEAR_INPUT ) != 0 )
	{
		//pinXml->SetAttribute("linearInput", "true");
		pin_json["linearInput"] = "true";
	}
	if( ( GetFlags() & IO_IGNORE_PATCH_CHANGE ) != 0 )
	{
		//pinXml->SetAttribute("ignorePatchChange", "true");
		pin_json["ignorePatchChange"] = "true";
	}
	if( ( GetFlags() & IO_AUTODUPLICATE ) != 0 )
	{
		//pinXml->SetAttribute("autoDuplicate", "true");
		pin_json["autoDuplicate"] = "true";
	}
	if( ( GetFlags() & IO_MINIMISED ) != 0 )
	{
		//pinXml->SetAttribute("isMinimised", "true");
		pin_json["isMinimised"] = "true";
	}
	if( ( GetFlags() & IO_PAR_POLYPHONIC ) != 0 )
	{
		//pinXml->SetAttribute("isPolyphonic", "true");
		pin_json["isPolyphonic"] = "true";
	}
	if( ( GetFlags() & IO_AUTOCONFIGURE_PARAMETER ) != 0 )
	{
		//pinXml->SetAttribute("autoConfigureParameter", "true");
		pin_json["autoConfigureParameter"] = "true";
	}
	if( ( GetFlags() & IO_PARAMETER_SCREEN_ONLY ) != 0 )
	{
		//pinXml->SetAttribute("noAutomation", "true");
//		pin_json["noAutomation"] = "true";
		pin_json["isMinimised"] = "true";
	}

	if( getParameterId(0) != -1 )
	{
//		pinXml->SetDoubleAttribute("parameterId", (double)getParameterId(0));
		pin_json["parameterId"] = getParameterId(0);

		if( getParameterFieldId(0) != FT_VALUE )
		{
			//pinXml->SetAttribute("parameterField", XmlStringFromParameterField(getParameterFieldId(0)));
			pin_json["parameterField"] = XmlStringFromParameterField(getParameterFieldId(0));
		}
	}

	HostControls hostControlId = getHostConnect(0);
	if( hostControlId != HC_NONE )
	{
		pin_json["hostConnect"] = WStringToUtf8(GetHostControlName(hostControlId));
		if( getParameterFieldId(0) != FT_VALUE )
		{
			pin_json["parameterField"] = XmlStringFromParameterField(getParameterFieldId(0));
		}
	}

	if( !GetEnumList().empty() )
	{
//		pinXml->SetAttribute("metadata", WStringToUtf8(GetEnumList()));
		pin_json["metadata"] = WStringToUtf8(GetEnumList());
	}

	pins_json.append(pin_json);
}

#endif


#if defined( SE_EDIT_SUPPORT )
bool InterfaceObject::can_set_value(IPlug* self)
{
	// change: can't set default on DR_OUT GUI plug (PAtchMem Float fix).
	if( self->getDatatype() == DT_MIDI )
		return false;

	// SDK2 GUI modules have had pins reversed (SE 1.1) so rules are different.
	bool actsAsInput;

	//* prevents my 'new' built-in sub-controls working
	if( self->isOldStyleGuiPlug() )
	{
		actsAsInput = self->GetDirection() != DR_IN;
	}
	else
		//*/
	{
		actsAsInput = self->GetDirection() == DR_IN;
	}

	return !self->isUnusedSpare() &&
	       ( actsAsInput || (GetFlags() & IO_SETABLE_OUTPUT) != 0 );
}

bool InterfaceObject::canAcceptConnection(IPlug* self)
{
	if(  self->GetNumConnections() == 0 || self->getDatatype() == DT_FSAMPLE || self->getDatatype() == DT_MIDI2 )
	{
		return true;
	}

	return self->GetDirection() == DR_OUT;
}
#endif
