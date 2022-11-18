#include "./ListEntry.h"

REGISTER_PLUGIN2 ( ListEntry, L"SE List Entry" );
SE_DECLARE_INIT_STATIC_FILE(ListEntry)

ListEntry::ListEntry()
{
	// Register pins.
	initializePin(pinValueIn);
	initializePin(pinValueOut);
}

void ListEntry::onSetPins()
{
	pinValueOut = pinValueIn;
}