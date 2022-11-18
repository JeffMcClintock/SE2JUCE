cmake_minimum_required(VERSION 3.15)

project(EnveloperAdsr)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES 
        Adsr.cpp
        Adsr.h
        envelope.cpp
        envelope.h
)
