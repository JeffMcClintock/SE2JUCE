#include "Decimator.h"


CDecimator::CDecimator(void)
{
}


CDecimator::~CDecimator(void)
{
}

/* L-Point Decimator
	Input: Left and Right Input Buffers with L samples per buffer
	Output:	Left and Right Input Samples ynL, ynR

	This function loops L times, decimating the outputs and returning the LAST output calculated

	CALLER SUPPLIES INPUT BUFFERS!
*/
void CDecimator::decimateSamples(float* pLeftDeciBuffer, float* pRightDeciBuffer,float& ynL, float& ynR)
{
	if(!pLeftDeciBuffer || !pRightDeciBuffer)
		return;

	// counter for decimator optimization
	m_nCurrentL = -1;

	for(int i=0; i<m_nL; i++)
	{
		float temp_ynL = 0;
		float temp_ynR = 0;

		// decimate next sample
		//
		// ynL and ynR are valid (and returned) after the last call to decimateNextOutputSample()
		this->decimateNextOutputSample(pLeftDeciBuffer[i], pRightDeciBuffer[i], temp_ynL, temp_ynR);

		if( i == 0)
		{
			ynL = temp_ynL;
			ynR = temp_ynR;
		}
	}
}

/* decimateNextOutputSample
	This does the work: 
		- Convovolve L times
		- throw out all but the last convolution (done above)
*/
bool CDecimator::decimateNextOutputSample(float xnL, float xnR, float& fLeftOutput, float& fRightOutput)
{
	// read inputs
	//
	// left
	m_pLeftInputBuffer[m_nWriteIndex] = xnL; 

	// right
	m_pRightInputBuffer[m_nWriteIndex] = xnR; 

	// OPTIMIZED THIS: Can skip L-1 convolutions! 
	m_nCurrentL++;

	if(m_nCurrentL != 0)
	{	
		// incremnent the pointers and wrap if necessaryx
		m_nWriteIndex++;
		if(m_nWriteIndex >= m_nIRLength)
			m_nWriteIndex = 0;

		return true;
	}

	// reset: read index for Delay Line -> write index
	m_nReadIndexDL = m_nWriteIndex;

	// reset: read index for IR - > top (0)
	m_nReadIndexH = 0;

	// accumulator
	float yn_accumL = 0;	
	float yn_accumR = 0;	

	// This can be optimized!! Don't have to convolve on the first L-1, only need one convolution at the end
	for(int i=0; i<m_nIRLength; i++)
	{
		// do the sum of products
		yn_accumL += m_pLeftInputBuffer[m_nReadIndexDL]*m_pIRBuffer[m_nReadIndexH];
		yn_accumR += m_pRightInputBuffer[m_nReadIndexDL]*m_pIRBuffer[m_nReadIndexH];

		// advance the IR index
		m_nReadIndexH++;

		// decrement the Delay Line index
		m_nReadIndexDL--;

		// check for wrap of delay line (no need to check IR buffer)
		if(m_nReadIndexDL < 0)
			m_nReadIndexDL = m_nIRLength -1;
	}

	// write out
	fLeftOutput = yn_accumL;

	// write out
	fRightOutput = yn_accumR; 

	// incremnent the pointers and wrap if necessaryx
	m_nWriteIndex++;
	if(m_nWriteIndex >= m_nIRLength)
		m_nWriteIndex = 0;

	return true;
}


