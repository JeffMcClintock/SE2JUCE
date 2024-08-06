// ug_logic_Bin_Count module

//

#pragma once

#include "ug_logic_complex.h"



class ug_logic_Bin_Count : public ug_logic_complex

{

public:

	ug_logic_Bin_Count();

	DECLARE_UG_BUILD_FUNC(ug_logic_Bin_Count);

	void input_changed() override;

	void ListInterface2(InterfaceObjectArray& PList) override;

private:

	bool cur_input;

	bool cur_reset;

	int cur_count;

};

