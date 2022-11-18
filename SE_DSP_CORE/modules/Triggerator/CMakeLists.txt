cmake_minimum_required(VERSION 3.15)

project(Triggerator)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    SOURCE_FILES 
        TriggeratorGui.cpp
        TriggeratorGui.h
        ${se_sdk_folder}/hasMidiTuning.h
)
