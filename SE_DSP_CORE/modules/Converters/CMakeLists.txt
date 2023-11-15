cmake_minimum_required(VERSION 3.15)

project(Converters)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    IS_OFFICIAL_MODULE
    HAS_DSP
    HAS_GUI
    SOURCE_FILES 
        BoolSplitterGui.cpp
        BoolSplitterGui.h
		BlobToBlob2.cpp
        CommandTriggerGui.cpp
        CommandTriggerGui.h
        Converters.cpp
        ConvertersGui.cpp
        ConvertersGui.h
        DbToAnimationGui.cpp
        DbToAnimationGui.h
        FloatFormula.cpp
        FloatFormula.h
        FloatFormulaGui.cpp
        FloatFormulaGui.h
        FloatToMIDI.cpp
        FloatScaler2Gui.cpp
        FloatScaler2Gui.h
        FloatToText.cpp
        FloatToText.h
        ImageToFrameGui.cpp
        ImageToFrameGui.h
        IntToBoolsGui.cpp
        IntToBoolsGui.h
        ListToAnimationGui.cpp
        ListToTextGui.cpp
        ListToTextGui.h
        MenuToListboxesGui.cpp
        my_type_convert.cpp
        my_type_convert.h
        UnitConverter.cpp
        UnitConverter.h
        VoltsToMidiCc.cpp
        VoltsToMidiCc.h
        ../shared/expression_evaluate.h
        ../shared/expression_evaluate.cpp
        ../shared/simdutf/simdutf.h
        ../shared/simdutf/simdutf.cpp
        ${se_sdk_folder}/it_enum_list.h
        ${se_sdk_folder}/it_enum_list.cpp
)

# enable super fast UTF conversions
target_compile_definitions(
  ${PROJECT_NAME} PRIVATE 
  USE_SIMD_UTF_CONVERSION
)
