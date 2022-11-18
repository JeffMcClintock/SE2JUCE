#include "./BoolSplitterGui.h"

using namespace gmpi;
using namespace gmpi_gui;

REGISTER_GUI_PLUGIN( BoolSplitterGui, L"SE Bool Splitter" );
GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, FloatSplitterGui, L"SE Float Splitter");


BoolSplitterGui::BoolSplitterGui( IMpUnknown* host ) : MpGuiBase(host)
,pinCount_(0)
{
	// initialise pins.
	pinOut.initialize(this, 0, static_cast<MpGuiBaseMemberPtr>(&BoolSplitterGui::onSetOutput));
}

int32_t BoolSplitterGui::initialize()
{
	getHost()->getPinCount(pinCount_);
/*
	// initialise auto-duplicate pins.
	int pinCount = 0;
	IMpPinIterator* it;
	if (MP_OK == getHost()->createPinIterator(reinterpret_cast<void**>(&it)))
	{
		int32_t inPinCount;
		it->getCount(inPinCount);
		--inPinCount;

		MpGuiPin<bool> initVal;
		pinIns.assign(inPinCount, initVal);

		int r = it->first();
		auto it2 = pinIns.begin();
		while (r == MP_OK)
		{
			int32_t direction;
			it->getPinDirection(direction);
			if (direction == gmpi::MP_IN)
			{
				int32_t pinId;
				it->getPinId(pinId);
				(*it2).initialize(this, pinId);
				it2++;
			}
			r = it->next();
		}

		it->release();
	}
	*/

	return MpGuiBase::initialize();
}

// handle pin updates.
void BoolSplitterGui::onSetOutput()
{
	//for (auto it = pinIns.begin(); it != pinIns.end(); ++it)
	//{
	//	(*it) = pinOut;
	//}

	bool boolPinValue = pinOut;
	for (int i = 0; i < pinCount_; ++i)
	{
		getHost()->pinTransmit(i, sizeof(boolPinValue), &boolPinValue);
	}
}
/*
int32_t BoolSplitterGui::notifyPin(int32_t pinId, int32_t voice)
{
	//if( pinId > 0 && (int32_t)pinIns.size() >= pinId ) // can be called before initialize().
	//{
	//	pinOut = pinIns[pinId - 1];
	//	onSetIn();
	//}
	return MpGuiBase::notifyPin(pinId, voice);
}
*/

int32_t BoolSplitterGui::setPin(int32_t pinId, int32_t voice, int32_t size, void* data)
{
	int32_t r = MpGuiBase::setPin(pinId, voice, size, data);

	if (pinId > 0)
	{
		bool boolPinValue;
		VariableFromRaw<bool>(size, data, boolPinValue);
		pinOut = boolPinValue;

		onSetOutput();
	}

	return r;
}
