#pragma once
#if defined( SE_EDIT_SUPPORT )
#include ".\plug_description.h"
#endif

#include "resource.h"
#include "UgDatabase.h"
#include "InterfaceObject.h"

typedef void (*ListInterface_ptr)(InterfaceObjectArray&);

#define REGISTER_MODULE_INTERNAL(mod_class) namespace{bool res = ModuleFactory()->RegisterModule( &mod_class::GetModuleProperties, &mod_class::GetPinProperties, &mod_class::Build );}

// Ensure linker includes file in static-library. See also INIT_STATIC_FILE in UgDatabase.cpp
#ifdef SE_DECLARE_INIT_STATIC_FILE
#undef SE_DECLARE_INIT_STATIC_FILE
#endif

#if defined(_DEBUG) && defined(SE_TARGET_PLUGIN)
// has extra debugging check
#define SE_DECLARE_INIT_STATIC_FILE(filename) void se_static_library_init_##filename(){} \
bool teststaticinit_##filename = ModuleFactory()->debugInitCheck( #filename );
#else
#define SE_DECLARE_INIT_STATIC_FILE(filename) void se_static_library_init_##filename(){}
#endif

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
