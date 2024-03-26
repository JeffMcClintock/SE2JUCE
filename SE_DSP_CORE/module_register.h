#pragma once
#if defined( SE_EDIT_SUPPORT )
#include ".\plug_description.h"
#endif

#include "resource.h"
//#include "UgDatabase.h"
#include "InterfaceObject.h"

// Flags for special CUG types CUG::getType()->GetFlags()
#define CF_IO_MOD       4

// This UG controls polyphony (!!could use unit_gen flag UGF_POLYPHONIC_GENERATOR?)
#define CF_NOTESOURCE   32

// ANY CONTAINER WITH A NOTESOURCE/Automator etc can't be expanded 'inline'
// because each container assumes only one.
#define CF_DONT_EXPAND_CONTAINER    64

// visible on control panel
#define CF_PANEL_VIEW               0x0080

// visible on structure view
#define CF_STRUCTURE_VIEW           0x0100
#define CF_DUMMY_VIEW				0x0200
#define CF_AUTOMATION_VIEW          0x0800
#define CF_OLD_STYLE_LISTINTERFACE  0x1000

#define CF_DEBUG_VIEW               0x8000

#define CF_SHELL_PLUGIN             0x10000
#define CF_IS_FEEDBACK              0x20000

// ONLY PIGYBACKED DURING SERIALISE TO MAINTAIN FILE-FORMAT.
#define CF_RACK_MODULE_FLAG         0x40000

#define CF_ALWAYS_EXPORT			(1 << 25)	// e.g. Voice-Mute.


typedef void (*ListInterface_ptr)(InterfaceObjectArray&);

// Ensure linker includes file in static-library. See also INIT_STATIC_FILE in UgDatabase.cpp
#ifdef SE_DECLARE_INIT_STATIC_FILE
#undef SE_DECLARE_INIT_STATIC_FILE
#endif

#if defined(_DEBUG) && defined(SE_TARGET_PLUGIN)
// has extra debugging check
#include "UgDatabase.h"
#undef SE_DECLARE_INIT_STATIC_FILE
#define SE_DECLARE_INIT_STATIC_FILE(filename) void se_static_library_init_##filename(){} \
bool teststaticinit_##filename = ModuleFactory()->debugInitCheck( #filename );
#else
#define SE_DECLARE_INIT_STATIC_FILE(filename) void se_static_library_init_##filename(){}
#endif

bool ModuleFactory_RegisterModule(const wchar_t* p_unique_id, int p_sid_name, int p_sid_group_name, class CDocOb* (*cug_create)(class Module_Info*), class ug_base* (*ug_create)(), int p_flags);
bool ModuleFactory_RegisterModule(const wchar_t* p_unique_id, const wchar_t* name, const wchar_t* group_name, class CDocOb* (*cug_create)(Module_Info*), class ug_base* (*ug_create)(), int p_flags);

// structure modules, using default CUG as GUI class
// macro for generating unique variable or function name.
#define PASTE_FUNCT2(x,y) x##y
#define PASTE_FUNCT1(x,y) PASTE_FUNCT2(x,y)
#define REGISTER_MODULE_1( module_id, sid_name, sid_group, ug, flags, desc ) bool PASTE_FUNCT1(res,__LINE__) = ModuleFactory_RegisterModule(module_id, sid_name, sid_group, 0, ug::CreateObject, flags);

class ug_base;

namespace internalSdk
{
	bool RegisterPlugin(ug_base *(*ug_create)(), const char* xml);

	template< class moduleClass >
	struct Register
	{
		static bool withXml(const char* xml)
		{
			return internalSdk::RegisterPlugin(
				[]() -> ug_base* { return new moduleClass(); },
				xml
			);
		}
	};
}
