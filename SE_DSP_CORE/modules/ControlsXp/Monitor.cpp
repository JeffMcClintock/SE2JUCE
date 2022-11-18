#include "MidiMonitor.h"
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

using namespace gmpi;

class Monitor : public MonitorBase
{
	IntInPin pinChannel;
	AudioInPin pinSignalin;
	AudioOutPin pinSignalOut;
	FloatInPin pinPrintTime;
	BoolInPin pinPrintInputStatChanges;
	BoolInPin pinPrintTickProcessing;
	IntInPin pinDispMode;
	BoolInPin pinUseTraceWindow;
	BoolInPin pinEngineOverloadtest;

	bool in_status = false;
	float prev_static_input = -99999999.12533f; // unlikely value.
	float last_input = -99999999.12533f;
	int denormal_count = 0;
	int last_denormal_count = 0;

	int delay;
	int delayCounter;

public:
	Monitor()
	{
		initializePin( pinMIDIIn );
		initializePin( pinChannel );
		initializePin( pinSignalin );
		initializePin( pinSignalOut );
		initializePin( pinPrintTime );
		initializePin( pinPrintInputStatChanges );
		initializePin(pinPrintTickProcessing );
		initializePin( pinDispMode );
		initializePin( pinUseTraceWindow );
		initializePin( pinEngineOverloadtest );
		initializePin(pinDispOut);
	}

	void onGraphStart() override
	{
		MonitorBase::onGraphStart();
		setSubProcess(&Monitor::subProcess);

		delay = (int)getSampleRate() / 10; // update 10Hz

		/*
			float print_interval_d = (float)print_time * SampleRate();
	print_interval = (int) print_interval_d;

	if( print_interval > 0 )
	{
		RUN_AT( SampleClock() + print_interval, &ug_monitor::printout );
	}

		*/

	}

	void subProcess( int sampleFrames )
	{
		// get pointers to in/output buffers.
		auto signalin = getBuffer(pinSignalin);
		auto signalOut = getBuffer(pinSignalOut);

		bool statusError = false;
		if (!pinSignalin.isStreaming())
		{
			for (int s = 0 ; s < sampleFrames; ++s)
			{
				statusError |= signalin[0] != signalin[s];
			}
		}

		samplesPassed += sampleFrames;

		delayCounter -= sampleFrames;
		if (delayCounter < 0)
		{
			printout();
			delayCounter = delay;
		}

		if (statusError)
		{
			Print(L"Status ERR!!");
		}
	}

	void printout()
	{
		if (pinPrintTime.getValue() <= 0.0f)
			return;

		std::wstring s;

		switch (pinDispMode.getValue())
		{
		case 0:	// func list
			break;

		case 1:	// func changes
			;
			break;

		case 2:	// input level
		{
			float new_input = pinSignalin.getValue(0);

			if (new_input != last_input)
			{
				last_input = new_input;
				//				s.Format((L"%f Volts\n"), last_input * 10.0 );
				std::wostringstream oss;
				oss << (last_input * 10.0) << L"%f Volts\n";
				Print(oss.str());
				//				Print(s);
			}
		}
		break;

		case 3: // denormal_count
			if (denormal_count != last_denormal_count)
			{
				last_denormal_count = denormal_count;
				//s.Format((L"%d Denormals\n"), last_denormal_count );
				//Print(s);
				std::wostringstream oss;
				oss << last_denormal_count << L" Denormals\n";
				Print(oss.str());
			}

			break;
		};
	}

	std::wstring GetStatString(bool status)
	{
		if (status)
			return L"ST_RUN";

		return L"ST_STATIC";
	}

	void Print( std::wstring newMessage)
	{
		lines.push_back(newMessage);
		if (lines.size() > 14)
			lines.pop_front();

		std::wstring printout;
		for (auto& s : lines)
			printout += s + +L"\n";

		//		msg = newMessage /*+ timeText */+ L"\n" + msg + L"\n";

		pinDispOut.setValue(printout, 0 );
	}

	void PrintTimeStamp()
	{
		std::wstring msg;
		/*
		int seconds = SampleClock() / (int)SampleRate();
		int samples = SampleClock() % (int)SampleRate();

		double millisecs = 1000 * (double) samples / SampleRate();

		msg.Format((L"%3d.%03d sec (%05d samps) "), seconds, (int) millisecs, samples );
		*/
		int64_t seconds = samplesPassed / (int64_t) getSampleRate();
		int64_t minutes = seconds / 60;
		seconds %= 60;
		int64_t samples = samplesPassed % (int64_t)getSampleRate();

		std::wostringstream oss;
		oss << minutes << L":" << std::setw(2) << seconds << L"+" << std::setw(2) << std::setw(5) << samples << L" ";

		msg = oss.str();
		Print(msg);
	}

	virtual void onSetPins() override
	{
		// Check which pins are updated.
		if( pinSignalin.isUpdated() )
		{
			auto prev_status = in_status;
			in_status = pinSignalin.isStreaming();

			// for purpose of detecting errors, st_stop noted as st_one_off
			//if (in_status == ST_STOP)
			//	in_status = ST_ONE_OFF;

			if (pinPrintInputStatChanges.getValue())
			{
				PrintTimeStamp();
				//			std::wstring msg;
				float in_val = pinSignalin.getValue();
				//			msg.Format((L"Inp %s.   %.3g V"), GetStatString(p_state), in_val * 10.f);
				std::wostringstream oss;
				oss << L"Inp " << GetStatString(in_status) << ".   " << (in_val * 10.f) << " V";
				Print(oss.str());
				//		_RPT1(_CRT_WARN, " input = %.10f\n", GetPlugInputVal( PN_INPUT1 ) * 10.f );
			}

			// Detect unnesc stat changes.
			if (in_status == false)//ST_ONE_OFF)
			{
				float in_val = pinSignalin.getValue();

				if (!prev_status && prev_static_input && in_val == prev_static_input)
				{
					PrintTimeStamp();
					std::wostringstream oss;
					oss << L"Unnesc ST_STATIC. Input " << (in_val * 10.f) << " V ->  " << (in_val * 10.f) << " V";
					Print(oss.str());
				}

				prev_static_input = in_val;
			}

			pinSignalOut.setStreaming(pinSignalin.isStreaming());
		}

		if( pinPrintTime.isUpdated() )
		{
		}
		if( pinPrintInputStatChanges.isUpdated() )
		{
		}
		if(pinPrintTickProcessing.isUpdated() )
		{
		}
		if( pinDispMode.isUpdated() )
		{
		}
		if( pinUseTraceWindow.isUpdated() )
		{
		}
		if( pinEngineOverloadtest.isUpdated() )
		{
			if (pinEngineOverloadtest.getValue())
				//				Sleep(300);
				std::this_thread::sleep_for(300ms);
		}

		// Set state of output audio pins.
		//pinSignalOut.setStreaming(true);

		// Set processing method.
		setSubProcess(&Monitor::subProcess);

		// Set sleep mode (optional).
		setSleep(false);
	}
};

namespace
{
	auto r = Register<Monitor>::withId(L"SE Monitor");
}
