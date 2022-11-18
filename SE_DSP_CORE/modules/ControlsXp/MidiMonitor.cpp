#include "MidiMonitor.h"

using namespace std;
using namespace gmpi;

const wchar_t* MonitorBase::CONTROLLER_ENUM_LIST =
{
L"<none>=-1,"
L"0 - Bank Select,"
L"1 - Mod Wheel,"
L"2 - Breath,"
L"3,"
L"4 - Foot pedal,"
L"5 - Portamento Time,"
L"6 - Data Entry Slider,"
L"7 - Volume,"
L"8 - Balance,"
L"9,"
L"10 - Pan,"
L"11 - Expression,"
L"12 - Effect 1 Control,"
L"13 - Effect 2 Control,"
L"14,15,"
L"16 - General Purpose Slider,"
L"17 - General Purpose Slider,"
L"18 - General Purpose Slider,"
L"19 - General Purpose Slider,"
L"20,21,22,23,24,25,26,27,28,29,30,31,32,"
L"33 - Mod Wheel LSB=33,"
L"34 - Breath LSB,"
L"35 - LSB,"
L"36 - Foot LSB,"
L"37 - Portamento Time LSB,"
L"38 - Data Entry Slider LSB,"
L"39 - Volume LSB,"
L"40 - Balance LSB,41,"
L"42 - Pan LSB=42,"
L"43 - Expression LSB,"
L"44 - Effect 1 LSB,"
L"45 - Effect 2 LSB,46,47,"
L"48 - General Purpose LSB=48,"
L"49 - General Purpose LSB,"
L"50 - General Purpose LSB,"
L"51 - General Purpose LSB,"
L"52,53,54,55,56,57,58,59,60,61,62,63,"
L"64 - Hold Pedal=64,"
L"65 - Portamento,"
L"66 - Sustenuto,"
L"67 - Soft pedal,"
L"68 - Legato,"
L"69 - Hold 2 Pedal,"
L"70 - Sound Variation,"
L"71 - Sound Timbre,"
L"72 - Release Time,"
L"73 - Attack Time,"
L"74 - Sound Brightness,"
L"75 - Sound Control,"
L"76 - Sound Control,"
L"77 - Sound Control,"
L"78 - Sound Control,"
L"79 - Sound Control,"
L"80 - General Purpose button,"
L"81 - General Purpose button,"
L"82 - General Purpose button,"
L"83 - General Purpose button,"
L"84,85,86,87,"
L"88 - Hires Velocity Prefix,"
L"89,90,"
L"91 - Effects Level,"
L"92 - Tremelo Level,"
L"93 - Chorus Level,"
L"94 - Celeste level,"
L"95 - phaser level,"
L"96 - Data button Increment,"
L"97 - data button decrement,"
L"98 - NRPN LO,"
L"98/99 - NRPN HI," //. Sets parameter for data button and data entry slider controllers
L"100 - RPN LO,"
L"100/101 - RPN HI,"
//		- 0000 Pitch bend range (course = semitones, fine = cents)
//		- 0001 Master fine tune (cents) 0x2000 = standard
//		- 0002 Mater fine tune semitones 0x40 = standard (fine byte ignored)
//		- 3FFF RPN Reset
L"102=102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,"
L"120 - All sound off,"
L"121 - All controllers off,"
L"122 - local keybord on/off,"
L"123 - All notes off,"
L"124 - Omni Off,"
L"125 - Omni on,"
L"126 - Monophonic,"
L"127 - Polyphonic"
};

class MidiMonitor : public MonitorBase
{
public:
	MidiMonitor()
	{
		initializePin(pinMIDIIn);
		initializePin(pinDispOut);
	}
};

namespace
{
	auto r = Register<MidiMonitor>::withId(L"SE Midi Monitor");
}
