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

		gmpi_sdk::mp_shared_ptr<gmpi::IEmbeddedFileSupport> dspHost;
		getHost()->queryInterface(gmpi::MP_IID_HOST_EMBEDDED_FILE_SUPPORT, dspHost.asIMpUnknownPtr());

		assert(dspHost); // new way
		{
			const auto filename = JmUnicodeConversions::WStringToUtf8(pinFilename.getValue());

			gmpi_sdk::MpString fullFilename;
			dspHost->resolveFilename(filename.c_str(), &fullFilename );

			gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> obj;
			dspHost->openUri(fullFilename.c_str(), &obj.get());

			if (obj)
			{
				gmpi_sdk::mp_shared_ptr<gmpi::IProtectedFile2> file;

				obj->queryInterface(gmpi::MP_IID_PROTECTED_FILE2, file.asIMpUnknownPtr());

				if (file)
				{
					const auto fullFilenameW = JmUnicodeConversions::Utf8ToWstring(fullFilename.str());
					sampleHandle = SampleManager::Instance()->Load(file.get(), fullFilenameW.c_str(), pinBank, pinPatch);
				}
			}
		}
#if 0
		else // old way (memory leak)
		{
			// resolve full filename path acording to host's prefered folder.
			const int maxCharacters = 500;
			wchar_t fullFilename[maxCharacters];
			getHost()->resolveFilename( pinFilename.getValue().c_str(), maxCharacters, fullFilename );

			// open imbedded file.
			gmpi::IProtectedFile* file = 0;
			int r = getHost()->open ProtectedFile( pinFilename.getValue().c_str(), &file );

			if( r == gmpi::MP_OK )
			{
				// Load soundfont.
				sampleHandle = SampleManager::Instance()->Load( file, fullFilename, pinBank, pinPatch  );
				file->close();
			}
			else
			{
				 sampleHandle = -1;
			}
		}
#endif
		pinSampleId = sampleHandle;

		// Release old sample last. So it stays in memory in case 'new' sample
		// comes from same soundfont. (else soundfont is unloaded/reloaded unnesesarily).
		SampleManager::Instance()->Release( oldSamplehandle );
	}
}

