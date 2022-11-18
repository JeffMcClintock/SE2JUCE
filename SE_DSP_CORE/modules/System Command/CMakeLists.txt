cmake_minimum_required(VERSION 3.15)

project(SystemCommand)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    SOURCE_FILES 
        SystemCommandGui.cpp
        SystemCommandGui.h
)

if(APPLE)

set_source_files_properties(SystemCommandGui.cpp PROPERTIES LANGUAGE OBJCXX)

# Cocoa needs CoreFoundation
FIND_LIBRARY(COREFOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)

FIND_LIBRARY(COCOA_LIBRARY Cocoa )
MARK_AS_ADVANCED (COCOA_LIBRARY)

TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${COCOA_LIBRARY} ${COREFOUNDATION_LIBRARY})

endif()