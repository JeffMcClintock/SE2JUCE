/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

/*
 * GMPI_SdkFactory
 *
 * This file provides a base class which implements the IGMPI_Factory
 * interface.
 *
 * This file creates a singleton GMPI_SdkFactory object.  The plugin code
 * should register it's IID(s) and a GMPI_Descriptor with the factory.
 * When the host wants to instantiate a given IID, the PluginCreateFunc
 * member of the descriptor will be invoked.
 *
 * This file also provides an implementation of GMPI_GetFactory(), which
 * will be called by the host to discover the IGMPI_Factory interface.
 */

#include <stdlib.h>
#include "gmpi.h" /* JM-MOD */
#include "Factory.h" /* JM-MOD */
#include "Plugin.h" /* JM-MOD */
#include "HostIntf.h" /* JM-MOD */
#include "Guid.h" /* JM-MOD */
#include "Descriptor.h" /* JM-MOD */
#include "lib.h" /* JM-MOD */
#include <utility>
#include <map>

typedef std::pair<GMPI_Guid, GMPI_Descriptor>	GMPI_GuidDescriptor;
typedef std::map<GMPI_Guid, GMPI_Descriptor>	GMPI_GuidDescriptorMap;

/*
 * This class is private - it's details don't need to be exposed.  This
 * will only be used as a static object, so refcounting is just for error
 * checking.
 */
class GMPI_SdkFactory: public IGMPI_Factory, public GMPI_Factory
{
public:
	GMPI_SdkFactory(void);
	virtual ~GMPI_SdkFactory(void);

	/* IGMPI_Unknown methods */
	virtual GMPI_Result QueryInterface(const GMPI_Guid& iid,
			IGMPI_Unknown** iobject);
	virtual int32_t AddRef(void);
	virtual int32_t Release(void);

	/* IGMPI_Factory methods */
	virtual GMPI_Result CreateInstance(const wchar_t *unique_id, //const GMPI_Guid& iid,
			GMPI_HostIntf& host, const char** args,
			IGMPI_Unknown** object);
	virtual GMPI_Result GetMetadata(int index,
			GMPI_Metadata** metadata);

	/* a map of all registered IIDs: public for factory access */
	GMPI_GuidDescriptorMap m_pluginMap;

private:
	/* reference counter */
	int32_t m_refcount;
};


GMPI_SdkFactory *Factory()
{
	/*
	 * This is the singleton factory object.  It can't be a function-scope static object,
	* because some compilers screw up on destructors of static objects at
	* sub-file scopes.  It can however be a static *pointer* to an object because pointers don't have destructors.
	*/

	static GMPI_SdkFactory *theFactory = 0;

	/*
	   Initialise on first use.  Ensures the factory is alive before any other object
	   including other global static objects (allows plugins to auto-register).
	*/
	if( theFactory == 0 )
	{
		theFactory = new GMPI_SdkFactory;
	}

	return theFactory;
}

/*
   Factory must live untill plugin dll is unloaded
   this helper manages it's destruction automatically
*/
class factory_deleter_helper
{
public:
	~factory_deleter_helper()
	{
		delete Factory();
	}
} grim_reaper;



/*
 * This is the DLL's main entry point.  It returns the root object
 * (IGMPI_Unknown) of the DLL's factory interface object.  The caller can
 * then call QueryInterface() to get the proper factory interface.
 */
extern "C" GMPI_Result GMPI_STDCALL
GMPI_GetFactory(IGMPI_Unknown** iobject)
{
	/* call QueryInterface() to keep refcounting in sync */
	return Factory()->QueryInterface(GMPI_IID_UNKNOWN, iobject);
}

/* register a plugin with the factory */
GMPI_Result
GMPI_FactoryRegisterIID(const wchar_t *unique_id, GMPI_Descriptor& descriptor)
{
	char buffer[40];
	GMPI_TRACE("GMPI_FactoryRegisterIID({%s})\n",
			GMPI_GuidString(&iid, buffer));
	Factory()->m_pluginMap.insert(GMPI_GuidDescriptor(iid, descriptor));
	return GMPI_SUCCESS;
}

/*
 * C to C++ thunks
 */

static GMPI_Result GMPI_STDCALL
GF_QueryInterface(IGMPI_Factory* igf, const GMPI_Guid* iid,
		IGMPI_Unknown** iobject)
{
	GMPI_SdkFactory* factory = static_cast<GMPI_SdkFactory*>(igf);
	return factory->QueryInterface(*iid, iobject);
}

static int32_t GMPI_STDCALL
GF_AddRef(IGMPI_Factory* igf)
{
	GMPI_SdkFactory* factory = static_cast<GMPI_SdkFactory*>(igf);
	return factory->AddRef();
}

static int32_t GMPI_STDCALL
GF_Release(IGMPI_Factory* igf)
{
	GMPI_SdkFactory* factory = static_cast<GMPI_SdkFactory*>(igf);
	return factory->Release();
}

static GMPI_Result GMPI_STDCALL
GF_CreateInstance(IGMPI_Factory* igf, const wchar_t *unique_id, //const GMPI_Guid* iid,
		IGMPI_Unknown* ghu, const char** args, IGMPI_Unknown** plugin)
{
	GMPI_SdkFactory* factory = static_cast<GMPI_SdkFactory*>(igf);
	GMPI_HostIntf* host;
	GMPI_Result r;

	//FIXME: as long as the host interface stays simple, this is OK.
	//If we start to have multiple interfaces that plugins use, then
	//we should just store the root pointer and let the plugin do the
	//QueryInterface for the interfaces it needs.
	{
		/* make a local UnknownIntf */
		GMPI_UnknownIntf unknown(ghu);

		/* ask the object for the GMPI Host interface */
		IGMPI_Unknown *ihost;
		r = unknown.QueryInterface(GMPI_IID_HOST, &ihost);
		//FIXME: check for error (and do what?)
		host = new GMPI_HostIntf(ihost);
	}

	r = factory->CreateInstance(*iid, *host, args, plugin);
	if (r != GMPI_SUCCESS) {
		delete host;
	}

	return r;
}

static GMPI_Result GMPI_STDCALL
GF_GetMetadata(IGMPI_Factory* igf, int index, GMPI_Metadata** metadata)
{
	GMPI_SdkFactory* factory = static_cast<GMPI_SdkFactory*>(igf);
	return factory->GetMetadata(index, metadata);
}

/* This defines the factory methods interface for the C linkage */
static IGMPI_FactoryMethods GF_Methods = {
	GF_QueryInterface,
	GF_AddRef,
	GF_Release,
	GF_CreateInstance,
	GF_GetMetadata,
};


/*
 * Factory methods
 */

GMPI_Result
GMPI_SdkFactory::QueryInterface(const GMPI_Guid& iid,
		IGMPI_Unknown** iobject)
{
	GMPI_TRACE("%p GMPI_SdkFactory::QueryInterface()\n", this);
	if (iid == GMPI_IID_UNKNOWN || iid == GMPI_IID_FACTORY) {
		IGMPI_Factory* ifactory = static_cast<IGMPI_Factory*>(this);
		*iobject = reinterpret_cast<IGMPI_Unknown*>(ifactory);
		AddRef();
		return GMPI_SUCCESS;
	}

	*iobject = NULL;
	return GMPI_NOSUPPORT;
}

int32_t
GMPI_SdkFactory::AddRef(void)
{
	m_refcount++;
	GMPI_TRACE("%p GMPI_SdkFactory::AddRef() = %d\n", this, m_refcount);
	return m_refcount;
}

int32_t
GMPI_SdkFactory::Release(void)
{
	m_refcount--;
	GMPI_TRACE("%p GMPI_SdkFactory::Release() = %d\n",
			this, m_refcount);
	if (m_refcount <= 0) {
		fprintf(stderr, "ERROR: GMPI_SdkFactory refcount hit 0\n");
		return 0;
	}
	return m_refcount;
}

GMPI_Result
GMPI_SdkFactory::CreateInstance(const wchar_t *unique_id, //const GMPI_Guid& iid,
		GMPI_HostIntf& host, const char** args,
		IGMPI_Unknown** object)
{
	GMPI_TRACE("%p GMPI_SdkFactory::CreateInstance()\n", this);

	/* search m_pluginMap for the requested IID */
	GMPI_GuidDescriptorMap::iterator it;
	it = m_pluginMap.find(iid);
	if (it == m_pluginMap.end()) {
		*object = NULL;
		return GMPI_NOSUPPORT;
	}

	/*
	 * Create an instance.  We cast to IGMPI_Plugin here so that a
	 * plugin author doesn't need to worry about casting from
	 * IGMPI_Plugin to IGMPI_Unknown for their result.
	 */
	GMPI_Descriptor& descriptor = (*it).second;
	return descriptor.CreateInstance(host, args,
			reinterpret_cast<IGMPI_Plugin**>(object));
}

GMPI_Result
GMPI_SdkFactory::GetMetadata(int index, GMPI_Metadata** metadata)
{
	GMPI_TRACE("%p GMPI_SdkFactory::GetMetadata()\n", this);

	/* scan m_pluginMap for the requested index */
	GMPI_GuidDescriptorMap::iterator it;
	it = m_pluginMap.begin();
	while (index && it != m_pluginMap.end()) {
		index--;
		it++;
	}
	if (it == m_pluginMap.end()) {
		*metadata = NULL;
		return GMPI_NOSUPPORT;
	}

	GMPI_Descriptor& descriptor = (*it).second;
	return descriptor.GetMetadata(metadata);
}

GMPI_SdkFactory::GMPI_SdkFactory(void)
{
	GMPI_TRACE("%p GMPI_SdkFactory::GMPI_SdkFactory()\n", this);
	IGMPI_Factory::methods = &GF_Methods;
	/* init the refcount to 1 - this class is used as a static */
	m_refcount = 1;
}

GMPI_SdkFactory::~GMPI_SdkFactory(void)
{
	GMPI_TRACE("%p GMPI_SdkFactory::~GMPI_SdkFactory()\n", this);
	m_pluginMap.clear();
}
