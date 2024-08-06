// ug_logic_counter module

// base class for logic device with many inputs, many outputs



#pragma once

#include "ug_logic_complex.h"



// old, retained for backward compat

class ug_logic_counter : public ug_logic_complex

{

public:

	ug_logic_counter();

	int Open() override;

	DECLARE_UG_BUILD_FUNC(ug_logic_counter);

	void input_changed() override;

	void ListInterface2(InterfaceObjectArray& PList) override;

protected:

	int max_count;

	int cur_count;

	bool cur_reset;

	bool cur_input;

};



// new, when clocked counts from output zero (not one as in older version)

class ug_logic_counter2 : public ug_logic_counter

{

public:

	DECLARE_UG_BUILD_FUNC(ug_logic_counter2);

	void input_changed() override;

};

