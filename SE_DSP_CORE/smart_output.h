#pragma once

#include "sample.h"
#include "math.h"
#include "se_types.h"
#include "ug_base.h"
#include "UPlug.h"

#if 0
class CFxRbjFilter
{
public:

	CFxRbjFilter()
	{
		// reset filter coeffs
		b0a0=b1a0=b2a0=a1a0=a2a0=0.0;
		// reset in/out history
		ou1=ou2=in1=in2=0.0f;
	};

	float filter(float in0)
	{
		// filter
		float const yn = b0a0*in0 + b1a0*in1 + b2a0*in2 - a1a0*ou1 - a2a0*ou2;
		// push in/out buffers
		in2=in1;
		in1=in0;
		ou2=ou1;
		ou1=yn;
		// return output
		return yn;
	};

	void calc_filter_coeffs(int const type,double const frequency,double const sample_rate,double const q,double const db_gain,bool q_is_bandwidth)
	{
		// temp pi
		double const temp_pi=3.1415926535897932384626433832795;
		// temp coef vars
		double alpha,a0,a1,a2,b0,b1,b2;

		// peaking, lowshelf and hishelf
		if(type>=6)
		{
			double const A		=	pow(10.0,(db_gain/40.0));
			double const omega	=	2.0*temp_pi*frequency/sample_rate;
			double const tsin	=	sin(omega);
			double const tcos	=	cos(omega);

			if(q_is_bandwidth)
				alpha=tsin*sinh(log(2.0)/2.0*q*omega/tsin);
			else
				alpha=tsin/(2.0*q);

			double const beta	=	sqrt(A)/q;

			// peaking
			if(type==6)
			{
				b0=float(1.0+alpha*A);
				b1=float(-2.0*tcos);
				b2=float(1.0-alpha*A);
				a0=float(1.0+alpha/A);
				a1=float(-2.0*tcos);
				a2=float(1.0-alpha/A);
			}

			// lowshelf
			if(type==7)
			{
				b0=float(A*((A+1.0)-(A-1.0)*tcos+beta*tsin));
				b1=float(2.0*A*((A-1.0)-(A+1.0)*tcos));
				b2=float(A*((A+1.0)-(A-1.0)*tcos-beta*tsin));
				a0=float((A+1.0)+(A-1.0)*tcos+beta*tsin);
				a1=float(-2.0*((A-1.0)+(A+1.0)*tcos));
				a2=float((A+1.0)+(A-1.0)*tcos-beta*tsin);
			}

			// hishelf
			if(type==8)
			{
				b0=float(A*((A+1.0)+(A-1.0)*tcos+beta*tsin));
				b1=float(-2.0*A*((A-1.0)+(A+1.0)*tcos));
				b2=float(A*((A+1.0)+(A-1.0)*tcos-beta*tsin));
				a0=float((A+1.0)-(A-1.0)*tcos+beta*tsin);
				a1=float(2.0*((A-1.0)-(A+1.0)*tcos));
				a2=float((A+1.0)-(A-1.0)*tcos-beta*tsin);
			}
		}
		else
		{
			// other filters
			double const omega	=	2.0*temp_pi*frequency/sample_rate;
			double const tsin	=	sin(omega);
			double const tcos	=	cos(omega);

			if(q_is_bandwidth)
				alpha=tsin*sinh(log(2.0)/2.0*q*omega/tsin);
			else
				alpha=tsin/(2.0*q);

			// lowpass
			if(type==0)
			{
				b0=(1.0-tcos)/2.0;
				b1=1.0-tcos;
				b2=(1.0-tcos)/2.0;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// hipass
			if(type==1)
			{
				b0=(1.0+tcos)/2.0;
				b1=-(1.0+tcos);
				b2=(1.0+tcos)/2.0;
				a0=1.0+ alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// bandpass csg
			if(type==2)
			{
				b0=tsin/2.0;
				b1=0.0;
				b2=-tsin/2;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// bandpass czpg
			if(type==3)
			{
				b0=alpha;
				b1=0.0;
				b2=-alpha;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// notch
			if(type==4)
			{
				b0=1.0;
				b1=-2.0*tcos;
				b2=1.0;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// allpass
			if(type==5)
			{
				b0=1.0-alpha;
				b1=-2.0*tcos;
				b2=1.0+alpha;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}
		}

		// set filter coeffs
		b0a0=float(b0/a0);
		b1a0=float(b1/a0);
		b2a0=float(b2/a0);
		a1a0=float(a1/a0);
		a2a0=float(a2/a0);
	};

private:

	// filter coeffs
	float b0a0,b1a0,b2a0,a1a0,a2a0;

	// in/out history
	float ou1,ou2,in1,in2;
};
#endif

enum smart_output_curve_type { SOT_NO_SMOOTHING, SOT_LINEAR, SOT_S_CURVE, SOT_LOW_PASS };

class smart_output
{
public:
    smart_output(smart_output_curve_type p_curve_type = SOT_LINEAR, int p_transition_samples = 4) :
	    output_level(0.f)
	    ,target(0.f)
	    ,plug(NULL)
	    ,m_transition_samples( p_transition_samples )
	    ,curve_type(p_curve_type)
	    ,static_output_count(0)
	    ,m_static_mode(true)
    {
    }
	void Resume()
    {
	    // if voice suspends/resumes, ug will be at random block pos,
	    // so static_output_count needs resetting
	    if( static_output_count > 0 )
	    {
		    static_output_count = plug->UG->AudioMaster()->BlockSize();
	    }
    }
	void SetPlug( UPlug* p_plug)
    {
	    assert( p_plug->Direction == DR_OUT );
	    assert( p_plug->DataType == DT_FSAMPLE );
	    plug = p_plug;
	    static_output_count = plug->UG->AudioMaster()->BlockSize();
	    m_output_ptr = plug->GetSamplePtr();
	    // send initial status.
	    target = 1.0f; // to ensure stat change is triggerd.
	    Set(0,0.0f,0);
    }
	void Set( timestamp_t p_sample_clock, float new_value )
    {
	    Set( p_sample_clock, new_value, m_transition_samples ); // use default smoothing
    }
	void Set( timestamp_t p_sample_clock, float new_value, int transition_samples)
    {
	    if(	new_value == target && m_static_mode ) // already set to this value and not headed elsewhere)
	    {
		    assert( output_level == new_value );
		    return;
	    }

	    m_static_mode = false;
	    smart_output_curve_type c_type = curve_type;

	    if( transition_samples <= 0 )
	    {
		    c_type = SOT_NO_SMOOTHING;
	    }

	    switch( c_type )
	    {
	    case SOT_NO_SMOOTHING:
		    transition_samples = 1;

		    // deliberate fall-through
	    case SOT_LINEAR:
		    InitLinear( new_value, transition_samples);
		    break;

	    case SOT_S_CURVE:
		    InitSmooth( new_value, transition_samples);
		    break;

	    case SOT_LOW_PASS:
    #if defined( _DEBUG )
		    assert( transition_samples > 1 );
		    InitLowPass( new_value, transition_samples);
    #endif
		    break;
	    };

	    if( transition_samples > 1 ) // minimise spurious stat changes
	    {
		    plug->TransmitState( p_sample_clock, ST_RUN );
	    }
    }
	inline void Process(int start_pos, int sampleframes, bool& can_sleep )
    {
	    assert( plug != NULL );

	    if( m_static_mode )
	    {
		    if( static_output_count > 0 )
		    {
			    //			plug->GetSampleBlock()->SetRange( start_pos, sampleframes, output_level );
			    float* out = m_output_ptr + start_pos;
			    int todo = sampleframes;

			    if( static_output_count < todo )
				    todo = static_output_count;

			    for( int s = todo ; s > 0 ; s-- )
			    {
				    *out++ = output_level;
			    }

			    static_output_count -= sampleframes;
			    can_sleep = false;
		    }
	    }
	    else
	    {
		    can_sleep = false;
		    bool more = process( start_pos, sampleframes );

		    if( !more )
		    {
			    m_static_mode = true;
			    static_output_count = plug->UG->AudioMaster()->BlockSize();
		    }
	    }
    }
	float output_level;
	UPlug* plug;
	int m_transition_samples;
	smart_output_curve_type curve_type;
	int static_output_count;
private:
	void InitSmooth( float p_end, int p_sample_count)
    {
	    float N = (float)p_sample_count; //(mili_seconds * 1000 ) / sample_rate;	//nr of samples
	    float A = p_end - output_level;   //amp
	    v   = output_level;
	    float NNN = N*N*N;
	    dv  = A*(3*N - 2)/(NNN);  //difference v
	    ddv = 6*A*(N - 2)/(NNN);  //difference dv
	    c   = -12*A/(NNN);        //constant added to ddv
	    count = p_sample_count-1;
	    target = p_end;
	    // take first step
	    v += dv;
	    dv += ddv;
	    ddv += c;
    }

	void InitLinear( float p_end, int p_sample_count)
    {
	    float N = (float) p_sample_count;
	    float A = p_end - output_level;   //amp
	    v   = output_level;
	    dv  = A / N;		//difference v
	    ddv = 0.0f;			//difference dv
	    c   = 0.0f;			//constant added to ddv
	    count = p_sample_count-1;
	    target = p_end;
	    // take first step
	    v += dv;
    }

    bool process( int start_pos, int sampleframes)
    {
	    float* out = m_output_ptr + start_pos;
    #if defined( _DEBUG )

	    if( curve_type == SOT_LOW_PASS )
	    {
		    for( int s = sampleframes ; s > 0 ; s-- )
		    {
			    *out++ = v * scale;
			    v = 0;
    #if defined( zero_stuffed )
			    v = m_filter.filter(0.0);
    #else
			    //				v = m_filter.filter(target);
    #endif
		    }

		    output_level = v * scale;
		    return true;
	    }

    #endif

		// more robust in face of rounding errors
		if (curve_type == SOT_LINEAR)
		{
			for (int s = sampleframes; s > 0; s--)
			{
				*out++ = v;

				if ((dv > 0.0 && v >= target) || (dv <= 0.0 && v <= target))
				{
					v = target;
					output_level = v;
					*(out - 1) = v; // ensure output *exact*

					plug->TransmitState(plug->UG->AudioMaster()->BlockStartClock() + start_pos + sampleframes - s, ST_STATIC);
					// fill remainder of block if nesc
					s--;

					for (; s > 0; s--)
					{
						*out++ = v;
					}

					return false;
				}
				v += dv;

				assert(ddv == 0.f);
				assert(c == 0.f);
			}
		}
		else
		{
			for (int s = sampleframes; s > 0; s--)
			{
				*out++ = v;

				if (--count < 0) // done?
				{
					v = target;
					output_level = v;
					*(out - 1) = v; // ensure output *exact*
					plug->TransmitState(plug->UG->AudioMaster()->BlockStartClock() + start_pos + sampleframes - s, ST_STATIC);
					// fill remainder of block if nesc
					s--;

					for (; s > 0; s--)
					{
						*out++ = v;
					}

					return false;
				}

				v += dv;
				dv += ddv;
				ddv += c;
			}
		}

	    output_level = v;
	    return true; // more to do
    }
	float* m_output_ptr;
	bool m_static_mode;
	float dv;	//difference v
	float ddv;	//difference dv
	float c;	//constant added to ddv
	float v;	// output val
	int count;
	float target;

#if defined( _DEBUG )
void InitLowPass( float p_end, int p_sample_count)
{
	float freq_hz = 0.30f * plug->UG->getSampleRate() / p_sample_count; // smooth at update freq / 2 (nyquist)
	m_filter_constant = expf( -PI2 * freq_hz / plug->UG->getSampleRate());
#if defined( zero_stuffed )
	scale = 0.63 * (float) p_sample_count;
#else
	scale = 1; //200;
#endif
	target = p_end;
	v   = output_level / scale;
	//	v = target + m_filter_constant * ( v - target );
//	m_filter.calc_filter_coeffs(0, freq_hz, plug->UG->SampleRate(), 0.3, 0 ,false);
	//	m_filter.calc_filter_coeffs(7, freq_hz, plug->UG->SampleRate(), 0.3, 0 ,false);
#if defined( zero_stuffed )
	v = m_filter.filter(target);
#endif
	v = target;
}
	float scale;
	float m_filter_constant;
//	CFxRbjFilter m_filter;
#endif
};

