/*
 FEATURE REQUESTS
-  two Midi learn Modes, actual if i move a hardware knob, the software knop jumps direct to the Hardware Knop position. Is it possible to  have a Midi Learn mode, that moves the soft knob only if the hardware knob reach the soft knob value ?? 
*/


#include "pch.h"
#include <assert.h>
#include <algorithm>
#include "./dsp_patch_manager.h"
#include "./dsp_patch_parameter_base.h"
#include "./my_msg_que_input_stream.h"
#include "./my_msg_que_output_stream.h"
#include "plug_description.h"
#include "./midi_defs.h"
#include "./dsp_patch_parameter.h"
#include "ug_patch_automator.h"
#include "./ug_patch_param_watcher.h"
#include "datatype_to_id.h"
#include "tinyxml/tinyxml.h"
#include "ug_midi_automator.h"
#include "modules/se_sdk3/mp_midi.h"
#include "ug_patch_param_setter.h"
#include "BundleInfo.h"

using namespace std;
using namespace GmpiMidi;
using namespace GmpiMidiHdProtocol;

//messy, just to get to que
#include "./ug_container.h"
#include "./iseshelldsp.h"

using namespace std;

namespace midi_2_0
{
}

// RPNs are 14 bit values, so this value never occurs, represents "no rpn"
#define NULL_RPN 0xffff


void VoiceControlState::OnKeyTuningChangedA(timestamp_t absolutetimestamp, int MidiKeyNumber, int /*tune*/)
{
	// Send pitch automation
	int automation_id = (ControllerType::Pitch << 24) | MidiKeyNumber; // noteNumber = note_num
	constexpr float oneTwelf = 1.0f / 12.0f;
	float volts = GetKeyTune(MidiKeyNumber) * oneTwelf - 0.75f;
	float normalised = volts * 0.1f;
	voiceControlContainer_->get_patch_manager()->vst_Automation(voiceControlContainer_, absolutetimestamp, automation_id, normalised);
}

DspPatchManager::DspPatchManager(ug_container* p_container) :
	vst_learn_parameter(0)
	, m_container(p_container)
	, program_(0)
	, incoming_rpn(NULL_RPN)
	, incoming_nrpn(NULL_RPN)
	, MidiCvControlsVoices_(false)
	, highResolutionVelocityPrefix(0)
{
	for (int i = 0; i < 32; i++)
	{
		two_byte_controler_value[i] = 0;
	}

#ifdef DEBUG_LOG_PM_TO_FILE
	wchar_t fname[50];
	swprintf(fname, L"c:\\temp\\DspPatchManagerLog_%d.txt", this);
	outputStream = _wfopen(fname, L"wt");
#endif
}

DspPatchManager::~DspPatchManager()
{
	for( auto parameter : m_parameters )
	{
		delete parameter;
	}

#ifdef DEBUG_LOG_PM_TO_FILE
	if (outputStream)
		fclose(outputStream);
#endif
}

void DspPatchManager::AddParam( dsp_patch_parameter_base* p_param )
{
	m_parameters.push_back( p_param  );
}

void DspPatchManager::vst_Automation(ug_container* voiceControlContainer, timestamp_t timestamp, int p_controller_id, float p_normalised_value, bool sendToMidiCv, bool sendToNonMidiCv)
{
	// handle learn mode
	if( vst_learn_parameter )
	{
		// Only certain types recognised by MIDI learn.
		static const int validTypes[] = { ControllerType::CC, ControllerType::RPN, ControllerType::NRPN, ControllerType::Bender, ControllerType::ChannelPressure };
		const int typ = p_controller_id >> 24;

		for (auto t : validTypes)
		{
			if (t == typ)
			{
				vst_learn_parameter->setAutomation(p_controller_id, true);
				break;
			}
		}
	}

	// update all registered parameters
	int lookupController = p_controller_id;
	int controllerType = lookupController >> 24;
	int voiceId = 0; //-1; // all voices

	// send to polyphonic parameters.
	int voiceContainerHandle = voiceControlContainer->Handle();
	if( ControllerType::isPolyphonic(controllerType) )
	{
		voiceId = lookupController & 0x7f;
		int polylookupController = lookupController & 0xffff0000; // mask off key number
		dsp_automation_map_type::iterator it = vst_automation_map.find(polylookupController);

		while( it != vst_automation_map.end() && (*it).first == polylookupController )
		{
			dsp_patch_parameter_base* parameter = (*it).second;

			if (parameter->isPolyphonic() && parameter->getVoiceContainerHandle() == voiceContainerHandle )
			{
				parameter->vst_automate(timestamp, voiceId, p_normalised_value, true);
			}
			it++;
		}
	}

	// send to monophonic note-specific parameters.
	dsp_automation_map_type::iterator it = vst_automation_map.find(lookupController);

	while( it != vst_automation_map.end() && (*it).first == lookupController )
	{
		dsp_patch_parameter_base* parameter = (*it).second;

		if (parameter->isPolyphonic() == false && (parameter->getVoiceContainerHandle() == voiceContainerHandle || parameter->getVoiceContainerHandle() == -1)) // Bender is monophonic, but still has a voice-container.
		{
			// Avoid crash caused by sending controllers from MIDI-CV to other un-related modules.
			// They should get MIDI only from patch-automator.
			const bool usedByMidiCv =
				   lookupController == (ControllerType::Bender << 24)     // Bender
				|| lookupController == ((ControllerType::CC << 24) | 64)  // HoldPedal
				|| lookupController == ((ControllerType::CC << 24) | 69); // HoldPedal 2

			bool send = true;
			if(usedByMidiCv)
			{
				const bool parameterControlsMidiCv = parameter->getHostControlId() != HC_NONE;
				send = (!parameterControlsMidiCv && sendToNonMidiCv)
						|| (parameterControlsMidiCv && sendToMidiCv);
			}

			if(send)
			{
				parameter->vst_automate(timestamp, voiceId, p_normalised_value, true);
			}
		}
		it++;
	}
}

// Host-Control Updated. Takes absolute value, not normalised. No MIDI learn.
void DspPatchManager::vst_Automation2(timestamp_t timestamp, int p_controller_id, const void* data, int size )
{
	// update all registered parameters
	int lookupController = p_controller_id;
	int voiceId = 0; //-1; // all voices
	// send to parameters.
	dsp_automation_map_type::iterator it = vst_automation_map.find(lookupController);

	while( it != vst_automation_map.end() && (*it).first == lookupController )
	{
		(*it).second->vst_automate2( timestamp, voiceId, data, size, true ); // True indicates GUI needs updating manually becuase we're operating outside VST3's synced GUI/DSP system.
		it++;
	}
}

namespace midi_2_0
{

}


// Problem, this method receives MIDI from both MIDI-CV and from Patch-Automator, yet we can save only one state for any given CC.
// It's possible that one stream has been modified and the CC memory here will get confused/corrupted.
// Fix would be to move all MIDI state and bytewise procesing to the two senders, one copy each, and have the sender just call the relevant methods here in a 'stateless' way,
// maybe just the unified_controller_id and the normalised values. 
void DspPatchManager::OnMidi(VoiceControlState* voiceState, timestamp_t timestamp, const unsigned char* midiMessage, int size, bool fromMidiCv)
{
	// if MIDI-CV is in same container as Patch-Automator, MIDI-CV takes over note allocation.
	// processNotes is passed as false from Patch-Automator, in which case we have to check flag.
	bool sendToNoteSource = true;
	bool sendToNonNoteSource = true;
	if (fromMidiCv)
	{
		sendToNonNoteSource = false;
	}
	else
	{
		// Ignore notes if MIDI-CV handling them, or if there are no polyphonic voices in this container.
		sendToNoteSource = !MidiCvControlsVoices_ && voiceState->voiceControlContainer_->isContainerPolyphonic();
	}

	const gmpi::midi::message_view msg(midiMessage, size);

	if (gmpi::midi_2_0::isMidi2Message(midiMessage, size))
	{
		const auto header = gmpi::midi_2_0::decodeHeader(msg);

		if (header.messageType == gmpi::midi_2_0::ChannelVoice64)
		{
			// Monophonic Controllers.
			switch (header.status)
			{
				case gmpi::midi_2_0::ControlChange:
				{
					const auto controller = gmpi::midi_2_0::decodeController(msg);
					const auto unified_controller_id = (ControllerType::CC << 24) | controller.type;

					if (!fromMidiCv)
					{
						vst_Automation(voiceState->voiceControlContainer_, timestamp, unified_controller_id, controller.value);
					}

					switch (controller.type)
					{
					case 64: // Damper/Hold Pedal on/off (Sustain).
					case 69: // Hold 2.
					{
						vst_Automation(voiceState->voiceControlContainer_, timestamp, unified_controller_id, controller.value, sendToNoteSource, sendToNonNoteSource);
					}
					break;

					case CC_ResetAllControllers:
					{
						if (!fromMidiCv)
						{
							highResolutionVelocityPrefix = 0;
							memset(two_byte_controler_value, 0, sizeof(two_byte_controler_value));

							for (int controller_id = 0; controller_id < CC_AllSoundOff; ++controller_id)
							{
								vst_Automation(voiceState->voiceControlContainer_, timestamp, (ControllerType::CC << 24) | controller_id, 0.0f);
							}
						}
					}
					break;
					}
				}
				break;

				case gmpi::midi_2_0::RPN:
				{
					const auto rpn = gmpi::midi_2_0::decodeRpn(msg);
					if (rpn.rpn == gmpi::midi_2_0::RpnTypes::PitchBendSensitivity)
					{
						// auto unified_controller_id = (ControllerType::RPN << 24) | rpn.rpn;

						// TODO!!! alter bend range
					}
				}
				break;
			}
			
			// Polyphonic messages
			if (sendToNoteSource)
			{
				switch (header.status)
				{
				case gmpi::midi_2_0::NoteOn:
				{
					const auto note = gmpi::midi_2_0::decodeNote(msg);

					_RPTN(0, "PM Note-on %d\n", note.noteNumber);
					if (gmpi::midi_2_0::attribute_type::Pitch == note.attributeType)
					{
						const auto timestamp_oversampled = voiceState->voiceControlContainer_->CalculateOversampledTimestamp(Container(), timestamp);

						const auto semitones = note.attributeValue;
						_RPTN(0, "      ..pitch = %f\n", semitones);

						voiceState->SetKeyTune(note.noteNumber, semitones);
						voiceState->OnKeyTuningChangedA(timestamp_oversampled, note.noteNumber, 0);

						// TODO!!!! seems a bit heavy? !! can we just flag it?
						{
							auto app = m_container->AudioMaster()->getShell();
							auto tuningTable = voiceState->getTuningTable();
							app->setPersisentHostControl(voiceState->voiceControlContainer_->Handle(), HC_VOICE_TUNING, RawView((const void*)tuningTable, sizeof(int) * 128));
						}
					}

					DoNoteOn(timestamp, voiceState->voiceControlContainer_, note.noteNumber, note.velocity);
				}
				break;

				case gmpi::midi_2_0::NoteOff:
				{
					const auto note = gmpi::midi_2_0::decodeNote(msg);
					DoNoteOff(timestamp, voiceState->voiceControlContainer_, note.noteNumber, note.velocity);
				}
				break;

				case gmpi::midi_2_0::PolyControlChange:
				{
					const auto polyController = gmpi::midi_2_0::decodePolyController(msg);

					if (polyController.type == gmpi::midi_2_0::PolyPitch)
					{
						const auto timestamp_oversampled = voiceState->voiceControlContainer_->CalculateOversampledTimestamp(Container(), timestamp);

						const auto semitones = gmpi::midi_2_0::decodeNotePitch(msg);
						_RPTN(0, "      .. bender pitch = %f\n", semitones);

						// voiceState->OnMidiTuneMessageA(timestamp_oversampled, midiMessage);
						voiceState->SetKeyTune(polyController.noteNumber, semitones);
						voiceState->OnKeyTuningChangedA(timestamp_oversampled, polyController.noteNumber, 0);

						// TODO!!!! seems a bit heavy? !! can we just flag it?
						{
							auto app = m_container->AudioMaster()->getShell();
							auto tuningTable = voiceState->getTuningTable();
							app->setPersisentHostControl(voiceState->voiceControlContainer_->Handle(), HC_VOICE_TUNING, RawView((const void*)tuningTable, sizeof(int) * 128));
						}
					}
					else
					{

						int32_t seControllerType = ControllerType::kVolumeTypeID;
						switch (polyController.type)
						{
						case gmpi::midi_2_0::PolyVolume:
							seControllerType = ControllerType::kVolumeTypeID;
							break;

						case gmpi::midi_2_0::PolyPan:
							seControllerType = ControllerType::kPanTypeID;
							break;
							/*
													case gmpi::midi_2_0::PolySoundController8: // Vibrato Depth
														seControllerType = ControllerType::kVibratoTypeID;
														break;
													case gmpi::midi_2_0::PolyExpression:
														seControllerType = ControllerType::kExpressionTypeID;
														break;
							*/
						case gmpi::midi_2_0::PolySoundController5: // Brightness
							seControllerType = ControllerType::kBrightnessTypeID;
							//							_RPTN(0, "decodeBrightness %d %f\n",polyControler.noteNumber, polyControler.value);

							break;
						}

						const int32_t automation_id = (ControllerType::VoiceNoteExpression << 24) | ((seControllerType & 0xFF) << 16) | polyController.noteNumber;
						vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, polyController.value);
					}
				}
				break;

				case gmpi::midi_2_0::PolyBender:
				{
					const auto bender = gmpi::midi_2_0::decodePolyController(msg);
//					_RPTN(0, "decodePolyBender %f\n", bender.value);
					const int32_t automation_id = (ControllerType::VoiceNoteExpression << 24) | ((ControllerType::kTuningTypeID & 0xFF) << 16) | bender.noteNumber;
					vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, bender.value);
				}
				break;

				case gmpi::midi_2_0::PolyAfterTouch:
				{
					const auto aftertouch = gmpi::midi_2_0::decodePolyController(msg);
					//_RPTN(0, "decodePolyPressure %d %f\n",aftertouch.noteNumber, aftertouch.value);
					const int32_t automation_id = (ControllerType::PolyAftertouch << 24) | aftertouch.noteNumber;
					vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, aftertouch.value);
				}
				break;

				case gmpi::midi_2_0::PolyNoteManagement:
				{
					// TODO
				}
				break;

				// control changes, but only those that affect polyphony.
				case gmpi::midi_2_0::ControlChange:
				{
					const auto controller = gmpi::midi_2_0::decodeController(msg);
					const auto unified_controller_id = (ControllerType::CC << 24) | controller.type;

					if (!fromMidiCv)
					{
						vst_Automation(voiceState->voiceControlContainer_, timestamp, unified_controller_id, controller.value);
					}

					// Must be after automation in case MIDI-Learn is active.
					switch (controller.type)
					{
						/* All Sound Off.
						The difference between this message and All Notes Off is that this message immediately mutes all
						sound on the device regardless of whether the Hold Pedal is on, and mutes the sound quickly
						regardless of any lengthy VCA release times. It's often used by sequencers to quickly mute all
						sound when the musician presses "Stop" in the middle of a song.
						*/
					case MIDI_CC_ALL_SOUND_OFF: // CC 120
					{
						for (int keyNumber = 0; keyNumber < 128; ++keyNumber)
						{
							// Send gate-off automation
							const int automation_id = (ControllerType::Gate << 24) | keyNumber;
							const float normalised = 0.0f;
							vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);

							voiceState->voiceControlContainer_->killVoice(timestamp, keyNumber);
						}
					}
					break;

					case MIDI_CC_ALL_NOTES_OFF: // CC 123
					{
						for (int keyNumber = 0; keyNumber < 128; ++keyNumber)
						{
							// Send velocity-off automation
							int automation_id = (ControllerType::VelocityOff << 24) | keyNumber;
							const float velocity = 0.5f;
							vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, velocity);

							// Send gate automation
							automation_id = (ControllerType::Gate << 24) | keyNumber;
							const float gateOff = 0.0f;
							vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, gateOff);
						}
					}
					break;

					case 64: // Damper/Hold Pedal on/off (Sustain).
					case 69: // Hold 2.
					{
						voiceState->voiceControlContainer_->SetHoldPedalState(controller.value >= 0.5f);
					}
					break;
					}
				}
				break;

				case gmpi::midi_2_0::ProgramChange:
				{
					// TODO
				}
				break;

				case gmpi::midi_2_0::ChannelPressue:
				{
					constexpr int automation_id = ControllerType::ChannelPressure << 24;
					const auto normalized = gmpi::midi_2_0::decodeController(msg).value;

					vst_Automation(
						voiceState->voiceControlContainer_,
						timestamp,
						automation_id,
						normalized,
						sendToNoteSource,
						sendToNonNoteSource
					);
				}
				break;

				case gmpi::midi_2_0::PitchBend:
				{
					constexpr int automation_id = ControllerType::Bender << 24;
					const auto normalized = gmpi::midi_2_0::decodeController(msg).value;

					vst_Automation(
						voiceState->voiceControlContainer_,
						timestamp,
						automation_id,
						normalized,
						sendToNoteSource,
						sendToNonNoteSource
					);
				}
				break;

				} // status
			} // sendToNoteSource

		} // ChannelVoice64
	}
	else
	{
		int status = midiMessage[0] & 0xf0;

		// Note offs can be note_on vel=0
		if (midiMessage[2] == 0 && status == NOTE_ON)
		{
			status = NOTE_OFF;
		}

		int b2, b3, midiChannel;// 3 bytes of MIDI message
		midiChannel = midiMessage[0] & 0x0f;
		b2 = midiMessage[1] & 0x7F;
		b3 = midiMessage[2] & 0x7F;
		int byte1 = b2; // confused? ;)
		int byte2 = b3;

		switch (status)
		{
		case NOTE_ON:
		{
			if (sendToNoteSource)
			{
				const int keyNumber = byte1;
				int velocity = (byte2 << 7) + highResolutionVelocityPrefix;
				highResolutionVelocityPrefix = 0;

				// note that with this scheme a velocity of 127 does not map to 1.0f, the lower 7 'prefix' bits need to be 127 also.
				constexpr float recip = 1.0f / (float)0x3fff;
				float velocityN = recip * (float)velocity;

				DoNoteOn(timestamp, voiceState->voiceControlContainer_, keyNumber, velocityN);
			}
		}
		break;

		case NOTE_OFF:
		{
			if (sendToNoteSource)
			{
				int velocity = byte2;
				constexpr float recip_127 = 1.0f / 127.0f;
				float velocityN = (float)velocity * recip_127;

				DoNoteOff(timestamp, voiceState->voiceControlContainer_, byte1, velocityN);
			}
		}
		break;

		case SYSTEM_MSG: // SYSEX.
			if (midiMessage[0] == SYSTEM_EXCLUSIVE)
			{
				// MIDI HD-PROTOCOL. (hd protocol)
				if (isWrappedHdProtocol(midiMessage, size))
				{
					int channelGroup;
					int keyNumber;
					int val_12b;
					int val_20b;

					DecodeHdMessage(midiMessage, size, status, midiChannel, channelGroup, keyNumber, val_12b, val_20b);

					switch (status)
					{
					case GmpiMidi::MIDI_NoteOn:
					{
						if (sendToNoteSource)
						{
							int velocity_12b = val_12b;
//							int directPitch_20b = val_20b;

							float velocityN = val20BitToFloat(velocity_12b);

							//						_RPT2(_CRT_WARN, "HD Note On vel %d:%f\n", velocity_12b, velocityN);
#ifdef DEBUG_LOG_PM_TO_FILE
							fwprintf(outputStream, L"\nHD Note On  %d (ts %d)\n", keyNumber, (int)timestamp);
#endif

							DoNoteOn(timestamp, voiceState->voiceControlContainer_, keyNumber, velocityN);
						}
					}
					break;

					case GmpiMidi::MIDI_NoteOff:
					{
						if (sendToNoteSource)
						{
							int velocity_12b = val_12b;
							constexpr float recip = 1.0f / (float)0xFFF;
							float velocityN = (float)velocity_12b * recip;

#ifdef DEBUG_LOG_PM_TO_FILE
							fwprintf(outputStream, L"\nHD Note Off  %d (ts %d)\n", keyNumber, (int)timestamp);
#endif

							DoNoteOff(timestamp, voiceState->voiceControlContainer_, keyNumber, velocityN);
						}
					}
					break;

					case GmpiMidi::MIDI_ControlChange:
					{
						int controllerNumber_12b = val_12b;
						int controllerValue_20b = val_20b;

						const float normalised = val20BitToFloat(controllerValue_20b);

						if (keyNumber == 0xFF) // Regular Monophonic CCs
						{
							if (!fromMidiCv)
							{
								// TODO: SE to handle 12bit controller numbers. !!!
								int unified_controller_id = (ControllerType::CC << 24) | controllerNumber_12b; // CC
								vst_Automation(voiceState->voiceControlContainer_, timestamp, unified_controller_id, normalised);
							}
						}
						else // Polyphonic CCs (Note Expression).
						{
							if (sendToNoteSource)
							{
								int controllerType = ControllerType::VoiceNoteExpression;

								// Send Sample-ID.
								if (controllerNumber_12b == 300) // Special case for Waves Sampler Region-ID. Pass integer value unmolested out Note-Expression.User3 
								{
									const float realWorld = (float)controllerValue_20b; // !! NOT Normalised. Direct int-to-float equality.
									controllerNumber_12b = ControllerType::kCustomStart + 2;

									int lookupController = (controllerType << 24) | ((controllerNumber_12b & 0xFF) << 16) | keyNumber;

									// Send direct without scaling to parameter min/max to avoid loss of precision.
									//int controllerType = lookupController >> 24;
									assert(controllerType == lookupController >> 24);
									int voiceId = keyNumber; //-1; // all voices
									//_RPT2(_CRT_WARN,"set Region MIDI( key %d, region %d)\n", voiceId, controllerValue_20b);

									// send to polyphonic parameters.
									int voiceContainerHandle = voiceState->voiceControlContainer_->Handle();
									if (ControllerType::isPolyphonic(controllerType))
									{
										voiceId = lookupController & 0x7f;
										int polylookupController = lookupController & 0xffff0000; // mask off key number
										dsp_automation_map_type::iterator it = vst_automation_map.find(polylookupController);

										while (it != vst_automation_map.end() && (*it).first == polylookupController)
										{
											dsp_patch_parameter_base* parameter = (*it).second;

											if (parameter->isPolyphonic() && parameter->getVoiceContainerHandle() == voiceContainerHandle)
											{
												parameter->vst_automate2(timestamp, voiceId, RawData3(realWorld), RawSize(realWorld), true);
											}
											it++;
										}
									}
								}
								else
								{
									//								_RPTN(0, "MIDI2: key %3d type %2d value %f\n", keyNumber, val_12b, normalised);

									int automation_id = (controllerType << 24) | ((controllerNumber_12b & 0xFF) << 16) | keyNumber;
									vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);
								}
							}
						}
					}
					break;
					}

					return;
				}

				int i = 1;
				// 1 or 2-byte manufacturer ID.
				int manufactureId = midiMessage[1];

				if (manufactureId == 0) // indicates next 2 bytes are ID.
				{
					manufactureId = (midiMessage[2] << 8) + midiMessage[3];
					i = 3;
				}

				// 'SYSEX channel'.
				// unsigned char deviceId = midiMessage[i];
				// Model ID not supported by all, Roland-yes, Yamaha-no.
				int automation_id = ControllerType::SYSEX << 24;// | manufactureId << 16;
				const unsigned char MIDI_MF_ROLAND = 0x41;

				// Heuristic for common manufacturer messages.
				if (manufactureId == MIDI_MF_ROLAND)
				{
					// F0 41 (Roland) 10 (DeviceID) 42 (Model) 12 (sending, or 11 rec) 00 00 01 (address) vv (val) xx (checksum) F7
					//					automation_id |= ;
					int hi = midiMessage[size - 3];
					constexpr float inv127 = 1.0f / 127.f;
					float normalised = hi * inv127;
					vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);
				}

				if (sendToNoteSource)
				{
					if (manufactureId == 0x7E || manufactureId == 0x7F) // 7F - Universal Real Time SysEx header, 7E - Universal Non-Real Time SysEx header
					{
						if (midiMessage[3] == 0x08)	// 08  sub-ID#1 = "MIDI Tuning Standard"
						{
							auto timestamp_oversampled = voiceState->voiceControlContainer_->CalculateOversampledTimestamp(Container(), timestamp);

							voiceState->OnMidiTuneMessageA(timestamp_oversampled, midiMessage);

							{
								auto app = m_container->AudioMaster()->getShell();
								auto tuningTable = voiceState->getTuningTable();
								app->setPersisentHostControl(voiceState->voiceControlContainer_->Handle(), HC_VOICE_TUNING, RawView((const void*)tuningTable, sizeof(int) * 128));

								//							_RPT2(_CRT_WARN, "DspPatchManager[%x] TUNE %x\n", voiceState->voiceControlContainer_->Handle(), voiceState->GetIntKeyTune(61)); // C#4
							}
						}
					}

					if (manufactureId == 0x7F) // Universal Real Time SysEx header
					{
						if (midiMessage[3] == 0x0A) // “Key-Based Instrument Control”
						{
							/*
							Key-Based Instrument Controller message. (Polyphonic controllers).
							F0 7F Universal Real Time SysEx header
							<device ID> ID of target device (7F = all devices)
							0A sub-ID#1 = “Key-Based Instrument Control”
							01 sub-ID#2 = 01 Basic Message
							0n MIDI Channel Number
							kk Key number
							[nn,vv] Controller Number and Value
							:
							F7 EOX
							*/

							midiChannel = midiMessage[5] & 0x0f;
							int keyNumber = midiMessage[6];
							int controllerId = midiMessage[7];
							int controllerValue = midiMessage[8];

							const unsigned char allSoundOff = 0x78;
							int controllerType = ControllerType::VoiceNoteExpression;
							int NoteExpressionCc = -1;
							switch (controllerId)
							{
							case allSoundOff:
							{
								voiceState->voiceControlContainer_->killVoice(timestamp, keyNumber);
							}
							break;

							case 7: // Volume.
								NoteExpressionCc = ControllerType::kVolumeTypeID;
								break;

							case 10: // Pan.
								NoteExpressionCc = ControllerType::kPanTypeID;
								break;

							case 77: // Vibrato Depth.
								NoteExpressionCc = ControllerType::kVibratoTypeID;
								break;

							case 1: // Expression.
								NoteExpressionCc = ControllerType::kExpressionTypeID;
								break;

							case 74: // Brightness.
								NoteExpressionCc = ControllerType::kBrightnessTypeID;
								break;

							case 65: // Portamento On/Off .
								controllerType = ControllerType::VoiceNoteControl;
								NoteExpressionCc = ControllerType::kPortamentoEnable;
								break;
							}

							if (NoteExpressionCc > -1)
							{
								float normalised = controllerValue * (1.0f / 127.0f);
								automation_id = (controllerType << 24) | (NoteExpressionCc << 16) | keyNumber;
								vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);
							}
						}
					}
				}
			}

			break;

		case PROGRAM_CHANGE:
		{
			//		_RPT3(_CRT_WARN,"ug_patch_automator::OnMidiData(%x, %d, %d)\n",(int)stat,(int)byte1,(int)byte2);
			if (!fromMidiCv)
			{
				_RPT0(_CRT_WARN, "ug_patch_automator::OnMidiData - Changing Patch via MIDI\n");
				/*patch_control_container->get_patch_manager()->*/ setProgramDspThread(byte1);
			}
		}
		break;

		case CONTROL_CHANGE:
		{
			int midi_controller_id = byte1;
			int midi_controller_val = byte2;

			// incoming change of NRPN or RPN ( non/registered parameter number)
			switch (midi_controller_id)
			{
			case NRPN_MSB:
				cntrl_update_msb(incoming_nrpn, static_cast<short>(midi_controller_val));
				incoming_rpn = NULL_RPN;	// ignore rpn msgs
				return;
				break;

			case NRPN_LSB:
				cntrl_update_lsb(incoming_nrpn, static_cast<short>(midi_controller_val));
				incoming_rpn = NULL_RPN;	// ignore rpn msgs
				return;
				break;

			case RPN_MSB:
				cntrl_update_msb(incoming_rpn, static_cast<short>(midi_controller_val));
				incoming_nrpn = NULL_RPN;	// ignore nrpn msgs
				return;
				break;

			case RPN_LSB:
				cntrl_update_lsb(incoming_rpn, static_cast<short>(midi_controller_val));
				incoming_nrpn = NULL_RPN;	// ignore nrpn msgs
				return;
				break;
			}

			bool useTwoByteMidiCcs = false; // CC 32-63 can be interpreted as fine-tunes on CC 0-31. Not commonly supported.

			useTwoByteMidiCcs |= (midi_controller_id & 0x1f) == RPN_CONTROLLER; // Exception is data-entry slider. Which controls RPNs.

			int base_midi_controller_id = midi_controller_id;

			int combined_controller_val;

			if (useTwoByteMidiCcs && midi_controller_id < 64) // handle 14 bit (two byte) CCs.
			{
				bool is_msb = midi_controller_id < 32;
				base_midi_controller_id &= 0x1f; // force cc < 32

				assert(base_midi_controller_id < 32);

				if (base_midi_controller_id == RPN_CONTROLLER) // data entry slider (RPN/NRPN)
				{
					// create a key unique to that RPN/NRPN
					int rpn_key;

					if (incoming_nrpn != NULL_RPN)
					{
						rpn_key = incoming_nrpn;
					}
					else
					{
						rpn_key = incoming_rpn | 0x4000;
					}

					// look up current value of that RPN
					map<int, int>::iterator it = m_rpn_memory.find(rpn_key);

					if (it == m_rpn_memory.end())
					{
						pair< map<int, int>::iterator, bool > result = m_rpn_memory.insert(std::pair<int, int>(rpn_key, 0));
						assert(result.second == true);
						it = result.first;
					}

					// update data entry slider
					two_byte_controler_value[base_midi_controller_id] = static_cast<short>((*it).second);

					if (is_msb) // midi_controller_id < 32 )
					{
						cntrl_update_msb(two_byte_controler_value[base_midi_controller_id], static_cast<short>(midi_controller_val));
					}
					else
					{
						cntrl_update_lsb(two_byte_controler_value[base_midi_controller_id], static_cast<short>(midi_controller_val));
					}

					// store in RPN mem
					(*it).second = two_byte_controler_value[base_midi_controller_id];
				}
				else
				{
					if (is_msb) // midi_controller_id < 32 )
					{
						cntrl_update_msb(two_byte_controler_value[base_midi_controller_id], static_cast<short>(midi_controller_val));
					}
					else
					{
						cntrl_update_lsb(two_byte_controler_value[base_midi_controller_id], static_cast<short>(midi_controller_val));
					}
				}

				combined_controller_val = two_byte_controler_value[base_midi_controller_id];
			}
			else
			{
				// standard 7-bit continuous controller.
				combined_controller_val = midi_controller_val << 7;
			}

			float normalised_value;
			int unified_controller_id;

			// handle RPN, NRPN
			if (base_midi_controller_id/*midi_controller_id*/ == RPN_CONTROLLER)	// data entry slider MSB
			{
				// RPN uses full 14-bit range.
				normalised_value = (float)combined_controller_val / (float)MAX_FULL_CNTRL_VAL;

				if (incoming_rpn != NULL_RPN) // RPN
				{
					unified_controller_id = (ControllerType::RPN << 24) | incoming_rpn;
				}
				else
				{
					if (incoming_nrpn != NULL_RPN) // NRPN
					{
						unified_controller_id = (ControllerType::NRPN << 24) | incoming_nrpn;
					}
					else // user is not using RPN/NRPN, just plain old data entry slider
					{
						unified_controller_id = (ControllerType::CC << 24) | base_midi_controller_id; //midi_controller_id; // CC
					}
				}
			}
			else
			{
				normalised_value = (float)combined_controller_val / (float)MAX_CNTRL_VAL;

				// don't know if LSB in use so MAX_CNTRL_VAL assume only MSB avail. Devices that use full 14-bit range
				// may send bigger values.
				if (normalised_value > 1.0f)
				{
					normalised_value = 1.0f;
				}

				unified_controller_id = (ControllerType::CC << 24) | base_midi_controller_id; //midi_controller_id; // CC
			}

			if (!fromMidiCv)
			{
				vst_Automation(voiceState->voiceControlContainer_, timestamp, unified_controller_id, normalised_value);
			}

			// Must be after automation in case MIDI-Learn is active ( don't want to 'learn' the VelocityOff message).
			switch (midi_controller_id)
			{
				/* All Sound Off.
				The difference between this message and All Notes Off is that this message immediately mutes all
				sound on the device regardless of whether the Hold Pedal is on, and mutes the sound quickly
				regardless of any lengthy VCA release times. It's often used by sequencers to quickly mute all
				sound when the musician presses "Stop" in the middle of a song.
				*/
			case MIDI_CC_ALL_SOUND_OFF: // CC 120
			{
				if (sendToNoteSource)
				{
					for (int keyNumber = 0; keyNumber < 128; ++keyNumber)
					{
						// Send gate automation
						int automation_id = (ControllerType::Gate << 24) | keyNumber;
						float normalised = 0.0f;
						vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);

						voiceState->voiceControlContainer_->killVoice(timestamp, keyNumber);
					}
				}
			}
			break;

			case MIDI_CC_ALL_NOTES_OFF: // CC 123
			{
				if (sendToNoteSource)
				{
					const int velocity = 64;

					for (int keyNumber = 0; keyNumber < 128; ++keyNumber)
					{
						// Send velocity-off automation
						int automation_id = (ControllerType::VelocityOff << 24) | keyNumber;
						float normalised = (float)velocity / 127.0f;
						vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);
						// Send gate automation
						automation_id = (ControllerType::Gate << 24) | keyNumber;
						normalised = 0.0f;
						vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);
					}
				}
			}
			break;

			case 64: // Damper/Hold Pedal on/off (Sustain).
			case 69: // Hold 2.
			{
				int automation_id = (ControllerType::CC << 24) | midi_controller_id;
				float normalised = (float)byte2 / 127.0f;
				vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised, sendToNoteSource, sendToNonNoteSource);

				if (sendToNoteSource)
				{
					voiceState->voiceControlContainer_->SetHoldPedalState(b3 >= 64);
				}
			}
			break;

			case CC_HighResolutionVelocityPrefix:
			{
				highResolutionVelocityPrefix = midi_controller_val;
			}
			break;

			case CC_ResetAllControllers:
			{
				highResolutionVelocityPrefix = 0;
				memset(two_byte_controler_value, 0, sizeof(two_byte_controler_value));

				if (!fromMidiCv)
				{
					for (int controller_id = 0; controller_id < CC_AllSoundOff; ++controller_id)
					{
						vst_Automation(voiceState->voiceControlContainer_, timestamp, (ControllerType::CC << 24) | controller_id, 0.0f);
					}
				}
			}
			break;
			}
		}
		break;

		case POLY_AFTERTOUCH:
			if (sendToNoteSource)
			{
				int automation_id = (ControllerType::PolyAftertouch << 24) | byte1; // byte1 = note_num
				float normalised = (float)byte2 / 127.0f;
				vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);
			}
			break;

		case CHANNEL_PRESSURE:
			if (sendToNoteSource)
			{
				int automation_id = ControllerType::ChannelPressure << 24;
				float normalised = (float)byte1 / 127.0f;
				vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalised);
			}
			break;

		case PITCHBEND:
		{
			const auto normalized = gmpi::midi::utils::bipoler14bitToNormalized(static_cast<uint8_t>(byte2), static_cast<uint8_t>(byte1));

			constexpr int automation_id = ControllerType::Bender << 24;
			vst_Automation(voiceState->voiceControlContainer_, timestamp, automation_id, normalized, sendToNoteSource, sendToNonNoteSource);
		}
		break;
		};
	}
}

void DspPatchManager::DoNoteOff(timestamp_t timestamp, ug_container* voiceControlContainer, int voiceId, float velocity)
{
	int automation_id;

	// Send gate automation.
	automation_id = ( ControllerType::Gate << 24 ) | voiceId;
	float normalised = 0.0f;
	vst_Automation(voiceControlContainer, timestamp, automation_id, normalised);

	// Send velocity-off automation
	automation_id = ( ControllerType::VelocityOff << 24 ) | voiceId;
	vst_Automation(voiceControlContainer, timestamp, automation_id, velocity);
}

void DspPatchManager::DoNoteOn(timestamp_t timestamp, ug_container* voiceControlContainer, int voiceId, float velocity)
{
	int automation_id;
	float normalised;
/* test, for poly glide allow pitch to be set first.
	// Send gate automation.
	automation_id = ( ControllerType::Gate << 24 ) | voiceId;
	float normalised = 1.0f;
	vst_Automation(voiceControlContainer, timestamp, automation_id, normalised);
*/
	/* should not have changed
	// Send pitch automation
	automation_id = ( ControllerType::Pitch << 24 ) | voiceId;
	vst_Automation(voiceControlContainer, timestamp, automation_id, pitch);
	*/

	// Send velocity-on automation
	automation_id = ( ControllerType::VelocityOn << 24 ) | voiceId;
	vst_Automation(voiceControlContainer, timestamp, automation_id, velocity);

	// Reset poly-aftertouch automation
	automation_id = ( ControllerType::PolyAftertouch << 24 ) | voiceId;
	normalised = 0;
	vst_Automation(voiceControlContainer, timestamp, automation_id, normalised);

	// Send gate automation.
	automation_id = ( ControllerType::Gate << 24 ) | voiceId;
	normalised = 1.0f;
	vst_Automation(voiceControlContainer, timestamp, automation_id, normalised);

/*
	// If auto-glide not triggered, just jump to pitch.
	if( !glide )
	{
		mrnPitch = pitch;
	}

	// Send glide start-pitch automation
	automation_id = ( ControllerType::GlideStart Pitch << 24 );// | keyNumber;
	normalised = mrnPitch * 0.1f;
	vst_Automation(voiceControlContainer, timestamp, automation_id, normalised);

	// Glide.
	mrnPitch = pitch;
	mrnTimeStamp = timestamp;
*/
}

float DspPatchManager::InitializeVoiceParameters(ug_container* voiceControlContainer, timestamp_t timestamp, Voice* voice, bool sendTrigger)
{
	const int voiceContainerHandle = voiceControlContainer->Handle();
	float pitch = 0;
	auto voiceId = voice->NoteNum;

	for( auto parameter : m_poly_parameters_cache )
	{
		assert(parameter->isPolyphonic());
		if (parameter->getVoiceContainerHandle() == voiceContainerHandle )
		{
			auto controllerId = parameter->UnifiedControllerId();
			switch( controllerId )
			{
			case ControllerType::Trigger << 24:
			{
				if (sendTrigger)
				{
					// not strictly needed because pin update is forced, but safer.
					nextVoiceReset_++; // actual value don't matter, only that it changes.
					parameter->SetValueRaw2(&nextVoiceReset_, sizeof(nextVoiceReset_), parameter->EffectivePatch(), voiceId);
					parameter->SendValuePt2(timestamp, voice);
				}
			}
			break;

			case ControllerType::Pitch << 24:
			{
				int32_t size = 0;
				pitch = *(float*) parameter->SerialiseForEvent(voiceId, size);
			}
				// deliberate fallthru.

			default:
				// send pin update (forced).
				parameter->SendValuePt2(timestamp, voice);
				break;
			}
		}
	}

	return pitch;
}

void DspPatchManager::InitializeAllParameters()
{
	parameterIndexes_.assign(m_parameters.size(), nullptr); // a rough guess, there might be more.

	// Cache parameter host index.
	for( auto parameter : m_parameters )
	{
		if( parameter->WavesParameterIndex >= 0 )
		{
			// cope with 'gaps' in the parameter indices.
			while (parameterIndexes_.size() <= parameter->WavesParameterIndex)
				parameterIndexes_.push_back(nullptr);

			parameterIndexes_[parameter->WavesParameterIndex] = parameter;
		}

		if( parameter->isPolyphonic() )
		{
			// cache polyphonic parameters (to save time vs traversing ALL parameters)
			m_poly_parameters_cache.push_back( parameter );
		}
	}

	// Send out initial values of all parameters.
	const timestamp_t timestamp = 0;
	for (auto parameter : m_parameters)
	{
		if (parameter->isPolyphonic())
		{
			// On startup, all gates go to zero regardless of previous values.
			if (parameter->isPolyphonicGate())
			{
				const float gateValue = 0.0f;
				for (int32_t voiceId = 0; voiceId < 128; ++voiceId)
				{
					parameter->vst_automate2(timestamp, voiceId, &gateValue, static_cast<int>(sizeof(gateValue)), false);
				}
			}
		}
		else
		{
			if (parameter->isTiming()) // need time/tempo update?
			{
				// Patch Automator will send timing controllers to patch manager.
				ug_patch_automator* automation_sender = Container()->front()->findPatchAutomator();
				automation_sender->setTempoTransmit();
			}

			parameter->SendValuePt2(timestamp, nullptr, true);
		}
	}
}

void DspPatchManager::vst_setAutomationId(dsp_patch_parameter_base* p_param, int p_controller_id)
{
	// remove current map (if any)
	for( dsp_automation_map_type::iterator it = vst_automation_map.begin(); it != vst_automation_map.end(); ++it )
	{
		if( (*it).second == p_param )
		{
			vst_automation_map.erase(it);
			break;
		}
	}

	// Cancel learn
	if( vst_learn_parameter == p_param )
	{
		vst_learn_parameter = 0;
	}

	if( p_controller_id > -1 ) // -1 = none, -2 = LEARN
	{
		int lookupController = p_controller_id;

		/*
		//		int voiceId = -1; // all voices
				if( ControllerType::isPolyphonic(lookupController >> 24 ) )
				{
					lookupController = lookupController & 0xff000000; // mask of key number
		//			voiceId = lookupController & 0x7f;
				}
		*/
		// Poly parameters respond to any voice.
		// mono parameters respond to a specific voice.
		if( p_param->isPolyphonic() )
		{
//			lookupController = lookupController & 0xff000000; // mask off key number
			lookupController = lookupController & 0xffff0000; // mask off key number
		}

		//_RPT2(_CRT_WARN, "vst_automation_map.insert %x (size =%d) \n", lookupController, vst_automation_map.size() );
		vst_automation_map.insert( pair<int,dsp_patch_parameter_base*>( lookupController, p_param ));
	}
	else
	{
		if( p_controller_id == -2 ) // MIDI learn
		{
			vst_learn_parameter = p_param;
		}
	}
}

dsp_patch_parameter_base* DspPatchManager::GetParameter( int handle )
{
	for( auto parameter : m_parameters )
	{
		if( parameter->Handle() == handle )
		{
			return parameter;
		}
	}

	return 0;
}

dsp_patch_parameter_base* DspPatchManager::GetParameter( int moduleHandle, int paramIndex )
{
	for( auto parameter : m_parameters )
	{
		if (parameter->getModuleHandle() == moduleHandle && parameter->getModuleParameterId() == paramIndex)
		{
			return parameter;
		}
	}

	return nullptr;
}

dsp_patch_parameter_base* DspPatchManager::ConnectParameter(int parameterHandle, UPlug* plug)
{
	auto parameter = GetParameter(parameterHandle);
	parameter->ConnectPin2(plug);
	return parameter;
}

dsp_patch_parameter_base* DspPatchManager::ConnectHostControl(HostControls hostConnect, UPlug* plug)
{
	auto parameter = GetParameter(plug->UG->parent_container, hostConnect);
	if (parameter)
	{
		parameter->ConnectPin2(plug);
	}

	return parameter;
}

void DspPatchManager::ConnectHostControl2(HostControls hostConnect, UPlug* toPlug)
{
	assert(hostConnect != HC_SNAP_MODULATION__DEPRECATED); // caller to handle

	auto destinationContainer = toPlug->UG->parent_container;
	auto parameter = GetParameter(destinationContainer, hostConnect);
	if (parameter)
	{
		ug_container* hostControlContainer;
		// If host-control is polyphonic (or related to notes), connect to pp-setter in it's voice-control container,
		// else just connect to pp-setter in patch-manager's container (which may be at a outer level).
		// Note: This will also catch HC_OVERSAMPLING_RATE and HC_OVERSAMPLING_FILTER. Which is hopefully harmless.
		if (HostControlAttachesToParentContainer(hostConnect))
		{
			hostControlContainer = destinationContainer->getOutermostPolyContainer();
		}
		else
		{
			// For most parameters, output pin will be in same container as patch-manager.
			hostControlContainer = Container();
		}

		hostControlContainer->GetParameterSetter()->ConnectHostParameter(parameter, toPlug);
	}
	else
	{
		// if parameter not found locally, defer to parent.
		assert(Container()->ParentContainer()); // an expected parameter is entirly missing

		if (Container()->ParentContainer())
		{
			Container()->ParentContainer()->ConnectHostControl(hostConnect, toPlug);
		}
	}
}

// Polyphonic determination helper routines
struct FeedbackTrace* DspPatchManager::InitSetDownstream(ug_container* voiceControlContainer)
{
	int voiceContainerHandle = voiceControlContainer->Handle();

	// For all poly parameters, set downstream flags on outputs.
	for (auto parameter : m_parameters)
	{
		if (parameter->isPolyphonic() && parameter->getVoiceContainerHandle() == voiceContainerHandle)
		{
			for (auto p : parameter->outputPins_)
			{
				assert((p->flags & PF_POLYPHONIC_SOURCE) != 0);

				for (auto to_plug : p->connections)
				{
					auto r = to_plug->PPSetDownstream();
					if (r != nullptr)
					{
						return r;
					}
				}
			}
		}
	}
	return nullptr;
}

dsp_patch_parameter_base* DspPatchManager::createPatchParameter( int typeIdentifier )
{
	dsp_patch_parameter_base* param = (dsp_patch_parameter_base*)( PersistFactory()->CreateObject( typeIdentifier ) );
	param->setPatchMgr(this);
	AddParam( param );
	return param;
}

#if 0
void DspPatchManager::RegisterParameterDestination(int32_t parameterHandle, UPlug* destinationPin)
{
	auto parameter = GetParameter(parameterHandle);
	if (destinationPin->Direction == DR_OUT)
	{
		Container()->GetParameterWatcher()->CreateParameterPlug(parameter, destinationPin);
	}
	else
	{
		Container()->GetParameterSetter()->ConnectParameter(parameter, destinationPin);
	}
}
#endif

void DspPatchManager::OnUiMsg(int p_msg_id, my_input_stream& p_stream)
{
	switch( p_msg_id )
	{

	case code_to_long('s','e','t','p'): // "setp" - patch change from GUI.
	{
		int32_t p;
		p_stream >> p;
		//_RPT1(_CRT_WARN, "DspPatchManager::OnUiMsg('patc') p=%d\n", program_ );
		UpdateProgram( p );
	}
	break;

	case code_to_long('s','e','t','c'): // "setc" - MIDI Chan change from GUI.
	{
		p_stream >> midiChannel_;
		Container()->automation_input_device->setMidiChannel(midiChannel_);
	}
	break;

	case id_to_long2("EIPC"): // Emulate Ignore Program Change
	{
		mEmulateIgnoreProgramChange = true;
	}
	break;
	};
}

#if defined(SE_TARGET_PLUGIN)
void DspPatchManager::setParameterNormalized( timestamp_t timestamp, int vstParameterIndex, float newValue )
{
	if( vstParameterIndex >= 0 && vstParameterIndex < (int) parameterIndexes_.size() && parameterIndexes_[vstParameterIndex] != 0 )
	{
		const int VoiceId = 0;
		parameterIndexes_[vstParameterIndex]->vst_automate(timestamp, VoiceId, newValue, false);
	}
}
#endif

dsp_patch_parameter_base* DspPatchManager::GetHostControl( int hostConnect )
{
	for( auto parameter : m_parameters )
	{
		// Does not handle polyphonic host-controls.
		if (parameter->getHostControlId() == hostConnect && parameter->getVoiceContainerHandle() == -1)
		{
			return parameter;
		}
	}
	return nullptr;
}

dsp_patch_parameter_base* DspPatchManager::GetParameter(ug_container* voiceControlContainer, HostControls hostConnect)
{
	int voiceContainerHandle = voiceControlContainer->Handle();

	for (auto parameter : m_parameters)
	{
		if (parameter->getHostControlId() == hostConnect && (parameter->getVoiceContainerHandle() == voiceContainerHandle || parameter->getVoiceContainerHandle() == -1))
		{
			return parameter;
		}
	}

	return nullptr;
}

#if defined(SE_TARGET_PLUGIN)
void DspPatchManager::getPresetState( std::string& chunk, bool saveRestartState)
{
	TiXmlDocument doc;
	doc.LinkEndChild( new TiXmlDeclaration( "1.0", "", "" ) );

	auto element = new TiXmlElement("Preset");
	doc.LinkEndChild(element);

	{
		char buffer[20];
		sprintf(buffer, "%08x", BundleInfo::instance()->getPluginId());
		element->SetAttribute("pluginId", buffer);
	}

	// Format for VST persistance.
	std::string nameString, categoryString;

	for( auto parameter : m_parameters )
	{
		if (parameter->getHostControlId() == HC_PROGRAM_NAME)
		{
			nameString = parameter->GetValueAsXml();
		}
		else if (parameter->getHostControlId() == HC_PROGRAM_CATEGORY)
		{
			categoryString = parameter->GetValueAsXml();
		}
		else
		{
			if(parameter->isStateFull(saveRestartState))
			{
				auto paramElement = new TiXmlElement("Param");
				element->LinkEndChild(paramElement);
				paramElement->SetAttribute("id", parameter->Handle());
				paramElement->SetAttribute("val", parameter->GetValueAsXml());

				// MIDI learn.
				if(parameter->UnifiedControllerId() != -1)
				{
					paramElement->SetAttribute("MIDI", parameter->UnifiedControllerId());

					if(!parameter->getAutomationSysex().empty())
						paramElement->SetAttribute("MIDI_SYSEX", WStringToUtf8(parameter->getAutomationSysex()));
				}
			}
		}
	}

	if(!nameString.empty())
	{
		element->SetAttribute("name", nameString);
	}
	if(!categoryString.empty())
	{
		element->SetAttribute("category", categoryString);
	}

	const char* xmlIndent = " ";

	TiXmlPrinter printer;
	printer.SetIndent( xmlIndent );
	doc.Accept( &printer );

	chunk = printer.CStr();

//	_RPT1(_CRT_WARN, "DspPatchManager::getPresetState:\n%s\n\n", printer.CStr());
}
#endif

#if defined(SE_TARGET_PLUGIN)
void DspPatchManager::setPresetState( const std::string& chunk, bool saveRestartStateUnused)
{
	TiXmlDocument doc;
	doc.Parse( chunk.c_str() );

	if ( doc.Error() )
	{
		//std::wostringstream oss;
		//oss << L"Module XML Error: [" << full_path << L"]" << doc.ErrorDesc() << L"." <<  doc.Value();
		//GetApp()->SeMessageBox( oss.str().c_str(), L"", MB_OK|MB_ICONSTOP );
		return;
	}

	auto params = doc.FirstChild("Preset"); // new format.
	if(params == nullptr)
		params = doc.FirstChild("Parameters"); // old format.

	auto presetXmlElement = params->ToElement();

	// Query plugin's 4-char code. Indicates also that preset format supports MIDI learn.
	int32_t fourCC = -1;
	int formatVersion = 0;
	{
		std::string hexcode;
		if (TIXML_SUCCESS == presetXmlElement->QueryStringAttribute("pluginId", &hexcode))
		{
			formatVersion = 1;
			try
			{
				fourCC = std::stoul(hexcode.c_str(), nullptr, 16);
			}
			catch (...)
			{
				// who gives a f*ck
			}
		}
	}

	std::string categoryName, presetName;
	presetXmlElement->QueryStringAttribute("name", &presetName);
	presetXmlElement->QueryStringAttribute("category", &categoryName);

	for( auto parameter : m_parameters )
	{
		if( parameter->getHostControlId() == HC_PROGRAM_NAME)
		{
			if (parameter->SetValueFromXml(presetName, 0, 0))
			{
				parameter->OnValueChangedFromGUI(false, 0);
			}
		}
		else if( parameter->getHostControlId() == HC_PROGRAM_CATEGORY )
		{
			if (parameter->SetValueFromXml(categoryName, 0, 0))
			{
				parameter->OnValueChangedFromGUI(false, 0);
			}
		}
	}

	for( TiXmlNode* node = params->FirstChild( "Param" ); node; node = node->NextSibling( "Param" ) )
	{
		// check for existing
		TiXmlElement* ParamElement = node->ToElement();
		int id = -1;
		ParamElement->QueryIntAttribute( "id", &id );

		auto parameter = GetParameter(id);

		if(!parameter)
		{
			continue;
		}

		// MIDI learn.
		if(formatVersion > 0)
		{
			int tempController = -1;
			ParamElement->QueryIntAttribute("MIDI", &tempController);
			if (tempController != -1)
			{
				std::string tempSysex;
				ParamElement->QueryStringAttribute("MIDI_SYSEX", &tempSysex);
				parameter->setAutomationSysex(Utf8ToWstring(tempSysex));
				parameter->setAutomation(tempController);
			}
		}

		if (parameter->isPolyphonic()) // crashes scopes.
			continue;

		if (mEmulateIgnoreProgramChange && parameter->ignorePatchChange())
			continue;

		{
			string v = ParamElement->Attribute("val");
			if (parameter->SetValueFromXml(v, 0, 0))
			{
				parameter->OnValueChangedFromGUI(false, 0);
			}
		}
	}

	// TODO: for each parameter not set, return it to it's default value. (would require DSP to store default value, or mayby just use init value.)
}
#endif

void DspPatchManager::setMidiChannel( int c )
{
	midiChannel_ = c;
}

// MIDI program change.
void DspPatchManager::setProgramDspThread( int program )
{
	UpdateProgram( program );
	// inform GUI
	my_msg_que_output_stream strm( m_container->AudioMaster()->getShell()->MessageQueToGui(), Container()->Handle(), "pat2");

	strm << (int) sizeof(program_);
	strm << program_;
	strm.Send();
}

void DspPatchManager::SendInitialUpdates()
{
	// Send MIDI program change.
	Container()->automation_output_device->sendProgramChange( program_ );
	// inform legacy SDK2 modules.
	Container()->SendProgChange( Container(), program_ );
	const int voiceId = 0; // assume poly param all Ignore PC.

	for( auto parameter : m_parameters )
	{
		if( !parameter->isPolyphonic() )
		{
			parameter->OnValueChangedFromGUI( true, voiceId, true );
		}
	}
}

void DspPatchManager::setProgram( int program )
{
	program_ = program;
}

void DspPatchManager::UpdateProgram( int program )
{
	// avoid spurious change when update already came from DSP via MIDI PC.
	if( program_ == program )
	{
		return;
	}

	int previousProgram = program_;
	setProgram( program );

	// Send MIDI program change.
	Container()->automation_output_device->sendProgramChange( program_ );
	// inform legacy SDK2 modules.
	Container()->SendProgChange( Container(), program_ );

	for( auto parameter : m_parameters )
	{
		parameter->OnPatchChanged( previousProgram, program );
	}
}

void DspPatchManager::setupContainerHandles(ug_container* subContainer)
{
	for( auto parameter : m_parameters )
	{
		if (parameter->getVoiceContainerHandle() == subContainer->Handle())
		{
			parameter->setVoiceContainer(subContainer);
		}
	}
}
