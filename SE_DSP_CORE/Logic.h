#define LOGIC_HI 0.33333333333333333f	// 3 1/3 V

#define LOGIC_LO 0.166666666666666666f	// 1 2/3 V



// converts input voltage to logic level (returned in cur_value)

// returns true if 'cur_value' changed

inline bool check_logic_input(float sample_value, bool& cur_value)

{
	if( cur_value ) // is high
	{
		if( sample_value < LOGIC_LO )
		{
			cur_value = false;
			return true;
		}

		return false;
	}

	if( sample_value > LOGIC_HI )
	{
		cur_value = true;
		return true;
	}

	return false;
}