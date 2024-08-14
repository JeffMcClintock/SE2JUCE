#include "SincFilter.h"

// Don't messs with this without confirming that it auto vectorises
/*

	float sum[4]{};
00007FFBBF7DAB1E  dec         r9d
00007FFBBF7DAB21  shr         r9d,2
00007FFBBF7DAB25  inc         r9d
00007FFBBF7DAB28  nop         dword ptr [rax+rax]
	{
		sum[0] += pSignal[0] * pCoefs_f[0];
00007FFBBF7DAB30  movups      xmm1,xmmword ptr [rdx]
		sum[1] += pSignal[1] * pCoefs_f[1];
		sum[2] += pSignal[2] * pCoefs_f[2];
		sum[3] += pSignal[3] * pCoefs_f[3];

		pSignal += 4;
00007FFBBF7DAB33  add         rdx,10h
00007FFBBF7DAB37  movups      xmm0,xmmword ptr [r8]
		pCoefs_f += 4;
00007FFBBF7DAB3B  add         r8,10h
00007FFBBF7DAB3F  mulps       xmm1,xmm0
00007FFBBF7DAB42  addps       xmm3,xmm1                          // WIDE add and multiply, 4 at a time
00007FFBBF7DAB45  sub         r9,1
00007FFBBF7DAB49  jne         SincFilter::ProcessIISingle_pt2+20h (07FFBBF7DAB30h)
	}

*/

float SincFilter::ProcessIISingle_pt2(const float* __restrict pSignal, const float* __restrict pCoefs_f, int todo, int histSize) const
{
	// Operate on as many as we can up to end of buffer (but no more than num coefs)
	float sum[4]{};
	for (int i = todo; i > 0; i -= 4)
	{
		sum[0] += pSignal[0] * pCoefs_f[0];
		sum[1] += pSignal[1] * pCoefs_f[1];
		sum[2] += pSignal[2] * pCoefs_f[2];
		sum[3] += pSignal[3] * pCoefs_f[3];

		pSignal += 4;
		pCoefs_f += 4;
	}

	// Process any leftover from start of hist buffer. (wrap).
	pSignal -= histSize - sseCount;
	for (int remain = coefs->numCoefs_ - todo; remain > 0; remain -= 4)
	{
		sum[0] += pSignal[0] * pCoefs_f[0];
		sum[1] += pSignal[1] * pCoefs_f[1];
		sum[2] += pSignal[2] * pCoefs_f[2];
		sum[3] += pSignal[3] * pCoefs_f[3];

		pSignal += 4;
		pCoefs_f += 4;
	}

	return sum[0] + sum[1] + sum[2] + sum[3];
}


