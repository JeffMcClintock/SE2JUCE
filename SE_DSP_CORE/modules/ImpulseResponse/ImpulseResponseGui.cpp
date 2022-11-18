#include "./ImpulseResponseGui.h"

#if defined(_WIN32) && !defined(_WIN64)

#define SCOPE_BUFFER_SIZE 512

REGISTER_GUI_PLUGIN(ImpulseResponseGui, L"SE Impulse Response");

ImpulseResponseGui::ImpulseResponseGui(IMpUnknown* host) : SeGuiCompositedGfxBase(host)
, displayOctaves(10.0f)
, displayDbMax(30.0f)
, displayDbRange(100.0f)
{
	initializePin(0, pinResults, static_cast<MpGuiBaseMemberPtr>(&ImpulseResponseGui::onValueChanged));
	initializePin(1, pinSampleRate, static_cast<MpGuiBaseMemberPtr>(&ImpulseResponseGui::onValueChanged));
}

void ImpulseResponseGui::onValueChanged()
{
	invalidateRect();
}

// handle pin updates.
int32_t ImpulseResponseGui::paint(HDC hDC)
{
	// Processor send an odd number of samples when in "Impulse" mode (not "Frequency Response" mode).
	bool rawImpulseMode = (pinResults.rawSize() & 1) == 1;

	// PAINT. /////////////////////////////////////////////////////////////////////////////////////////////////
	MpRect r = getRect();
	int width = r.right - r.left;
	int height = r.bottom - r.top;

	HFONT font_handle;// = (HFONT) GetStockObject( ANSI_VAR_FONT ); // temp fix.
	MpFontInfo fontInfo_;
	getGuiHost()->getFontInfo(L"tty", fontInfo_, font_handle);

	float scale = height * 0.95f;
	int mid_x = width / 2;
	int mid_y = height / 2;

	if (fontInfo_.colorBackground >= 0) // -1 indicates transparent background
	{
		// Fill in solid background black
		HBRUSH background_brush = CreateSolidBrush(fontInfo_.colorBackground);

		RECT r;
		r.top = r.left = 0;
		r.right = width + 1;
		r.bottom = height + 1;
		FillRect(hDC, &r, background_brush);

		// cleanup objects
		DeleteObject(background_brush);
	}

	// create a dark green pen
	// int darked_col = (fontInfo_.color >> 1) &0x7f7f7f; // bit darker.
	int darked_col = (fontInfo_.color >> 2) & 0x3f3f3f; // more darker.
	HPEN pen = CreatePen(PS_SOLID, 1, darked_col);

	// 'select' it
	HGDIOBJ old_pen = SelectObject(hDC, pen);

	int fontHeight = fontInfo_.fontHeight;

	HGDIOBJ old_font = SelectObject(hDC, font_handle);

	SetTextColor(hDC, fontInfo_.color);
	SetBkMode(hDC, TRANSPARENT);

	double samplerate = (double)pinSampleRate;

	// Frequency labels.
	SetTextAlign(hDC, TA_CENTER);

	if (!rawImpulseMode)
	{
		// Highest mark is rounded to nearest 10kHz.
		double topFrequency = floor(samplerate / 20000.0) * 10000.0;
		double frequencyStep = 1000.0;
		if (width < 500)
		{
			frequencyStep = 10000.0;
		}
		double hz = topFrequency;
		int previousTextLeft = INT_MAX;
		int x = INT_MAX;
		do {
			float normalizedFrequency = 2.0 * hz / samplerate;
			float octave = (log(normalizedFrequency) - log(1.0f)) / log(2.0f);
			x = (int)(((octave + displayOctaves) * width) / displayOctaves);

			bool extendLine = false;

			// Text.
			if (samplerate > 0)
			{
				wchar_t txt[10];

				// Large values printed in kHz.
				if (hz > 999.0)
				{
					swprintf(txt, L"%.0fk", hz * 0.001);
				}
				else
				{
					swprintf(txt, L"%.0f", hz);
				}

				int stringLength = wcslen(txt);
				SIZE size;
				::GetTextExtentPoint32(hDC, txt, stringLength, &size);

				// Ensure text don't overwrite text to it's right.
				if (x + size.cx / 2 < previousTextLeft)
				{
					extendLine = true;
					TextOut(hDC, x, height - fontHeight, txt, stringLength);
					previousTextLeft = x - (2 * size.cx) / 3; // allow for text plus whitepace.
				}
			}

			// Vertical line.
			int lineBottom = height - fontHeight;
			if (!extendLine)
			{
				lineBottom -= 2;
			}
			MoveToEx(hDC, x, 0, 0);
			LineTo(hDC, x, lineBottom);

			if (frequencyStep > hz * 0.99)
			{
				frequencyStep *= 0.1;
			}

			hz = hz - frequencyStep;

		} while (x > 25 && hz > 5.0);

		/* mark in octaves, possible option?
			for( float octave = 0.0 ; octave > -displayOctaves ; octave -= 1.0f )
			{
				float Hz = samplerate / ( powf( 2.0f, 1.0f - octave) );
				int x = (int) (((octave + displayOctaves) * width) / displayOctaves);

				MoveToEx(hDC, x,0, 0);
				LineTo(hDC, x, height - fontHeight - 2 );

				if( samplerate > 0 )
				{
					wchar_t txt[10];
					swprintf(txt, L"%2.0f", Hz);

					if( x < 20 )
					{
						SetTextAlign( hDC, TA_LEFT );
					}
					else
					{
						if( x > width - 20 )
						{
							SetTextAlign( hDC, TA_RIGHT );
						}
						else
						{
							SetTextAlign( hDC, TA_CENTER );
						}
					}

					TextOut(hDC, x, height - fontHeight, txt, (int) wcslen(txt));
				}
			}
			*/

			// dB labels.
		if (height > 30)
		{
			SetTextAlign(hDC, TA_LEFT);

			float db = displayDbMax;
			float y = 0;
			while (y < height)
			{
				y = (displayDbMax - db) * scale / displayDbRange;

				MoveToEx(hDC, 20, y, 0);
				LineTo(hDC, width, y);

				wchar_t txt[10];
				swprintf(txt, L"%2.0f", (float)db);

				TextOut(hDC, 0, y - fontHeight / 2, txt, (int)wcslen(txt));

				db -= 10.0f;
			}

			// extra line at -3dB. To help check filter cuttoffs.
			db = -3.f;
			y = (displayDbMax - db) * scale / displayDbRange;

			MoveToEx(hDC, 20, y, 0);
			LineTo(hDC, width, y);
		}
	}
	else
	{
		// Impulse mode.

		// Zero-line
		MoveToEx(hDC, 20, mid_y, 0);
		LineTo(hDC, width, mid_y);

		// volt ticks
		for (int v = 10; v >= -10; --v)
		{
			int y = (int) (mid_y - (scale * v) * 0.1f);
			MoveToEx(hDC, 0, y, 0);
			LineTo(hDC, 20, y);
		}
	}

	SelectObject(hDC, old_font);

	// trace
	HPEN newPen = CreatePen(PS_SOLID, 1, RGB(150, 150, 0)); // dark yellow
	SelectObject(hDC, newPen); // will deselect current pen.
	DeleteObject(pen);
	pen = newPen;

	bool voicesStillActive = false;

	if (pinResults.rawSize() == sizeof(float) * SCOPE_BUFFER_SIZE)
	{
		newPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0)); // bright green
		SelectObject(hDC, newPen); // will deselect current pen.
		DeleteObject(pen);
		pen = newPen;
		float* capturedata = (float*)pinResults.rawData();
		int count = pinResults.rawSize() / sizeof(float);

		{
			float dbScaler = scale / displayDbRange; // 90db Range
			//float db = 10.f * log10f( *capturedata ) - 10.0f; // db of amplitude is * 10, db of power is * 20.
			//float y = - db * dbScaler;

			// Ignoring DC. Can't show on log scale.
			++capturedata;

			for (int i = 1; i < SCOPE_BUFFER_SIZE; i++)
			{
				float normalizedFrequency = (float)i / (float)SCOPE_BUFFER_SIZE;
				float octave = (log(normalizedFrequency) - log(1.0f)) / log(2.0f);
				//int x = (i * width) / SCOPE_BUFFER_SIZE;
				int x = (int)(((octave + displayOctaves) * width) / displayOctaves);
				float db = 20.f * log10f(*capturedata) - displayDbMax;
				float y = -db * dbScaler;
				if (i == 1)
				{
					MoveToEx(hDC, 0, (int)y, 0);
				}
				else
				{
					LineTo(hDC, x, (int)y);
				}
				++capturedata;
			}
		}
	}
	else
	{
		if (rawImpulseMode)
		{
			newPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0)); // bright green
			SelectObject(hDC, newPen); // will deselect current pen.
			DeleteObject(pen);
			pen = newPen;
			float* capturedata = (float*)pinResults.rawData();
			int count = pinResults.rawSize() / sizeof(float);
			float dbScaler = scale;

			for (int i = 0; i < count; ++i)
			{
				int x = 20 + i;
				float y = mid_y - *capturedata * scale;
				if (i == 0)
				{
					MoveToEx(hDC, 0, (int)y, 0);
				}
				else
				{
					LineTo(hDC, x, (int)y);
				}
				++capturedata;
			}
		}
	}

	// cleanup
	SelectObject(hDC, old_pen);
	DeleteObject(pen);

	return gmpi::MP_OK;
}

#endif
