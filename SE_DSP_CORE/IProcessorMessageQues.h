#pragma once

// AU2 has to access the controller in-memory. VST3 communicates via DAW. Abstract this.
struct IProcessorMessageQues
{
	virtual IWriteableQue* MessageQueToGui() = 0;
	virtual void Service() = 0;
	virtual	interThreadQue* ControllerToProcessorQue() = 0; // TODO rename MessageQueToProcessor, or just pollMessage() directly if that's all it;s used for.
};