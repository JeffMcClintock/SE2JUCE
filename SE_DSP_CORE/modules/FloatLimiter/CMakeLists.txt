cmake_minimum_required(VERSION 3.15)

project(FloatLimiter)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_GUI
    SOURCE_FILES 
        FloatLimiterGui.cpp
        FloatLimiterGui.h
        ${se_sdk_folder}/TimerManager.h
        ${se_sdk_folder}/TimerManager.cpp
 )

if(APPLE)

# Timers need CoreFoundation
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COREFOUNDATION_LIBRARY} )

endif()