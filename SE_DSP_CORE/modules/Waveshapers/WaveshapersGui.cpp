// prevent MS CPP - 'swprintf' was declared deprecated warning
#define _CRT_SECURE_NO_DEPRECATE 
#pragma warning(disable : 4996)

#include "waveshapersGui.h"

#if defined(_WIN32) && !defined(_WIN64)


#define NODE_SIZE 6
#define TABLE_SIZE 512.f
#define DEFAULT_VALUE L"(-5.0,-5.0)(-4.0,-4.0)(-3.0,-3.0)(-2.0,-2.0)(-1.0,-1.0)(0.0,0.0)(1.0,1.0)(2.0,2.0)(3.0,3.0)(4.0,4.0)(5.0,5.0)"

REGISTER_GUI_PLUGIN( waveshaperGui, L"SynthEdit Waveshaper3" );

waveshaperGui::waveshaperGui(IMpUnknown* host) : SeGuiWindowsGfxBase(host)
,drag_node(-1)
,m_inhibit_update(false)
{
	//initializePin(0, pinShape, static_cast<MpGuiBaseMemberPtr>(&waveshaperGui::onValueChanged));
	pinShape.initialize( this, 0, static_cast<MpGuiBaseMemberPtr>(&waveshaperGui::onValueChanged));
}

int32_t waveshaperGui::paint(HDC hDC)
{
	MpRect r2 = getRect();
	int width = r2.right - r2.left;
	int height = r2.bottom - r2.top;

	HFONT font_handle;
	getGuiHost()->getFontInfo(L"tty", fontInfo, font_handle);

	float vscale = height * 0.01f;
	float hscale = width * 0.01f;

	// Fill in solid background black
	HBRUSH background_brush = CreateSolidBrush(RGB(0,0,0));

	RECT r;
	r.top = r.left = 0;
	r.right = width + 1;
	r.bottom = height + 1;
	FillRect(hDC, &r, background_brush);

	// cleanup objects
	DeleteObject(background_brush);

	// draw scale markings
	DrawScale(hDC);

	// create a green pen
	HPEN pen = CreatePen(PS_SOLID, 1, RGB(0,255,0)); // light green

	// 'select' it
	HGDIOBJ old_pen = SelectObject(hDC, pen);

	int i = WS_NODE_COUNT - 1;

	MoveToEx(hDC, (int) (nodes[i].x * hscale + 0.5f), (int) (nodes[i].y * vscale + 0.5f), 0);
	i--;

	for(; i >= 0 ; i--)
	{
		LineTo(hDC, (int)(nodes[i].x * hscale + 0.5f), (int)(nodes[i].y * vscale + 0.5f));
	}

	// nodes
	POINT pts[5]; // holds square node points

	for(i = WS_NODE_COUNT - 1; i >= 0 ; i--)
	{
		int x = (int)(nodes[i].x * hscale - NODE_SIZE/2 + 0.5f);
		int y = (int)(nodes[i].y * vscale - NODE_SIZE/2 + 0.5f);
		pts[0].x = x;
		pts[0].y = y;
		pts[1].x = x + NODE_SIZE;
		pts[1].y = y;
		pts[2].x = x + NODE_SIZE;
		pts[2].y = y + NODE_SIZE;
		pts[3].x = x;
		pts[3].y = y + NODE_SIZE;
		pts[4] = pts[0];
		Polyline(hDC, pts, 5);
	}

	// display drag node co-ords
	if(drag_node > -1)
	{
		const int fontHeight = 10;

		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.

		wcscpy(lf.lfFaceName, L"Terminal");    // face name
		lf.lfHeight = -fontHeight;

		HFONT font = CreateFontIndirect(&lf);
		HGDIOBJ old_font = SelectObject(hDC, font);

		SetTextColor(hDC, RGB(0,250,0));
		SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_LEFT);

		mypoint p(nodes[drag_node]);
		double x = p.x/10.0 - 5.0;
		double y = 5.0 - p.y/10.0;
		wchar_t txt[20];

		swprintf(txt, L"%3.1f,%3.1f",x,y);

		TextOut(hDC, 0, 0, txt, (int) wcslen(txt));

		// cleanup
		SelectObject(hDC, old_font);
		DeleteObject(font);
	}

	// cleanup
	SelectObject(hDC, old_pen);
	DeleteObject(pen);
	return gmpi::MP_OK;
}

void waveshaperGui::onValueChanged()
{
/*
	if(pinGate.empty()) // set a default straight line
	{
		pinGate = DefaultValue();
		return; // rely on recursion to re-call this routine
	}
*/
	if(!m_inhibit_update)
	{
		updateNodes(nodes, pinShape);
		invalidateRect();
	}
}

// convert string value_ into segment list
void waveshaperGui::updateNodes(mypoint *nodes, const std::wstring &p_values)
{
	std::wstring s(p_values);

	// convert CString of numbers to array of screen co-ords
	int i = 0;
	while(s.length() > 0 && i < WS_NODE_COUNT)
	{
		int p1 = (int) s.find(L"(");
		int p2 = (int) s.find(L",");
		int p3 = (int) s.find(L")");
		if(p3 < 1)
			return;

		float x = StringToFloat(s.substr(p1+1,p2-p1-1));
		float y = StringToFloat(s.substr(p2+1,p3-p2-1));
		x = (x + 5.f) * 10.f; // convert to screen co-ords
		y = (-y + 5.f) * 10.f;

		nodes[i].x = x;
		nodes[i++].y = y;

		p3++;
		s = s.substr(p3, s.length() - p3); //.Right(s.length() - p3 - 1);
	}

	if(i == 0) // if string empty, set defaults
	{
		updateNodes(nodes, DEFAULT_VALUE);
	}
}

float waveshaperGui::StringToFloat(const std::wstring &p_string)
{
	wchar_t*  temp;	// convert string to float
	return (float) wcstod(p_string.c_str(), &temp);
}

void waveshaperGui::DrawScale(HDC hDC)
{
	MpRect r2 = getRect();
	int width = r2.right - r2.left;
	int height = r2.bottom - r2.top;

	float v_scale = height / 2.15f;
	float h_scale = width / 2.15f;
	int mid_x = width/2;
	int mid_y = height/2;

	// create a green pen
	HPEN pen = CreatePen(PS_SOLID, 1, RGB(0,100,0)); // dark green

	// 'select' it
	HGDIOBJ old_pen = SelectObject(hDC, pen);

	// BACKGROUND LINES
	// horizontal line
	MoveToEx(hDC, 0, mid_y, 0);
	LineTo(hDC, width, mid_y);
	// horiz center line
	MoveToEx(hDC, mid_x, 0, 0);
	LineTo(hDC, mid_x, height);

	// vertical center line
	MoveToEx(hDC, mid_x,0, 0);
	LineTo(hDC, mid_x, height);

	// diagonal
	MoveToEx(hDC, 0, height, 0);
	LineTo(hDC, width,0);

	int tick_width;
	for(int v = -10 ; v < 11 ; v += 2)
	{
		float y = v * v_scale / 10.f;
		float x = v * h_scale / 10.f;
		
		if(v % 5 == 0)
			tick_width = 4;
		else
			tick_width = 2;

		// X-Axis ticks
		MoveToEx(hDC, mid_x - tick_width,mid_y + (int)y, 0);
		LineTo(hDC, mid_x + tick_width,mid_y + (int)y);

		// Y-Axis ticks
		MoveToEx(hDC,mid_x + (int)x, mid_y - tick_width, 0);
		LineTo(hDC,mid_x + (int)x, mid_y + tick_width);
	}

	// cleanup
	SelectObject(hDC, old_pen);
	DeleteObject(pen);

	// labels
	if(height > 30)
	{
		// Set up the font
		int fontHeight = 10;

		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.

		wcscpy(lf.lfFaceName, L"Terminal");    // face name
		lf.lfHeight = -fontHeight;

		HFONT font = CreateFontIndirect(&lf);
		HGDIOBJ old_font = SelectObject(hDC, font);

		SetTextColor(hDC, RGB(0,250,0));
		SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_LEFT);

		wchar_t txt[10];
		// Y-Axis text
		for(float fv = -5 ; fv < 5.1 ; fv += 2.0)
		{
			float y = fv * v_scale / 5.f;
			if(fv != -1.f)
			{
				swprintf(txt, L"%2.0f", fv);
				TextOut(hDC, mid_x + tick_width, mid_y - (int) y - fontHeight/2, txt, (int) wcslen(txt));
			}
		}

		int orig_ta = SetTextAlign(hDC, TA_CENTER);

		// X-Axis text
		for(float fv = -4 ; fv < 4.1 ; fv += 2.0)
		{
			float y = fv * h_scale / 5.f;
			if(fv != -1.f)
			{
				swprintf(txt, L"%2.0f", fv);
				TextOut(hDC, mid_x + (int)y, mid_y + tick_width, txt, (int) wcslen(txt));
			}
		}

		// cleanup
		SelectObject(hDC, old_font);
		DeleteObject(font);
		SetTextAlign(hDC, orig_ta);
	}
}

int32_t waveshaperGui::onLButtonDown(UINT flags, POINT point)
{
	/* testing pop up menu...
	// get this module's window handle
	HWND hw = (HWND) CallHost(seGuiHostGetWindowHandle, wi->context_handle);

	RECT r;
	// find it's location (so we know where to draw pop up menu)
	GetWindowRect(hw, &r);

	// create a pop-up menu
	HMENU hm = CreatePopupMenu();

	// add some items to it..
	AppendMenu(hm, MF_STRING , 1, "Cat");
	AppendMenu(hm, MF_STRING , 2, "Dog");

	// show the menu
	int selection = TrackPopupMenu(hm, TPM_LEFTALIGN|TPM_NONOTIFY|TPM_RETURNCMD,r.left ,r.top,0, hw, 0);

	// clean up
	DestroyMenu(hm);
	return;
*/
	MpRect r2 = getRect();
	int width = r2.right - r2.left;
	int height = r2.bottom - r2.top;

	float vscale = height * 0.01f;
	float hscale = width * 0.01f;

	drag_node = -1;

	for(int i = WS_NODE_COUNT-1; i >= 0 ; i--)
	{
		int x = (int)(nodes[i].x * hscale);
		int y = (int)(nodes[i].y * vscale);
		RECT n;
		n.top = y - NODE_SIZE/2;
		n.bottom = y + NODE_SIZE/2;
		n.left = x - NODE_SIZE/2;
		n.right = x + NODE_SIZE/2;
		if(PtInRect(&n,point))
		{
			drag_node = i;
			m_ptPrev = point;
			setCapture(); // get mouse moves
			break;
		}
	}
	return gmpi::MP_OK;
}

int32_t waveshaperGui::onMouseMove(UINT flags, POINT point)
{
	if(!getCapture())
		return gmpi::MP_OK;

	if(drag_node > -1)
	{
		MpRect r2 = getRect();
		int width = r2.right - r2.left;
		int height = r2.bottom - r2.top;
		float vscale = height * 0.01f;
		float hscale = width * 0.01f;

		float left = 0.f;
		float right = 100.f;

		if(drag_node > 0)
			left = nodes[drag_node - 1].x + 1;

		if(drag_node < WS_NODE_COUNT - 1)
			right = nodes[drag_node + 1].x - 1;

		if(drag_node == 0)
			right = 0.f;

		if(drag_node == WS_NODE_COUNT-1)
			left = 100.f;

		mypoint p(nodes[drag_node]);
		p.x += (float)(point.x - m_ptPrev.x) / hscale;
		p.y += (float)(point.y - m_ptPrev.y) / vscale;

		// constain
		p.x = max(p.x,left);
		p.x = min(p.x,right);
		p.y = max(p.y,0.f);
		p.y = min(p.y,100.f);

		nodes[drag_node] = p;
		m_ptPrev = point;

		wchar_t v[300] = L"";
		for(int i = 0 ; i < WS_NODE_COUNT; i++)
		{
			mypoint p(nodes[i]);
			wchar_t pt[20];
			double x = p.x/10.0 - 5.0;
			double y = 5.0 - p.y/10.0;
			swprintf(pt, L"(%3.1f,%3.1f)",x,y);
			wcscat(v,pt);
			assert(wcslen(v) < 280);
		}

		m_inhibit_update = true; // prevent jitter due to float->text->float conversion
		pinShape = v;
		m_inhibit_update = false;
		invalidateRect();
	}
	
	return gmpi::MP_OK;
}

int32_t waveshaperGui::onLButtonUp(UINT flags, POINT point)
{
	if(!getCapture())
		return gmpi::MP_OK;

	releaseCapture(); // don't want further mouse move events
	drag_node = -1;
	// clear on-screen drag co-ords
	invalidateRect();
	return gmpi::MP_OK;
}

int32_t waveshaperGui::measure(MpSize availableSize, MpSize &returnDesiredSize)
{
	const int prefferedSize = 100;
	const int minSize = 15;

	returnDesiredSize.x = availableSize.x;
	returnDesiredSize.y = availableSize.y;
	if(returnDesiredSize.x > prefferedSize)
	{
		returnDesiredSize.x = prefferedSize;
	}
	else
	{
		if(returnDesiredSize.x < minSize)
		{
			returnDesiredSize.x = minSize;
		}
	}
	if(returnDesiredSize.y > prefferedSize)
	{
		returnDesiredSize.y = prefferedSize;
	}
	else
	{
		if(returnDesiredSize.y < minSize)
		{
			returnDesiredSize.y = minSize;
		}
	}
	return gmpi::MP_OK;
};

#endif