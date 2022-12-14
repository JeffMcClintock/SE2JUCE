L     V       .rsrc             <   L         @  ?    ?v?G       H  ?  ?    ?v?G          0  ?    ?v?G       	  X    G M P X M L   h   ?          <?xml version="1.0" encoding="utf-8" ?>

<PluginList>
  <Plugin id="SynthEdit Scope3" name="Scope3" category="Controls" about="made by Jeff" graphicsApi="HWND">
    <Parameters>
      <Parameter id="0" name="Capture Data A" datatype="blob" private="true" ignorePatchChange="true" isPolyphonic="true" />
      <Parameter id="1" name="Capture Data B" datatype="blob" private="true" ignorePatchChange="true" isPolyphonic="true" />
    </Parameters>
    <Audio>
      <Pin id="0" name="Capture Data A" direction="out" datatype="blob"  parameterId="0" private="true" isPolyphonic="true"/>
      <Pin id="1" name="Capture Data B" direction="out" datatype="blob"  parameterId="1" private="true" isPolyphonic="true"/>
      <Pin id="2" name="Signal A" direction="in" datatype="float" rate="audio"/>
      <Pin id="3" name="Signal B" direction="in" datatype="float" rate="audio"/>
      <Pin id="4" name="VoiceActive" hostConnect="Voice/Active" direction="in" datatype="float" isPolyphonic="true" />
    </Audio>
    <GUI>
      <Pin id="0" name="Capture Data A" direction="in" datatype="blob" parameterId="0" private="true" isPolyphonic="true"/>
      <Pin id="1" name="Capture Data B" direction="in" datatype="blob" parameterId="1" private="true" isPolyphonic="true"/>
      <Pin id="3" name="VoiceGate" direction="in" datatype="float" hostConnect="Voice/Gate" isPolyphonic="true" private="true"/>
    </GUI>
  </Plugin>
</PluginList>X        .rsrc              