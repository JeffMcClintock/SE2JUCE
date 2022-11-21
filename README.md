# Introduction 
SE2JUCE supports exporting SynthEdit projects to C++ JUCE projects. 

# Prerequisites
Install Visual Studio or your IDE of choice
Install CMake

# Getting Started with the PD303 example
clone SE2JUCE repo
clone JUCE, Add VST2 headers if you need them. Clone AAX SDK if you need it.

clone SE2JUCE_Projects repo

Open SE
open project from SE2JUCE_Plugins/PD303/SE_Project/PD303.se1
"File/Export Juce"
close SE

Open Cmake
Under "where is the source code" enter location of SE2JUCE_Plugins folder
Under "where to build the binaries" enter something like C:\build_SE2JUCE_Plugins (or anywhere you prefer to put the temporary files created during the build).
Click 'Configure", choose whatever IDE you prefer. Ignore the error message.
enter the correct JUCE folder, and the correct SE2JUCE folder
click 'generate'
click 'open project' (your IDE should open)
build and try out the plugin
