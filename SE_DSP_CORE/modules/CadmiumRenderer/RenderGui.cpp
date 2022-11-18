
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
#include "mp_sdk_gui2.h"
#include "Drawing.h"

using namespace gmpi;
using namespace GmpiDrawing;

#if 0
class RenderGui final : public SeGuiInvisibleBase
{
 	void update()
	{
		// pinAnimationPosition changed
	}

	BlobGuiPin pinBrush;
	BlobGuiPin pinGeometry;

public:
	RenderGui()
	{
		initializePin(pinBrush, static_cast<MpGuiBaseMemberPtr2>(&RenderGui::update) );
		initializePin(pinGeometry, static_cast<MpGuiBaseMemberPtr2>(&RenderGui::update) );
	}
	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext ) override
	{
		Graphics g(drawingContext);
		const auto boundsRect = getRect();

		// TODO pushTransformRAii()
		const auto originalTransform = g.GetTransform();
		const auto adjustedTransform = Matrix3x2::Translation(boundsRect.getWidth() * 0.5f, boundsRect.getHeight() * 0.5f) * originalTransform;
		g.SetTransform(adjustedTransform);

		auto textFormat = GetGraphicsFactory().CreateTextFormat();
		auto brush = g.CreateSolidColorBrush(Color::Red);

//		g.DrawTextU("Hello World!", textFormat, 0.0f, 0.0f, brush);

		Rect rect(-50, -50, 50, 50);
		g.FillRectangle(rect, brush);

		g.SetTransform(originalTransform);
		return gmpi::MP_OK;
	}
};
#endif

namespace
{
//	auto r = Register<RenderGui>::withId(L"SE Render");
	auto r = Register<SeGuiInvisibleBase>::withId(L"SE Render");
}
