#pragma once

#include <vector>
#include <typeinfo>
#include <string>
#include "HostControls.h"
#if defined( SE_EDIT_SUPPORT )
#include "jsoncpp/json/json.h"
#include "../SynthEdit/PlugDescriptionDecorator.h"
#else
#include "./mfc_emulation.h"
#include "./modules/se_sdk2/se_datatypes.h"
class IPlugDescriptionDecorator
{
};
class IPlug
{
};
#endif

// handy type for refering to arrays of interfaceobjects
typedef std::vector<class InterfaceObject*> InterfaceObjectArray;

// same as above but deletes it's pointers on destruction
class SafeInterfaceObjectArray : public InterfaceObjectArray
{
public:
	~SafeInterfaceObjectArray();
};

// Describe plug or parameter of a unit_gen
class InterfaceObject : public IPlugDescriptionDecorator, public CObject
{
public:
	InterfaceObject( void* addr, const wchar_t* p_name, EDirection p_direction, EPlugDataType p_datatype, const wchar_t* def_val, const wchar_t* = L"-1", int flags = 0, const wchar_t* p_comment = L"", float** p_sample_ptr = nullptr );
	InterfaceObject( int p_id, struct pin_description& p_plugs_info ); // new
	InterfaceObject( int p_id, struct pin_description2& p_plugs_info ); // newer

#if defined( SE_EDIT_SUPPORT )
	// OVERIDES
	// query behaviour
	virtual bool can_set_value(IPlug* self);
	virtual bool can_rename(IPlug* self)
	{
		return self->isRenamable() && !self->isUnusedSpare();
	}
	virtual bool canAcceptConnection(IPlug* self);
	virtual bool canRemove(IPlug* self)
	{
		// If this is a dynamic plug, it may need to be deleted
		return self->autoDuplicate() && self->GetNumConnections() == 0;
	}
	virtual bool has_patches(IPlug* /*self*/)
	{
		return false;
	}
	virtual bool SetsOwnValue(IPlug* self);
	virtual bool connectedControlsIgnorePatchChange(IPlug* /*self*/)
	{
		return ( GetFlags() & IO_IGNORE_PATCH_CHANGE ) != 0;
	}
	virtual bool GetVisible(IPlug* self);
	virtual bool is_filename(IPlug* /*self*/)
	{
		return (GetFlags() & IO_FILENAME) != 0;
	}
	virtual bool isUnusedSpare(IPlug* self)
	{
		return self->autoDuplicate() && self->GetNumConnections() == 0;
	}
	virtual bool isRenamable(IPlug* /*self*/)
	{
		return (GetFlags() & IO_RENAME) != 0;
	}
	virtual bool isBinaryDataPlug(IPlug* /*self*/)
	{
		return ( GetFlags() & IO_BINARY_DATA ) != 0;
	}

	bool isPrivateParameter()
	{
		return ( GetFlags() & IO_PAR_PRIVATE ) != 0;
	}
	//int automation()				{return automation_;};
	//const wchar_t* automationSysex()	{return automationSysex_.c_str();};

	virtual bool isOldStyleGuiPlug(IPlug* /*self*/)
	{
		return 0 != (GetFlags() & IO_OLD_STYLE_GUI_PLUG);
	}
	virtual bool isMinimised(IPlug* /*self*/)
	{
		return (GetFlags() & IO_MINIMISED) != 0;
	}
	virtual bool autoConfigureParameter(IPlug* /*self*/)
	{
		return (GetFlags() & IO_AUTOCONFIGURE_PARAMETER) != 0;
	}
	virtual bool isIoPlug(IPlug* /*self*/)
	{
		return false;
	}
	virtual bool isUnconnectedIOPlug(IPlug* /*self*/)
	{
		return false;
	}
	virtual bool isSettableOutput(IPlug* /*self*/)
	{
		return ( GetFlags() & IO_SETABLE_OUTPUT ) != 0;
	}
	virtual bool RedrawOnChange(IPlug* /*self*/)
	{
		return ( GetFlags() & IO_REDRAW_ON_CHANGE ) != 0;
	}
	virtual bool CheckEnumOnConnection(IPlug* /*self*/)
	{
		return (GetFlags() & IO_DONT_CHECK_ENUM) == 0;
	}
	virtual bool UsesAutoEnumList(IPlug* self);

	// get properties
	virtual CUG* UG(IPlug* /*self*/)
	{
		assert(false);
		return 0;
	}
	virtual IPlug* ConnectedTo(IPlug* /*self*/)
	{
		assert(false);
		return {};
	}
	// SDK2 GUI plugs override to return actual live value, all else returns default, possibly from plug decorator default (not always this).
	virtual std::wstring getValue(IPlug* self)
	{
		return self->GetDefault();
	}
	virtual std::wstring getName(IPlug* /*self*/)
	{
		return Name;
	}
	virtual EPlugDataType getDatatype(IPlug* /*self*/)
	{
		return datatype;
	}
	virtual std::wstring getFileExt(IPlug* self);
#if defined( SE_EDIT_SUPPORT )
	void Export(class Json::Value& object_json, enum ExportFormatType targetType);
	virtual std::wstring GetComments(IPlug* /*self*/) override
	{
		return comment;
	}

	virtual class TiXmlElement* ExportXml(IPlug* self);

	virtual void Export(IPlug* /*self*/, Json::Value& /*object_json*/, int /*targetType*/) override
	{
	}
	void Import(IPlug* /*self*/, tinyxml2::XMLElement* /*xml*/, ExportFormatType /*targetType*/) override
	{
	}
	void Export(IPlug* /*self*/, tinyxml2::XMLElement* /*xml*/, ExportFormatType /*targetType*/) override
	{
	}

	std::wstring GetComments()
	{
		return comment;
	}
#endif
	virtual std::wstring getDefaultEnumList(IPlug* /*self*/)
	{
		return subtype;
	}
	virtual EDirection GetDirection(IPlug* /*self*/)
	{
		return Direction;
	}
	virtual sRange GetDefaultRange(IPlug* self);
	// not logically part of this
	virtual std::wstring getUnitsLabel(IPlug* self);
	virtual connectors_t& Connectors(IPlug* /*self*/)
	{
		assert(false);
		static connectors_t temp;
		return temp;
	}

	// set properties
	virtual void SetDefault(IPlug* self, const std::wstring& val);
	virtual void SetDefaultQuiet(IPlug* self, const std::wstring& val);
	virtual void setName(IPlug* self, const std::wstring& p_name);
	virtual void setValue(IPlug* /*self*/, const std::wstring& /*p_name*/)
	{
		assert(false);
	}
	virtual void SetUG(IPlug* /*self*/, CUG* /*p_ug*/)
	{
		assert(false);
	}

	// notification
	virtual void OnNewConnection(IPlug* /*self*/, CLine2* /*p_line*/) {}
	virtual void OnRemove(IPlug* /*self*/) {}
	virtual void OnUiDisconnect(IPlug* /*self*/) {}
	virtual void DeleteDecorators(IPlug* /*self*/) {}

	// actions
	virtual void Initialise(IPlug* /*self*/, bool /*loaded_from_file*/ = false ) {} // called on first create and serialise
	virtual void Disconnect_pt1(IPlug* /*self*/, CLine2* /*p_line*/) {}
	virtual void Disconnect_pt2(IPlug* /*self*/, CLine2* /*p_line*/) {}
	virtual void RemoveLines(IPlug* /*self*/)
	{
		assert(false);
	}
	virtual void PropogateBack(IPlug* /*self*/, IPlug* /*plug*/, int /*msg_id*/)
	{
		assert(false);
	}
	virtual void setPlugDescID( IPlug* self, int id );		// autoduplicate plugs only (SDK3).

#if defined( SE_EDIT_SUPPORT )
	virtual void AddConnectorQuiet(IPlug* /*self*/, CLine2* /*p_line*/)
	{
		assert(false);
	}
	virtual CObject* MfcSerialisationObject()
	{
		return this;
	}
#endif

	virtual int getDecoratorSortOrder(IPlug* /*self*/)
	{
		return 100;
	} // always last
	virtual IPlugDescriptionDecorator* getPlugDescription()
	{
		return 0;
	}
	virtual void setPlugDescription(IPlugDescriptionDecorator* /*p_description*/)
	{
		assert(false);
	}
#endif

	virtual bool isUiPlug(IPlug* /* self */)
	{
		return ( GetFlags() & IO_UI_COMMUNICATION ) != 0;
	} // plug used by GUI
	virtual bool isDualUiPlug(IPlug* /* self */)
	{
		return ( GetFlags() & IO_UI_COMMUNICATION_DUAL ) == IO_UI_COMMUNICATION_DUAL;
	}
	virtual bool isParameterPlug(IPlug* /* self */)
	{
		return ( GetFlags() & (IO_PATCH_STORE|IO_UI_DUAL_FLAG) ) != 0;
	}
	virtual int getParameterId(IPlug* /* self */)
	{
		return parameterId_;
	}
	virtual int getParameterFieldId(IPlug* /* self */)
	{
		return parameterFieldId_;
	}
	virtual HostControls getHostConnect(IPlug* /* self */)
	{
		return hostConnect_;
	}
	virtual bool DisableIfNotConnected(IPlug* /* self */)
	{
		return (GetFlags() & IO_DISABLE_IF_POS) != 0;
	}
	const std::string& GetClassName(IPlug* /* self */)
	{
		return this->classname;
	}
	virtual bool autoDuplicate(IPlug* /* self */)
	{
		return (GetFlags() & IO_AUTODUPLICATE) != 0;
	}
	virtual int getPlugDescID(IPlug* /* self */)
	{
		return m_id;
	}	// older modules return -1, in which case use plugin index instead.
	virtual bool isHostControlledPlug(IPlug* /* self */)
	{
		return ( GetFlags() & IO_HOST_CONTROL ) != 0;
	}
	virtual bool isCustomisable(IPlug* /* self */)
	{
		return (GetFlags() & IO_CUSTOMISABLE) != 0;
	}
	virtual std::wstring GetDefault(IPlug* /* self */)
	{
		return DefaultVal;
	}
	virtual bool isPolyphonic(IPlug* /* self */)
	{
		return ( GetFlags() & IO_PAR_POLYPHONIC ) != 0;
	}
	virtual bool isPolyphonicGate(IPlug* /* self */)
	{
		return ( GetFlags() & IO_PAR_POLYPHONIC_GATE ) != 0;
	}

	EDirection CheckDirection(const std::type_info* dtype);
	void* GetVarAddr();
	bool GetPPActiveFlag();
	const std::wstring& GetEnumList();
	EPlugDataType GetDatatype()
	{
		return datatype;
	}
	const std::wstring& GetDefaultVal();
	int GetFlags()
	{
		return Flags;
	}
	bool isContainerIoPlug()
	{
		return 0 != (GetFlags() & IO_CONTAINER_PLUG);
	}
	const std::wstring& GetName();
	EDirection GetDirection();
	void SetFlags(int p_flags)
	{
		Flags = p_flags;
	}

	void setId(int p_id)
	{
		m_id = p_id;
	}
	void setParameterId(int p_id)
	{
		parameterId_ = p_id;
	}
	void setParameterFieldId(int p_id)
	{
		parameterFieldId_ = p_id;
	}
	void clearVariableAddress()
	{
		address=0;
	}
	// don't really make sense here. this should only hold info common to all instances. remove.
	float**		sample_ptr; // allows plug to set a pointer to it's samples automatically

protected:
	InterfaceObject() : m_id(-1) {}
	DECLARE_SERIAL( InterfaceObject )
#if defined( SE_EDIT_SUPPORT )
	virtual void Serialize( CArchive& ar );
#endif
private:
	EDirection	Direction;
	std::wstring		Name;
	std::wstring		DefaultVal;
	std::wstring		subtype;		// info for enum and range type
	int			Flags;

	EPlugDataType datatype;
	std::string classname;

	void* address;
	int m_id; // sequential number
	int parameterId_;
	int parameterFieldId_;
	HostControls hostConnect_;

#if defined( SE_EDIT_SUPPORT )
	std::wstring		comment;
#endif
};
