#include "./VoltMeter.h"
#include <math.h>

SE_DECLARE_INIT_STATIC_FILE(VoltMeter)
REGISTER_PLUGIN2 ( VoltMeter, L"SE Volt Meter" );

VoltMeter::VoltMeter( ) :
	count(0)
	, m_value(0)
{
	// Register pins.
	initializePin( pinSignalin );
	initializePin( pinMode );
	initializePin( pinUpdateRate );
	initializePin( pinpatchValue );
}

void VoltMeter::subProcess(int sampleFrames)
{
	// get pointers to in/output buffers.
	float* signalin = getBuffer(pinSignalin);

	float sum = 0.f;
	for (int s = sampleFrames; s > 0; --s)
	{
		sum += *signalin++;
	}

	// average out current value.
	const float c = 0.950f;
	sum /= (float)sampleFrames;
	m_value = sum + c * (m_value - sum);

	count -= sampleFrames;
	if (count < 0)
	{
		UpdateGui();
	}
}

void VoltMeter::subProcessAc(int sampleFrames)
{
	// get pointers to in/output buffers.
	float* signalin = getBuffer(pinSignalin);

	float sum = 0.f;
	for (int s = sampleFrames; s > 0; --s)
	{
		sum += *signalin * *signalin; // squared

		// Increment buffer pointers.
		++signalin;
	}

	// average out current value
	m_value = (m_value * (1000.f - sampleFrames) + sum) * 0.001f;

	count -= sampleFrames;
	if (count < 0)
	{
		UpdateGui();
	}
}

void VoltMeter::onSetPins()
{
	// Set processing method.
	if(pinMode == 0)
		setSubProcess(&VoltMeter::subProcess);
	else
		setSubProcess(&VoltMeter::subProcessAc);

	// Set sleep mode (optional).
	setSleep(false);
}

void VoltMeter::UpdateGui()
{
	if (!isfinite(m_value))
	{
		m_value = 0.f; // else gets stuck on infinite
	}

	if (last_value == m_value && !pinSignalin.isStreaming())
		setSleep(true);

	last_value = m_value;

	pinpatchValue.setValue(m_value * 10.0f, 0);

	int update_rate = pinUpdateRate;
	// older version used 0 to mean 2Hz, quick fix
	if (update_rate <= 0)
		update_rate = 2;

	count = getSampleRate() / update_rate;
}

