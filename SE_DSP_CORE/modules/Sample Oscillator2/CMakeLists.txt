cmake_minimum_required(VERSION 3.15)

project(SampleOscillator2)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_GUI
    HAS_DSP
    SOURCE_FILES 
        SampleOscillator2.h
        SampleOscillator2.cpp
        SampleLoader2.h
        SampleLoader2.cpp
        SampleLoader2Gui.h
        SampleLoader2Gui.cpp
        SampleManager.h
        SampleManager.cpp
        SampleExclusiveFilter.h
        SampleExclusiveFilter.cpp
        CSoundFont.h
        CSoundFont.cpp
        soundfont_user.h
        soundfont_user.cpp
        RiffFile2.h
        RiffFile2.cpp
)
