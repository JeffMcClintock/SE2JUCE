#include "./OversamplingControlGui.h"

using namespace gmpi;
using namespace gmpi_gui;

GMPI_REGISTER_GUI(MP_SUB_TYPE_GUI2, OversamplingControlGui, L"SE Oversampling Control");
SE_DECLARE_INIT_STATIC_FILE(OversamplingControl_Gui);

OversamplingControlGui::OversamplingControlGui()
{
	// initialise pins.
	initializePin(pinHostOversampling,&OversamplingControlGui::onSetHostOversampling );
	initializePin(pinHostListItems,&OversamplingControlGui::onSetHostLists );
	initializePin(pinHostOversamplingFilter,&OversamplingControlGui::onSetHoatOversamplingFilter );
	initializePin(pinHostListItems2, &OversamplingControlGui::onSetHostLists );
	initializePin(pinOversampling,&OversamplingControlGui::onSetOversampling );
	initializePin(pinListItems);
	initializePin(pinOversamplingFilter,&OversamplingControlGui::onSetOversamplingFilter );
	initializePin(pinListItems2);
}

// handle pin updates.
void OversamplingControlGui::onSetHostOversampling()
{
	pinOversampling = pinHostOversampling;
}

void OversamplingControlGui::onSetHostLists()
{
	pinListItems = pinHostListItems;
	pinListItems2 = pinHostListItems2;
}

void OversamplingControlGui::onSetHoatOversamplingFilter()
{
	pinOversamplingFilter = pinHostOversamplingFilter;
}

void OversamplingControlGui::onSetOversampling()
{
	pinHostOversampling = pinOversampling;
}

void OversamplingControlGui::onSetOversamplingFilter()
{
	pinHostOversamplingFilter = pinOversamplingFilter;
}

