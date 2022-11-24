cmake_minimum_required(VERSION 3.15)

project(Waveshapers)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    HAS_GUI
    SOURCE_FILES 
        Waveshapers.h
        Waveshapers.cpp
        Waveshaper2.h
        Waveshaper2.cpp
        ../shared/unicode_conversion.h
        ../shared/expression_evaluate.h
        ../shared/expression_evaluate.cpp
      
)
# 32-bit only
#        WaveshapersGui.h
#        WaveshapersGui.cpp
