<img src="Docs/Images/SE2JUCE.gif"/>

# Introduction 
SE2JUCE supports exporting SynthEdit projects to C++ JUCE projects.

JUCE Projects
* Can target more plugin formats than SynthEdit alone, like AAX, Standalone, VST2 and CLAP etc.
* Hides all resources and SEMs inside a single binary file.
* Can display the GUI you created in SynthEdit, or a custom GUI made with JUCE.

(SynthEdit GUIs are supported on macOS and Windows, but not Linux or IOS)

Note that using SE2JUCE is a complex, advanced proceedure that involves programming in C++ and having an understanding of CMake and JUCE.
Also SE2JUCE requires you to have access to the source-code of any SEMs you wish to use. This may not be possible in some cases,
 especially with 3rd-party modules. 3rd-party module creators have no obligation to share their source-code.

# Prerequisites

Install Visual Studio Community or your IDE of choice. https://visualstudio.microsoft.com/vs/

 When installing select only the "Desktop development with C++" workload.

Install SynthEdit 1.5. https://synthedit.com/downloads/?url=/downloads

Get a SynthEdit license if you don't already have one. This is required for the JUCE export feature.

Install CMake Windows 64 Installer. https://cmake.org/download/

Install git for Windows: https://git-scm.com/install/windows

 (untick "Windows Explorer Integration", "Use Notepad as Git's default editor", and select default for everything else)

Recomended: Install a graphical git client, e.g. GitHub Desktop for Windows. https://desktop.github.com/download/

# Getting Started with the PD303 example
Use git to clone the SE2JUCE_Projects repository: https://github.com/JeffMcClintock/SE2JUCE_Projects 

* Open SynthEdit and open the project: Documents/Github/SE2JUCE_Plugins/PD303/SE_Project/PD303.se1

* Choose menu "File/Export Juce" This will copy the project and its skin to the 'Resources' folder of the JUCE project.

* Close SynthEdit

* Open CMake GUI

* Under "where is the source code" enter the location of SE2JUCE_Projects folder. e.g. ...\Documents\Github\SE2JUCE_Projects
* Under "where to build the binaries" enter ...\Documents\SE2JUCE_Projects\build (or anywhere you prefer).

* Click 'Configure", and choose the default compiler, Visual Studio. Choose to "create the build folder".

* Tick "SE_LOCAL_BUILD"

* Click "Generate"

* Click "Open Project". Build and try out the plugins.

Advanced (optional):
* add VST2 headers to JUCE if you need to make VST2 plugins.
* get AAX SDK if you need it, and enter its location into the CMake GUI. https://www.avid.com/alliance-partner-program/aax-connectivity-toolkit

# Exporting your own plugin

Copy the entire ...SE2JUCE_Projects\PD303 folder, rename it and edit the two CMakeLists.txt files, changing the plugin name etc

# Missing modules

Once you start exporting your own plugins with SE2JUCE you will likely experience crashes due to the plugin not containing some module it requires.

To identify which modules need to be added to the project, run the plugin or standalone app in a debugger. ('Set as Startup Project' the Standalone, set Solution Configuration to 'Debug', press 'Start Debugging F5')
<img src="Docs/Images/SE2JUCE_MIssingModule3.PNG"/>

When you run the standalone, it will crash (assert) at the point where it is trying to load the missing modules. (hit 'Retry' to break into the debugger)

<img src="Docs/Images/SE2JUCE_MIssingModule1.PNG"/>

You should see in the 'Output' Window a list of the missing modules.
<img src="Docs/Images/SE2JUCE_MIssingModule2.PNG"/>

In the example above, it's the 'SE Keyboard (MIDI)' that is missing from the build.

# Adding an additional module

To add an extra module to the build you will need access to its source code. In the case of the keyboard, the code is in the *SE2JUCE\SE_DSP_CORE\modules\ControlsXp* folder, but not actually included in the build yet.

Open the *SE2JUCE_Plugins\PD303\CMakeLists.txt* file, look for the part that mentions 'Adsr4.cpp'. Add an additional reference to the new module you want to include. If the module has both GUI and DSP parts, add both.
<img src="Docs/Images/SE2JUCE_AddModule1.png"/>

Now open the file *SE2JUCE_Plugins/PD303/Source/ExtraModules.cpp* and add lines like the following.
<img src="Docs/Images/SE2JUCE_AddModule2.png"/>

Finally, add the *SE_DECLARE_INIT_STATIC_FILE* macro line to each module file (if not already done)
<img src="Docs/Images/SE2JUCE_AddModule3.png"/>

Build and run the Standalone app. The Keyboard module now works in the JUCE plugin.
<img src="Docs/Images/SE2JUCE_AddModule4.png"/>

# Presets
The plugin you build with SE2JUCE will save presets in xmlformat. Any preset the user saves will go to the folder:

Windows: C:\ProgramData\\*YourVendorName*\\*YourPluginName*\USER Presets

Mac: /Library/Application Support/*YourVendorName*/*YourPluginName*/USER Presets

Your plugin can also include read-only factory presets. To include any factory preset into the plugin binary, add it to the folder Resources/presets


# Further help and information

SynthEdit - https://groups.io/g/synthedit

JUCE - https://forum.juce.com/

AAX - https://www.avid.com/alliance-partner-program/aax-connectivity-toolkit

CLAP - https://github.com/free-audio/clap-juce-extensions

VST2 - https://forum.juce.com/t/how-to-offer-vst2-plugins-now/39195

