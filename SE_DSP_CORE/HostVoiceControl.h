#pragma once

#include <vector>
#include "se_types.h"

// Voice/Active host control is unusual. It's not backed by a patch-parameter.
// This class stands in.
class HostVoiceControl
{
public:
	void ConnectPin( class UPlug* pin );
	void sendValue( timestamp_t clock, class ug_container* container, int physicalVoiceNumber, int32_t size, void* data );

private:
	std::vector<class UPlug*> outputPins_; // output pins on the patch-automator/s
};

