cmake_minimum_required(VERSION 3.15)

project(OversamplingControl)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    HAS_DSP
    SOURCE_FILES 
        OversamplingControlGui.h
        OversamplingControlGui.cpp
        LatencyTest.h
        LatencyTest.cpp
)
