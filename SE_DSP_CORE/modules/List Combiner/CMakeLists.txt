cmake_minimum_required(VERSION 3.15)

project(ListCombiner)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    SOURCE_FILES 
        ListCombinerGui.cpp
        ListCombinerGui.h
        ${se_sdk_folder}/it_enum_list.h
        ${se_sdk_folder}/it_enum_list.cpp
)
