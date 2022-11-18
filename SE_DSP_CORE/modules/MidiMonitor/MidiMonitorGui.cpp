#include <sstream>
#include <iomanip>
#include "./MidiMonitorGui.h"
#include "Drawing.h"
#include "../se_sdk3/mp_midi.h"
#include "../se_sdk3/it_enum_list.h"
#include "../shared/unicode_conversion.h"

using namespace std;
using namespace gmpi;
using namespace GmpiDrawing;
using namespace GmpiMidi;
using namespace GmpiMidiHdProtocol;
using namespace JmUnicodeConversions;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, MidiMonitorGui, L"EA MIDI Monitor" );

MidiMonitorGui::MidiMonitorGui()
{
	init_controller_descriptions();
}

int32_t MidiMonitorGui::receiveMessageFromAudio(int32_t id, int32_t size, const void* messageDatav)
{
	const unsigned char* messageData = (const unsigned char*) messageDatav;

	int stat, byte1, byte2, chan; // 3 bytes of MIDI message
	chan = messageData[0] & 0x0f;
	stat = messageData[0] & 0xf0;
	bool is_system_msg = (stat & MIDI_SystemMessage) == MIDI_SystemMessage;
	byte1 = messageData[1] & 0x7F;
	byte2 = messageData[2] & 0x7F;

	std::string msg;
	//	if( chan == channel || channel == -1 || is_system_msg ) //-1 = All channels
	{
		//			msg.Format("MIDI MONITOR :%2d :", chan );
		// Note offs can be note_on vel=0
		if (byte2 == 0 && stat == MIDI_NoteOn)
			stat = MIDI_NoteOff;

		std::ostringstream oss;

		switch (stat)
		{
		case MIDI_NoteOn:
			//			msg.Format(("Note On  (%3d,%3d)"), byte1, byte2 );
			oss << "Note On  (" << byte1 << "," << byte2 << ")";
			msg = oss.str();
			break;

		case MIDI_NoteOff:
			// msg.Format(("Note Off (%3d,%3d)"), byte1, byte2 );
			oss << "Note Off  (" << byte1 << "," << byte2 << ")";
			msg = oss.str();
			break;

		case MIDI_PolyAfterTouch:
			oss << "Aftertouch  (" << byte1 << "," << byte2 << ")";
			msg = oss.str();
			break;

		case MIDI_ControlChange:
		{
			if (byte1 >= 0 && byte1 < 128)
			{
				auto desc = CONTROLLER_DESCRIPTION[byte1];

				// remove CC number from description. TRim to 20 char.
				size_t p = desc.find(L'-');

				if (p != string::npos)
				{
//					desc = Left(Right(desc, desc.size() - p), 20);
					desc = desc.substr(p);
				}

				oss << "CC" << byte1 << ":" << setw(3) << right << byte2 << " " << desc;
				msg = oss.str();
			}
			else
			{
				msg = ("Cntlr: ILEGAL CNTRLR");
			}
		}
		break;

		case MIDI_ProgramChange:
			//msg.Format(("Prg change (%3d)"), byte1 + 1 );
			oss << "Prg change  (" << (byte1 + 1) << ")";
			msg = oss.str();
			break;

		case MIDI_ChannelPressue:
			//msg.Format(("Channel Pressure (%3d)"), byte1);
			oss << "Channel Pressure  (" << byte1 << ")";
			msg = oss.str();
			break;

		case MIDI_PitchBend:
		{
			int val = (byte2 << 7) + byte1 - 8192;
			float normalized = val / 8192.0f;
			//		msg.Format(("Bender (%.3d) %.3f"), val, normalized );
			oss << "Bender  (" << val << ") " << normalized;
			msg = oss.str();
		}
		break;

		case 0:
			msg = ("Zeros bytes!");
			break;

		case MIDI_SystemMessage:
		{
			switch (messageData[0])
			{
			case MIDI_SystemMessage:
			{
				std::ostringstream oss;

				// Tuning?
				if ((messageData[1] == MIDI_Universal_Realtime || messageData[1] == MIDI_Universal_NonRealtime) &&
					messageData[3] == MIDI_Sub_Id_Tuning_Standard)
				{
					int tuningCount = 0;
					int tuningDataPosition = 0;
					bool entireBank = false;
					bool retunePlayingNotes = true;

					switch (messageData[4])
					{
					case 1:
						/*
						[BULK TUNING DUMP]
						A bulk tuning dump comprises frequency data in the 3-byte format outlined in section 1, for all 128 MIDI key numbers, in order from note 0 (earliest sent) to note 127 (latest sent), enclosed by a system exclusive header and tail. This message is sent by the receiving instrument in response to a tuning dump request.

						F0 7E <device ID> 08 01 tt <tuning name> [xx yy zz] ... chksum F7

						F0 7E  Universal Non-Real Time SysEx header
						<device ID>  ID of responding device
						08  sub-ID#1 (MIDI Tuning)
						01  sub-ID#2 (bulk dump reply)
						tt  tuning program number (0 – 127)
						<tuning name>  16 ASCII characters
						[xx yy zz]  frequency data for one note (repeated 128 times)
						chksum  cchecksum (XOR of 7E <device ID> nn tt <388 bytes>)
						F7  EOX

						*/
					{
						tuningCount = 128;
						entireBank = true;
						tuningDataPosition = 22;
					}
					break;

					case 2: // SINGLE NOTE TUNING CHANGE (REAL-TIME)
					{
						tuningCount = messageData[6];
						tuningDataPosition = 7;
					}
					break;

					case 3: // BULK TUNING DUMP REQUEST (BANK)
						break;

					case 4: // KEY-BASED TUNING DUMP
					{
						tuningCount = 128;
						entireBank = true;
						tuningDataPosition = 23;
					}
					break;

					case 7: // SINGLE NOTE TUNING CHANGE (REAL/NON REAL-TIME) (BANK)
					{
						/*
						This is identical to the current SINGLE NOTE TUNING CHANGE (REAL-TIME)
						except for the addition of the bank select byte (bb) and the change to
						a NON REAL -TIME header. This message allows the sender to specify a new
						tuning change that will NOT update the currently sounding notes.
						*/
						tuningCount = messageData[7];
						tuningDataPosition = 8;
						retunePlayingNotes = false;
					}
					break;
					}

					if (tuningCount > 1)
					{
						oss << "TUNE (BULK)";
					}
					else
					{
						const unsigned char* tuneEntry = &(messageData[tuningDataPosition]);
						int midiKeyNumber = *tuneEntry++;
						int tune = (tuneEntry[0] << 14) + (tuneEntry[1] << 7) + tuneEntry[2];
						float semitone = tune / (float)0x4000;

						oss << "TUNE: k" << midiKeyNumber << " " << semitone;
					}
				}
				else
				{
					int c = min(size, 10);
					oss << "SYSEX";

					for (int i = 0; i < c; i++)
					{
						oss << " " << std::hex << (int)messageData[i];
					}

					if (c < size)
						oss << ("...");
				}

				msg = oss.str();
			}
			break;

			case 0xF1:
				oss << "F1 " << std::hex << messageData[1] << " (MTC QF)";
				msg = oss.str();
				break;

			case 0xF2:
				//msg.Format(("F2 %X%X (SPP)"), messageData[1], messageData[2]);
				oss << "F2 " << std::hex << messageData[1] << std::hex << messageData[2] << " (SPP)";
				msg = oss.str();
				break;

			case 0xF3:
				//msg.Format(("F3 %X (SS)"), messageData[1]);
				oss << "F3 " << std::hex << messageData[1] << " (SS)";
				msg = oss.str();
				break;

			case 0xF6:
				msg = "F6 - Tune Request";
				break;

			case 0xF8:
				msg = "F8 - CLOCK";
				break;

			case 0xFA:
				msg = "FA - CLOCK - Start";
				break;

			case 0xFB:
				msg = "FB - CLOCK - Continue";
				break;

			case 0xFC:
				msg = "FC - CLOCK - Stop";
				break;

			case 0xFE:
				msg = "FE - Active Sensing";
				break;

			case 0xFF:
				msg = "FF - System Reset";
				break;

			default:
				//msg.Format(("SYSTEM (%x)"), messageData[0]);
				oss << "SYSTEM (" << std::hex << messageData[1] << ")";
				msg = oss.str();
			};
		}
		break;

		default:
			//msg.Format(("Byte = %02x %02x %02x"), stat, byte1, byte2 );
			oss << "Byte = " << std::hex << stat << " " << std::hex << byte1 << " " << std::hex << byte2;
			msg = oss.str();
		}

		if (!is_system_msg)
		{
			//std::wstring chan_st;
			//chan_st.Format(("C%2d "),chan+1);
			std::ostringstream oss2;
			oss2 << "C" << (chan + 1) << " ";
			// msg = oss.str();
			msg = oss2.str() + msg;
		}

		// add time
		/*
		timestamp_t seconds = SampleClock() / (int)SampleRate();
		timestamp_t minutes = seconds / 60;
		seconds %= 60;
		timestamp_t samples = SampleClock() % (timestamp_t)SampleRate();
		std::wstring time_st;
		//		time_st.Format(("%01d:%02d+%05d "), minutes, seconds, samples);
		msg = time_st + msg;
		*/
		int lineLength = 40;
/*
		if (msg.size() > lineLength)
		{
			msg = Left(msg, lineLength) + string("\n                            ") + Right(msg, msg.size() - lineLength);
		}
*/

		msg += "\n";

		messages.push_back(msg);

		const int MAX_MESSAGES = 15;
		if (messages.size() > MAX_MESSAGES)
		{
			messages.pop_front();
		}

		invalidateRect(); // redraw
	}

	return gmpi::MP_OK;
}

// handle pin updates.
int32_t MidiMonitorGui::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext )
{
	Graphics g(drawingContext);

	auto textFormat = GetGraphicsFactory().CreateTextFormat();
	auto brush = g.CreateSolidColorBrush(Color::Red);

	float y = 0;
	for (auto& m : messages)
	{
		g.DrawTextU(m, textFormat, 0.0f, y, brush);

		y += 14.0f;
	}

	return gmpi::MP_OK;
}

const wchar_t* CONTROLLER_ENUM_LIST =

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

L"84,85,86,87,88,89,90,"

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

L"127 - Polyphonic";


void MidiMonitorGui::init_controller_descriptions(void)
{
	if (CONTROLLER_DESCRIPTION.size() == 128)
		return;

	CONTROLLER_DESCRIPTION.assign(128, "");

	it_enum_list itr(CONTROLLER_ENUM_LIST);

	for (itr.First(); !itr.IsDone(); itr.Next())
	{
		auto e = itr.CurrentItem();
		if (e->value >= 0) // ignore "none"
		{
			CONTROLLER_DESCRIPTION[e->value] = WStringToUtf8( e->text );
		}
	}

	// provide default descriptions
	for (int i = 0; i < 128; i++)
	{
		if (CONTROLLER_DESCRIPTION[i].empty())
		{
			std::ostringstream oss;
			oss << L"CC" << setw(3) << i;
			CONTROLLER_DESCRIPTION[i] = oss.str();
		}
	}
}