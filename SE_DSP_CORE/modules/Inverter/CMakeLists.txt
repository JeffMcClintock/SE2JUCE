cmake_minimum_required(VERSION 3.15)

project(Inverter)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES 
        Inverter.cpp
        Inverter.h
)
