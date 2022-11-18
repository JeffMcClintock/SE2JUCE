// test_host.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "gmpi.h"

#define ERROR_CHECK(r, msg) { if(GMPI_SUCCESS != r ){ printf(msg); return 0;} }

//int _tmain(int argc, _TCHAR* argv[])
int main(int argc, char* argv[])
{
    // assuming Plugin ID sourced elsewhere (from metadata file)
    // format of plugin unique ID undecided. for now using GUID
    static const GMPI_GUID IID_GMPI_Prototype_DSP = 	// {5CC85CAD-DC0C-42c2-80FD-DC3ADBC70641}
    { 0x5cc85cad, 0xdc0c, 0x42c2, { 0x80, 0xfd, 0xdc, 0x3a, 0xdb, 0xc7, 0x6, 0x41 } };

    const GMPI_GUID &plugin_id  = IID_GMPI_Prototype_DSP;
    const char *plugin_filename = "../test_plugin/debug/test_plugin.dll";
	const char *dll_entrypoint_name = "GMPI_GetFactory"; //DllGetClassObject"; // TODO: better name? (or stick with COM standard name?)

    GMPI_DllEntry dll_entry_point;
    GMPI_DllHandle dll_handle;
    IGMPI_Factory *factory;
    GMPI_IUnknown *com_object;
    IGMPI_Plugin *my_plugin;
    GMPI_RESULT r;

    // STEP 1: Load DLL
    r = GMPI_DllLoad( &dll_handle, plugin_filename );

    ERROR_CHECK(r,"dll load fail\n");


    // STEP 2: Locate entrypoint
    r = GMPI_DllSymbol( &dll_handle, dll_entrypoint_name, (void**) &dll_entry_point );

    ERROR_CHECK(r,"fail to locate entry point\n");


    // STEP 3: Create factory
 //   r = dll_entry_point( &IID_GMPI_Factory, (GMPI_GUID *) &IID_GMPI_Factory, (void**) &factory );
 //   r = dll_entry_point( &IID_GMPI_Factory, (void**) &factory );
    r = dll_entry_point( (void**) &com_object );

    ERROR_CHECK(r,"fail to create factory\n");

    // STEP 5: Ask plugin to provide GIMPI Factory V1 interface
    r = com_object->QueryInterface( IID_GMPI_Factory, (void**) &factory );

    ERROR_CHECK(r,"plugin does not support GMPI V 1.0 Factory Interface\n");

	// release temporary object
    r = com_object->Release();

    // STEP 4: Ask factory to instantiate plugin
    r = factory->CreateInstance( plugin_id, (void**) &com_object );

    ERROR_CHECK(r,"fail to create plugin\n");


    // STEP 5: Ask plugin to provide GIMPI Plugin V1 interface
    r = com_object->QueryInterface( IID_GMPI_Plugin, (void**) &my_plugin );

    ERROR_CHECK(r,"plugin does not support GMPI V 1.0 plugin Interface\n");


    // test a call into plugin
    r = my_plugin->Placeholder1( 42 );


    // STEP 6: release factory and plugin
    r = com_object->Release();
    r = my_plugin->Release();
    r = factory->Release();

	// STEP 7: Unload DLL
    r = GMPI_DllUnload( &dll_handle );
}

