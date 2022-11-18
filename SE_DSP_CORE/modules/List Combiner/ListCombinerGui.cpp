#include "./ListCombinerGui.h"
#include "it_enum_list.h"

REGISTER_GUI_PLUGIN(ListCombinerGui, L"SE List Combiner");
REGISTER_GUI_PLUGIN(ListCombinerGui2, L"SE List Combiner2");

ListCombinerGui::ListCombinerGui( IMpUnknown* host ) : MpGuiBase(host)
{
	// initialise pins.
	itemListA.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&ListCombinerGui::onSetItemListIn) );
	choiceA.initialize( this, 1 );
	itemListB.initialize( this, 2, static_cast<MpGuiBaseMemberPtr>(&ListCombinerGui::onSetItemListIn) );
	choiceB.initialize( this, 3);
	itemListOut.initialize( this, 4 );
	choiceOut.initialize( this, 5, static_cast<MpGuiBaseMemberPtr>(&ListCombinerGui::onSetChoiceOut) );
	AMomentary.initialize( this, 6  );
	BMomentary.initialize( this, 7  );
}

ListCombinerGui2::ListCombinerGui2(IMpUnknown* host) : ListCombinerGui(host)
{
}

void ListCombinerGui::onSetItemListIn()
{
	std::wstring combinedList;
	bool firstOne = true;

	it_enum_list it( itemListA );
	for( it.First(); !it.IsDone(); it.Next() )
	{
		enum_entry* e = it.CurrentItem();

		if( firstOne )
		{
			combinedList.append( e->text );
			combinedList.append( L"=1" );
			firstOne = false;
		}
		else
		{
			combinedList.append( L"," );
			combinedList.append( e->text );
		}
	}

	it_enum_list it2( itemListB );
	for( it2.First(); !it2.IsDone(); it2.Next() )
	{
		enum_entry* e = it2.CurrentItem();
		if( firstOne )
		{
			combinedList.append( e->text );
			combinedList.append( L"=1" );
			firstOne = false;
		}
		else
		{
			combinedList.append( L"," );
			combinedList.append( e->text );
		}
	}

	itemListOut = combinedList;
}

void ListCombinerGui::onSetChoiceOut()
{
	if( choiceOut == 0 )
	{
		if( AMomentary )
		{
			choiceA = 0;
		}
		if( BMomentary )
		{
			choiceB = 0;
		}
		return;
	}

	int indexOut = 1;

	it_enum_list it( itemListA );
	for( it.First(); !it.IsDone(); it.Next() )
	{
		enum_entry* e = it.CurrentItem();
		if( indexOut == choiceOut )
		{
			choiceA = e->value;
			return;
		}
		++indexOut;
	}

	it_enum_list it2( itemListB );
	for( it2.First(); !it2.IsDone(); it2.Next() )
	{
		enum_entry* e = it2.CurrentItem();
		if( indexOut == choiceOut )
		{
			choiceB = e->value;
			return;
		}
		++indexOut;
	}
}

// better handling of momentary. More in line with popup menu behavior
void ListCombinerGui2::onSetChoiceOut()
{
	int indexOut = 1;

	it_enum_list it(itemListA);
	for (it.First(); !it.IsDone(); it.Next())
	{
		enum_entry* e = it.CurrentItem();
		if (indexOut == choiceOut)
		{
			if (AMomentary && choiceA == e->value)
			{
				choiceA = -1;
			}
			choiceA = e->value;
			return;
		}
		++indexOut;
	}

	it_enum_list it2(itemListB);
	for (it2.First(); !it2.IsDone(); it2.Next())
	{
		enum_entry* e = it2.CurrentItem();
		if (indexOut == choiceOut)
		{
			if (BMomentary && choiceB == e->value)
			{
				choiceB = -1;
			}
			choiceB = e->value;
			return;
		}
		++indexOut;
	}
}
