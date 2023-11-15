cmake_minimum_required(VERSION 3.15)

project(MidiLogger)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_DSP
    SOURCE_FILES 
        IdeLogger.cpp
        MidiLogger.cpp
        MidiLogPlayback.cpp
        MidiLogger.h
        MidiLogPlayback.h
)
