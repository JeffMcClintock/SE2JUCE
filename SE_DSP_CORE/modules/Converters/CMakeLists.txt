cmake_minimum_required(VERSION 3.15)

project(Converters)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    HAS_GUI
    SOURCE_FILES 
        BoolSplitterGui.cpp
        BoolSplitterGui.h
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
        ${se_sdk_folder}/it_enum_list.h
        ${se_sdk_folder}/it_enum_list.cpp
)
