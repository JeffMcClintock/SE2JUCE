/*
 * Copyright 2004-2005 by Tim Hockin and Jeff McClintock
 */

#ifndef GMPI_H_INCLUDED
#define GMPI_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h" /* JM-MOD */
#include "stdint.h" /* JM-MOD */
#include "util.h" /* JM-MOD */

/*
 * Version code manipulation. These need to be macros so they can be used
 * in initializers for static variables.
 */
#define GMPI_MKVERSION(major, minor, micro) \
	((uint32_t)(((major) & 0xff)<<24) \
		  | (((minor) & 0xff)<<16) \
		  | (((micro) & 0xffff)))
#define GMPI_VER_MAJOR(ver)		(((ver)>>24) & 0xff)
#define GMPI_VER_MINOR(ver)		(((ver)>>16) & 0xff)
#define GMPI_VER_MICRO(ver)		((ver) & 0xffff)
#define GMPI_VER_CMP(a, b)		((a) - (b))

/* The GMPI API version defined by this file */
#define GMPI_API_VERSION		GMPI_MKVERSION(0,0,1)


/* All GMPI methods return a common error code */
typedef int32_t GMPI_Result;

/* A GMPI_Result may take one of these values */
enum {
	GMPI_SUCCESS		=  0,		/* success */
	GMPI_FAIL		= -1,		/* general failure */
	GMPI_NOMEM		= -2,		/* no memory available */
	GMPI_NOSUPPORT		= -3,		/* not supported */
	GMPI_DLLFAIL		= -4,		/* DLL hook failed */
};

/*
 * Forward declarations
 */
typedef struct GMPI_Guid GMPI_Guid;
typedef struct IGMPI_Unknown IGMPI_Unknown;
typedef struct IGMPI_Plugin IGMPI_Plugin;
typedef struct IGMPI_Factory IGMPI_Factory;
typedef struct IGMPI_Host IGMPI_Host;
typedef struct GMPI_Metadata GMPI_Metadata;

/*
 * Globally Unique Identifier
 *
 * This is the established layout for a GUID.  GUIDs are used to identify
 * plugins and interfaces (also called Interface IDs or IIDs).
 */
struct GMPI_Guid {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	unsigned char data4[8];
};


/*
 * IGMPI_Unknown
 *
 * This is the base GMPI interface.  All the other IGMPI interfaces
 * "inherit" from this (have the same base API).
 */

struct IGMPI_Unknown {
	struct IGMPI_UnknownMethods* methods;
};

typedef struct IGMPI_UnknownMethods {
	/*
	 * IGMPI_UnknownMethods.QueryInterface()
	 *
	 * Query the object for a specific interface.  The caller
	 * specifies the interface ID to lookup.  If the object supports
	 * that IID, the interface argument will be assigned to a pointer
	 * to the interface object.  If the object does not support the
	 * IID, this method will return GMPI_NOSUPPORT.  The interface
	 * object returned by this method has already had it's reference
	 * count incremented for the caller.
	 *
	 * In:
	 *   iid: The requested interface ID.
	 *
	 * Out:
	 *   iobject: A pointer to a pointer to the interface object for
	 *   the specified IID.  This object must be released.
	 *
	 * Return:
	 *   GMPI_SUCCESS if the specified IID is supported or
	 *   GMPI_NOSUPPORT if the specified IID is not supported.
	 */
	GMPI_Result (GMPI_STDCALL *QueryInterface)(IGMPI_Unknown*,
			const GMPI_Guid* iid, IGMPI_Unknown** iobject);

	/*
	 * IGMPI_UnknownMethods.AddRef()
	 *
	 * Increment the reference count of an object.  This method should
	 * be called whenever a new reference to an object is being
	 * stored.
	 *
	 * In:
	 *   None.
	 *
	 * Out:
	 *   None.
	 *
	 * Return:
	 *   The value of the reference count after being incremented.
	 */
	int32_t (GMPI_STDCALL *AddRef)(IGMPI_Unknown*);

	/*
	 * IGMPI_UnknownMethods.Release()
	 *
	 * Decrement the reference count of an object and possibly release
	 * it's resources.  This method should be called whenever a
	 * reference to an object is being removed.  After calling this,
	 * the caller should assume the object has been released, and must
	 * not access it again.
	 *
	 * In:
	 *   None.
	 *
	 * Out:
	 *   None.
	 *
	 * Return:
	 *   The value of the reference count after being decremented.
	 */
	int32_t (GMPI_STDCALL *Release)(IGMPI_Unknown*);
} IGMPI_UnknownMethods;

/* GUID for IGMPI_Unknown - {00000000-0000-C000-0000-000000000046} */
static const GMPI_Guid GMPI_IID_UNKNOWN = {
	0x00000000, 0x0000, 0xC000,
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
};


/*
 * IGMPI_Plugin
 *
 * This is the GMPI plugin interface.
 */
struct IGMPI_Plugin {
	struct IGMPI_PluginMethods* methods;
};

typedef struct IGMPI_PluginMethods {
	/* IUnknown methods - see IGMPI_UnknownMethods */
	GMPI_Result (GMPI_STDCALL *QueryInterface)(IGMPI_Plugin*,
			const GMPI_Guid* iid, IGMPI_Unknown** iobject);
	int32_t (GMPI_STDCALL *AddRef)(IGMPI_Plugin*);
	int32_t (GMPI_STDCALL *Release)(IGMPI_Plugin*);

	/*
	 * IGMPI_PluginMethods.Process()
	 *
	 * Process a time slice.  The plugin will process it's audio and
	 * control inputs for a number of sample frames, specified by the
	 * count argument.  The count argument can be any non-zero number.
	 *
	 * In:
	 *   count: The number of sample frames to process.
	 *
	 * Out:
	 *
	 * Return:
	 *   GMPI_SUCCESS if the processing succeeded or GMPI_FAIL if the
	 *   processing did not succeed.
	 */
	GMPI_Result (GMPI_STDCALL *Process)(IGMPI_Plugin*, uint32_t count);

	GMPI_Result (GMPI_STDCALL *GetPinMetadata)(IGMPI_Plugin*, int32_t arg, struct GMPI_PinMetadata *metadata);
} IGMPI_PluginMethods;

/* GUID for IGMPI_Plugin  - {7D2423C8-73A0-4D3B-BDB4-831D11554D0F} */
static const GMPI_Guid GMPI_IID_PLUGIN = {
	0x7D2423C8, 0x73A0, 0x4D3B,
	{0xBD, 0xB4, 0x83, 0x1D, 0x11, 0x55, 0x4D, 0x0F}
};


/*
 * IGMPI_Factory
 *
 * This is the GMPI plugin factory interface.  The factory is used to
 * create plugin instances, and similar tasks.
 */
struct IGMPI_Factory {
	struct IGMPI_FactoryMethods* methods;
};

typedef struct IGMPI_FactoryMethods {
	/* IUnknown methods - see IGMPI_UnknownMethods */
	GMPI_Result (GMPI_STDCALL *QueryInterface)(IGMPI_Factory*,
			const GMPI_Guid* iid, IGMPI_Unknown** iobject);
	int32_t (GMPI_STDCALL *AddRef)(IGMPI_Factory*);
	int32_t (GMPI_STDCALL *Release)(IGMPI_Factory*);

	/*
	 * IGMPI_FactoryMethods.CreateInstance()
	 *
	 * Instantiate a plugin.  This method will lookup the specified
	 * IID and produce an instance of that plugin object.  The host
	 * argument is passed to the new instance to provide host
	 * services.  The args argument is an array of strings, terminated
	 * by a NULL pointer, which is passed to the plugin at creation.
	 * The first entry in the args string is the path to the plugin
	 * DLL file. If the requested IID is supported by the factory, the
	 * object argument will be assigned to a pointer to the plugin object.
	 *
	 * In:
	 *   iid: The requested interface ID.
	 *   host: The unknown object which can be queried for the host
	 *     object for the plugin.
	 *   args: The arguments to the plugin.
	 *
	 * Out:
	 *   object: A pointer to a pointer to a plugin object.  This
	 *   object must be released.
	 *
	 * Return:
	 *   GMPI_SUCCESS if the specified IID is supported or
	 *   GMPI_NOSUPPORT if the specified IID is not supported.
	 */
	GMPI_Result (GMPI_STDCALL *CreateInstance)(IGMPI_Factory*,
			const GMPI_Guid* iid, IGMPI_Unknown* host,
			const char** args, IGMPI_Unknown** object);

	/*
	 * IGMPI_FactoryMethods.GetMetadata()
	 *
	 * Query the factory for inline metadata.  Plugins may provide
	 * either inline or external metadata.  The host calls this
	 * function in a loop, with a series of index values starting at
	 * zero.  This method will lookup the GMPI_Metadata structure for
	 * the corresponding plugin and store a pointer to it in the
	 * metadata argument.  Plugins are indexed consecutively, but the
	 * index has no further meaning.
	 * FIXME: how to indicate that we have external metadata instead?
	 *
	 * In:
	 *   index: The sequential index of the plugin within this
	 *   factory.
	 *
	 * Out:
	 *   metadata: A pointer to a pointer to the metadata structure.
	 *   The plugin DLL owns the actual metadata structures, the
	 *   caller gets a pointer.  This structure does not need to be
	 *   released.
	 *
	 * Return:
	 *   GMPI_SUCCESS if the metadata structure for the specified
	 *   index was found or GMPI_NOSUPPORT if index was not found.
	 */
	GMPI_Result (GMPI_STDCALL *GetMetadata)(IGMPI_Factory*,
			int index, GMPI_Metadata** metadata);
} IGMPI_FactoryMethods;

/* GUID for IGMPI_Factory - {00000001-0000-C000-0000-000000000046} */
static const GMPI_Guid GMPI_IID_FACTORY = {
	0x00000001, 0x0000, 0xC000,
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}
};


/*
 * GMPI_Metadata
 *
 * This is a set of GMPI plugin metadata information.  Hosts can use this
 * information to know how to identify a plugin and what the plugin can
 * do.
 */

struct GMPI_Metadata {
	/*
	 * The Guid field uniquely identifies a plugin.  When saving
	 * state, the host should identify plugins by their Guid and
	 * VerCode.  This allows project data to be moved between GMPI
	 * installations without losing track of plugins.
	 */
	GMPI_Guid Guid;

	/*
	 * The ApiVersions field is a pointer to an array of
	 * GMPI_API_VERSION codes that this plugin supports.  Hosts must
	 * select one of the api_versions to communicate with the plugin.
	 * This array is terminated by a zero entry.
	 */
	uint32_t* ApiVersions;

	/*
	 * The VerCode field is an encoded 'major.minor.micro' tuple, like
	 * GMPI_API_VERSION.  Hosts should use this only to compare
	 * versions of a given plugin.
	 */
	uint32_t VerCode;		/* eg: GMPI_MKVERSION(3,14,15) */

	/*
	 * These are display-friendly fields, which hosts can display to
	 * users.  If a plugin is localized, these strings should be
	 * translated.
	 */
	const char* Name;		/* eg: "Foo Bar Plugin" */
	const char* Version;		/* eg: "version 3.14.15" */
	const char* Author;		/* eg: "Foo Corporation" */
	const char* Copyright;		/* eg: "copyright 2005, Foo Corp." */
	const char* Notes;		/* eg: about box info */

	/*
	 * These fields contain universal resource indicators (URIs) for
	 * accessing more data.
	 */
	const char* Home;		/* eg: "http://foo.example.com" */
	const char* Docs;		/* eg: "http://foo.example.com/docs" */
};

enum GMPI_PinDirection{ GMPI_IN, GMPI_OUT };
enum GMPI_PinDatatype{ GMPI_AUDIO, GMPI_FLOAT32, GMPI_INT32, GMPI_TEXT, GMPI_BLOB };

struct GMPI_PinMetadata {
	/*
	 * The Guid field uniquely identifies a plugin.  When saving
	 * state, the host should identify plugins by their Guid and
	 * VerCode.  This allows project data to be moved between GMPI
	 * installations without losing track of plugins.
	 */
	const char *name;
	enum GMPI_PinDirection direction;
	enum GMPI_PinDatatype datatype;
};

/*
 * IGMPI_Host
 *
 * This is the GMPI host interface.  Plugins can use this to get
 * information about the host or to utilize host services.
 */

struct IGMPI_Host {
	struct IGMPI_HostMethods* methods;
};

typedef struct IGMPI_HostMethods {
	/* IUnknown methods - see IGMPI_UnknownMethods */
	GMPI_Result (GMPI_STDCALL *QueryInterface)(IGMPI_Host*,
			const GMPI_Guid* iid, IGMPI_Unknown** iobject);
	int32_t (GMPI_STDCALL *AddRef)(IGMPI_Host*);
	int32_t (GMPI_STDCALL *Release)(IGMPI_Host*);

	/* placeholder */
	GMPI_Result (GMPI_STDCALL *Placeholder1)(IGMPI_Host*);
} IGMPI_HostMethods;

/* GUID for IGMPI_Host - {E507ED6A-8DD9-11D9-8BDE-F66BAD1E3F3A} */
static const GMPI_Guid GMPI_IID_HOST = {
	0xE507ED6A, 0x8DD9, 0x11D9,
	{0x8B, 0xDE, 0xF6, 0x6B, 0xAD, 0x1E, 0x3F, 0x3A}
};


/* The GMPI DLL entrypoint signatures */
typedef GMPI_Result (GMPI_STDCALL *GMPI_DllEntry)(IGMPI_Unknown**);
typedef GMPI_Result (GMPI_STDCALL *GMPI_DllHookPre)(void);
typedef void        (GMPI_STDCALL *GMPI_DllHookPost)(void);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GMPI_H_INCLUDED */

