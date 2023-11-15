cmake_minimum_required(VERSION 3.15)

project(UserSettingText)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_GUI
    HAS_DSP
    SOURCE_FILES 
        UserSettingText.cpp
        UserSettingTextGui.cpp
        UserSettingTextController.cpp
        ../tinyXml2/TinyXml2.h
        ../tinyXml2/TinyXml2.cpp
)
