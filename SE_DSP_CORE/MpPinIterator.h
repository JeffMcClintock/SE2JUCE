#pragma once

#include <vector>
#include "modules/se_sdk3/mp_sdk_common.h"
#include "ug_base.h"

class MpPinIterator final :
	public gmpi::IMpPinIterator
{
public:
	MpPinIterator( class ug_base* module )
    {
	    module_ = module;
    }
	// IMpPinIterator support.
	int32_t MP_STDCALL getCount( int32_t &returnCount ) override
    {
		returnCount = (int32_t) module_->plugs.size();
	    return gmpi::MP_OK;
    }
    int32_t MP_STDCALL first() override
    {
	    it_ = module_->plugs.begin();

	    if( it_ == module_->plugs.end() )
	    {
		    return gmpi::MP_FAIL;
	    }
	    else
	    {
		    return gmpi::MP_OK;
	    }
    }
	int32_t MP_STDCALL next() override
    {
	    ++it_;

	    if( it_ == module_->plugs.end() )
	    {
		    return gmpi::MP_FAIL;
	    }
	    else
	    {
		    return gmpi::MP_OK;
	    }
    }
	int32_t MP_STDCALL getPinId( int32_t& returnValue ) override
    {
		// due to shitty coding, UniqueId() was producing an index.
		// this has now been fixed, but to spare existing module from crashing, we're gonna return the index.

		//	    returnValue = (*it_)->UniqueId();
		returnValue = (*it_)->getPlugIndex();
	    return gmpi::MP_OK;
    }
	int32_t MP_STDCALL getPinDirection( int32_t& returnValue ) override
    {
	    returnValue = (*it_)->Direction;
	    return gmpi::MP_OK;
    }
    int32_t MP_STDCALL getPinDatatype( int32_t& returnValue ) override
    {
	    returnValue = (*it_)->DataType;
        return gmpi::MP_OK;
    }

	GMPI_QUERYINTERFACE1(gmpi::MP_IID_PIN_ITERATOR, gmpi::IMpPinIterator)
	GMPI_REFCOUNT

private:
	ug_base* module_;
	std::vector<class UPlug*>::iterator it_;
};

