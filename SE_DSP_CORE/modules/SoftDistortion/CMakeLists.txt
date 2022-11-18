cmake_minimum_required(VERSION 3.15)

project(Softdistortion)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES 
        Softdistortion.h
        Softdistortion.cpp
 )
