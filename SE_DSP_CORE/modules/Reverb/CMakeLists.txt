cmake_minimum_required(VERSION 3.15)

project(Reverb)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES 
        Reverb.h
        Reverb.cpp
)
