// Copyright 2008 Jeff McClintock

#ifndef SMART_AUDIO_PIN_H_INCLUDED
#define SMART_AUDIO_PIN_H_INCLUDED

#include "mp_sdk_audio.h" 
#include <limits>

// This class supports audio-rate control signal outputs with smoothing.
/*
#include "..\se_sdk3\smart_audio_pin.h"
*/

class SmartAudioPin : public AudioOutPin
{
public:
	SmartAudioPin();

	// Pointer to sound processing member function.
	typedef void (SmartAudioPin::* SapSubProcessMethodPointer)( int bufferOffset, int sampleFrames, bool& canSleep );

	float operator=(const float value)
	{
		setValue(value);
		return value;
	}
	// glide to new value.
	void setValue( float targetValue, int blockPosition = -1 );
	// jump to new value.
	void setValueInstant( float targetValue, int blockPosition = -1 );
	void pulse( float pulseHeight, int blockPosition = -1 );
	float getInstantValue();
	void setTransitionTime( float transitionTime ); // in samples. -1 = auto.
	void subProcess( int bufferOffset, int sampleFrames, bool& canSleep )
	{
		(this->*(curSubProcess_))( bufferOffset, sampleFrames, canSleep );
	}
	void setCurveType( int curveMode );
	enum CurveType{ Linear, Curved, LinearAdaptive };

private:
	void subProcessFirstSample( int bufferOffset, int sampleFrames, bool& canSleep );
	void subProcessStatic( int bufferOffset, int sampleFrames, bool& canSleep );
	void subProcessRamp( int bufferOffset, int sampleFrames, bool& canSleep );
	void subProcessCurve( int bufferOffset, int sampleFrames, bool& canSleep );
	void subProcessPulse( int bufferOffset, int sampleFrames, bool& canSleep );

	SapSubProcessMethodPointer curSubProcess_;
	float transitionTime_;
	float inverseTransitionTime_;
	float currentValue_;
	float targetValue_;
	int mode_;

	// Curve variables.
	float dv;	//difference v (velocity)
	float ddv;	//difference dv
	float c;	//constant added to ddv

	int count;
	float adaptiveHi_;
	float adaptiveLo_;
};

class RampGenerator
{
public:
	RampGenerator();
	void setTransitionTime( float transitionSamples );
	void setTarget( float targetValue );
	void setValueInstant( float targetValue );
	float getInstantValue();
	float getTargetValue();
	float getNext();
	bool isDone()
	{
		return dv == 0.0f;
	}

private:
	float dv;
	float currentValue_;
	float targetValue_;
	float inverseTransitionTime_;
	float transitionTime_;
};

class RampGeneratorAdaptive
{
	float adaptiveHi_;
	float adaptiveLo_;

public:
	RampGeneratorAdaptive() :
		currentValue_((std::numeric_limits<float>::max)())
		, dv(0.0f)
	{}

	void Init( float sampleRate)
	{
		inverseTransitionTime_ = 1.0f / ( sampleRate * 0.015f ); // 15ms default.

		adaptiveLo_ = 1.0f / ( sampleRate * 0.050f ); // 50ms max.
		adaptiveHi_ = 1.0f / ( sampleRate * 0.001f ); // 1ms min.
	}
	void setTarget(float targetValue)
	{
		if( currentValue_ == ( std::numeric_limits<float>::max )( ) )
		{
			currentValue_ = targetValue_ = targetValue;
			dv = 0.0f;
			return;
		}

		if( currentValue_ == targetValue_ )
		{
			if( inverseTransitionTime_ < adaptiveHi_ )
			{
				inverseTransitionTime_ *= 1.05f; // slower 'decay', kind of peak follower.
			}
		}
		else
		{
			if( inverseTransitionTime_ > adaptiveLo_ )
			{
				inverseTransitionTime_ *= 0.9f;
			}
		}

		targetValue_ = targetValue;
		dv = ( targetValue_ - currentValue_ ) * inverseTransitionTime_;
	}

	void setValueInstant(float targetValue)
	{
		currentValue_ = targetValue_ = targetValue;
		dv = 0;
	}

	float getInstantValue() const
	{
		return currentValue_;
	}

	float getTargetValue() const
	{
		return targetValue_;
	}

	inline float getNext()
	{
		currentValue_ += dv;

		if( dv > 0 )
		{
			if( currentValue_ >= targetValue_ )
			{
				currentValue_ = targetValue_;
				dv = 0.0f;
			}
		}
		else
		{
			if( currentValue_ <= targetValue_ )
			{
				currentValue_ = targetValue_;
				dv = 0.0f;
			}
		}

		return currentValue_;
	}

	inline bool isDone()
	{
		return dv == 0.0f;
	}

	void jumpToTarget()
	{
		setValueInstant(targetValue_);
	}
private:
	float dv;
	float currentValue_;
	float targetValue_;
	float inverseTransitionTime_;
};

#endif // .H INCLUDED
