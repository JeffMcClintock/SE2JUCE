cmake_minimum_required(VERSION 3.15)

project(ControlsXp)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_DSP
    HAS_GUI
    SOURCE_FILES 
        ClassicControlGuiBase.cpp
        ClassicControlGuiBase.h
        DrawKeyboard.h
        FreqAnalyser.cpp
        FreqAnalyser.h
        FreqAnalyserGui.cpp
        FreqAnalyserGui.h
        ../ImpulseResponse/Impulse.h
        ../ImpulseResponse/Impulse.cpp
        ImpulseResponse2.cpp
        ImpulseResponse2Gui.cpp
        ../keyboard2/keyboard.h
        ../keyboard2/keyboard.cpp
        Keyboard2Gui.cpp
        Keyboard2Gui.h
        KeyboardMidi.cpp
        KeyboardMidiGui.cpp
        KeyboardMpe.cpp
        KeyboardMpeGui.cpp
        ListEntry.cpp
        ListEntry.h
        ListEntryGui.cpp
        ListEntryGui.h
        MidiMonitor.cpp
        MidiMonitor.h
        MidiMonitorGui.cpp
        Monitor.cpp
        Scope3.cpp
        Scope3.h
        Scope3Gui.cpp
        Scope3Gui.h
        Slider.cpp
        Slider.h
        Slider2.cpp
        Slider2.h
        Slider2Gui.cpp
        Slider2Gui.h
        SliderGui.cpp
        SliderGui.h
        TextEntry.cpp
        TextEntry.h
        TextEntryGui.cpp
        TextEntryGui.h
        TriggerScope.cpp
        TriggerScope.h
        VoltMeter.cpp
        VoltMeter.h
        VoltMeterGui.cpp
        VoltMeterGui.h
        Waveshaper3Xp.cpp
        Waveshaper3XpGui.cpp
        ../sharedLegacyWidgets/BitmapWidget.cpp
        ../sharedLegacyWidgets/BitmapWidget.h
        ../sharedLegacyWidgets/EditWidget.cpp
        ../sharedLegacyWidgets/EditWidget.h
        ../sharedLegacyWidgets/ImageBase.cpp
        ../sharedLegacyWidgets/ImageBase.h
        ../sharedLegacyWidgets/ListWidget.cpp
        ../sharedLegacyWidgets/ListWidget.h
        ../sharedLegacyWidgets/RotarySwitchWidget.h
        ../sharedLegacyWidgets/SubControlBase.h
        ../sharedLegacyWidgets/TextEditWidget.cpp
        ../sharedLegacyWidgets/TextEditWidget.h
        ../sharedLegacyWidgets/TextWidget.cpp
        ../sharedLegacyWidgets/TextWidget.h
        ../sharedLegacyWidgets/Widget.cpp
        ../sharedLegacyWidgets/Widget.h
        ../sharedLegacyWidgets/WidgetHost.h
        ../shared/FontCache.h
        ../shared/FontCache.cpp
        ../shared/ImageCache.h
        ../shared/ImageCache.cpp
        ../shared/ImageMetadata.h
        ../shared/ImageMetadata.cpp
        ../shared/expression_evaluate.h
        ../shared/expression_evaluate.cpp
        ../shared/real_fft.h
        ../shared/real_fft.cpp
        ${se_sdk_folder}/smart_audio_pin.h
        ${se_sdk_folder}/smart_audio_pin.cpp
        ${se_sdk_folder}/it_enum_list.h
        ${se_sdk_folder}/it_enum_list.cpp
        ${se_sdk_folder}/TimerManager.h
        ${se_sdk_folder}/TimerManager.cpp
)

if(APPLE)

# Timers need CoreFoundation
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )

endif()