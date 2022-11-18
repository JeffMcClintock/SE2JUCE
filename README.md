# Introduction 
This repository supports exporting SynthEdit projects to C++ JUCE projects. 

# Getting Started
clone SynthEditJUCE repo
clone JUCE, Add VST2 headers if you need them. Clone AAX SDK if you need it.

Open SE
open project from SE_Project folder
open Projucer (might need to build)
File/Gobal paths - set location of your copy of JUCE
under File Explorer/Resources/skin add any file from SE_Project/Resources that is not an .xml
Click 'settings' icon (like a gear at top left), set 'Project Name', 'Company Name', 'Company Website' Plugin Code and Manufacturer code

Click 'Exporter' icon (at top right of 'Selected Exporter) in JUCE to export
