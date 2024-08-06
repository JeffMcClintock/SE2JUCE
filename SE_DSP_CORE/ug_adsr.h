#pragma once



#include "ug_envelope.h"



class ug_adsr : public ug_envelope

{

public:

	DECLARE_UG_INFO_FUNC2;

	DECLARE_UG_BUILD_FUNC(ug_adsr);



	ug_adsr();

	int Open() override;

};

