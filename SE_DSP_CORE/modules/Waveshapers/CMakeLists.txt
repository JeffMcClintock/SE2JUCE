cmake_minimum_required(VERSION 3.15)

project(Waveshapers)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_DSP
    HAS_GUI
    SOURCE_FILES 
        Waveshapers.h
        Waveshapers.cpp
        Waveshaper2.h
        Waveshaper2.cpp
        Waveshaper2BGUIGui.cpp
        ../shared/unicode_conversion.h
        ../shared/expression_evaluate.h
        ../shared/expression_evaluate.cpp
        ../shared/FontCache.h
        ../shared/FontCache.cpp
        ../shared/ImageMetadata.h
        ../shared/ImageMetadata.cpp
      
)
# 32-bit only
#        WaveshapersGui.h
#        WaveshapersGui.cpp
