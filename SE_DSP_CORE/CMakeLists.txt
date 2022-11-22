cmake_minimum_required(VERSION 3.15)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)

project(SynthEditLib)

add_definitions(-DSE_TARGET_PLUGIN)

set(GMPI_SDK ${CMAKE_CURRENT_SOURCE_DIR}/../GMPI_SDK/Core)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GMPI_SDK}
    )


set(sdk_srcs
${se_sdk_folder}/mp_sdk_common.h
${se_sdk_folder}/mp_sdk_common.cpp
${se_sdk_folder}/mp_sdk_audio.h
${se_sdk_folder}/mp_sdk_audio.cpp
${se_sdk_folder}/mp_sdk_gui.h
${se_sdk_folder}/mp_sdk_gui.cpp
${se_sdk_folder}/TimerManager.h
${se_sdk_folder}/TimerManager.cpp
${se_sdk_folder}/smart_audio_pin.h
${se_sdk_folder}/smart_audio_pin.cpp

modules/shared/it_enum_list.h
modules/shared/it_enum_list.cpp
modules/shared/PatchCables.h
modules/shared/PatchCables.cpp
modules/shared/xp_dynamic_linking.h
modules/shared/xp_dynamic_linking.cpp
modules/shared/FileWatcher.h
modules/shared/FileWatcher.cpp
modules/shared/xplatform_modifier_keys.h
modules/shared/xplatform_modifier_keys.cpp
modules/shared/FileFinder.h
modules/shared/FileFinder.cpp

)

set(srcs_json
../Shared/jsoncpp/json/json.h
../Shared/jsoncpp/jsoncpp.cpp
)

set(srcs_xml
tinyxml/tinyxml.h
tinyxml/tinyxml.cpp
tinyxml/tinyxmlerror.cpp
tinyxml/tinyxmlparser.cpp
modules/tinyXml2/tinyxml2.h
modules/tinyXml2/tinyxml2.cpp
)

set(srcs_modules_internal
modules_internal/VoiceMute/VoiceMute.h
modules_internal/VoiceMute/VoiceMute.cpp
modules_internal/CpuMeter/CpuMeterGui.cpp
modules_internal/MIDI2Converter.cpp
modules_internal/MPEToMIDI2.cpp
modules_internal/PatchPoints.cpp
modules_internal/PatchPointsGui.h
modules_internal/PatchPointsGui.cpp
modules_internal/PrefabBrowser/PrefabBrowserGui.cpp
modules_internal/RegistrationCheck.cpp
)

set(srcs_dspfilters
DSPFilters/Biquad.cpp
DSPFilters/Butterworth.cpp
DSPFilters/Cascade.cpp
DSPFilters/Elliptic.cpp
DSPFilters/Filter.cpp
DSPFilters/Param.cpp
DSPFilters/PoleFilter.cpp
DSPFilters/Biquad.h
DSPFilters/Cascade.h
DSPFilters/Common.h
DSPFilters/Design.h
DSPFilters/Elliptic.h
DSPFilters/Filter.h
DSPFilters/Layout.h
DSPFilters/MathSupplement.h
DSPFilters/Params.h
DSPFilters/PoleFilter.h
DSPFilters/State.h
DSPFilters/Types.h
)


set(srcs
../Shared/AuPreset.h
../Shared/AuPreset.cpp

modules/se_sdk3_hosting/gmpi_gui_hosting.h
modules/se_sdk3_hosting/gmpi_gui_hosting.cpp
modules/se_sdk3_hosting/DirectXGfx.h
modules/se_sdk3_hosting/DirectXGfx.cpp
modules/se_sdk3_hosting/DrawingFrame_win32.h
modules/se_sdk3_hosting/DrawingFrame_win32.cpp
modules/se_sdk3_hosting/Controller.h
modules/se_sdk3_hosting/Controller.cpp
modules/se_sdk3_hosting/ContainerView.h
modules/se_sdk3_hosting/ContainerView.cpp
modules/se_sdk3_hosting/ViewBase.h
modules/se_sdk3_hosting/ViewBase.cpp
modules/se_sdk3_hosting/SubViewPanel.h
modules/se_sdk3_hosting/SubViewPanel.cpp
modules/se_sdk3_hosting/MpParameter.h
modules/se_sdk3_hosting/MpParameter.cpp
modules/se_sdk3_hosting/ConnectorView.h
modules/se_sdk3_hosting/ConnectorView.cpp
modules/se_sdk3_hosting/ModuleView.h
modules/se_sdk3_hosting/ModuleView.cpp
modules/se_sdk3_hosting/GmpiResourceManager.h
modules/se_sdk3_hosting/GmpiResourceManager.cpp
modules/se_sdk3_hosting/GuiPatchAutomator3.h
modules/se_sdk3_hosting/GuiPatchAutomator3.cpp
modules/se_sdk3_hosting/IViewChild.h
modules/se_sdk3_hosting/IViewChild.cpp

../Shared/VstPreset.h
../Shared/VstPreset.cpp
../Shared/Vst2Preset.h
../Shared/Vst2Preset.cpp

${VST3_SDK}/public.sdk/source/vst/vstpresetfile.h
${VST3_SDK}/public.sdk/source/vst/vstpresetfile.cpp

UgDebugInfo.cpp

Base64.h
BundleInfo.cpp
BundleInfo.h
cancellation.h
CMakeLists.txt
conversion.cpp
conversion.h
CpuConsumption.h
CVoiceList.cpp
CVoiceList.h
datatype_to_id.cpp
datatype_to_id.h
denormal_fixer.h
DspFilterJeff.h
DspPatchManagerProxy.cpp
DspPatchManagerProxy.h
DspRingBufferReader.h
DspRingBufferWriter.h
dsp_msg_target.h
dsp_patch_manager.cpp
dsp_patch_manager.h
dsp_patch_parameter.cpp
dsp_patch_parameter.h
dsp_patch_parameter_base.h
ElatencyContraintType.h
EventProcessor.cpp
EventProcessor.h
fixed_memory_manager.h
HostControls.cpp
HostControls.h
HostVoiceControl.h
IDspPatchManager.h
IGuiHost2.h
InterfaceObject.cpp
InterfaceObject.h
interThreadQue.cpp
interThreadQue.h
IPluginGui.cpp
IPluginGui.h
ISeAudioMaster.h
iseshelldsp.h
iterator.h
lock_free_fifo.h
Logic.h
mfc_emulation.h
mfc_wstring.cpp
mfc_wstring.h
MidiEvent.h
midistreambuffer.h
MidiToCv2.cpp
MidiToCv2.h
midi_defs.cpp
midi_defs.h
module_info.h
Module_Info3.cpp
Module_Info3.h
Module_Info3_base.cpp
Module_Info3_base.h
Module_Info3_internal.h
module_register.h
MpPinIterator.h
MySmartPtr.h
MyTypeTraits.cpp
MyTypeTraits.h
my_input_stream.cpp
my_input_stream.h
my_msg_que_input_stream.h
my_msg_que_output_stream.h
notify.cpp
notify.h
Notify_msg.h
PatchStorage.cpp
PatchStorage.h
pch.cpp
pch.h
plug_description.h
ProtectedFile.cpp
ProtectedFile.h
QueClient.h
RawConversions.cpp
RawConversions.h
resource.h
sample.h
Sdk3CloneIterator.h
SeAudioMaster.cpp
SeAudioMaster.h
SerializationHelper_XML.h
se_types.h
SincFilter.h
smart_output.h
srange.h
SynthRuntime.cpp
SynthRuntime.h
TypeToEDatatype.h
UgDatabase.cpp
UgDatabase.h
ug_adder2.cpp
ug_adder2.h
ug_adsr.cpp
ug_adsr.h
ug_base.cpp
ug_base.h
ug_clipper.cpp
ug_clipper.h
ug_combobox.cpp
ug_combobox.h
ug_comparator.cpp
ug_comparator.h
ug_container.cpp
ug_container.h
ug_control.h
ug_cross_fade.cpp
ug_cross_fade.h
ug_cv_midi.cpp
ug_cv_midi.h
ug_default_setter.cpp
ug_default_setter.h
ug_delay.cpp
ug_delay.h
ug_denormal_detect.cpp
ug_denormal_detect.h
ug_denormal_stop.cpp
ug_denormal_stop.h
ug_envelope.cpp
ug_envelope.h
ug_envelope_base.cpp
ug_envelope_base.h
ug_event.h
ug_feedback_delays.cpp
ug_feedback_delays.h
ug_filter_1pole_lp.cpp
ug_filter_1pole_lp.h
ug_filter_allpass.cpp
ug_filter_allpass.h
ug_filter_biquad.cpp
ug_filter_biquad.h
ug_filter_sv.cpp
ug_filter_sv.h
ug_fixed_values.cpp
ug_fixed_values.h
ug_flags.h
ug_float_to_volts.cpp
ug_float_to_volts.h
ug_generic_1_1.cpp
ug_generic_1_1.h
ug_io_mod.cpp
ug_io_mod.h
ug_latency_adjust.h
ug_latency_adjust_event.h
ug_logic_Bin_Count.cpp
ug_logic_Bin_Count.h
ug_logic_complex.cpp
ug_logic_complex.h
ug_logic_counter.cpp
ug_logic_counter.h
ug_logic_decade.cpp
ug_logic_decade.h
ug_logic_gate.cpp
ug_logic_gate.h
ug_logic_not.cpp
ug_logic_not.h
ug_logic_shift.cpp
ug_logic_shift.h
ug_math_base.cpp
ug_math_base.h
ug_math_ceil.cpp
ug_math_ceil.h
ug_math_floor.cpp
ug_math_floor.h
ug_midi_automator.cpp
ug_midi_automator.h
ug_midi_controllers.cpp
ug_midi_controllers.h
ug_midi_device.cpp
ug_midi_device.h
ug_midi_filter.cpp
ug_midi_filter.h
ug_midi_keyboard.cpp
ug_midi_keyboard.h
ug_midi_to_cv.cpp
ug_midi_to_cv.h
ug_monostable.cpp
ug_monostable.h
ug_multiplier.cpp
ug_multiplier.h
ug_notesource.h
ug_oscillator2.cpp
ug_oscillator2.h
ug_oscillator_pd.cpp
ug_oscillator_pd.h
ug_oversampler.cpp
ug_oversampler.h
ug_oversampler_in.h
ug_oversampler_io.h
ug_oversampler_out.cpp
ug_oversampler_out.h
ug_pan.cpp
ug_pan.h
ug_patch_automator.cpp
ug_patch_automator.h
ug_patch_param_setter.cpp
ug_patch_param_setter.h
ug_patch_param_watcher.cpp
ug_patch_param_watcher.h
ug_peak_det.cpp
ug_peak_det.h
ug_plugin3.cpp
ug_plugin3.h
ug_quantiser.cpp
ug_quantiser.h
ug_random.cpp
ug_random.h
ug_sample_hold.cpp
ug_sample_hold.h
ug_slider.cpp
ug_slider.h
ug_switch.cpp
ug_switch.h
ug_switch2.h
ug_system_command.cpp
ug_system_command.h
ug_system_modules.cpp
ug_test_tone.cpp
ug_test_tone.h
ug_text_entry.cpp
ug_text_entry.h
ug_vca.cpp
ug_vca.h
ug_voice_monitor.cpp
ug_voice_monitor.h
ug_voice_splitter.cpp
ug_voice_splitter.h
ug_voltage_to_enum.cpp
ug_voltage_to_enum.h
ug_volts_to_float.cpp
ug_volts_to_float.h
ug_vst_in.cpp
ug_vst_in.h
ug_vst_out.cpp
ug_vst_out.h
ULookup.cpp
ULookup.h
UMidiBuffer2.h
UniqueSnowflake.cpp
UniqueSnowflake.h
UPlug.cpp
UPlug.h
USampBlock.h
variable_policies.cpp
variable_policies.h
version.h
conversion.h
conversion.cpp
)

IF(APPLE)
set(srcs ${srcs}
modules/se_sdk3_hosting/EventHelper.h
modules/se_sdk3_hosting/SynthEditCocoaView.h
modules/se_sdk3_hosting/SynthEditCocoaView.mm
modules/se_sdk3_hosting/CocoaGuiHost.h
modules/se_sdk3_hosting/CocoaGuiHost.mm
)
ENDIF()


IF(SE2JUCE)
set(srcs
${srcs}
modules/shared/real_fft.h
modules/shared/real_fft.cpp
modules/shared/expression_evaluate.h
modules/shared/expression_evaluate.cpp


modules/EnvelopeAdsr/envelope.cpp
modules/EnvelopeAdsr/Adsr.cpp
modules/ControlsXp/ListEntry.cpp
modules/ControlsXp/Scope3.cpp
modules/Converters/FloatToText.cpp
modules/Converters/VoltsToMidiCc.cpp

modules/PatchMemory/PatchMemoryBool.cpp
modules/PatchMemory/PatchMemoryFloat.cpp
modules/PatchMemory/PatchMemoryInt.cpp
modules/PatchMemory/PatchMemoryList.cpp
modules/PatchMemory/PatchMemoryText.cpp
modules/PatchMemory/PatchBrowserGui.cpp
modules/PatchMemory/PatchInfoGui.cpp
modules/PatchMemory/PatchMemoryBoolOutGui.cpp
modules/PatchMemory/PatchMemoryfloatGui.cpp
modules/PatchMemory/PatchMemoryFloatOutGui.cpp
modules/PatchMemory/PatchMemoryListGui.cpp
modules/PatchMemory/PatchMemoryTextOutGui.cpp

modules/FloatLimiter/FloatLimiterGui.cpp
modules/SubControlsXp/Image2GuiXp.cpp
modules/SubControlsXp/PanelGroupGui.cpp
modules/SubControlsXp/PlainImageGui.cpp
modules/PolyphonyControl/PolyphonyControlGui.cpp
modules/SubControlsXp/PopupMenuGui.cpp
modules/SubControlsXp/TextEntry4Gui.cpp
modules/SubControlsXp/TextSubcontrol.cpp
modules/UserSetting/UserSettingText.cpp
modules/UserSetting/UserSettingTextController.cpp
modules/UserSetting/UserSettingTextGui.cpp
modules/VaFilters/SvFilter.cpp
modules/BPMClock3/BpmClock3.cpp
modules/BPMClock3/BpmTempo.cpp
modules/Filters/ButterworthLp.cpp
modules/Converters/Converters.cpp
modules/ControlsXp/FreqAnalyser.cpp
modules/ImpulseResponse/ImpulseResponse.cpp
modules/Increment3/Increment3Gui.cpp
modules/Inverter/Inverter.cpp
modules/MidiPlayer2/MidiToGate.cpp
"modules/MIDI Logger/IdeLogger.cpp"
modules/Converters/my_type_convert.cpp
modules/NoteExpression/NoteExpression.cpp
modules/OscillatorNaive/OscillatorNaive.cpp
modules/OversamplingControl/OversamplingControlGui.cpp
modules/ControlsXp/Slider.cpp
modules/SoftDistortion/SoftDistortion.cpp
modules/Switches/Switches.cpp
modules/Converters/UnitConverter.cpp
modules/ControlsXp/VoltMeter.cpp
modules/ControlsXp/Waveshaper3Xp.cpp
modules/Waveshapers/Waveshapers.cpp
modules/EnvelopeAdsr/Adsr.h
modules/EnvelopeAdsr/Envelope.h
modules/ControlsXp/ClassicControlGuiBase.h
modules/ControlsXp/FreqAnalyserGui.h
modules/ControlsXp/ListEntry.h
modules/ControlsXp/Scope3.h
modules/ControlsXp/Scope3Gui.h
modules/ControlsXp/SliderGui.h
modules/ControlsXp/VoltMeterGui.h
modules/BoolInverter/BoolInverterGui.h
modules/Converters/Converters.h
modules/Converters/ConvertersGui.h
modules/Converters/FloatToText.h
modules/Converters/VoltsToMidiCc.h
modules/PatchMemory/PatchInfoGui.h
modules/PatchMemory/PatchMemoryBoolOutGui.h
modules/PatchMemory/PatchMemoryFloatGui.h
modules/PatchMemory/PatchMemoryFloatOutGui.h
modules/PatchMemory/PatchMemoryList.h
modules/PatchMemory/PatchMemoryListGui.h
modules/PatchMemory/PatchMemoryTextOutGui.h
modules/sharedLegacyWidgets/BitmapWidget.h
modules/FloatLimiter/FloatLimiterGui.h
modules/SubControlsXp/Image2GuiXp.h
modules/SubControlsXp/TextSubcontrol.h
modules/UserSetting/SettingFile.h
modules/VaFilters/SvFilter.h
modules/Filters/ButterworthLp.h
modules/ControlsXp/FreqAnalyser.h
modules/Increment3/Increment3Gui.h
modules/OscillatorNaive/OscillatorNaive.h
modules/OversamplingControl/OversamplingControlGui.h
modules/ControlsXp/Slider.h
modules/ControlsXp/VoltMeter.h

# Common GUI modules
modules/sharedLegacyWidgets/EditWidget.h
modules/sharedLegacyWidgets/ImageBase.h
modules/sharedLegacyWidgets/ListWidget.h
modules/sharedLegacyWidgets/RotarySwitchWidget.h
modules/sharedLegacyWidgets/SubControlBase.h
modules/sharedLegacyWidgets/TextEditWidget.h
modules/sharedLegacyWidgets/TextWidget.h
modules/sharedLegacyWidgets/Widget.h
modules/sharedLegacyWidgets/WidgetHost.h

modules/sharedLegacyWidgets/BitmapWidget.cpp
modules/sharedLegacyWidgets/EditWidget.cpp
modules/sharedLegacyWidgets/ImageBase.cpp
modules/sharedLegacyWidgets/ListWidget.cpp
modules/sharedLegacyWidgets/TextEditWidget.cpp
modules/sharedLegacyWidgets/TextWidget.cpp
modules/sharedLegacyWidgets/Widget.cpp

modules/shared/ImageCache.h
modules/shared/ImageCache.cpp
modules/shared/FontCache.h
modules/shared/FontCache.cpp
modules/shared/ImageMetadata.h
modules/shared/ImageMetadata.cpp

modules/ControlsXp/ListEntryGui.h
modules/ControlsXp/ListEntryGui.cpp
modules/ControlsXp/FreqAnalyserGui.cpp
modules/ControlsXp/ClassicControlGuiBase.cpp
modules/ControlsXp/Scope3Gui.cpp
modules/ControlsXp/SliderGui.h
modules/ControlsXp/SliderGui.cpp
modules/ControlsXp/VoltMeterGui.cpp

modules/SubControlsXp/PanelGroupGui.h
modules/SubControlsXp/PopupMenuGui.h
modules/SubControlsXp/ListEntry4Gui.cpp
modules/SubControlsXp/ListEntry4Gui.h
modules/SubControlsXp/TextEntry4Gui.h
modules/SubControlsXp/PlainImageGui.h
modules/SubControlsXp/PlainImageGui.cpp

modules/BoolInverter/BoolInverterGui.cpp
modules/Converters/ConvertersGui.cpp
modules/PolyphonyControl/PolyphonyControlGui.h

)
endif()

# message(STATUS "DSP_CORE: ${DSP_CORE}")
# message(STATUS "se_sdk_folder: ${se_sdk_folder}")

include_directories(
${DSP_CORE}
${se_sdk_folder}
${VST3_SDK}
${GMPI_SDK}
)

source_group(se_sdk FILES ${sdk_srcs})
source_group(json FILES ${srcs_json})
source_group(modules_internal FILES ${srcs_modules_internal})
source_group(xml FILES ${srcs_xml})
source_group(DSPFilters FILES ${srcs_dspfilters})

if(APPLE)
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)
endif()

include (GenerateExportHeader)
add_library(${PROJECT_NAME} STATIC ${sdk_srcs} ${srcs} ${srcs_json} ${srcs_modules_internal} ${srcs_xml} ${srcs_dspfilters})

target_compile_definitions(${PROJECT_NAME} PRIVATE 
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )

IF(WIN32)
target_link_libraries(${PROJECT_NAME} d3d11.lib d2d1 dwrite windowscodecs)
endif()

target_compile_definitions (${PROJECT_NAME} PRIVATE -D_UNICODE -DUNICODE)

