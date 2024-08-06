#pragma once
#include "./DspFilters/PoleFilter.h"
#include "./DspFilters/Elliptic.h"


//TODO!!! check out much better Vicanek biquad filters: https://signalsmith-audio.co.uk/code/dsp/html/group___filters.html
// also: https://github.com/RafaGago/artv-audio/releases/tag/v1.4.0

// alternatly : https://github.com/berndporr/iir1

#if defined( _DEBUG )
#include <iomanip>
// Fix for <sstream> on Mac (sstream uses undefined int_64t)
#include "./modules/se_sdk3/mp_api.h"
#include <sstream>
#endif

class FilterDesignPolymorphicBase
{
public:
	virtual ~FilterDesignPolymorphicBase() {}
	virtual int getNumStages() = 0;
	virtual double GetCoefA( int stage, int index ) = 0;
	virtual double GetCoefB( int stage, int index ) = 0;
};

template<class FilterDesign>
class FilterDesignPolymorphic :
	public FilterDesign , public FilterDesignPolymorphicBase
{
public:
	FilterDesignPolymorphic(int order,
						 double cutoff,
						 double rippleDb,
						 double rolloff)
	{
		FilterDesign::setup( order, 1.0, cutoff, rippleDb, rolloff);
	};

	virtual int getNumStages()
	{
		return FilterDesign::getNumStages();
	};

	virtual double GetCoefA( int stage, int index )
	{
		switch(index)
		{
		case 0:
			return (*this)[stage].getA0();
			break;
		case 1:
			return (*this)[stage].getA1();
			break;
		case 2:
			return (*this)[stage].getA2();
			break;
		};
		return 0.0f;
	};
	virtual double GetCoefB( int stage, int index )
	{
		switch(index)
		{
		case 0:
			return (*this)[stage].getB0();
			break;
		case 1:
			return (*this)[stage].getB1();
			break;
		case 2:
			return (*this)[stage].getB2();
			break;
		};
		return 0.0f;
	};
};

const int maxStages = 5; // 10 pole filter max

// !!TODO: only 2 history vars needed.
struct FilterStage
{
	float h[4]; // history.
	float a[3]; // a coefs
	float b[3]; // b coefs
};

// optimized version of Dsp Polefilter, all data stored in same structure.
class CascadeFilter2
{
public:
	// Process single sample using Direct Form II
	inline float ProcessIISingle( float in )
	{
		FilterStage* s = stages;

		for( int i = stageCount_ ; i ; --i, ++s )
		{
			float w	= in - s->a[1] * s->h[1] - s->a[2] * s->h[2];
			in		=      s->b[0] * w    + s->b[1] * s->h[1] + s->b[2] * s->h[2];

			s->h[2] = s->h[1];
			s->h[1] = w;

			/* waves handles this
#if !defined( _MSC_VER )
			// On Mac can't disable denormals via FPU flag. Do so manually.
			kill_denormal(s->h[1]);
#endif
			*/
		}

		return in;
	};

	FilterStage stages[maxStages];
	int stageCount_;

	void Init( FilterDesignPolymorphicBase* unoptimized )
	{
		stageCount_ = unoptimized->getNumStages();

		for(int i = 0 ; i < stageCount_ ; ++i )
		{
			for( int j = 0 ; j < 3 ; ++j )
			{
				stages[i].a[j] = (float) unoptimized->GetCoefA(i,j);
				stages[i].b[j] = (float) unoptimized->GetCoefB(i,j);;
			}

			for( int j = 0 ; j < 4 ; ++j )
			{
				stages[i].h[j] = 0.0f;
			}
		}
	};

	void Init( int stageCount, double filterCoefs[][2][3] )
	{
		stageCount_ = stageCount;

		for(int i = 0 ; i < stageCount_ ; ++i )
		{
			for( int j = 0 ; j < 3 ; ++j )
			{
				stages[i].a[j] = (float) filterCoefs[i][0][j];
				stages[i].b[j] = (float) filterCoefs[i][1][j];
			}

			for( int j = 0 ; j < 4 ; ++j )
			{
				stages[i].h[j] = 0.0f;
			}
		}
	};

#if defined( _DEBUG )
	void DiagnosticDump()
	{
		std::ostringstream oss;

		oss << std::endl << "Oversample Filter Coefs." << std::endl;
		for(int i = 0 ; i < stageCount_ ; ++i )
		{
			oss << "Stage " << i << std::endl << "A[] = ";
			for( int j = 0 ; j < 3 ; ++j )
			{
				oss << std::setw(8) << stages[i].a[j];
				if( j < 2 )
					oss << ", ";
			}
			oss << std::endl << "B[] = ";
			for( int j = 0 ; j < 3 ; ++j )
			{
				oss << std::setw(8) << stages[i].b[j];
			
				if( j < 2 )
					oss << ", ";
			}
			oss << std::endl << std::endl;
		}
	#if defined(__APPLE__ )
		printf( "%s", oss.str().c_str() );
	#else
		_RPT1(_CRT_WARN, "%s\n", oss.str().c_str() );
	#endif
	}
#endif
};



