cmake_minimum_required(VERSION 3.15)

project(Spring2)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    SOURCE_FILES 
        Spring2Gui.h
        Spring2Gui.cpp
 )
