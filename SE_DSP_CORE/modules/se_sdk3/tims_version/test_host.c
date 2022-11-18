// test_host.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "gmpi.h"

#define ERROR_CHECK(r, msg) { if(r != GMPI_SUCCESS){ printf(msg); return 0;} }

/* handy macros to make calling COM from C less painful */
#define CALL_COM_0( object, function )                  object->Vtable->function(object)
#define CALL_COM_1( object, function, arg1 )            object->Vtable->function(object, arg1)
#define CALL_COM_2( object, function, arg1, arg2 )      object->Vtable->function(object, arg1, arg2)
#define CALL_COM_3( object, function, arg1, arg2, arg3 )object->Vtable->function(object, arg1, arg2, arg3)


//int _tmain(int argc, _TCHAR* argv[])
int main(int argc, char* argv[])
{
	/* signature of Factory construction function */
	GMPI_DllEntry dll_entry_point;
	GMPI_DllHandle dll_handle;
	IGMPI_Factory *factory;
	GMPI_IUnknown *com_object;
	IGMPI_Plugin *my_plugin;
	GMPI_RESULT r;

	/* assuming Plugin ID sourced elsewhere (from metadata file) */
	/* format of plugin unique ID undecided. for now using GUID */
	/* {5CC85CAD-DC0C-42c2-80FD-DC3ADBC70641} */
	static const GUID IID_GMPI_Prototype_DSP = 
	{ 0x5cc85cad, 0xdc0c, 0x42c2, { 0x80, 0xfd, 0xdc, 0x3a, 0xdb, 0xc7, 0x6, 0x41 } };


    /* STEP 1: Load DLL */
	r = GMPI_DllLoad( &dll_handle, "../test_plugin/debug/test_plugin.dll" );

	ERROR_CHECK(r,"dll load fail\n");


    /* STEP 2: Locate entrypoint */
	r = GMPI_DllSymbol( &dll_handle, "DllGetClassObject", &dll_entry_point );

	ERROR_CHECK(r,"fail to locate entry point\n");


    /* STEP 3: Create factory */
	r = dll_entry_point( &IID_GMPI_Factory, &IID_GMPI_Factory, &factory );

	ERROR_CHECK(r,"fail to create factory\n");

    /* STEP 4: Ask factory to instantiate plugin */
//	r = factory->Vtable->CreateInstance(factory,  &IID_GMPI_Prototype_DSP, &com_object);
	r = CALL_COM_2( factory, CreateInstance, &IID_GMPI_Prototype_DSP, &com_object );

	ERROR_CHECK(r,"fail to create plugin\n");


    /* STEP 5: Ask plugin to provide GIMPI Plugin V1 interface */
//	r = com_object->Vtable->QueryInterface(com_object,  &IID_GMPI_Plugin, &my_plugin);
	r = CALL_COM_2( com_object, QueryInterface,  &IID_GMPI_Plugin, &my_plugin );

	ERROR_CHECK(r,"plugin does not support GMPI V 1.0 Interface\n");


	/* test a call into plugin */
//	r = my_plugin->Vtable->Placeholder1(my_plugin, 42);
	r = CALL_COM_1( my_plugin, Placeholder1, 42 );


    /* STEP 6: delete factory and plugin */
//	r = com_object->Vtable->Release(com_object);
//	r = my_plugin->Vtable->Release(my_plugin);
//	r = factory->Vtable->Release(factory);
	r = CALL_COM_0( com_object, Release );
	r = CALL_COM_0( my_plugin,  Release );
	r = CALL_COM_0( factory,    Release );
	
	return 0;
}

