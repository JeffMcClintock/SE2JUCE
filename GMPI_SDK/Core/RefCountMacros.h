#pragma once
/*
#include "RefCountMacros.h"
*/

// macros to save typing the reference counting.
#define GMPI_QUERYINTERFACE( INTERFACE_IID, CLASS_NAME ) \
	gmpi::ReturnCode queryInterface(const gmpi::api::Guid* iid, void** returnInterface) override{ \
	*returnInterface = {}; \
	if ((*iid) == INTERFACE_IID || (*iid) == gmpi::api::IUnknown::guid ){ \
	*returnInterface = static_cast<CLASS_NAME*>(this); addRef(); \
	return gmpi::ReturnCode::Ok;} \
	return gmpi::ReturnCode::NoSupport;}

#ifndef GMPI_REFCOUNT
#define GMPI_REFCOUNT int refCount2_ = 1; \
	int32_t addRef() override {return ++refCount2_;} \
	int32_t release() override {if (--refCount2_ == 0){delete this;return 0;}return refCount2_;}

#define GMPI_REFCOUNT_NO_DELETE \
	int32_t addRef() override{return 1;} \
	int32_t release() override {return 1;}
#endif
