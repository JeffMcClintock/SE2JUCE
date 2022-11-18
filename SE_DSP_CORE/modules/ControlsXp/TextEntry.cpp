#include "./TextEntry.h"

REGISTER_PLUGIN2 ( TextEntry, L"SE Text Entry" );

TextEntry::TextEntry( )
{
	initializePin( pinpatchValue );
	initializePin( pinTextOut );
}

void TextEntry::onSetPins()
{
	pinTextOut = pinpatchValue;
}

