cmake_minimum_required(VERSION 3.15)

project(SampleRate)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_DSP
    SOURCE_FILES 
        SampleRate.h
        SampleRate.cpp
 )
