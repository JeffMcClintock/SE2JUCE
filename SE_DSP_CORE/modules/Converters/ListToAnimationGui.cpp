#include <vector>
#include <algorithm>
#include "../se_sdk3/mp_sdk_gui2.h"
#include "it_enum_list.h"
#include <climits>

using namespace gmpi;

class ListToAnimationGui : public SeGuiInvisibleBase
{
	std::vector<int> listValues;
	int currentValue = INT_MIN; // unlikely initial value.

	void onSetInput()
	{
		int newValue = static_cast<int32_t>(pinChoice);

		// Prevent visual jitter.
		if (newValue == currentValue)
			return;

		currentValue = newValue;

		int itemPosition = 0;
		auto it = std::find(listValues.begin(), listValues.end(), currentValue);
		if (it != listValues.end())
			itemPosition = static_cast<int32_t>(it - listValues.begin());

		float normalised = (itemPosition + 0.5f) / (float)static_cast<int32_t>(listValues.size());

		normalised = (std::max)(0.0f, (std::min)(normalised, 1.0f));

		pinAnimationPosition = normalised;
	}

	void onSetList()
	{
		it_enum_list it(pinItemList.getValue());

		listValues.clear();

		for (it.First(); !it.IsDone(); ++it)
		{
			listValues.push_back(it.CurrentItem()->value);
		}

		if (listValues.empty())
		{
			listValues.push_back(0);
		}

		currentValue = INT_MIN; // current output is invalidated.
		onSetInput();
	}

 	void onSetOutput()
	{
		int lastIndex = static_cast<int32_t>(listValues.size()) - 1;
		int index = FastRealToIntFloor(0.5f + pinAnimationPosition * lastIndex);
		index = (std::max)(0, (std::min)(index, lastIndex));

		currentValue = listValues[index];
		pinChoice = currentValue;
	}

	IntGuiPin pinChoice;
	StringGuiPin pinItemList;
	FloatGuiPin pinAnimationPosition;

public:
	ListToAnimationGui()
	{
		initializePin(pinChoice, static_cast<MpGuiBaseMemberPtr2>(&ListToAnimationGui::onSetInput));
		initializePin(pinItemList, static_cast<MpGuiBaseMemberPtr2>(&ListToAnimationGui::onSetList));
		initializePin(pinAnimationPosition, static_cast<MpGuiBaseMemberPtr2>(&ListToAnimationGui::onSetOutput) );

		listValues.push_back(0); // safe initial condition (no division by zero).
	}
};

namespace
{
	auto r = Register<ListToAnimationGui>::withId(L"SE List to Animation");
}
