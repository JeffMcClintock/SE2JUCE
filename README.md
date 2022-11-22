# Introduction 
SE2JUCE supports exporting SynthEdit projects to C++ JUCE projects.

JUCE Projects
* Can target more plugin formats, like AAX, Standalone, VST2 and CLAP etc.
* Hide all resources and SEMs inside a single binary file.
* Can display the GUI you created in SynthEdit, or a custom GUI made with JUCE.

Note that using SE2JUCE is a complex, advanced proceedure that involves programming in C++ and having an understanding of CMake and JUCE.
Also SE2JUCE requires you to have access to the source-code of any SEMs you wish to use. This may not be possible in some cases,
 especially with 3rd-party modules. 3rd-party module creators have no obligation to share their source-code.

# Prerequisites

Install SynthEdit 1.5 Alpha. https://synthedit.com/alpha/?url=/alpha

Install Visual Studio or your IDE of choice. https://visualstudio.microsoft.com/vs/

Install CMake. https://cmake.org/download/

# Getting Started with the PD303 example
clone SE2JUCE repo. https://github.com/JeffMcClintock/SE2JUCE

get JUCE. https://juce.com/get-juce/download

Optional:
* Add VST2 headers to JUCE if you need them.
* get AAX SDK if you need it. https://www.avid.com/alliance-partner-program/aax-connectivity-toolkit

clone SE2JUCE_Projects repo. This provides an example plugin you can copy. https://github.com/JeffMcClintock/SE2JUCE_Projects

(to export you own plugin, copy the entire PD303 folder, rename it and edit the two cmakelists.txt files, changing the plugin name etc)

Open SE

open project from SE2JUCE_Plugins/PD303/SE_Project/PD303.se1

"File/Export Juce" This will copy the project and it's skin to the 'Resources' folder of the JUCE project.

close SE

Open Cmake

Under "where is the source code" enter location of SE2JUCE_Plugins folder

Under "where to build the binaries" enter something like C:\build_SE2JUCE_Plugins (or anywhere you prefer to put the temporary files created during the build).

Click 'Configure", choose whatever IDE you prefer. Ignore the error message.

enter the correct JUCE folder, and the correct SE2JUCE folder

click 'generate'

click 'open project' (your IDE should open)

build and try out the plugin

# Help and information

SynthEdit - https://groups.io/g/synthedit

JUCE - https://forum.juce.com/

AAX - https://www.avid.com/alliance-partner-program/aax-connectivity-toolkit

CLAP - https://github.com/free-audio/clap-juce-extensions

VST2 - https://forum.juce.com/t/how-to-offer-vst2-plugins-now/39195

