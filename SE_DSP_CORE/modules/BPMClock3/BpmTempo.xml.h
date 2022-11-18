// Converted from XML using Convert-String http://www.bosseye.com/utilities.htm
const char* BPMTEMPO_XML =
"<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
""
"<PluginList>"
"  "
"  <Plugin id=\"SE BPM Tempo\" name=\"BPM Tempo\" category=\"Special\" graphicsApi=\"none\" helpUrl=\"BpmClock3.htm\">"
"    <Audio>"
"      <Pin id=\"0\" name=\"Host BPM\" direction=\"in\" datatype=\"float\" hostConnect=\"Time/BPM\" />"
"      <Pin id=\"1\" name=\"Host Transport\" direction=\"in\" datatype=\"bool\" hostConnect=\"Time/TransportPlaying\" />"
"      <Pin id=\"2\" name=\"Tempo Out\" direction=\"out\" datatype=\"float\" rate=\"audio\"/>"
"      <Pin id=\"3\" name=\"Transport Run\" direction=\"out\" datatype=\"float\" rate=\"audio\"/>"
"    </Audio>"
"  </Plugin>"
""
"</PluginList>"
;