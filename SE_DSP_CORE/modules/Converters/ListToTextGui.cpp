#include "./ListToTextGui.h"
#include "../se_sdk3/it_enum_list.h"

REGISTER_GUI_PLUGIN( ListToTextGui, L"SE ListToText" );

ListToTextGui::ListToTextGui( IMpUnknown* host ) : MpGuiBase(host)
{
	// initialise pins.
	choice.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>( &ListToTextGui::onListChanged ) );
	itemList.initialize( this, 1, static_cast<MpGuiBaseMemberPtr>(&ListToTextGui::onListChanged) );
	itemText.initialize( this, 2 );
}

void ListToTextGui::onListChanged()
{
	it_enum_list it( itemList.getValue() );
	it.FindValue( choice );

	if(it.IsDone())
	{
		itemText = std::string();
	}
	else
	{
		itemText = it.CurrentItem()->text;
	}
}
