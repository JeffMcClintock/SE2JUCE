cmake_minimum_required(VERSION 3.15)

project(SubControls)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_GUI
    HAS_DSP
    SOURCE_FILES 
        CPUFamilyGui.cpp
        FileDialogGui.h
        FileDialogGui.cpp
        PitchBender.cpp
        PitchBenderGui.cpp
        OkCancelDialogGui.cpp
        ${se_sdk_folder}/it_enum_list.h
        ${se_sdk_folder}/it_enum_list.cpp
)

if(APPLE)

# Timers need CoreFoundation
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )

endif()