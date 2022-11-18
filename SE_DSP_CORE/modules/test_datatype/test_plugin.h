
#if !defined( GMPI_PROTOTYPE_H_INCLUDED )
#define GMPI_PROTOTYPE_H_INCLUDED

#include "plugin.h"

class Prototype_Plugin : public GMPI_Plugin
{
public:
	// create function
	static GMPI_RESULT CreateInstance(GMPI_Plugin **plugin);

	// test function
	virtual GMPI_RESULT GMPI_STDCALL Placeholder1( int32_t );
};

#endif



