cmake_minimum_required(VERSION 3.15)

project(Increment3)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    SOURCE_FILES 
        Increment3Gui.cpp
        Increment3Gui.h
        ${se_sdk_folder}/it_enum_list.h
        ${se_sdk_folder}/it_enum_list.cpp
)
