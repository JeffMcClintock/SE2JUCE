#include "./SampleLoader2.h"
#include "./SampleManager.h"
#include "../shared/unicode_conversion.h"
#include "../se_sdk3/MpString.h"

REGISTER_PLUGIN ( SampleLoader2, L"SE Sample Loader2" );

SampleLoader2::SampleLoader2( IMpUnknown* host ) : MpBase( host )
,sampleHandle(-1)
{
	// Register pins.
	initializePin( 0, pinFilename );
	initializePin( 1, pinBank );
	initializePin( 2, pinPatch );
	initializePin( 3, pinSampleId );
}

SampleLoader2::~SampleLoader2()
{
	SampleManager::Instance()->Release( sampleHandle );
}

void SampleLoader2::onSetPins()
{
	if( pinFilename.isUpdated() || pinBank.isUpdated() || pinPatch.isUpdated() )
	{
		int oldSamplehandle = sampleHandle;

		const auto fullFilename = host.resolveFilename(pinFilename);
		auto file = host.openUri(fullFilename);
		
		if (file)
		{
			const auto fullFilenameW = JmUnicodeConversions::Utf8ToWstring(fullFilename);
			sampleHandle = SampleManager::Instance()->Load(file.get(), fullFilenameW.c_str(), pinBank, pinPatch);
		}
		
		pinSampleId = sampleHandle;

		// Release old sample last. So it stays in memory in case 'new' sample
		// comes from same soundfont. (else soundfont is unloaded/reloaded unnesesarily).
		SampleManager::Instance()->Release( oldSamplehandle );
	}
}

