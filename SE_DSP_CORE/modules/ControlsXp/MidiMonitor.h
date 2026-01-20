#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <list>
#include "../se_sdk3/mp_sdk_audio.h"
#include "../se_sdk3/mp_midi.h"
#include "../se_sdk3/it_enum_list.h"

using namespace gmpi;
using namespace GmpiMidi;

class MonitorBase : public MpBase2
{
protected:
	MidiInPin pinMIDIIn;
	StringOutPin pinDispOut;

	int64_t samplesPassed;
	float secondsPassed;

	const int lineLength = 40;
	std::list<std::wstring> lines;
	bool noteStatus[GmpiMidi::MidiChannels::MIDI_ChannelCount][GmpiMidi::MidiLimits::MIDI_KeyCount];
	int64_t noteStatusTimestamps[GmpiMidi::MidiChannels::MIDI_ChannelCount][GmpiMidi::MidiLimits::MIDI_KeyCount];
	std::vector<std::wstring> CONTROLLER_DESCRIPTION;
	static const wchar_t* CONTROLLER_ENUM_LIST;

	float midi2NoteTune[256];
	uint8_t midi2NoteToKey[256];

public:
	MonitorBase()
	{
		memset(noteStatus, 0, sizeof(noteStatus));
		memset(noteStatusTimestamps, 0, sizeof(noteStatusTimestamps));
		samplesPassed = 0;
		lines.push_back(L"Sound ON");

		{
			CONTROLLER_DESCRIPTION.assign(128, L"");

			std::wstring enumList(CONTROLLER_ENUM_LIST);
			it_enum_list itr(enumList);

			for (itr.First(); !itr.IsDone(); itr.Next())
			{
				enum_entry* e = itr.CurrentItem();
				if (e->value >= 0) // ignore "none"
				{
					CONTROLLER_DESCRIPTION[e->value] = e->text;
				}
			}

			// provide default descriptions
			for (int i = 0; i < 128; i++)
			{
				if (CONTROLLER_DESCRIPTION[i].empty())
				{
					std::wostringstream oss;
					oss << L"CC" << std::setw(3) << i;
					CONTROLLER_DESCRIPTION[i] = oss.str();
				}
			}
		}

		for (size_t i = 0; i < std::size(midi2NoteToKey); ++i)
		{
			midi2NoteToKey[i] = static_cast<uint8_t>(i);
			midi2NoteTune[i] = static_cast<float>(i);
		}
	}

	void subProcess(int sampleFrames)
	{
		samplesPassed += sampleFrames;
	}

	void onGraphStart() override
	{
		MpBase2::onGraphStart();

		setSubProcess(&MonitorBase::subProcess);

		setSleep(false);

		pinDispOut = lines.back();
	}
	/* never called, no non-midi pins
	virtual void onSetPins() override
	{
	}
	*/

	void onMidiMessage(int pin, const unsigned char* midiMessage, int size) override
	{
		midi::message_view msg((const uint8_t*) midiMessage, size);

		std::wstring newMessage;

		if (gmpi::midi_2_0::isMidi2Message(msg))
		{
			std::wostringstream oss;

			const auto header = gmpi::midi_2_0::decodeHeader(msg);

			oss << L"2C" << (header.channel + 1) << " ";

			if (header.messageType == gmpi::midi_2_0::ChannelVoice64)
			{
				// Monophonic Controllers.
				switch (header.status)
				{
				case gmpi::midi_2_0::ControlChange:
				{
					const auto controller = gmpi::midi_2_0::decodeController(msg);

					std::wstring desc;
					
					if (controller.type < 128)
					{
						desc = CONTROLLER_DESCRIPTION[controller.type];
					}
					else
					{
						desc = std::to_wstring(controller.type);
					}

					// remove CC number from description. Trim to 20 char.
					size_t p = desc.find(L'-');

					if (p != std::string::npos)
					{
						desc = desc.substr(p, (std::min)(desc.size() - p, (size_t)20));
					}
					
					const auto percentage = static_cast<int>(floorf(0.5f + controller.value * 100.0f));

//					oss << L"CC" << controller.type << ": " << std::fixed << std::setprecision(3) << std::showpos << percentage << "% " << desc;

					oss << L"CC " << desc
						<< std::fixed << std::setprecision(3) << std::showpos << std::setw(5) << std::setfill(L' ')
						<< percentage << "%";
				}
				break;

				case gmpi::midi_2_0::RPN:
                {
                    const auto rpn = gmpi::midi_2_0::decodeRpn(msg);

                    oss << L"RPN " << rpn.rpn << " :" << rpn.value;
                }
				break;

				case gmpi::midi_2_0::NoteOn:
				{
					const auto note = gmpi::midi_2_0::decodeNote(msg);

					const auto keyNumber = static_cast<uint8_t>(static_cast<int>(floorf(0.5f + midi2NoteTune[note.noteNumber])) & 0x7f);
					midi2NoteToKey[note.noteNumber] = keyNumber;

					float tempPitch = midi2NoteTune[note.noteNumber];
					if (gmpi::midi_2_0::attribute_type::Pitch == note.attributeType)
					{
						tempPitch = note.attributeValue;
					}

					oss << L"Note On  (" << note.noteNumber << ", " << std::setprecision(2) << note.velocity << ")";
					// display pitch if not default for that key
					if (tempPitch != (float)note.noteNumber)
					{
						oss << " P" << tempPitch;
					}
				}
				break;

				case gmpi::midi_2_0::NoteOff:
				{
					const auto note = gmpi::midi_2_0::decodeNote(msg);

					//oss.precision(2);
					//oss.setf(std::ios::fixed, std::ios::floatfield);
					oss << L"Note Off  (" << note.noteNumber << ", " << std::setprecision(2) << note.velocity << ")";

					// reset pitch of note number.
					midi2NoteToKey[note.noteNumber] = note.noteNumber;
				}
				break;

				case gmpi::midi_2_0::PolyControlChange:
				{
					const auto polyController = gmpi::midi_2_0::decodePolyController(msg);

					if (polyController.type == gmpi::midi_2_0::PolyPitch)
					{
						const auto semitones = gmpi::midi_2_0::decodeNotePitch(msg);
						midi2NoteTune[polyController.noteNumber] = semitones;
					}

//					oss << L"PolyCC (" << polyController.type << L", " << polyController.noteNumber << L", " << std::setw(3) << polyController.value << L")";
     
                    std::wstring desc;
					
					if (polyController.type < 128)
					{
						desc = CONTROLLER_DESCRIPTION[polyController.type];
					}
					else
					{
						desc = std::to_wstring(polyController.type);
					}

					// remove CC number from description. Trim to 20 char.
					size_t p = desc.find(L'-');

					if (p != std::string::npos)
					{
						desc = desc.substr(p, (std::min)(desc.size() - p, (size_t)20));
					}
     
                    const auto percentage = static_cast<int>(floorf(0.5f + polyController.value * 100.0f));

//					oss << L"CC" << controller.type << ": " << std::fixed << std::setprecision(3) << std::showpos << percentage << "% " << desc;

					oss << L"PolyCC " << polyController.type << desc << L" N" << polyController.noteNumber << L" "
						<< std::fixed << std::setprecision(3) << std::showpos << std::setw(5) << std::setfill(L' ')
						<< percentage << "%";

				}
				break;

				case gmpi::midi_2_0::PolyBender:
					oss << L"PolyBender";
				break;

				case gmpi::midi_2_0::PolyAfterTouch:
				{
					const auto aftertouch = gmpi::midi_2_0::decodePolyController(msg);
					oss << L"PolyAfterTouch  (" << aftertouch.noteNumber << "," << aftertouch.value << ")";
				}
				break;

				case gmpi::midi_2_0::PolyNoteManagement:
					oss << L"PolyNoteManagement";
				break;

				case gmpi::midi_2_0::ProgramChange:
				{
					// Note: banks and programs number from 1
					if (msg[3] & 1)
					{
						oss << L"BankChang " << 1 + ((msg[6] << 7) | msg[7]);
					}
					oss << L"ProgramChang " << 1 + msg[4];
				}
				break;

				case gmpi::midi_2_0::ChannelPressue:
                {
                    const auto chanPressure = gmpi::midi_2_0::decodeController(msg);
                    oss << L"ChannelPressue " << chanPressure.value;
                }
				break;

				case gmpi::midi_2_0::PitchBend:
				{
					const auto normalized = gmpi::midi_2_0::decodeController(msg).value;
					const auto bend = floorf(0.5f + normalized * 200.0f - 100.f);

					oss << L"PitchBend "
						<< std::fixed << std::setprecision(3) << std::showpos << std::setw(5) << std::setfill(L' ')
						<< static_cast<int>(bend) << "%";
				}
				break;

				}
			}

			if (oss.str().empty())
			{
                oss << std::setprecision(2) << std::hex;
                for(int i = 0 ; i < (std::min)(8, (int) msg.size()) ; ++i)
                {
                    oss << (int) msg[i] << " ";
                }
			}

			{
				newMessage = oss.str();
			}
		}
		else // MIDI 1.0
		{
			int stat, byte1, byte2, chan; // 3 bytes of MIDI message
			chan = midiMessage[0] & 0x0f;
			stat = midiMessage[0] & 0xf0;
			bool is_system_msg = (stat & MIDI_SystemMessage) == MIDI_SystemMessage;
			byte1 = midiMessage[1] & 0x7F;
			byte2 = midiMessage[2] & 0x7F;
			//	_RPT3(_CRT_WARN,"ug_midi_monitor::OnMidiData(%x,%x,%x)\n", (int)stat,(int)byte1,(int)byte2);
			//	if( chan == channel || channel == -1 || is_system_msg ) //-1 = All channels
			{
				//			newMessage.Format("MIDI MONITOR :%2d :", chan );
				// Note offs can be note_on vel=0
				if (byte2 == 0 && stat == MIDI_NoteOn)
					stat = MIDI_NoteOff;

				std::wostringstream oss;

				switch (stat)
				{
				case MIDI_NoteOn:
					// Note On and Note-Off at same time?
					if (!noteStatus[chan][byte1] && noteStatusTimestamps[chan][byte1] == samplesPassed)
					{
						oss << L"!!AMBIGUOUS!! ";
					}
					//			newMessage.Format((L"Note On  (%3d,%3d)"), byte1, byte2 );
					oss << L"Note On  (" << byte1 << "," << byte2 << ")";
					newMessage = oss.str();
					noteStatus[chan][byte1] = true;
					noteStatusTimestamps[chan][byte1] = samplesPassed;
					break;

				case MIDI_NoteOff:
					// Note On and Note-Off at same time?
					if (noteStatus[chan][byte1] && noteStatusTimestamps[chan][byte1] == samplesPassed)
					{
						oss << L"!!AMBIGUOUS!! ";
					}

					// newMessage.Format((L"Note Off (%3d,%3d)"), byte1, byte2 );
					oss << L"Note Off  (" << byte1 << "," << byte2 << ")";
					newMessage = oss.str();
					noteStatus[chan][byte1] = false;
					noteStatusTimestamps[chan][byte1] = samplesPassed;
					break;

				case MIDI_PolyAfterTouch:
					oss << L"PolyAfterTouch  (" << byte1 << "," << byte2 << ")";
					newMessage = oss.str();
					break;

				case MIDI_ControlChange:
				{
					if (byte1 >= 0 && byte1 < 128)
					{
						std::wstring desc = CONTROLLER_DESCRIPTION[byte1];
																		//newMessage.Format((L"CC%s (%d)"), desc, byte2 );
																		// remove CC number from description. TRim to 20 char.
						size_t p = desc.find(L'-');

						if (p != std::string::npos)
						{
							//							desc = Left(Right(desc, desc.size() - p), 20);
							desc = desc.substr(p, (std::min)(desc.size() - p, (size_t)20));
						}

						//newMessage.Format((L"CC%d %d %s"), byte1, byte2, desc );
						oss << L"CC" << byte1 << ":" << std::setw(3) << std::right << byte2 << " " << desc;
						newMessage = oss.str();
					}
					else
					{
						newMessage = (L"Cntlr: ILEGAL CNTRLR");
					}
				}
				break;

				case MIDI_ProgramChange:
					oss << L"Prg change  (" << (byte1 + 1) << ")";
					newMessage = oss.str();
					break;

				case MIDI_ChannelPressue:
					oss << L"Channel Pressure  (" << byte1 << ")";
					newMessage = oss.str();
					break;

				case MIDI_PitchBend:
				{
					const int val = (byte2 << 7) + byte1 - 8192;
					const auto normalized = (midi::utils::bipoler14bitToNormalized(byte2, byte1) * 2.0f) - 1.0f; // -1.0 -> + 1.0

					oss << L"Bender  (" << val << ") " << normalized;
					newMessage = oss.str();
				}
				break;

				case 0:
					newMessage = (L"Zeros bytes!");
					break;

				case MIDI_SystemMessage:
				{
					switch (midiMessage[0])
					{
					case MIDI_SystemMessage: // SYSEX
					{
						std::wostringstream oss;
						oss.precision(2);

						// Tuning?
						if ((midiMessage[1] == MIDI_Universal_Realtime || midiMessage[1] == MIDI_Universal_NonRealtime) &&
							midiMessage[3] == MIDI_Sub_Id_Tuning_Standard)
						{
							int tuningCount = 0;
							int tuningDataPosition = 0;
							bool entireBank = false;
							bool retunePlayingNotes = true;

							switch (midiMessage[4])
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
								tuningCount = midiMessage[6];
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
								tuningCount = midiMessage[7];
								tuningDataPosition = 8;
								retunePlayingNotes = false;
							}
							break;
							}

							if (tuningCount > 1)
							{
								oss << L"TUNE (BULK)";
							}
							else
							{
								const unsigned char* tuneEntry = &(midiMessage[tuningDataPosition]);
								int midiKeyNumber = *tuneEntry++;
								int tune = (tuneEntry[0] << 14) + (tuneEntry[1] << 7) + tuneEntry[2];
								float semitone = tune / (float)0x4000;

								oss.precision(6);
								oss << L"TUNE: k" << midiKeyNumber << " " << semitone;
							}
						}
						else
						{
							// MIDI HD-PROTOCOL. (hd protocol)
							if (GmpiMidiHdProtocol::isWrappedHdProtocol(midiMessage, size))
							{
								int channelGroup;
								int keyNumber;
								int val_12b;
								int val_20b;
								int status;
								int midiChannel;

								GmpiMidiHdProtocol::DecodeHdMessage(midiMessage, size, status, midiChannel, channelGroup, keyNumber, val_12b, val_20b);

								switch (status)
								{
								case GmpiMidi::MIDI_NoteOn:
								{
									/*
									int velocity_12b = val_12b;
									int directPitch_20b = val_20b;

									const float oneTwelf = 1.0f / 12.0f;
									float pitch = voiceState->GetKeyTune(keyNumber) * oneTwelf - 0.75f;

									const float recip = 1.0f / (float)0xFFF;
									float velocityN = (float)velocity_12b * recip;
									*/

									oss << L"HDNoteOn: k" << keyNumber;
								}
								break;

								case GmpiMidi::MIDI_NoteOff:
								{
									/*
									int velocity_12b = val_12b;
									const float recip = 1.0f / (float)0xFFF;
									float velocityN = (float)velocity_12b * recip;
									*/

									oss << L"HDNoteOff: k" << keyNumber;
								}
								break;

								case GmpiMidi::MIDI_ControlChange:
								{
									int controllerNumber_12b = val_12b;
									int controllerValue_20b = val_20b;

									constexpr float recip = 1.0f / (float)0xFFFFF;
									float normalised = (float)controllerValue_20b * recip;

									if (keyNumber == 0xFF) // Monophonic CCs
									{
										oss << L"HDCC:" << controllerNumber_12b << " v" << normalised;
									}
									else // Polyphonic CCs (Note Expression).
									{
										oss << L"HDCC" << controllerNumber_12b << L" k" << keyNumber << " v" << normalised;
									}
								}
								break;
								}
							}
							else
							{
								// REgualr SYSEX
								int c = (std::min)(size, 10);
								oss << L"SYSEX";

								for (int i = 0; i < c; i++)
								{
									oss << L" " << std::hex << (int)midiMessage[i];
								}

								if (c < size)
									oss << (L"...");
							}
						}

						newMessage = oss.str();
					}
					break;

					case 0xF1:
						oss << L"F1 " << std::hex << midiMessage[1] << " (MTC QF)";
						newMessage = oss.str();
						break;

					case 0xF2:
						//newMessage.Format((L"F2 %X%X (SPP)"), midiMessage[1], midiMessage[2]);
						oss << L"F2 " << std::hex << midiMessage[1] << std::hex << midiMessage[2] << " (SPP)";
						newMessage = oss.str();
						break;

					case 0xF3:
						//newMessage.Format((L"F3 %X (SS)"), midiMessage[1]);
						oss << L"F3 " << std::hex << midiMessage[1] << " (SS)";
						newMessage = oss.str();
						break;

					case 0xF6:
						newMessage = L"F6 - Tune Request";
						break;

					case 0xF8:
						newMessage = L"F8 - CLOCK";
						break;

					case 0xFA:
						newMessage = L"FA - CLOCK - Start";
						break;

					case 0xFB:
						newMessage = L"FB - CLOCK - Continue";
						break;

					case 0xFC:
						newMessage = L"FC - CLOCK - Stop";
						break;

					case 0xFE:
						newMessage = L"FE - Active Sensing";
						break;

					case 0xFF:
						newMessage = L"FF - System Reset";
						break;

					default:
						//newMessage.Format((L"SYSTEM (%x)"), midiMessage[0]);
						oss << L"SYSTEM (" << std::hex << midiMessage[1] << ")";
						newMessage = oss.str();
					};
				}
				break;

				default:
					//newMessage.Format((L"Byte = %02x %02x %02x"), stat, byte1, byte2 );
					oss << L"Byte = " << std::hex << stat << " " << std::hex << byte1 << " " << std::hex << byte2;
					newMessage = oss.str();
				}

				if (!is_system_msg)
				{
					std::wostringstream oss2;
					oss2 << L"1C" << (chan + 1) << " ";
					newMessage = oss2.str() + newMessage;
				}

				// add time
				int64_t seconds = samplesPassed / (int)this->getSampleRate();
				int64_t minutes = seconds / 60;
				seconds %= 60;
				int64_t samples = samplesPassed % (int64_t)getSampleRate();
				std::wstring time_st;
				//		time_st.Format((L"%01d:%02d+%05d "), minutes, seconds, samples);
				newMessage = time_st + newMessage;
				const size_t lineLength = 40;

				if (newMessage.size() > lineLength)
				{
					//					newMessage = Left(newMessage, lineLength) + std::wstring(L"\n                            ") + Right(newMessage, newMessage.size() - lineLength);
					newMessage = newMessage.substr(lineLength) + std::wstring(L"\n                            ") + newMessage.substr(lineLength);
				}

//				_RPTW1(_CRT_WARN, L"MM: %s\n", newMessage );
				//				Print(newMessage);
			}
		}
		lines.push_back(newMessage);
#ifdef _WIN32
#ifdef _DEBUG
		if (newMessage.find(L"Note On") != std::string::npos)
		{
			_RPTW1(_CRT_WARN, L"MM: %s\n", newMessage.c_str());
		}
#endif
#endif
		if (lines.size() > 14)
			lines.pop_front();

		std::wstring printout;
		for (auto& s : lines)
			printout += s + +L"\n";

		//		msg = newMessage /*+ timeText */+ L"\n" + msg + L"\n";

		pinDispOut = printout;
	}
};
