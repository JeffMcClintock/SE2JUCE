cmake_minimum_required(VERSION 3.15)

project(SubControlsXp)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_GUI
    SOURCE_FILES 
        ConcatGui.cpp
        GraphicsFrameRateGui.h
        GraphicsFrameRateGui.cpp
        Image2GuiXp.h
        Image2GuiXp.cpp
        ../sharedLegacyWidgets/ImageBase.h
        ../sharedLegacyWidgets/ImageBase.cpp
        ImageTinted2Gui.h
        ImageTinted2Gui.cpp
        JoystickImageGui.h
        JoystickImageGui.cpp
        ListboxGui.cpp
        ListEntry4Gui.h
        ListEntry4Gui.cpp
        MenuCombinerGui.cpp
        MPEControlGui.cpp
        PanelGroupGui.h
        PanelGroupGui.cpp
        PlainImageGui.h
        PlainImageGui.cpp
        PopupMenuGui.h
        PopupMenuGui.cpp
        RectangleGui.h
        RectangleGui.cpp
        TextEntry4Gui.h
        TextEntry4Gui.cpp
        StructureGroupGui.cpp
        TextSubcontrol.h
        TextSubcontrol.cpp
        VectorRingGui.cpp

        ${se_sdk_folder}/it_enum_list.h
        ${se_sdk_folder}/it_enum_list.cpp
        ${se_sdk_folder}/TimerManager.h
        ${se_sdk_folder}/TimerManager.cpp
        ../shared/FontCache.h
        ../shared/FontCache.cpp
        ../shared/ImageCache.h
        ../shared/ImageCache.cpp
        ../shared/ImageMetadata.h
        ../shared/ImageMetadata.cpp
        ../shared/xplatform_modifier_keys.h
        ../shared/xplatform_modifier_keys.cpp
)

if(APPLE)

# Timers need CoreFoundation
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )

endif()