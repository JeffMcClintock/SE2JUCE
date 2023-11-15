cmake_minimum_required(VERSION 3.15)

project(BoolInverter)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_GUI
    SOURCE_FILES BoolInverterGui.h BoolInverterGui.cpp
)