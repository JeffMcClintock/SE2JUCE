#pragma once

#include <string>
#include "./modules/se_sdk2/se_datatypes.h"

class CDocOb;
class Module_Info;
class ug_base;

struct parameter_description
{
	int id = 0;
	int flags = IO_PARAMETER_PERSISTANT;
	int automation = -1;
	std::wstring name;
	std::wstring defaultValue;
	std::wstring metaData; // file extension or enum list.
	EPlugDataType datatype = DT_ENUM;
};

struct patchpoint_description
{
	int dspPin;
	int x;
	int y;
	int radius;
};

struct pin_description
{
	const wchar_t* name;

	EDirection direction;

	EPlugDataType datatype;

	const wchar_t* default_value;

	const wchar_t* meta_data;

	int flags;

	const wchar_t* notes;

};


struct pin_description2
{
	int flags = {};
	EDirection direction = DR_IN;
	EPlugDataType datatype = DT_FSAMPLE;
	std::wstring name;
	std::wstring notes;
	std::wstring meta_data;
	std::wstring default_value;
	std::wstring hostConnect;
	std::string classname; // custom datatype (for Cadmium)

	void clear()
	{
		flags = 0;
		name.clear();
		notes.clear();
		meta_data.clear();
		default_value.clear();
		hostConnect.clear();
		classname.clear();
	}
};

struct module_description
{
	const char* unique_id;

	const wchar_t* name;

	int flags;

	const wchar_t* notes;
};

typedef	bool (*get_pin_properties_func)(int, pin_description&);
typedef	void (*get_module_properties_func)(module_description&);

struct module_description_internal
{
	int structure_version;

	const char* unique_id;

	int name_string_resource_id;

	int category_string_resource_id;

	CDocOb* (*cug_create)(Module_Info*);

	ug_base* (*ug_create)();

	int flags;

	pin_description* pin_descriptions;

	int pin_descriptions_count;
};

struct module_description_dsp
{
	const char* unique_id;

	int name_string_resource_id;

	int category_string_resource_id;

	ug_base* (*ug_create)();

	int flags;

	pin_description* pin_descriptions;

	int pin_descriptions_count;

};

