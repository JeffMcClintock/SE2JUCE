
/* Copyright (c) 2007-2021 SynthEdit Ltd
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name SEM, nor SynthEdit, nor 'Music Plugin Interface' nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY SynthEdit Ltd ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL SynthEdit Ltd BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
#include "../shared/GraphHelpers.h"
*/

#include "Drawing.h"
#include <vector>

using namespace GmpiDrawing;

inline void SimplifyGraph(const std::vector<Point>& in, std::vector<Point>& out)
{
	out.clear();

	if (in.size() < 2)
	{
		out = in;
		return;
	}

	const float tollerance = 0.3f;

	float slope{};
	bool first = true;
	Point prev{};

	for (int i = 0; i < in.size(); ++i)
	{
		if (first)
		{
			prev = in[i];
			out.push_back(prev);

			assert(i != in.size() - 1); // should never be last one?

			slope = (in[i + 1].y - prev.y) / (in[i + 1].x - prev.x);
			++i; // next one can be assumed to fit the prediction (so skip it).
			first = false;
		}
		else
		{
			const float predictedY = prev.y + slope * (in[i].x - prev.x);
			const float err = in[i].y - predictedY;

			if (err > tollerance || err < -tollerance)
			{
				i -= 2; // insert prev in 'out', then recalc slope
				first = true;
			}
		}
	}

	if (out.back() != in.back())
	{
		out.push_back(in.back());
	}
}

inline PathGeometry DataToGraph(Graphics& g, const std::vector<Point>& inData)
{
	auto geometry = g.GetFactory().CreatePathGeometry();
	auto sink = geometry.Open();
	bool first = true;
	for (const auto& p : inData)
	{
		if (first)
		{
			sink.BeginFigure(p);
			first = false;
		}
		else
		{
			sink.AddLine(p);
		}
	}

	sink.EndFigure(FigureEnd::Open);
	sink.Close();

	return geometry;
}
