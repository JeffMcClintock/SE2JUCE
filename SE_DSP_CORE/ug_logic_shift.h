// ug_logic_shift module

// base class for logic device with many inputs, many outputs



#pragma once

#include "ug_logic_complex.h"



class ug_logic_shift : public ug_logic_complex

{

public:

	ug_logic_shift();

	DECLARE_UG_BUILD_FUNC(ug_logic_shift);

	void input_changed() override;

	void ListInterface2(InterfaceObjectArray& PList) override;

private:

	int cur_count;

	bool cur_reset;

	bool cur_input;

	bool cur_clock;

};

