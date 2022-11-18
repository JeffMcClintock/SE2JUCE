#include "MoogFilterStage.h"

CMoogFilterStage::CMoogFilterStage()
{
	rho = 0;
	sampleRate = 44100;
}

CMoogFilterStage::~CMoogFilterStage()
{
}

void CMoogFilterStage::calculateCoeffs(float fc)
{
	// fc = 0->10 ==> rho = -1.00 -> 0.16
	fc /= 10.0;
	rho = fc*1.16 - 1.0;

	// bounding
//	rho = min(rho, 0.14);
//	rho = max(rho, -0.99);

	// calc the scalar
	scalar = (1 + rho)/1.3;

	m_f_a0 = 1.0;
	m_f_a1 = 0.3;
	m_f_a2 = 0.0;
	m_f_b1 = rho;  
	m_f_b2 = 0.0;
}

float CMoogFilterStage::doFilterStage(float xn)
{		
	return scalar*doBiQuad(xn);
}	
