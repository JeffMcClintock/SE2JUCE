cmake_minimum_required(VERSION 3.15)

project(BpmClock3)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES BpmClock3.cpp BpmClock4.h BpmClock4.cpp BpmTempo.cpp
)
