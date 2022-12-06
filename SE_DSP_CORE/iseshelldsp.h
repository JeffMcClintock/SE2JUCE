
#pragma once
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#include <string>
#include <vector>
#include <unordered_map>
#include "modules/shared/xplatform.h"
#include "modules/shared/RawView.h"
#include "ElatencyContraintType.h"

/*
#include "iseshelldsp.h"
*/

enum class audioMasterState { Starting, Running, AsyncRestart, Stopping, Stopped };

class ISpecialIoModule
{
public:
	virtual ~ISpecialIoModule() {}
};

class ISpecialIoModuleAudio : public ISpecialIoModule
{
public:
	virtual void setIoBuffers(float* const* p_outputs, int numChannels, int stride = 1) = 0;
	virtual void startFade(bool isDucked) = 0;
};

// Core Functionality provided by all environments.
// Needs clarification as to threading.
class ISeShellDsp
{
public:
	virtual class IWriteableQue* MessageQueToGui() = 0;
	virtual void ServiceDspRingBuffers() = 0;
	virtual void ServiceDspWaiters2(int sampleframes, int guiFrameRateSamples) = 0;
	virtual void RequestQue( class QueClient* client, bool noWait = false ) = 0;
	virtual void NeedTempo() = 0;

	virtual std::wstring ResolveFilename(const std::wstring& name, const std::wstring& extension) = 0;
	virtual std::wstring getDefaultPath(const std::wstring& p_file_extension ) = 0;
	virtual void GetRegistrationInfo(std::wstring& p_user_email, std::wstring& p_serial) = 0;
	virtual void DoAsyncRestart() = 0;

    virtual void OnSaveStateDspStalled() = 0;
    
    // Ducking fade-out complete.
    virtual void OnFadeOutComplete() = 0;

	virtual int32_t SeMessageBox(const wchar_t* msg, const wchar_t* title, int flags) = 0;

	virtual std::unordered_map<int32_t, int32_t>& GetModuleLatencies() = 0;

	virtual int RegisterIoModule(ISpecialIoModule*) = 0;
};

// implement common functionality
class SeShellDsp : public ISeShellDsp
{
public:
	std::unordered_map<int32_t, int32_t>& GetModuleLatencies() override
	{
		return moduleLatencies;
	}

	virtual void Clear()
	{
		moduleLatencies.clear();
	}

private:
	std::unordered_map<int32_t, int32_t> moduleLatencies;
};
