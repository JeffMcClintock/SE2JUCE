cmake_minimum_required(VERSION 3.15)

project(Patchcablediag)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_GUI
    SOURCE_FILES 
        PatchcablediagGui.h
        PatchcablediagGui.cpp
        ../shared/PatchCables.h
        ../shared/PatchCables.cpp
        ../tinyxml2/tinyxml2.h
        ../tinyxml2/tinyxml2.cpp
)
