#include "./Scope3.h"
#include "../shared/xplatform.h"

REGISTER_PLUGIN(Scope3, L"SE Scope3 XP");
SE_DECLARE_INIT_STATIC_FILE(Scope3XP);

/* TODO !!!
	properties->flags = UGF_VOICE_MON_IGNORE;
	properties->gui_flags = CF_CONTROL_VIEW|CF_STRUCTURE_VIEW;
*/

Scope3::Scope3( IMpUnknown* host ) : MpBase( host )
,index_( 0 )
//,sleepCount( -1 )
,currentVoice_( 0 )
{
}

int32_t Scope3::open()
{
	// Associate each pin object with it's ID in the XML file.
	// In this case must be done after Triggered Scope inits it's pin.
	initializePin( pinSamplesA );
	initializePin( pinSamplesB );
	initializePin( pinSignalA );
	initializePin( pinSignalB );
	initializePin( pinVoiceActive );
	initializePin( pinPolyDetect );

	MpBase::open();	// always call the base class

	SET_PROCESS( &Scope3::subProcess );

//	channelActive_[0] = channelActive_[1] = true; // TODO !! getPin(i)->IsConnected();

	// Module must transmit an initial value on all output pins. [ now handled by SDK ]
	//pinSamplesA.sendPinUpdate();
	//pinSamplesB.sendPinUpdate();

	// determine if polyphonic or not.
	int isCloned;
	getHost()->isCloned( &isCloned );
	pinPolyDetect = isCloned != 0;

		captureSamples = SCOPE_BUFFER_SIZE;

	return gmpi::MP_OK;
}

// wait for waveform to restart.
void Scope3::waitForTrigger1(int bufferOffset, int sampleFrames)
{
//	float* signala = bufferOffset + pinSignalA.getBuffer();
	float* signala = bufferOffset + getTriggerPin()->getBuffer();
	
	for(int s = sampleFrames ; s > 0 ; s--)
	{
		if(*signala++ <= 0.f)
		{
			index_ = 0;
			SET_PROCESS(&Scope3::waitForTrigger2);
			waitForTrigger2(bufferOffset + sampleFrames - s, s);
			return;
		}
	}

	timeoutCount_ -= sampleFrames;
	if(timeoutCount_ < 0)
	{
		SET_PROCESS(&Scope3::waitForTrigger2);
	}
}

void Scope3::waitForTrigger2(int bufferOffset, int sampleFrames)
{
//	float* signala	= bufferOffset + pinSignalA.getBuffer();
	float* signala = bufferOffset + getTriggerPin()->getBuffer();

	for(int s = sampleFrames ; s > 0 ; s--)
	{
		if(*signala++ > 0.f)
		{
			forceTrigger();

			(this->*(getSubProcess()))(bufferOffset + sampleFrames - s, s);
			return;
		}
	}

	timeoutCount_ -= sampleFrames;
	if(timeoutCount_ < 0)
	{
		forceTrigger();
	}
}

void Scope3::subProcess(int bufferOffset, int sampleFrames)
{
	// get pointers to in/output buffers
	float* signalA	= bufferOffset + pinSignalA.getBuffer();
	float* signalB	= bufferOffset + pinSignalB.getBuffer();

	int count = captureSamples - index_;
	if(count > sampleFrames)
		count = sampleFrames;

	int remain = sampleFrames - count;

//	if( channelActive_[0] )
	if (channelsleepCount_[0] > 0)
	{
		int i = index_;
		for(int c = count ; c > 0 ;c--)
		{
			assert( i < captureSamples );
			resultsA_[i++] = *signalA++;
		}
	}
//	if( channelActive_[1] )
	if (channelsleepCount_[1] > 0)
	{
		int i = index_;
		for(int c = count ; c > 0 ;c--)
		{
			resultsB_[i++] = *signalB++;
		}
	}

	index_ += count;

	if(index_ >= captureSamples )
	{
		sendResultToGui(bufferOffset);

		// process remaining samples.
		(this->*(getSubProcess()))(bufferOffset + sampleFrames - remain, remain);
	}
}

// do nothing for 1/25th second.  Gives GUI time to display image.
void Scope3::subProcessCruise(int bufferOffset, int sampleFrames)
{
	timeoutCount_ -= sampleFrames;
	if(timeoutCount_ < 0)
	{
		// in absence of trigger signal, redraw 3 times per second.
		timeoutCount_ = getTimeOut(); // (int)getSampleRate() / 3;
		SET_PROCESS(&Scope3::waitForTrigger1);
	}
}

void Scope3::forceTrigger()
{
	index_ = 0;
	SET_PROCESS(&Scope3::subProcess);
}

void Scope3::sendResultToGui(int block_offset)
{
	{
		/*
		if( captureSamples > SCOPE_BUFFER_SIZE * 2 ) // oversampling? If so compress timescale.
		{
			int j = 0;
			for( int i = 0 ; i < SCOPE_BUFFER_SIZE ; ++i )
			{
				resultsA_[i] = (resultsA_[j] + resultsA_[j+1] + resultsA_[j+2] + resultsA_[j+3]) * 0.25f;
				resultsB_[i] = (resultsB_[j] + resultsB_[j+1] + resultsB_[j+2] + resultsB_[j+3]) * 0.25f;
				j += 4;
			}
		}
		else
		{
			int j = 0;
			for( int i = 0 ; i < SCOPE_BUFFER_SIZE ; ++i )
			{
				resultsA_[i] = (resultsA_[j] + resultsA_[j+1]) * 0.5f;
				resultsB_[i] = (resultsB_[j] + resultsB_[j+1]) * 0.5f;
				j += 2;
			}
		}
		*/
		const int datasize = SCOPE_BUFFER_SIZE * sizeof(resultsA_[0]);

		if (channelsleepCount_[0] > 0)
		{
			pinSamplesA.setValueRaw( datasize, &resultsA_ );
			pinSamplesA.sendPinUpdate( block_offset );
		}

		if(channelsleepCount_[1] > 0)
		{
			pinSamplesB.setValueRaw(datasize, &resultsB_);
			pinSamplesB.sendPinUpdate(block_offset);
		}
	}

	if (!pinSignalA.isStreaming())
	{
		--channelsleepCount_[0];
	}
	if (!pinSignalB.isStreaming())
	{
		--channelsleepCount_[1];
	}

	if (channelsleepCount_[0] <= 0 && channelsleepCount_[1] <= 0 && !getTriggerPin()->isStreaming())
	{
		SET_PROCESS(&Scope3::subProcessNothing);
		setSleep(true);
	}
	else
	{
		// waste of CPU to send updates more often than GUI can repaint,
		// wait approx 1/10th seccond between captures.
		timeoutCount_ = (int)getSampleRate() / 10;
		SET_PROCESS(&Scope3::subProcessCruise);
	}

	index_ = 0; // ensure we are ready for next capture cycle, even if subProcessCruise gets bypassed by suspend/resume.
}

void Scope3::onSetPins()  // one or more pins_ updated.  Check pin update flags to determin which ones.
{
	if (pinSignalA.isUpdated())
		channelsleepCount_[0] = 2; // need to send at least 2 captures to ensure flat-line signal captured.

	if (pinSignalB.isUpdated())
		channelsleepCount_[1] = 2;

	/*
	if( pinSignalA.isStreaming() || pinSignalB.isStreaming() || getTriggerPin()->isStreaming() )
	{
		sleepCount = -1; // indicates no sleep.
	}
	else
	{
		// need to send at least 2 captures to ensure flat-line signal captured.
		// Current capture may be half done.
		sleepCount = 2;
	}

	if (pinSignalA.isUpdated())
	{
		channelActive_[0] = true;
	}

	if (pinSignalB.isUpdated())
	{
		channelActive_[1] = true;
	}
	*/

	// Avoid resetting capture unless module is actually asleep.
	if( getSubProcess() == &Scope3::subProcessNothing )
	{
		SET_PROCESS(&Scope3::subProcess);
	}

	if( pinVoiceActive.isUpdated() )
	{
		int32_t isPolyphonic;
		getHost()->isCloned( &isPolyphonic );

//		_RPT2(_CRT_WARN, "Scope3::onSetPins pinVoiceActive = %f [%x]\n", (double) pinVoiceActive, this );
		if( pinVoiceActive <= 0.0f && isPolyphonic != 0 )
		{
			// send blank capture to indicate voice muted.
			pinSamplesA.setValueRaw(0, 0);
			pinSamplesA.sendPinUpdate();
			pinSamplesB.setValueRaw(0, 0);
			pinSamplesB.sendPinUpdate();

			// do nothing.
			SET_PROCESS(&Scope3::subProcessNothing);
//			setSleep( true );
		}

	}

	setSleep(false);
}


