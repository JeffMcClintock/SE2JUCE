
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
#include <array>

#define DRAW_DEBUG_GRID 0

using namespace gmpi;
using namespace GmpiDrawing;

class KeyboardMpeGui final : public gmpi_gui::MpGuiGfxBase
{
	static const int MAX_KEYS = 128;
//	std::array<bool, MAX_KEYS> keyStates = {};

	int midiNote_ = -1;
	int baseKey_ = 36; // MIDI key num of leftmost key.
	int keyBoardLeft = 0;

	FloatArrayGuiPin pinGates;
	FloatArrayGuiPin pinTriggers;
	FloatArrayGuiPin pinPitches;
	FloatArrayGuiPin pinBenders;
	FloatArrayGuiPin pinVelocities;
	FloatArrayGuiPin pinPressures;
	FloatArrayGuiPin pinBrightness;

public:
	KeyboardMpeGui()
	{
		initializePin(0, pinGates, static_cast<MpGuiBaseMemberIndexedPtr2>(&KeyboardMpeGui::redraw));
		initializePin(1, pinTriggers, static_cast<MpGuiBaseMemberIndexedPtr2>(&KeyboardMpeGui::redraw));
		initializePin(2, pinPitches, static_cast<MpGuiBaseMemberIndexedPtr2>(&KeyboardMpeGui::redraw));
		initializePin(3, pinBenders, static_cast<MpGuiBaseMemberIndexedPtr2>(&KeyboardMpeGui::redraw));
		initializePin(4, pinVelocities, static_cast<MpGuiBaseMemberIndexedPtr2>(&KeyboardMpeGui::redraw));
		initializePin(5, pinPressures, static_cast<MpGuiBaseMemberIndexedPtr2>(&KeyboardMpeGui::redraw));
		initializePin(6, pinBrightness, static_cast<MpGuiBaseMemberIndexedPtr2>(&KeyboardMpeGui::redraw));
	}

	void redraw(int /*voice*/)
	{
		invalidateRect();
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override
	{

		Graphics g(drawingContext);
		const auto r = getRect();
		ClipDrawingToBounds c(g, r);

		const float width = r.getWidth();
		const float keyHeight = r.getHeight();
		
		const float keyUnit = floorf(keyHeight * 0.1f); // 1cm on a Roli Seaboard block. half width of white key.
		const float keySpacing = keyUnit * 2.0f; // width of a white key.

		const float blackKeyHeight = (2 * keyHeight + 1) / 3;
		const float blackKeyTopIndent = 10;

		const float greys[] = { 0.3f, 0.1f, 0.05 };

		Color highlightGrey { greys[0], greys[0], greys[0] };
		Color backgroundGrey{ greys[1], greys[1], greys[1] };
		Color shadowGrey    { greys[2], greys[2], greys[2] };

		auto backgroundBrush = g.CreateSolidColorBrush(backgroundGrey);
		auto limeDiagBrush = g.CreateSolidColorBrush(Color::Lime);

//		auto brushShadow = g.CreateSolidColorBrush(Color::SlateGray);
		
		// don't draw partialy visible keys.
		const int maxWhiteKeys = 75;
		const int visibleWhiteKeyCount = (std::min)(static_cast<int>(r.getWidth() / keySpacing), maxWhiteKeys);
		const float usableWidth = visibleWhiteKeyCount * keySpacing;

		// always draw from a C
		const int visibleWholeOctaves = visibleWhiteKeyCount / 7;
		const int wholeMidiOctaves = 10;
		const int lowestMidiKey = 72 - (visibleWholeOctaves / 2) * 12; // 72 - middle-C
		assert((lowestMidiKey % 12) == 0); // its a C
		
		baseKey_ = lowestMidiKey;

		keyBoardLeft = static_cast<int>(width - usableWidth);

		// Background
		g.FillRectangle(usableWidth, 0, static_cast<float>(keyBoardLeft), keyHeight, backgroundBrush);

		// 'black' keys
		//if (baseKey_ == 21) // Special case for piano.
		//{
		//	kn = 5;
		//}
//		int key = baseKey_ + 1;

//		auto brushDiag = g.CreateSolidColorBrush(Color::FromArgb(0x0A00ffff));
		auto brushDiag = g.CreateSolidColorBrush(Color::Gray);
		auto brush = g.CreateSolidColorBrush(Color::Black);
		const auto grad1 = Point(0.f, 0.f);
		const auto grad2 = Point(0.f, (float)blackKeyHeight);

		//auto gradientBrush = g.CreateLinearGradientBrush( grad1, grad2, Color::Black, Color::Gray);
		auto gradientBrush = g.CreateLinearGradientBrush(Color::Black, Color::Gray, grad1, grad2);
		//auto gradientBrush3 = g.CreateLinearGradientBrush( grad1, Point(0.f, (float)blackKeyHeight / 2.0f), Color::Gray, Color::Black);
		auto gradientBrush3 = g.CreateLinearGradientBrush(Color::Gray, Color::Black, grad1, Point(0.f, (float)blackKeyHeight / 2.0f));

		auto whiteBrush = g.CreateSolidColorBrush(Color::White);

		StrokeStyleProperties strokeStyleProperties;
		strokeStyleProperties.setCapStyle(CapStyle::Round);
		auto blackKeyLineStyle = g.GetFactory().CreateStrokeStyle(strokeStyleProperties);


		// line between keys.
		{
			auto key = baseKey_ + 1;
			for (float x = 0.5f; x < usableWidth - keySpacing; x += keySpacing)
			{
				if constexpr (DRAW_DEBUG_GRID)
				{
					// draw line all the way to top when debugging.
					g.DrawLine(keyBoardLeft + x, 0, keyBoardLeft + x, keyHeight, brushDiag);
				}
				else
				{
					g.DrawLine(keyBoardLeft + x, blackKeyHeight, keyBoardLeft + x, keyHeight, brushDiag, 0.5);
				}

				key += 2;

				int t = key % 12;
				if (t == 1 || t == 6) // skip 'gaps' in black keys
				{
					key--;
				}
			}
		}

		{
			auto key = baseKey_ + 1;

			int kn = 0;

			for (float wx = 0; wx < usableWidth - keySpacing; wx += keySpacing) // wx = key-center
			{
				int t = kn++ % 7;
				if (t != 0 && t != 3) // black keys
				{
					float x = keyBoardLeft + wx - keySpacing * 0.5f; // left side

					// left side of black key, trapezoid.
					{
						const float x1 = x;
						const float x2 = x + keyUnit * (0.5f + 0.35f);

						Point points[]{
								{ x1, keyUnit * 1.0f },
								{ x2, keyUnit * 2.0f },
								{ x2, keyUnit * 6.0f },
								{ x1, keyUnit * 7.0f },
						};

						GradientStop gradientStops[]
						{
							{ 0.0f, backgroundGrey },
							{ 0.2f, highlightGrey },
							{ 0.8f, highlightGrey },
							{ 1.0f, backgroundGrey },
						};

						auto gradientBrush2 = g.CreateLinearGradientBrush(
							gradientStops,
							{ x1, 0.f },
							{ x2, 0.f }
						);

						// draw it.
						g.FillPolygon(points, gradientBrush2);
					}

					// right side of black key, trapezoid.
					{
						const float x1 = x + keyUnit * 1.15f;
						const float x2 = x + keyUnit * 2.0f;

						Point points[]{
								{ x1, keyUnit * 2.0f },
								{ x2, keyUnit * 1.0f },
								{ x2, keyUnit * 7.0f },
								{ x1, keyUnit * 6.0f },
						};

						GradientStop gradientStops[]
						{
							{ 0.0f, backgroundGrey },
							{ 0.2f, shadowGrey },
							{ 0.8f, shadowGrey },
							{ 1.0f, backgroundGrey },
						};

						auto gradientBrush2 = g.CreateLinearGradientBrush(
							gradientStops,
							{ x1, 0.f },
							{ x2, 0.f }
						);

						// draw it.
						g.FillPolygon(points, gradientBrush2);
					}
					// White line on 'black' keys.
					const auto radius = keyUnit * 0.15f;
					g.DrawLine(
						{
							x + keySpacing * 0.5f ,
							keyUnit * 2.0f + radius
						},
						{
							x + keySpacing * 0.5f,
							keyUnit * 6.0f - radius
						},
						whiteBrush,
						radius,
						blackKeyLineStyle
					);

					// Diagnostic outline
					// 'black' key outline.
					if constexpr (DRAW_DEBUG_GRID)
					{
						/*
												g.DrawRectangle(
													{ x1,
													  y1,
													  x2,
													  y2
													}
													, brushDiag
												);
						*/
						Point shape[4];

						// Outer
						{
							const float x1 = x;
							const float x2 = x + keyUnit * 2.0f;
							const float y1 = keyUnit * 1.0f;
							const float y2 = keyUnit * 7.0f;

							auto geometry = g.GetFactory().CreatePathGeometry();
							auto sink = geometry.Open();

							const float halfUnit = keyUnit * 0.5f;

							sink.BeginFigure(x1, y1);
							sink.AddLine({ x2 - halfUnit, y1 });
							sink.AddLine(GmpiDrawing::Point(x2, y1 + halfUnit));
							sink.AddLine(GmpiDrawing::Point(x2, y2));
							sink.AddLine(GmpiDrawing::Point(x1, y2));

							sink.EndFigure();
							sink.Close();

							g.DrawGeometry(geometry, brushDiag);

							shape[0] = { x2 - halfUnit, y1 };
							shape[1] = { x2, y1 + halfUnit };
						}

						// inner
						{
							const float x1 = x + keyUnit * (1.0f - 0.15f);
							const float x2 = x + keyUnit * (1.0f + 0.15f);
							const float y1 = keyUnit * (2.0f - 0.15f);
							const float y2 = keyUnit * (6.0f + 0.15f);

							auto geometry = g.GetFactory().CreatePathGeometry();
							auto sink = geometry.Open();

							const float halfUnit = keyUnit * 0.15f * 0.5f;

							sink.BeginFigure(x1, y1);
							sink.AddLine(GmpiDrawing::Point(x2 - halfUnit, y1));
							sink.AddLine(GmpiDrawing::Point(x2, y1 + halfUnit));
							sink.AddLine(GmpiDrawing::Point(x2, y2));
							sink.AddLine(GmpiDrawing::Point(x1, y2));

							sink.EndFigure();
							sink.Close();

							g.DrawGeometry(geometry, brushDiag, 0.25);

							shape[2] = { x2, y1 + halfUnit };
							shape[3] = { x2 - halfUnit, y1 };
						}

						{
							auto geometry = g.GetFactory().CreatePathGeometry();
							auto sink = geometry.Open();

							sink.BeginFigure(shape[0], FigureBegin::Filled);
							sink.AddLine(shape[1]);
							sink.AddLine(shape[2]);
							sink.AddLine(shape[3]);

							sink.EndFigure();
							sink.Close();

							g.FillGeometry(geometry, brushDiag);

							g.DrawGeometry(geometry, whiteBrush, 0.25);
						}
					}

					key += 2;
					int t2 = key % 12;
					if (t2 == 0 || t2 == 5) // skip 'gaps' in black keys
					{
						key++;
					}
				}
			}
		}

		// Draw notes
		auto fingerBrush = g.CreateSolidColorBrush(Color::Orange);
		auto velocityBrush = g.CreateSolidColorBrush(Color::Black);
		for (int key = 0; key < pinGates.size(); ++key)
		{
			if (pinGates[key] == 0.f)
				continue;

			const auto pitchAsKey = (pinPitches[key] - 5.0f) * 12.f + 69.f // 69 = Middle-A
								+ pinBenders[key] * 0.1f * 48.f;

//_RPTN(0, "pitch %f bend %f\n", (float)pinPitches[key], (float) pinBenders[key]);

			const auto octavef = (pitchAsKey - static_cast<float>(baseKey_)) / 12.f;
			const auto octaveWhole = static_cast<int>(octavef);
			const auto semitones = (octavef - octaveWhole) * 12.f;

			auto inOctaveOffset = semitones;

			// stretch gap where first black key missing.
			if (semitones > 4.0f)
				inOctaveOffset = 4.0f + (semitones - 4.0f) * 2.0f;

			if (semitones > 5.0f)
				inOctaveOffset = 6.0f + (semitones - 5.0f);

			if (semitones > 11.0f)
				inOctaveOffset = 12.0f + (semitones - 11.0f) * 2.0f;


			const auto x = keyBoardLeft + (octaveWhole * 7.f + 0.5f) * keySpacing
						+ inOctaveOffset * keyUnit;

			const auto y = (1.f - pinBrightness[key] * 0.1f) * keyHeight;
			const auto radius_aftertouch = (0.1f + pinPressures[key] * 0.1f) * keyUnit * 2.f;
			const auto radius_velocity = (0.1f + pinVelocities[key] * 0.1f) * keyUnit * 2.f;

			g.FillCircle({ x, y }, radius_aftertouch, fingerBrush);

			g.DrawCircle({ x, y }, radius_velocity, velocityBrush);
		}

		return gmpi::MP_OK;
	}
};

namespace
{
	auto r = Register<KeyboardMpeGui>::withId(L"SE Keyboard (MPE)");
}
