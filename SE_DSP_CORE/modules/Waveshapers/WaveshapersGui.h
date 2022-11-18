#pragma once

#include <string>
#include "mp_sdk_gui.h"

#if defined(_WIN32) && !defined(_WIN64)

#define WS_NODE_COUNT 11

struct mypoint
{
	mypoint(float px=0, float py=0){x=px;y=py;};
	mypoint(mypoint &other){x=other.x;y=other.y;};
	float x;
	float y;
};

class waveshaperGui :
	public SeGuiWindowsGfxBase
{
public:
	waveshaperGui(IMpUnknown* host);

	// IMpGraphicsWinGdi overrides.
	virtual int32_t MP_STDCALL measure( MpSize availableSize, MpSize &returnDesiredSize );

	// SeGuiWindowsGfxBase overrides.
	virtual int32_t MP_STDCALL paint( HDC hDC );
	virtual int32_t MP_STDCALL onLButtonDown( UINT flags, POINT point );
	virtual int32_t MP_STDCALL onLButtonUp( UINT flags, POINT point );
	virtual int32_t MP_STDCALL onMouseMove( UINT flags, POINT point );

	static void updateNodes(mypoint *nodes, const std::wstring &p_values);

	StringGuiPin pinShape;
private:
	static float StringToFloat(const std::wstring &p_string);
	void DrawScale(HDC hDC);
	void onValueChanged();

	mypoint nodes[WS_NODE_COUNT]; // x,y co-ords of control points
	MpFontInfo fontInfo;
	bool m_inhibit_update;
	POINT m_ptPrev;
	int drag_node;
};

#endif