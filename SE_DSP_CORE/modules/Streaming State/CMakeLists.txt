cmake_minimum_required(VERSION 3.15)

project(StreamingState)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_DSP
    SOURCE_FILES 
        StreamingState.h
        StreamingState.cpp
        ${se_sdk_folder}/smart_audio_pin.h
        ${se_sdk_folder}/smart_audio_pin.cpp
)
