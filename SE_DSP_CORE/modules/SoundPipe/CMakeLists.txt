cmake_minimum_required(VERSION 3.15)

project(Soundpipe)
#..\se_sdk3;..\..\..\SDKs\Soundpipe\h\;..\..\..\SDKs\Soundpipe\;..\..\..\SDKs\Soundpipe\lib\dr_wav\
#;..\..\..\SDKs\Soundpipe\lib\faust\;%(AdditionalIncludeDirectories)
include_directories(
    ${external_sdk_folder}/Soundpipe
    ${external_sdk_folder}/Soundpipe/h
    ${external_sdk_folder}/Soundpipe/lib/dr_wav
    ${external_sdk_folder}/Soundpipe/lib/faust
)

set(soundpipe_srcs
    ${external_sdk_folder}/Soundpipe/modules/base.c
    ${external_sdk_folder}/Soundpipe/modules/jcrev.c
    ${external_sdk_folder}/Soundpipe/modules/revsc.c
    ${external_sdk_folder}/Soundpipe/modules/zitarev.c
    ${external_sdk_folder}/Soundpipe/h/revsc.h
    ${external_sdk_folder}/Soundpipe/h/zitarev.h
)

build_gmpi_plugin(
    PROJECT_NAME ${PROJECT_NAME}
    HAS_DSP
    HAS_GUI
    SOURCE_FILES 
        ReverbChowning.cpp
        ReverbZita.cpp
        ReverbSp.cpp
        ${soundpipe_srcs}
)

source_group(Soundpipe FILES ${soundpipe_srcs})

add_definitions(-DNO_LIBSNDFILE)

