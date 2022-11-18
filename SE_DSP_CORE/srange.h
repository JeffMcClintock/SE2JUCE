#pragma once
#include <string>
#include <vector>
#include "conversion.h"

/* ---------------------------------------------------------------------------------------------------------------------
	The range of values a slider displays / Transmits
 ----------------------------------------------------------------------------------------------------------------------- */
struct sRange
{
	sRange (double DHi = 10.0, double DLo = 0.0, double THi = 10.0, double TLo = 0)
	:
	MaxVis(DHi),
	MinVis(DLo),
	MaxVal(THi),
	MinVal(TLo)
	{ };

	sRange(std::wstring s)
	:
	MaxVis(100),
	MinVis(0),
	MaxVal(10),
	MinVal(0)
	{
		std::vector<std::wstring> words;
		while(s.size() > 0)
		{
			size_t i = s.find(L",");
			if( i == std::string::npos )
			{
				words.push_back(s);
				break;
			}
			else
			{
				std::wstring s2 = s.substr(0, i);
				size_t remainder = s.size() - i - 1;
				s = Right(s, remainder);
				s = TrimLeft(s);
				words.push_back(s2);
			}
		}
		if(words.size() == 4)
		{
			MaxVis = StringToDouble(words[0]);
			MinVis = StringToDouble(words[1]);
			MaxVal = StringToDouble(words[2]);
			MinVal = StringToDouble(words[3]);
		}
	}
	double	MaxVis;
	double	MinVis;
	double	MaxVal;
	double	MinVal;
};
