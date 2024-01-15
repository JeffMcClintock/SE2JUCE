#pragma once

#include <utility>
#include <vector>
#include "modules/shared/RawView.h"
#include "modules/shared/xplatform.h"

// how many patches
#define PP_ARRAY_SIZE 128

class PatchStorageBase
{
public:
	virtual ~PatchStorageBase() {}
	// returns true if value has changed.
	virtual bool SetValue(const void* data, size_t size, int patch = 0) = 0;
	virtual bool SetValue(class my_input_stream& p_stream, int patch = 0) = 0;
	virtual RawView GetValueRaw(int patch = 0) = 0;
	void SetValueRaw(RawView& val, int patch = 0)
	{
		SetValue(val.data(), val.size(), patch);
	}

	int GetValueSize(int patch = 0)
	{
		auto temp = GetValueRaw(patch);
		return static_cast<int>( temp.size() );
	}

	const void* GetValue(int patch = 0)
	{
		auto temp = GetValueRaw(patch);
		return temp.data();
	}

	int getPatchCount()
	{
		return static_cast<int>(dirtyFlags.size());
	}
	virtual void setPatchCount(int newPatchCount)
	{
		dirtyFlags.resize(newPatchCount); // also sets values to false;
	}
	bool isDirty(int patch)
	{
		return dirtyFlags[patch];
	}
	void setDirty(int patch, bool dirty)
	{
		dirtyFlags[patch] = dirty;
	}
private:
	std::vector<bool> dirtyFlags;
};

class PatchStorageVariableSize : public PatchStorageBase
{
public:
	PatchStorageVariableSize( int patchCount );
	~PatchStorageVariableSize();
	virtual bool SetValue(const void* data, size_t size, int patch = 0) override;
	virtual bool SetValue(my_input_stream& p_stream, int patch = 0) override;
	virtual RawView GetValueRaw(int patch = 0) override;
	virtual void setPatchCount(int newPatchCount) override;

private:
	int patchCount_;
	std::pair<int,char*>* patchMemory_;
};

class PatchStorageFixedSize : public PatchStorageBase
{
public:
	PatchStorageFixedSize( int patchCount, int size );
	~PatchStorageFixedSize();
	virtual bool SetValue(const void* data, size_t size, int patch = 0) override;
	virtual bool SetValue(my_input_stream& p_stream, int patch = 0) override;
	virtual RawView GetValueRaw(int patch = 0) override;
	virtual void setPatchCount(int newPatchCount) override;

private:
	int size_;
	char* patchMemory_;
};
