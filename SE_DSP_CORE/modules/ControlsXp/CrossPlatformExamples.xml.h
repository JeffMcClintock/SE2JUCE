// File genereated by XmlToHeaderFile.exe from module xml definition.
const char* CROSSPLATFORMEXAMPLES_XML = 
"<?xml version=\"1.0\" ?>"
"<PluginList>"
  "<Plugin id=\"SDK Slider\" name=\"Slider\" category=\"SDK Examples\" graphicsApi=\"GmpiGui\">"
    "<Parameters>"
      "<Parameter id=\"0\" datatype=\"float\" name=\"s1\" />"
    "</Parameters>"
    "<Audio>"
      "<Pin name=\"S0\" datatype=\"float\" private=\"true\" parameterId=\"0\" />"
      "<Pin name=\"Value Out\" datatype=\"float\" rate = \"audio\" direction=\"out\" autoConfigureParameter=\"true\" />"
    "</Audio>"
    "<GUI>"
      "<Pin name=\"Value In\" datatype=\"float\" private=\"true\" parameterId=\"0\" />"
      "<Pin name=\"Name In\" datatype=\"string\" private=\"true\" parameterId=\"0\" parameterField=\"ShortName\" />"
      "<Pin name=\"Menu Items\" datatype=\"string\" private=\"true\" parameterId=\"0\" parameterField=\"MenuItems\" />"
      "<Pin name=\"Menu Selection\" datatype=\"int\" private=\"true\" parameterId=\"0\" parameterField=\"MenuSelection\" />"
      "<Pin name=\"Mouse Down\" datatype=\"bool\" private=\"true\" parameterId=\"0\" parameterField=\"Grab\" />"
      "<Pin name=\"RangeLo\" datatype=\"float\" private=\"true\" parameterId=\"0\" parameterField=\"RangeMinimum\" />"
      "<Pin name=\"RangeHi\" datatype=\"float\" private=\"true\" parameterId=\"0\" parameterField=\"RangeMaximum\" />"
      "<Pin name=\"Reset Value\" datatype=\"float\" isMinimised=\"true\"/>"
      "<Pin name=\"Appearance\" datatype=\"enum\" default=\"0\" isMinimised=\"true\" metadata=\"Slider,H-Slider=2,Knob,Button, Button (toggle), Knob (small)=7, Button(small), Button (small toggle)\" />"
      "<Pin name=\"Show Title On Panel\" datatype=\"bool\" default=\"1\" isMinimised=\"true\" redrawOnChange=\"true\"/>"
    "</GUI>"
  "</Plugin>"

  "<Plugin id=\"SDK List Entry\" name=\"List Entry\" category=\"SDK Examples\" graphicsApi=\"GmpiGui\">"
    "<Parameters>"
      "<Parameter id=\"0\" datatype=\"enum\" name=\"s1\" />"
    "</Parameters>"
    "<Audio>"
      "<Pin name=\"Value In\" datatype=\"int\" private=\"true\" parameterId=\"0\" />"
      "<Pin name=\"Value Out\" datatype=\"enum\" direction=\"out\" autoConfigureParameter=\"true\" />"
    "</Audio>"
    "<GUI>"
      "<Pin name=\"Value In\" datatype=\"int\" private=\"true\" parameterId=\"0\" />"
      "<Pin name=\"EnumList\" datatype=\"string\" private=\"true\" parameterId=\"0\" parameterField=\"EnumList\" />"
      "<Pin name=\"Appearance\" datatype=\"enum\" default=\"0\" isMinimised=\"true\" metadata=\"Dropdown,Rot Switch=5,Rot Switch (no labels)=6\" />"
      "<Pin name=\"Show Title On Panel\" datatype=\"bool\" default=\"1\" isMinimised=\"true\" redrawOnChange=\"true\"/>"
    "</GUI>"
  "</Plugin>"
  "<Plugin id=\"SE Joystick Image\" name=\"Joystick Image\" category=\"Sub-Controls/Cross Platform\" graphicsApi=\"GmpiGui\">"
    "<GUI>"
      "<Pin name=\"Animation Position\" datatype=\"float\" />"
      "<Pin name=\"Filename\" datatype=\"string\" default=\"joystick_knob\" isFilename=\"true\" metadata=\"bmp\" />"
      "<Pin name=\"Hint\" datatype=\"string\" />"
      "<Pin name=\"Menu Items\" datatype=\"string\" />"
      "<Pin name=\"Menu Selection\" datatype=\"int\" />"
      "<Pin name=\"Mouse Down\" datatype=\"bool\" />"
      "<Pin name=\"Frame Count\" datatype=\"int\" direction=\"out\" />"
      "<Pin name=\"Position X\" datatype=\"float\" />"
      "<Pin name=\"Position Y\" datatype=\"float\" />"
    "</GUI>"
  "</Plugin>"
"</PluginList>"
;
