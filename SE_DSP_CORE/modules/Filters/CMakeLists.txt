cmake_minimum_required(VERSION 3.15)

project(Filters)

set( DSP_FILTERS_SRC
        DSPFilters/Biquad.h
        DSPFilters/Biquad.cpp
        DSPFilters/Butterworth.h
        DSPFilters/Butterworth.cpp
        DSPFilters/Cascade.h
        DSPFilters/Cascade.cpp
        DSPFilters/Elliptic.h
        DSPFilters/Elliptic.cpp
        DSPFilters/PoleFilter.h
        DSPFilters/PoleFilter.cpp
        DSPFilters/Common.h
        DSPFilters/Layout.h
        DSPFilters/MathSupplement.h
)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    SOURCE_FILES
        IIR_Filters2.cpp
        Lookahead.cpp
        ButterworthLp.cpp
        ButterworthLp.h
        ../shared/FilterBase.h
        ${DSP_FILTERS_SRC}

)

source_group(DspFilter FILES ${DSP_FILTERS_SRC})
