// Definition of a sample datatype
#pragma once

// Define the type SAMPLE and it's limits
// And conversion macros
// For denormal removal
// 1 bit at 24 bits sample is approx 1e-10, denormals start arround 1e-38
#define INSIGNIFIGANT_SAMPLE 1.0e-20

// Voltages range from -10.0 to +10.0 and are type double
#define MAX_VOLTS ( 10.f )
#define FLOAT_STEP_PER_NOTE ( 1.f / ( MAX_VOLTS * 12.f ) )

// This macro is used to convert a 32 bit sample to a 24 bit one
#define LONG_TO_SAMPLE(x) ((x) / 0x0100)
#define SAMPLE_TO_LONG(x) ((x) * 0x0100)
#define FSampleToVoltage(s) ( (float) (s) * (float) MAX_VOLTS)
#define VoltageToFSample(volts)	 ((float) (volts) * 0.1f)
#define SampleToFloat(s) ( (double) (s) * (double) MAX_VOLTS * 0.1f / (double) MAX_SAMPLE)
// handy routines, defined in unit_gen.cpp
// timecents range from -12000 to +8000 (0.0001 to 100 secs)
// we use the range -8000 to +4000 (0.001 to 10 s) 0 to 10V

/*
0.0V  0.010 s
1.0V  0.020 s
2.0V  0.039 s
3.0V  0.079 s
4.0V  0.157 s
5.0V  0.315 s
6.0V  0.630 s
7.0V  1.260 s
8.0V  2.520 s
9.0V  5.040 s
10.0V  10.079 s
11.0V  20.159 s
12.0V  40.317 s
13.0V  80.635 s
14.0V  161.270 s
15.0V  322.540 s
16.0V  645.080 s
17.0V  1290.159 s
18.0V  2580.318 s
19.0V  5160.637 s
20.0V  10321.273 s
*/

#define TimecentToSecond(t) powf( 2.0f, (t) / 1200.0f )

// tc = 1200 * log2( s )
//    = 1200 * ln(s)/ln(2)
//    = 1731 * ln(s)
#define SecondToTimecent(s) ( 1731.234049067 * log( s ))

// timecents range from -12000 to 8000 ( 1ms to 100s), Voltage from 0 to 20V
#define TimecentToVoltage(t) ( (((t) + 8000.f)*10.f)/12000.f)

// 0 Volts = -8000 timecents = approx 10ms
#define VoltageToTimecent(v) ( ((v) * 12000.f) / 10.f - 8000.f )

// centibel is degree of attenuation 1000 cb = 100db full attenuation
#define CentibelToVoltage(c) (10.0 - (double) (c) / 100 )