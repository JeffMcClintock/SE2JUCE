#include <codecvt>
#include <locale>
#include "./WaveRecorder2.h"
#include "../shared/string_utilities.h"
#include "../shared/xp_simd.h"
#include "../se_sdk3/PinIterator.h"
#include "../shared/xplatform.h"

using namespace std;
using namespace gmpi;

REGISTER_PLUGIN2 ( WaveRecorder2, L"SE Wave Recorder2" );

#define WAVE_FORMAT_PCM     1
#define  WAVE_FORMAT_IEEE_FLOAT 0x0003  /*  Microsoft Corporation  */

WaveRecorder2::WaveRecorder2( ) :
outputStream(nullptr)
{
}

int32_t WaveRecorder2::open()
{
	auto r = MpPluginBase::open();

	// Register pins.
	initializePin(  pinFileName );
	initializePin(  pinFormat );
	initializePin(  pinTimeLimit );

	PinIterator it(this);
	{
		int idx = 0;
		for( it.first() ; !it.isDone() ; it.next() )
		{
			if( it.getDirection() == gmpi::MP_IN && it.getDatatype() == 5 )
			{
				AudioIns.push_back(std::unique_ptr<AudioInPin>(new AudioInPin()));
				initializePin(idx, *( AudioIns.back() ));
			}
			++idx;
		}
	}


	float* zero = nullptr;
	AudioInPtrs.assign(AudioIns.size(), zero);

	return r;
}

void WaveRecorder2::subProcess(int sampleFrames)
{
	// get pointers to in/output buffers.
	for( size_t i = 0; i < AudioIns.size(); ++i )
	{
		AudioInPtrs[i] = getBuffer(*AudioIns[i]);
	}

	float* buffer = (float*)&( AudioBuffer[0] );

	for( int s = sampleFrames; s > 0; --s )
	{
		for( size_t i = 0; i < AudioInPtrs.size(); ++i )
		{
			*buffer++ = *AudioInPtrs[i]++;
		}
	}

	int64_t todo = sampleFrames;
	if (maxFrames > 0)
	{
		todo = (std::min)(todo, maxFrames - sampleFrameCount);
		if (todo == 0)
		{
			SET_PROCESS2(&WaveRecorder2::subProcessNothing);
			return;
		}
	}

	const int sampleSizeBytes = sizeof(float) / sizeof(char);
	size_t Channels = AudioInPtrs.size();
	size_t s = todo * sampleSizeBytes * Channels;
	if( fwrite(&( AudioBuffer[0] ), 1, s, outputStream) != s )
	{
		//error
	}

	sampleFrameCount += todo;
}

void WaveRecorder2::subProcess16bit(int sampleFrames)
{
	// get pointers to in/output buffers.
	for( size_t i = 0; i < AudioIns.size(); ++i )
	{
		AudioInPtrs[i] = getBuffer(*AudioIns[i]);
	}

	short* buffer = (short*)&( AudioBuffer[0] );

	const float scale_factor = 0x7fff;

	for( int s = sampleFrames; s > 0; --s )
	{
		for( size_t i = 0; i < AudioInPtrs.size(); ++i )
		{
			float sample = scale_factor * (std::min)(1.0f, (std::max)(-1.0f, *AudioInPtrs[i]++));
			
			*buffer++ = (short) FastRealToIntTruncateTowardZero(sample); // fast float-to-int using SSE. truncation toward zero.
		}
	}

	int64_t todo = sampleFrames;
	if(maxFrames > 0)
	{
		todo = (std::min)(todo, maxFrames - sampleFrameCount);
		if (todo == 0)
		{
			CloseFile();
			SET_PROCESS2(&WaveRecorder2::subProcessNothing);
			return;
		}
	}

	const int sampleSizeBytes = sizeof(short) / sizeof(char);
	const auto Channels = AudioInPtrs.size();
	size_t s = todo * sampleSizeBytes * Channels;
	if( fwrite(&( AudioBuffer[0] ), 1, s, outputStream) != s )
	{
		//error
	}

	sampleFrameCount += todo;
}

void WaveRecorder2::onSetPins()
{
	// Check which pins are updated.
	if( pinFileName.isUpdated() )
	{
		int Channels = AudioInPtrs.size();

		if( Channels > 0 )
		{
			wstring filename = StripExtension(pinFileName) + L".wav";

			wchar_t fullFilename[500];
			getHost()->resolveFilename(filename.c_str(), sizeof(fullFilename) / sizeof(fullFilename[0]), fullFilename);

			if( outputStream != 0 )
			{
				fclose(outputStream);
			}
#ifdef _WIN32
			outputStream = _wfopen(fullFilename, L"wb");
#else
            std::wstring_convert<std::codecvt_utf8<wchar_t> > stringConverter;
            auto utf8Filename = stringConverter.to_bytes(fullFilename);
            outputStream = fopen(utf8Filename.c_str(), "wb");
#endif

			if( outputStream == 0 )
			{
#ifdef _WIN32
				MessageBoxA(0, "WaveRecorder2: Failed to open output file.", "debug msg", MB_OK);
#endif
				return;
			}
	/*
			fwprintf(outputStream, L"SynthEdit MIDI Event log\n");
			fwprintf(outputStream, L"Samplerate:%f\n", getSampleRate());
			fwprintf(outputStream, L"Block Size:%d\n", getBlockSize());
	*/
			// Write partial wave file header.
			memset(&waveHeader, 0, sizeof(waveHeader));

			// write wave header.
			memcpy(waveHeader.chnk1_name, "RIFF", 4);
			memcpy(waveHeader.chnk2_name, "WAVE", 4);
			memcpy(waveHeader.chnk3_name, "fmt ", 4);
			memcpy(waveHeader.chnk4_name, "data", 4);

			// write wave header.
			if( pinFormat == 0 )
			{
				waveHeader.wFormatTag = WAVE_FORMAT_PCM;
				waveHeader.wBitsPerSample = 16;
				SET_PROCESS2(&WaveRecorder2::subProcess16bit);
			}
			else
			{
				waveHeader.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
				waveHeader.wBitsPerSample = 32;
				SET_PROCESS2(&WaveRecorder2::subProcess);
			}
			waveHeader.chnk3_size = 16;
			waveHeader.nChannels = AudioInPtrs.size();
			waveHeader.nSamplesPerSec = (int)getSampleRate();
			waveHeader.nAvgBytesPerSec = waveHeader.nSamplesPerSec * waveHeader.nChannels * (waveHeader.wBitsPerSample / 8);
			waveHeader.nBlockAlign = waveHeader.nChannels * (waveHeader.wBitsPerSample / 8);

			if( fwrite(&waveHeader, 1, sizeof(waveHeader), outputStream) != sizeof(waveHeader) )
			{
				//error
			}

			size_t s = getBlockSize() * waveHeader.nBlockAlign;

			AudioBuffer.resize(s);

			sampleFrameCount = 0;

			// Set sleep mode (optional).
			setSleep(false);
		}
		else
		{
			SET_PROCESS2(&WaveRecorder2::subProcessNothing);
			setSleep(true);
		}
	}

	if( pinTimeLimit.isUpdated() )
	{
		maxFrames = static_cast<int64_t>(pinTimeLimit.getValue() * getSampleRate());
	}
}

WaveRecorder2::~WaveRecorder2()
{
	CloseFile();
}

void WaveRecorder2::CloseFile()
{
	if( outputStream )
	{
		fseek(outputStream, 0, SEEK_SET);

		// Complete header with number of frames.
		waveHeader.chnk4_size = (LONG)( sampleFrameCount * waveHeader.nChannels * waveHeader.wBitsPerSample / 8);
		waveHeader.chnk1_size = waveHeader.chnk4_size + 36;

		if( fwrite(&waveHeader, 1, sizeof(waveHeader), outputStream) != sizeof(waveHeader) )
		{
			//error
		}

		// close the file.
		fclose(outputStream);
		outputStream = {};
	}
}
