cmake_minimum_required(VERSION 3.15)

project(ImpulseResponse)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_DSP
    HAS_GUI
    SOURCE_FILES 
        ImpulseResponse.cpp
        ImpulseResponse.h
        ImpulseResponseGui.cpp
        ImpulseResponseGui.h
 )
