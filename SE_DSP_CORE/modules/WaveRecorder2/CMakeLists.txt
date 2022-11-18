cmake_minimum_required(VERSION 3.15)

project(WaveRecorder2)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES 
        WaveRecorder2.h
        WaveRecorder2.cpp
        ${se_sdk_folder}/PinIterator.h
        ../shared/string_utilities.h
)
