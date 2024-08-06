#pragma once
#include "modules/se_sdk3/mp_api.h"

/*
template < class ImplementsInterface>
class MpUnknown : public ImplementsInterface
{
public:
	MpUnknown() :
	  refCount_(0)
	{
	}

	virtual ~MpUnknown()
	{
	}

	// IUnknown methods
	virtual int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface)
	{
		if( iid == gmpi::MP_IID_UNKNOWN )
		{
			*object = static_cast<gmpi::IMpUnknown*>( this );
			add Ref();
			return gmpi::MP_OK;
		}

		*object = 0;
		return gmpi::MP_NOSUPPORT;
	}

	// Provide some boilerplate code to help derived class' support their interface.
	int32_t queryInterfaceHelper(const gmpi::MpGuid& iid, gmpi::IMpUnknown** object, const gmpi::MpGuid& supported_iid, gmpi::IMpUnknown* supported_interface )
	{
		if( iid == supported_iid )
		{
			*object = supported_interface;
			add Ref();
			return gmpi::MP_OK;
		}

		return MpUnknown::queryInterface( iid, object );
	}


	virtual int32_t MP_STDCALL add Ref()
	{
		refCount_++;
		return refCount_;
	}

	virtual int32_t MP_STDCALL rele ase()
	{
		refCount_--;

		if( refCount_ == 0 )
		{
			delete this;
			return 0;
		}
		return refCount_;
	}

private:
	int32_t refCount_;
};
*/

class Sdk3CloneIterator : public gmpi::IMpCloneIterator
{
public:
	Sdk3CloneIterator(class ug_base* ug);

	virtual int32_t MP_STDCALL first() override
    {
        current_ = first_;
        return gmpi::MP_OK;
    }
    
	virtual int32_t MP_STDCALL next(gmpi::IMpUnknown** returnInterface) override;

	GMPI_QUERYINTERFACE1(gmpi::MP_IID_CLONE_ITERATOR, gmpi::IMpCloneIterator)
	GMPI_REFCOUNT

private:
	ug_base* first_;
	ug_base* current_;
};

