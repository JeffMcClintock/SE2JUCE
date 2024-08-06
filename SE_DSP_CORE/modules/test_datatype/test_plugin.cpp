// test_plugin.cpp : Defines the entry point for the DLL application.
//

#include "test_plugin.h"
#include "factory.h"

// Plugin unique ID
// IID vs URI undecided. For now using GUID

// {5CC85CAD-DC0C-42c2-80FD-DC3ADBC70641}
static const GMPI_GUID IID_GMPI_Prototype_DSP = 
{ 0x5cc85cad, 0xdc0c, 0x42c2, { 0x80, 0xfd, 0xdc, 0x3a, 0xdb, 0xc7, 0x6, 0x41 } };

// register plugin ID and creation function with factory
namespace
{
    GMPI_RESULT r = Factory()->RegisterPlugin( IID_GMPI_Prototype_DSP, &Prototype_Plugin::CreateInstance );
}

// instantiate a new plugin
GMPI_RESULT Prototype_Plugin::CreateInstance(GMPI_Plugin **plugin)
{
    *plugin = new Prototype_Plugin();

    if( *plugin )
        return GMPI_SUCCESS;
    else
        return GMPI_FAIL;
}

// test function
GMPI_RESULT Prototype_Plugin::Placeholder1( int32_t )
{
	return GMPI_SUCCESS;
}




//TODO: move to own file in windows folder
/* Windows Specific.  Provide DLLMain function. */
/* Usually ignored. Provided here in case you need to customise it */
#if defined( GMPI_PLATFORM_WINDOWS )
#include "windows.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
#endif