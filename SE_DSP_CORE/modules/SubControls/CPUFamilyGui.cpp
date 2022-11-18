/* Copyright (c) 2007-2022 SynthEdit Ltd
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
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#include "mp_sdk_gui2.h"
#include "Drawing.h"

using namespace gmpi;
using namespace GmpiDrawing;

class CPUFamilyGui final : public gmpi_gui::MpGuiGfxBase
{
public:
	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override
	{
		const char* cpuFamily = "Unknown";

#if defined(__arm64__)
		{
			cpuFamily = "PROCESSOR: ARM (Apple Silicon)";
		}
#else
		{

			bool processIsTranslated = false;
#if defined(__APPLE__)
			{
				int ret = 0;
				size_t size = sizeof(ret);
				if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) != -1)
				{
					processIsTranslated = ret > 0;
				}
			}
#endif
			if (processIsTranslated)
			{
				cpuFamily = "PROCESSOR: Intel translated (Rosetta)";
			}
			else
			{
				cpuFamily = "PROCESSOR: Intel";
			}
		}
#endif

		Graphics g(drawingContext);

		auto textFormat = GetGraphicsFactory().CreateTextFormat();
		auto brush = g.CreateSolidColorBrush(Color::Red);

		g.DrawTextU(cpuFamily, textFormat, 0.0f, 0.0f, brush);

		return gmpi::MP_OK;
	}
};

namespace
{
	auto r = Register<CPUFamilyGui>::withId(L"SE CPU Family");
}
