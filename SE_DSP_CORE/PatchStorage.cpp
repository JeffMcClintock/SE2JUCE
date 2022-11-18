#include "pch.h"

#include <assert.h>
#include "mfc_emulation.h"
#include "PatchStorage.h"
#include "my_input_stream.h"
#include "iseshelldsp.h"

#ifdef __linux__
#include <string.h>
#include <stdlib.h>
#endif


// how many patches
#define PP_ARRAY_SIZE 128

PatchStorageVariableSize::PatchStorageVariableSize( int patchCount ) : patchCount_(patchCount)
{
	PatchStorageBase::setPatchCount( patchCount );
	patchMemory_ = new std::pair<int,char*>[patchCount];

	for(int i = 0 ; i < patchCount ; i++ )
	{
		patchMemory_[i].first = 0;
		patchMemory_[i].second = 0;
	}
};

PatchStorageVariableSize::~PatchStorageVariableSize()
{
	for(int i = 0 ; i < patchCount_ ; i++ )
	{
		delete [] patchMemory_[i].second;
	}

	delete [] patchMemory_;
};


void PatchStorageVariableSize::setPatchCount(int newPatchCount)
{
	PatchStorageBase::setPatchCount(newPatchCount);

	assert(newPatchCount == 1 || newPatchCount == 128);
	std::pair<int,char*>* newPatchMemory = new std::pair<int,char*>[newPatchCount];

	int fromPatch = 0;
	int size = patchMemory_[fromPatch].first;

	for( int i = 0 ; i < newPatchCount ; i++ )
	{
		newPatchMemory[i].first = size;
		newPatchMemory[i].second = new char[size];
		memcpy( newPatchMemory[i].second, patchMemory_[fromPatch].second, size );
	}

	for(int i = 0 ; i < patchCount_ ; i++ )
	{
		delete [] patchMemory_[i].second;
	}

	delete [] patchMemory_;
	patchMemory_ = newPatchMemory;
	patchCount_ = newPatchCount;
}


bool PatchStorageVariableSize::SetValue( const void* data, size_t size, int patch )
{
	assert( patch >= 0 && patch < patchCount_ );
	char* dest = patchMemory_[patch].second;
	bool isChanged = (dest == 0) || size != patchMemory_[patch].first || 0 != memcmp(dest, data, size);

	// Avoid memory allocation if at all possible.
	if( patchMemory_[patch].first != size )
	{
		delete [] dest;
		patchMemory_[patch].first = (int) size;
		dest = patchMemory_[patch].second = new char[size];
	}

	memcpy( dest, data, size );
	return isChanged;
};

void PatchStorageVariableSize::SetValue(my_input_stream& p_stream, int patch )
{
	int size;
	p_stream >> size;
	assert( patch >= 0 && patch < patchCount_ );
	char* dest = patchMemory_[patch].second;

	// Avoid memory allocation if at all possible.
	if( patchMemory_[patch].first < size )
	{
		delete [] dest;
		dest = patchMemory_[patch].second = new char[size];
	}
	patchMemory_[patch].first = size;

	p_stream.Read(dest,size);
}

RawView PatchStorageVariableSize::GetValueRaw(int patch)
{
	return RawView(patchMemory_[patch].second, patchMemory_[patch].first);
}


//int PatchStorageVariableSize::GetValueSize(int patch)
//{
//	assert( patch < patchCount_ );
//	return patchMemory_[patch].first;
//};
//
//void* PatchStorageVariableSize::GetValue(int patch)
//{
//	assert( patch < patchCount_ );
//	return patchMemory_[patch].second;
//};

PatchStorageFixedSize::PatchStorageFixedSize( int patchCount, int size ) : size_(size)
{
	PatchStorageBase::setPatchCount( patchCount );
	patchMemory_ = new char[size * patchCount];
};

PatchStorageFixedSize::~PatchStorageFixedSize()
{
	delete [] patchMemory_;
};

void PatchStorageFixedSize::setPatchCount(int newPatchCount)
{
	auto patchCount = getPatchCount();
	assert(newPatchCount == 1 || newPatchCount == 128);
	PatchStorageBase::setPatchCount(newPatchCount);

	if( patchCount == newPatchCount )
		return;

	char* newPatchMemory_ = new char[size_ * newPatchCount];

	if(newPatchCount < patchCount)
	{
		memcpy(newPatchMemory_, patchMemory_, size_ * newPatchCount);
	}
	else
	{
		for(int i = 0 ; i < newPatchCount ; i++ )
		{
			char* dest = newPatchMemory_ + i * size_;
			memcpy(dest, patchMemory_, size_ );
		}
	}

	delete [] patchMemory_;
	patchMemory_ = newPatchMemory_;
	patchCount = newPatchCount;
}

bool PatchStorageFixedSize::SetValue(const void* data, size_t size, int patch)
{
	assert( size == size_);

	// when upgrading pre 1.1 files, all patches are loaded, even if 'Ignore PC' is set.
	// This gives user chance to recitfy bug where IPC don't work for CControl-based classes.
	// When DSP patch mem is created however, only one patch is allocated. Need to check GUI
	// ain't updating a patch we don't have.  Happens when loading fxb banks etc.
	if( patch >= 0 && patch < getPatchCount())
	{
		char* dest = patchMemory_ + patch * size_;
		bool isChanged = 0 != memcmp(dest, data, size_);
		memcpy(dest, data, size_);
		return isChanged;
	}

	return false;
};


void PatchStorageFixedSize::SetValue(my_input_stream& p_stream, int patch )
{
	// when upgrading pre 1.1 files, all patches are loaded, even if 'Ignore PC' is set.
	// This gives user chance to recitfy bug where IPC don't work for CControl-based classes.
	// When DSP patch mem is created however, only one patch is allocated. Need to check GUI
	// ain't updating a patch we don't have.  Happens when loading fxb banks etc.
	if( patch >= 0 && patch < getPatchCount())
	{
		char* dest = patchMemory_ + patch * size_;
		p_stream.Read(dest,size_);
	}
}

RawView PatchStorageFixedSize::GetValueRaw(int patch)
{
	const char* dest = patchMemory_ + patch * size_;
	return RawView(dest, size_);
}
