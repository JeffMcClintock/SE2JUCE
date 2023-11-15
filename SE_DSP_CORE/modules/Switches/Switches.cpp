#include "./Switches.h"
#include "mp_midi.h"
SE_DECLARE_INIT_STATIC_FILE(Switch)

typedef SwitchManyToOne<float> SwitchFloat;
typedef SwitchManyToOne<int> SwitchInt;
typedef SwitchManyToOne<bool> SwitchBool;
typedef SwitchManyToOne<MpBlob> SwitchBlob;
typedef SwitchManyToOne<std::wstring> SwitchText;

REGISTER_PLUGIN2(SwitchFloat, L"SE Switch > (Float)");
REGISTER_PLUGIN2(SwitchInt, L"SE Switch > (Int)");
REGISTER_PLUGIN2(SwitchBool, L"SE Switch > (Bool)");
REGISTER_PLUGIN2(SwitchBlob, L"SE Switch > (Blob)");
REGISTER_PLUGIN2(SwitchText, L"SE Switch > (Text)");

typedef SwitchOneToMany<float> SwitchFloatToMany;
typedef SwitchOneToMany<int> SwitchIntToMany;
typedef SwitchOneToMany<bool> SwitchBoolToMany;
typedef SwitchOneToMany<MpBlob> SwitchBlobToMany;
typedef SwitchOneToMany<std::wstring> SwitchTextToMany;

REGISTER_PLUGIN2(SwitchFloatToMany, L"SE Switch < (Float)");
REGISTER_PLUGIN2(SwitchIntToMany, L"SE Switch < (Int)");
REGISTER_PLUGIN2(SwitchBoolToMany, L"SE Switch < (Bool)");
REGISTER_PLUGIN2(SwitchBlobToMany, L"SE Switch < (Blob)");
REGISTER_PLUGIN2(SwitchTextToMany, L"SE Switch < (Text)");

class SwitchMidiToOne : public MpBase2
{
public:
	SwitchMidiToOne()
	{
		// Register pins.
		initializePin(pinChoice);
		initializePin(pinOutput);
	}
	int32_t MP_STDCALL open() override
	{
		// initialise auto-duplicate pins.
		const int numOtherPins = 2;
		gmpi_sdk::mp_shared_ptr<gmpi::IMpPinIterator> it;
		if (gmpi::MP_OK == getHost()->createPinIterator(it.getAddressOf()))
		{
			int32_t inPinCount;
			it->getCount(inPinCount);
			inPinCount -= numOtherPins; // calc number of input values (not counting "choice" anf "output")

			pinInputs.assign(inPinCount, {});

			for (auto& p : pinInputs)
			{
				initializePin(p);
			}
		}

		return MpBase2::open();
	}

	void onSetPins() override
	{
		if (prevOutput > -1)
		{
			const auto out = gmpi::midi_2_0::makeController(
				123, // all notes off
				1.0f
			);

			pinOutput.send(out.m); // send all note off to previous output.
		}

		prevOutput = pinChoice;
	}

	void onMidiMessage(int pin, const unsigned char* midiMessage, int size) override
	{
		if (pin - 2 == pinChoice.getValue())
			pinOutput.send(midiMessage, size);
	}

private:
	IntInPin pinChoice;
	MidiOutPin pinOutput;
	std::vector< MidiInPin > pinInputs;
	int prevOutput = -1;
};

class SwitchOneToMidi : public MpBase2
{
public:
	SwitchOneToMidi()
	{
		// Register pins.
		initializePin(pinChoice);
		initializePin(pinInput);
	}

	int32_t MP_STDCALL open() override
	{
		// initialise auto-duplicate pins.
		const int numOtherPins = 2;

		gmpi_sdk::mp_shared_ptr<gmpi::IMpPinIterator> it;
		if (gmpi::MP_OK == getHost()->createPinIterator(it.getAddressOf()))
		{
			int32_t inPinCount;
			it->getCount(inPinCount);
			inPinCount -= numOtherPins; // calc number of input values (not counting "choice" anf "output")

			pinOutputs.assign(inPinCount, {});

			for (auto& p : pinOutputs)
			{
				initializePin(p);
			}
		}

		return MpBase2::open();
	}

	void onSetPins() override
	{
		if (prevOutput > -1)
		{
			const auto out = gmpi::midi_2_0::makeController(
				123, // all notes off
				1.0f
			);

			pinOutputs[prevOutput].send(out.m); // send all note off to previous output.
		}

		prevOutput = pinChoice;
	}

	void onMidiMessage(int pin, const unsigned char* midiMessage, int size) override
	{
		const auto choice = pinChoice.getValue();
		if (choice >= 0 && choice < pinOutputs.size()) // cope with zero outputs situation.
			pinOutputs[choice].send(midiMessage, size);
	}

private:
	IntInPin pinChoice;
	MidiInPin pinInput;
	std::vector< MidiOutPin > pinOutputs;
	int prevOutput = -1;
};

REGISTER_PLUGIN2(SwitchMidiToOne, L"SE Switch > (MIDI)");
REGISTER_PLUGIN2(SwitchOneToMidi, L"SE Switch < (MIDI)");
