cmake_minimum_required(VERSION 3.15)

project(VaFilters)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES 
        KorgFilter.cpp
        KorgFilter.h
        VAOnePoleFilter.cpp
        VAOnePoleFilter.h
        LadderFilter.cpp
        LadderFilter.h
        RMS/Rms.cpp
        RMS/Rms.h
        SvFilter.cpp
        SvFilter.h
        VAFilters/pluginconstants.h
        VAFilters/pluginobjects.cpp
        VAFilters/MoogFilterStage.cpp
        VAFilters/MoogFilterStage.h
        VAFilters/MoogLadderFilter.cpp
        VAFilters/MoogLadderFilter.h
        VAFilters/TPTMoogFilterStage.cpp
        VAFilters/TPTMoogFilterStage.h
        VAFilters/TPTMoogLadderFilter.cpp
        VAFilters/TPTMoogLadderFilter.h
        SvFilterClassic/SvFilter2.cpp
        SvFilterClassic/SvFilter2.h
)
