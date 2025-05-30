#include "pch.h"

#if defined(_WIN32) // skip compilation on macOS

#include <d2d1_2.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <wrl.h> // Comptr
#include <Windowsx.h>
#include <commctrl.h>
#include "./DrawingFrame_win32.h"
#include "../shared/xp_dynamic_linking.h"
#include "../shared/xp_simd.h"
#include "IGuiHost2.h"

using namespace std;
using namespace gmpi;
using namespace gmpi_gui;
using namespace GmpiGuiHosting;
using namespace GmpiDrawing_API;

using namespace Microsoft::WRL;
using namespace D2D1;

namespace GmpiGuiHosting
{

LRESULT CALLBACK DrawingFrameWindowProc(
	HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	auto drawingFrame = (DrawingFrame*)(LONG_PTR)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (drawingFrame)
	{
		return drawingFrame->WindowProc(hwnd, message, wParam, lParam);
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

// copied from MP_GetDllHandle
HMODULE local_GetDllHandle_randomshit()
{
	HMODULE hmodule = 0;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&local_GetDllHandle_randomshit, &hmodule);
	return (HMODULE)hmodule;
}

bool registeredWindowClass = false;
WNDCLASS windowClass;
wchar_t gClassName[100];

void DrawingFrame::open(void* pParentWnd, const GmpiDrawing_API::MP1_SIZE_L* overrideSize)
{
	parentWnd = (HWND)pParentWnd;

	if (!registeredWindowClass)
	{
		registeredWindowClass = true;
		OleInitialize(0);

		swprintf(gClassName, sizeof(gClassName) / sizeof(gClassName[0]), L"GMPIGUI%p", local_GetDllHandle_randomshit());

		windowClass.style = CS_GLOBALCLASS;// | CS_DBLCLKS;//|CS_OWNDC; // add Private-DC constant 

		windowClass.lpfnWndProc = DrawingFrameWindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = local_GetDllHandle_randomshit();
		windowClass.hIcon = 0;

		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
#if DEBUG_DRAWING
		windowClass.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
#else
		windowClass.hbrBackground = 0;
#endif
		windowClass.lpszMenuName = 0;
		windowClass.lpszClassName = gClassName;
		RegisterClass(&windowClass);

		//		bSwapped_mouse_buttons = GetSystemMetrics(SM_SWAPBUTTON) > 0;
	}

	RECT r{};
	if (overrideSize)
	{
		// size to document
		r.right = overrideSize->width;
		r.bottom = overrideSize->height;
	}
	else
	{
		// auto size to parent
		GetClientRect(parentWnd, &r);
	}

	int style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;// | WS_OVERLAPPEDWINDOW;
	int extended_style = 0;

	windowHandle = CreateWindowEx(extended_style, gClassName, L"",
		style, 0, 0, r.right - r.left, r.bottom - r.top,
		parentWnd, NULL, local_GetDllHandle_randomshit(), NULL);

	if (windowHandle)
	{
		SetWindowLongPtr(windowHandle, GWLP_USERDATA, (__int3264)(LONG_PTR)this);
		//		RegisterDragDrop(windowHandle, new CDropTarget(this));

		CreateRenderTarget();

		initTooltip();

		int dpiX, dpiY;
		{
			HDC hdc = ::GetDC(windowHandle);
			dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
			dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
			::ReleaseDC(windowHandle, hdc);
		}

		const GmpiDrawing_API::MP1_SIZE available{
			static_cast<float>(((r.right - r.left) * 96) / dpiX),
			static_cast<float>(((r.bottom - r.top) * 96) / dpiY)
		};

		GmpiDrawing_API::MP1_SIZE desired{};
		gmpi_gui_client->measure(available, &desired);
		gmpi_gui_client->arrange({ 0, 0, available.width, available.height });

		// starting Timer latest to avoid first event getting 'in-between' other init events.
		StartTimer(15); // 16.66 = 60Hz. 16ms timer seems to miss v-sync. Faster timers offer no improvement to framerate.
	}
}

void DrawingFrameBase::initTooltip()
{
	if (tooltipWindow == nullptr && getWindowHandle())
	{
		auto instanceHandle = local_GetDllHandle_randomshit();
		{
			TOOLINFO ti{};

			// Create the ToolTip control.
			HWND hwndTT = CreateWindow(TOOLTIPS_CLASS, TEXT(""),
				WS_POPUP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				NULL, (HMENU)NULL, instanceHandle,
				NULL);

			// Prepare TOOLINFO structure for use as tracking ToolTip.
			ti.cbSize = TTTOOLINFO_V1_SIZE; // win 7 compatible. sizeof(TOOLINFO);
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = (HWND)getWindowHandle();
			ti.uId = (UINT)0;
			ti.hinst = instanceHandle;
			ti.lpszText = const_cast<TCHAR*> (TEXT("This is a tooltip"));
			ti.rect.left = ti.rect.top = ti.rect.bottom = ti.rect.right = 0;

			// Add the tool to the control
			if (!SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti))
			{
				DestroyWindow(hwndTT);
				return;
			}

			tooltipWindow = hwndTT;
		}
	}
}

void DrawingFrameBase::TooltipOnMouseActivity()
{
	if(toolTipShown)
	{
		if (toolTiptimer < -20) // ignore spurious MouseMove when Tooltip shows
		{
			HideToolTip();
			toolTiptimer = toolTiptimerInit;
		}
	}
	else
		toolTiptimer = toolTiptimerInit;
}

void DrawingFrameBase::ShowToolTip()
{
//	_RPT0(_CRT_WARN, "YEAH!\n");

	//UTF8StringHelper tooltipText(tooltip);
	//if (platformObject)
	{
		auto platformObject = tooltipWindow;

		RECT rc;
		rc.left = (LONG)0;
		rc.top = (LONG)0;
		rc.right = (LONG)100000;
		rc.bottom = (LONG)100000;
		TOOLINFO ti = { 0 };
		ti.cbSize = TTTOOLINFO_V1_SIZE; // win 7 compatible. sizeof(TOOLINFO);
		ti.hwnd = (HWND)getWindowHandle(); // frame->getSystemWindow();
		ti.uId = 0;
		ti.rect = rc;
		ti.lpszText = (TCHAR*)(const TCHAR*)toolTipText.c_str();
		SendMessage((HWND)platformObject, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
		SendMessage((HWND)platformObject, TTM_NEWTOOLRECT, 0, (LPARAM)&ti);
		SendMessage((HWND)platformObject, TTM_POPUP, 0, 0);
	}

	toolTipShown = true;
}

void DrawingFrameBase::HideToolTip()
{
	toolTipShown = false;
//	_RPT0(_CRT_WARN, "NUH!\n");

	if (tooltipWindow)
	{
		TOOLINFO ti = { 0 };
		ti.cbSize = TTTOOLINFO_V1_SIZE; // win 7 compatible. sizeof(TOOLINFO);
		ti.hwnd = (HWND)getWindowHandle(); // frame->getSystemWindow();
		ti.uId = 0;
		ti.lpszText = 0;
		SendMessage((HWND)tooltipWindow, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
		SendMessage((HWND)tooltipWindow, TTM_POP, 0, 0);
	}
}

LRESULT DrawingFrameBase::WindowProc(
	HWND hwnd, 
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	if(!gmpi_gui_client)
		return DefWindowProc(hwnd, message, wParam, lParam);

	switch (message)
	{
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		{
			GmpiDrawing::Point p(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
			p = WindowToDips.TransformPoint(p);

			// Cubase sends spurious mouse move messages when transport running.
			// This prevents tooltips working.
			if (message == WM_MOUSEMOVE)
			{
				if (cubaseBugPreviousMouseMove == p)
				{
					return TRUE;
				}
				cubaseBugPreviousMouseMove = p;
			}
			else
			{
				cubaseBugPreviousMouseMove = GmpiDrawing::Point(-1, -1);
			}

			TooltipOnMouseActivity();

			int32_t flags = gmpi_gui_api::GG_POINTER_FLAG_INCONTACT | gmpi_gui_api::GG_POINTER_FLAG_PRIMARY | gmpi_gui_api::GG_POINTER_FLAG_CONFIDENCE;

			switch (message)
			{
				case WM_MBUTTONDOWN:
				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN:
					flags |= gmpi_gui_api::GG_POINTER_FLAG_NEW;
					break;
			}

			switch (message)
			{
			case WM_LBUTTONUP:
			case WM_LBUTTONDOWN:
				flags |= gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON;
				break;
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
				flags |= gmpi_gui_api::GG_POINTER_FLAG_SECONDBUTTON;
				break;
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				flags |= gmpi_gui_api::GG_POINTER_FLAG_THIRDBUTTON;
				break;
			}

			if (GetKeyState(VK_SHIFT) < 0)
			{
				flags |= gmpi_gui_api::GG_POINTER_KEY_SHIFT;
			}
			if (GetKeyState(VK_CONTROL) < 0)
			{
				flags |= gmpi_gui_api::GG_POINTER_KEY_CONTROL;
			}
			if (GetKeyState(VK_MENU) < 0)
			{
				flags |= gmpi_gui_api::GG_POINTER_KEY_ALT;
			}

			int32_t r;
			switch (message)
			{
			case WM_MOUSEMOVE:
				{
					r = gmpi_gui_client->onPointerMove(flags, p);

					// get notified when mouse leaves window
					if (!isTrackingMouse)
					{
						TRACKMOUSEEVENT tme{};
						tme.cbSize = sizeof(TRACKMOUSEEVENT);
						tme.dwFlags = TME_LEAVE;
						tme.hwndTrack = hwnd;

						if (::TrackMouseEvent(&tme))
						{
							isTrackingMouse = true;
						}
						gmpi_gui_client->setHover(true);
					}
				}
				break;

			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
				r = gmpi_gui_client->onPointerDown(flags, p);
				::SetFocus(hwnd);
				break;

			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
			case WM_LBUTTONUP:
				r = gmpi_gui_client->onPointerUp(flags, p);
				break;
			}
		}
		break;

	case WM_MOUSELEAVE:
		isTrackingMouse = false;
		gmpi_gui_client->setHover(false);
		break;

	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			// supplied point is relative to *screen* not window.
			POINT pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			MapWindowPoints(NULL, getWindowHandle(), &pos, 1); // !!! ::ScreenToClient() might be more correct. ref MyFrameWndDirectX::OnMouseWheel

			GmpiDrawing::Point p(static_cast<float>(pos.x), static_cast<float>(pos.y));
			p = WindowToDips.TransformPoint(p);

            //The wheel rotation will be a multiple of WHEEL_DELTA, which is set at 120. This is the threshold for action to be taken, and one such action (for example, scrolling one increment) should occur for each delta.
			const auto zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

			int32_t flags = gmpi_gui_api::GG_POINTER_FLAG_PRIMARY | gmpi_gui_api::GG_POINTER_FLAG_CONFIDENCE;

			if (WM_MOUSEHWHEEL == message)
				flags |= gmpi_gui_api::GG_POINTER_SCROLL_HORIZ;

			const auto fwKeys = GET_KEYSTATE_WPARAM(wParam);
			if (MK_SHIFT & fwKeys)
			{
				flags |= gmpi_gui_api::GG_POINTER_KEY_SHIFT;
			}
			if (MK_CONTROL & fwKeys)
			{
				flags |= gmpi_gui_api::GG_POINTER_KEY_CONTROL;
			}
			//if (GetKeyState(VK_MENU) < 0)
			//{
			//	flags |= gmpi_gui_api::GG_POINTER_KEY_ALT;
			//}

			/*auto r =*/ gmpi_gui_client->onMouseWheel(flags, zDelta, p);
		}
		break;

	case WM_CHAR:
		if(gmpi_key_client)
			gmpi_key_client->OnKeyPress((wchar_t) wParam);
		break;

	case WM_PAINT:
	{
		OnPaint();
		//		return ::DefWindowProc(hwnd, message, wParam, lParam); // clear update rect.
	}
	break;

	case WM_SIZE:
	{
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);

		OnSize(width, height);
		return ::DefWindowProc(hwnd, message, wParam, lParam); // clear update rect.
	}
	break;
	
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);

	}
	return TRUE;
}

void DrawingFrameBase::OnSize(UINT width, UINT height)
{
	assert(m_swapChain);
	assert(mpRenderTarget);

	mpRenderTarget->SetTarget(nullptr);

	if (S_OK == m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))
	{
		CreateDeviceSwapChainBitmap();
	}
	else
	{
		ReleaseDevice();
	}

	int dpiX, dpiY;
	{
		HDC hdc = ::GetDC(getWindowHandle());
		dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
		dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
		::ReleaseDC(getWindowHandle(), hdc);
	}

	const GmpiDrawing_API::MP1_SIZE available{
		static_cast<float>(((width) * 96) / dpiX),
		static_cast<float>(((height) * 96) / dpiY)
	};

	gmpi_gui_client->arrange({0, 0, available.width, available.height });
}

// Ideally this is called at 60Hz so we can draw as fast as practical, but without blocking to wait for Vsync all the time (makes host unresponsive).
bool DrawingFrameBase::OnTimer()
{
	auto hwnd = getWindowHandle();
	if (hwnd == nullptr || gmpi_gui_client == nullptr)
		return true;

	// Tooltips
	if (toolTiptimer-- == 0 && !toolTipShown)
	{
		POINT P;
		GetCursorPos(&P);

		// Check mouse in window and not captured.
		if (WindowFromPoint(P) == hwnd && GetCapture() != hwnd)
		{
			ScreenToClient(hwnd, &P);

			const auto point = WindowToDips.TransformPoint(GmpiDrawing::Point(static_cast<float>(P.x), static_cast<float>(P.y)));

			gmpi_sdk::MpString text;
			gmpi_gui_client->getToolTip(point, &text);
			if (!text.str().empty())
			{
				toolTipText = Utf8ToWstring(text.str());
				ShowToolTip();
			}
		}
	}
	
	if (frameUpdateClient)
	{
		frameUpdateClient->PreGraphicsRedraw();
	}

	// Queue pending drawing updates to backbuffer.
	const BOOL bErase = FALSE;

	for (auto& invalidRect : backBufferDirtyRects)
	{
		::InvalidateRect(hwnd, reinterpret_cast<RECT*>(&invalidRect), bErase);
	}
	backBufferDirtyRects.clear();

	return true;
}

void RenderLog(ID2D1RenderTarget* context_, IDWriteFactory* writeFactory, ID2D1Factory1* factory)
{
	IDWriteTextFormat* textformata0c73080 = nullptr;
	writeFactory->CreateTextFormat(L"Sans Serif", NULL, (DWRITE_FONT_WEIGHT)400, (DWRITE_FONT_STYLE)0, DWRITE_FONT_STRETCH_NORMAL, 10.000000, L"", &textformata0c73080);
	IDWriteTextFormat* textformata0c73860 = nullptr;
	writeFactory->CreateTextFormat(L"Sans Serif", NULL, (DWRITE_FONT_WEIGHT)400, (DWRITE_FONT_STYLE)0, DWRITE_FONT_STRETCH_NORMAL, 10.000000, L"", &textformata0c73860);
	IDWriteTextFormat* textformata0c746d0 = nullptr;
	writeFactory->CreateTextFormat(L"Sans Serif", NULL, (DWRITE_FONT_WEIGHT)400, (DWRITE_FONT_STYLE)0, DWRITE_FONT_STRETCH_NORMAL, 12.000000, L"", &textformata0c746d0);

	// ==================================================
	context_->BeginDraw();
	{
		auto t = D2D1::Matrix3x2F(1.000, 0.000, 0.000, 1.000, 0.000, 0.000);
		context_->SetTransform(t);
	}
	{
		auto r = D2D1::RectF(0.000f, 0.000f, 1718.000f, 865.000);
		context_->PushAxisAlignedClip(&r, D2D1_ANTIALIAS_MODE_ALIASED);
	}
	{
		auto c = D2D1::ColorF(0.651f, 0.651f, 0.651f, 1.000f);
		context_->Clear(c);
	}

	{
		D2D1_MATRIX_3X2_F t;
		context_->GetTransform(&t);
	}
	{
		D2D1_MATRIX_3X2_F t;
		context_->GetTransform(&t);
	}
	{
		auto t = D2D1::Matrix3x2F(1.000, 0.000, 0.000, 1.000, 186.000, 114.000);
		context_->SetTransform(t);
	}
	{
		D2D1_MATRIX_3X2_F t;
		context_->GetTransform(&t);
	}
	ID2D1PathGeometry* geometrya1bdb2e0 = nullptr;
	{
		factory->CreatePathGeometry(&geometrya1bdb2e0);
	}
	ID2D1GeometrySink* sinka1bdb760 = nullptr;
	{
		geometrya1bdb2e0->Open(&sinka1bdb760);
	}
	sinka1bdb760->BeginFigure(D2D1::Point2F(12.500000, 0.000000), (D2D1_FIGURE_BEGIN)0);
	sinka1bdb760->AddLine(D2D1::Point2F(6.500000, 6.000000));
	sinka1bdb760->AddLine(D2D1::Point2F(6.500000, 102.000000));
	sinka1bdb760->AddLine(D2D1::Point2F(12.500000, 108.000000));
	sinka1bdb760->AddLine(D2D1::Point2F(95.500000, 108.000000));
	sinka1bdb760->AddLine(D2D1::Point2F(101.500000, 102.000000));
	sinka1bdb760->AddLine(D2D1::Point2F(101.500000, 6.000000));
	sinka1bdb760->AddLine(D2D1::Point2F(95.500000, 0.000000));
	sinka1bdb760->EndFigure((D2D1_FIGURE_END)1);
	sinka1bdb760->Close();
	sinka1bdb760->Release();
	sinka1bdb760 = nullptr;
	ID2D1SolidColorBrush* brusha1053870 = nullptr;
	{
		auto c = D2D1::ColorF(0.127f, 0.301f, 0.847f, 1.000f);
		context_->CreateSolidColorBrush(c, &brusha1053870);
	}
	context_->FillGeometry(geometrya1bdb2e0, brusha1053870, nullptr);
	geometrya1bdb2e0->Release();
	ID2D1SolidColorBrush* brusha1053800 = nullptr;
	{
		auto c = D2D1::ColorF(0.216f, 0.216f, 0.216f, 1.000f);
		context_->CreateSolidColorBrush(c, &brusha1053800);
	}
	ID2D1SolidColorBrush* brusha1054130 = nullptr;
	{
		auto c = D2D1::ColorF(0.000f, 0.000f, 0.000f, 1.000f);
		context_->CreateSolidColorBrush(c, &brusha1054130);
	}
	{
		auto r = D2D1::RectF(11.000, -1.000, 108.000, 97.000f);
		context_->DrawTextW(L"MIDI In\n\nChannel Lo\nChannel Hi\nNote Lo\nNote Hi\nVelocity Lo\nVelocity Hi\nProgram Change", 85, textformata0c73080, &r, brusha1053800, (D2D1_DRAW_TEXT_OPTIONS)0);
	}
	{
		auto r = D2D1::RectF(11.000, -1.000, 108.000, 97.000f);
		context_->DrawTextW(L"\nMIDI Out\n\n\n\n\n\n\n", 16, textformata0c73860, &r, brusha1053800, (D2D1_DRAW_TEXT_OPTIONS)0);
	}
	{
		auto r = D2D1::RectF(10.000, -17.000, 108.000, 98.000f);
		context_->DrawTextW(L"MIDI Filter", 11, textformata0c746d0, &r, brusha1053800, (D2D1_DRAW_TEXT_OPTIONS)0);
	}
	brusha1054130->Release();
	brusha1054130 = nullptr;
	brusha1053800->Release();
	brusha1053800 = nullptr;
	brusha1053870->Release();
	brusha1053870 = nullptr;
	{
		auto t = D2D1::Matrix3x2F(1.000, 0.000, 0.000, 1.000, 493.000, 120.000);
		context_->SetTransform(t);
	}
	{
		D2D1_MATRIX_3X2_F t;
		context_->GetTransform(&t);
	}
	ID2D1PathGeometry* geometrya1bdac20 = nullptr;
	{
		factory->CreatePathGeometry(&geometrya1bdac20);
	}
	ID2D1GeometrySink* sinka1bdb280 = nullptr;
	{
		geometrya1bdac20->Open(&sinka1bdb280);
	}
	sinka1bdb280->BeginFigure(D2D1::Point2F(12.500000, 0.000000), (D2D1_FIGURE_BEGIN)0);
	sinka1bdb280->AddLine(D2D1::Point2F(6.500000, 6.000000));
	sinka1bdb280->AddLine(D2D1::Point2F(6.500000, 78.000000));
	sinka1bdb280->AddLine(D2D1::Point2F(12.500000, 84.000000));
	sinka1bdb280->AddLine(D2D1::Point2F(95.500000, 84.000000));
	sinka1bdb280->AddLine(D2D1::Point2F(101.500000, 78.000000));
	sinka1bdb280->AddLine(D2D1::Point2F(101.500000, 6.000000));
	sinka1bdb280->AddLine(D2D1::Point2F(95.500000, 0.000000));
	sinka1bdb280->EndFigure((D2D1_FIGURE_END)1);
	sinka1bdb280->Close();
	sinka1bdb280->Release();
	sinka1bdb280 = nullptr;
	ID2D1SolidColorBrush* brusha1054130b = nullptr;
	{
		auto c = D2D1::ColorF(0.127f, 0.301f, 0.847f, 1.000f);
		context_->CreateSolidColorBrush(c, &brusha1054130b);
	}
	context_->FillGeometry(geometrya1bdac20, brusha1054130b, nullptr);
	geometrya1bdac20->Release();
	ID2D1SolidColorBrush* brusha10542f0 = nullptr;
	{
		auto c = D2D1::ColorF(0.0f, 0, 0, 1.000f);
		context_->CreateSolidColorBrush(c, &brusha10542f0);
	}
	ID2D1SolidColorBrush* brusha1053790 = nullptr;
	{
		auto c = D2D1::ColorF(0.000f, 0.000f, 0.000f, 1.000f);
		context_->CreateSolidColorBrush(c, &brusha1053790);
	}
	{
		auto r = D2D1::RectF(11.000, -1.000, 84.000, 97.000f);
		context_->DrawTextW(L"\n\n\n\n\n\n", 6, textformata0c73080, &r, brusha10542f0, (D2D1_DRAW_TEXT_OPTIONS)0);
	}
	{
		auto r = D2D1::RectF(11.000, -1.000, 84.000, 97.000f);
		context_->DrawTextW(L"Name\nValue\nAnimation Position\nMenu Items\nMenu Selection\nMouse Down\nValue Out", 76, textformata0c73860, &r, brusha10542f0, (D2D1_DRAW_TEXT_OPTIONS)0);
	}
	{
		auto r = D2D1::RectF(-1.469f, -17.000f, 84.000f, 109.469f);
		context_->DrawTextW(L"PatchMemory Float3", 18, textformata0c746d0, &r, brusha10542f0, (D2D1_DRAW_TEXT_OPTIONS)0);
	}
	brusha1053790->Release();
	brusha1053790 = nullptr;
	brusha10542f0->Release();
	brusha10542f0 = nullptr;
	brusha1054130b->Release();
	brusha1054130b = nullptr;
	{
		auto t = D2D1::Matrix3x2F(1.000, 0.000, 0.000, 1.000, 0.000, 0.000);
		context_->SetTransform(t);
	}
	context_->PopAxisAlignedClip();
	context_->EndDraw();


	textformata0c73080->Release();
	textformata0c73860->Release();
	textformata0c746d0->Release();

} // RenderLog ====================================================

void DrawingFrameBase::OnPaint()
{
#ifdef USE_BEGINPAINT
	PAINTSTRUCT ps;
	BeginPaint(getWindowHandle(), &ps);
#else
	// First clear update region (else windows will pound on this repeatedly).

	updateRegion_native.copyDirtyRects(getWindowHandle(), swapChainSize);
	ValidateRect(getWindowHandle(), NULL); // Clear invalid region for next frame.
#endif

	// prevent infinite assert dialog boxes when assert happens during painting.
	if (reentrant)
	{
		return;
	}
	reentrant = true;

#ifdef USE_BEGINPAINT
	std::vector<GmpiDrawing::RectL> dirtyRects;
	dirtyRects.push_back(GmpiDrawing::RectL(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom));

	if (containerView)
#else
	auto& dirtyRects = updateRegion_native.getUpdateRects();
	if (gmpi_gui_client && !dirtyRects.empty())
#endif
	{
		//	_RPT1(_CRT_WARN, "OnPaint(); %d dirtyRects\n", dirtyRects.size() );

		if (!mpRenderTarget) // not quite right, also need to re-create any resources (brushes etc) else most object draw blank. Could refresh the view in this case.
		{
			CreateDevice();
		}
/* didn't help
		ID2D1Multithread* m_D2DMultithread;
		DrawingFactory.getD2dFactory()->QueryInterface(IID_PPV_ARGS(&m_D2DMultithread));
		m_D2DMultithread->Enter();
*/

		if (false) // Use render log
		{
			RenderLog(context->native(), DrawingFactory.getDirectWriteFactory(), DrawingFactory.getFactory());
		}
		else
		{
			GmpiDrawing::Graphics graphics(context.get());

			graphics.BeginDraw();
			graphics.SetTransform(viewTransform);

			{
				// clip and draw each react individually (causes some objects to redraw several times)
				for (auto& r : dirtyRects)
				{
					auto r2 = WindowToDips.TransformRect(GmpiDrawing::Rect(static_cast<float>(r.left), static_cast<float>(r.top), static_cast<float>(r.right), static_cast<float>(r.bottom)));

					// Snap to whole DIPs.
					GmpiDrawing::Rect temp;
					temp.left = floorf(r2.left);
					temp.top = floorf(r2.top);
					temp.right = ceilf(r2.right);
					temp.bottom = ceilf(r2.bottom);

					graphics.PushAxisAlignedClip(temp);

					gmpi_gui_client->OnRender(static_cast<GmpiDrawing_API::IMpDeviceContext*>(context.get()));
					graphics.PopAxisAlignedClip();
				}
			}

			// Print OS Version.
#if 0 //def _DEBUG
			{
				OSVERSIONINFO osvi;
				memset(&osvi, 0, sizeof(osvi));

				osvi.dwOSVersionInfoSize = sizeof(osvi);
				GetVersionEx(&osvi);

				char versionString[100];
				sprintf(versionString, "OS Version %d.%d", (int)osvi.dwMajorVersion, (int)osvi.dwMinorVersion);

				auto brsh = graphics.CreateSolidColorBrush(GmpiDrawing::Color::Black);
				graphics.DrawTextU(versionString, graphics.GetFactory().CreateTextFormat(12), GmpiDrawing::Rect(2.0f, 2.0f, 200, 200), brsh);
			}
#endif

			// Print Frame Rate
	//		const bool displayFrameRate = true;
			const bool displayFrameRate = false;
			//		static int presentTimeMs = 0;
			if (displayFrameRate)
			{
				static int frameCount = 0;
				static char frameCountString[100] = "";
				if (++frameCount == 60)
				{
					auto timenow = std::chrono::steady_clock::now();
					auto elapsed = std::chrono::steady_clock::now() - frameCountTime;
					auto elapsedSeconds = 0.001f * (float)std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

					float frameRate = frameCount / elapsedSeconds;

					//				sprintf(frameCountString, "%3.1f FPS. %dms PT", frameRate, presentTimeMs);
					sprintf_s(frameCountString, sizeof(frameCountString), "%3.1f FPS", frameRate);
					frameCountTime = timenow;
					frameCount = 0;

					auto brush = graphics.CreateSolidColorBrush(GmpiDrawing::Color::Black);
					auto fpsRect = GmpiDrawing::Rect(0, 0, 50, 18);
					graphics.FillRectangle(fpsRect, brush);
					brush.SetColor(GmpiDrawing::Color::White);
					graphics.DrawTextU(frameCountString, graphics.GetFactory().CreateTextFormat(12), fpsRect, brush);

					dirtyRects.push_back(GmpiDrawing::RectL(0, 0, 100, 36));
				}
			}

			/*const auto r =*/ graphics.EndDraw();

		}

		//	frontBufferDirtyRects.insert(frontBufferDirtyRects.end(), dirtyRects.begin(), dirtyRects.end());

		// Present the backbuffer (if it has some new content)
		if (firstPresent)
		{
			firstPresent = false;
			const auto hr = m_swapChain->Present(1, 0);
			if (S_OK != hr && DXGI_STATUS_OCCLUDED != hr)
			{
				// DXGI_ERROR_INVALID_CALL 0x887A0001L
				ReleaseDevice();
			}
		}
		else
		{
			HRESULT hr = S_OK;
			{
				assert(!dirtyRects.empty());
				DXGI_PRESENT_PARAMETERS presetParameters{ (UINT)dirtyRects.size(), reinterpret_cast<RECT*>(dirtyRects.data()), nullptr, nullptr, };
/*
				presetParameters.pScrollRect = nullptr;
				presetParameters.pScrollOffset = nullptr;
				presetParameters.DirtyRectsCount = (UINT) dirtyRects.size();
				presetParameters.pDirtyRects = reinterpret_cast<RECT*>(dirtyRects.data()); // should be exact same layout.
*/
				// checkout DXGI_PRESENT_DO_NOT_WAIT
//				hr = m_swapChain->Present1(1, DXGI_PRESENT_TEST, &presetParameters);
//				_RPT1(_CRT_WARN, "Present1() test = %x\n", hr);
/* NEVER returns DXGI_ERROR_WAS_STILL_DRAWING
	//			_RPT1(_CRT_WARN, "Present1() DirtyRectsCount = %d\n", presetParameters.DirtyRectsCount);
				hr = m_swapChain->Present1(1, DXGI_PRESENT_DO_NOT_WAIT, &presetParameters);
				if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
				{
					_RPT1(_CRT_WARN, "Present1() Blocked\n", hr);
*/
					// Present(0... improves framerate only from 60 -> 64 FPS, so must be blocking a little with "1".
//				auto timeA = std::chrono::steady_clock::now();
				hr = m_swapChain->Present1(1, 0, &presetParameters);
				//auto elapsed = std::chrono::steady_clock::now() - timeA;
				//presentTimeMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
//				}
/* could put this in timer to reduce blocking, agregating dirty rects until call successful.
*/			}

			if (S_OK != hr && DXGI_STATUS_OCCLUDED != hr)
			{
				// DXGI_ERROR_INVALID_CALL 0x887A0001L
				ReleaseDevice();
			}
		}
	}

	reentrant = false;

#ifdef USE_BEGINPAINT
	EndPaint(getWindowHandle(), &ps);
#endif
}

void DrawingFrameBase::CreateDevice()
{
	ReleaseDevice();

	// Create a Direct3D Device
	UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	// you must explicity install DX debug support for this to work.
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	// Comment out first to test lower versions.
	D3D_FEATURE_LEVEL d3dLevels[] = 
	{
#if	ENABLE_HDR_SUPPORT
		D3D_FEATURE_LEVEL_11_1,
#endif
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2, // flip SUPPORTED on my PC
		D3D_FEATURE_LEVEL_9_1, // NO flip
	};

	D3D_FEATURE_LEVEL currentDxFeatureLevel;
	ComPtr<ID3D11Device> D3D11Device;
	
	HRESULT r = DXGI_ERROR_UNSUPPORTED;

	// Create Hardware device.
	do {
		r = D3D11CreateDevice(nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			flags,
			d3dLevels, sizeof(d3dLevels) / sizeof(d3dLevels[0]),
			D3D11_SDK_VERSION,
			D3D11Device.GetAddressOf(),
			&currentDxFeatureLevel,
			nullptr);

		CLEAR_BITS(flags, D3D11_CREATE_DEVICE_DEBUG);

	} while (r == 0x887a002d); // The application requested an operation that depends on an SDK component that is missing or mismatched. (no DEBUG LAYER).

	bool DX_support_sRGB;
	{
		/* !! only good for detecting Windows 7 !!
		Applications not manifested for Windows 8.1 or Windows 10 will return the Windows 8 OS version value (6.2).
		Once an application is manifested for a given operating system version,
		GetVersionEx will always return the version that the application is manifested for
		*/
		OSVERSIONINFO osvi;
		memset(&osvi, 0, sizeof(osvi));

		osvi.dwOSVersionInfoSize = sizeof(osvi);
		GetVersionEx(&osvi);

		DX_support_sRGB =
			((osvi.dwMajorVersion > 6) ||
				((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion > 1))); // Win7 = V6.1
	}

	// Support for HDR displays.
#if	ENABLE_HDR_SUPPORT
	float whiteMult{ 1.0f };
#endif
	{
		// query for the device object�s IDXGIDevice interface
		ComPtr<IDXGIDevice> dxdevice;
		D3D11Device.As(&dxdevice);

		// Retrieve the display adapter
		ComPtr<IDXGIAdapter> adapter;
		dxdevice->GetAdapter(adapter.GetAddressOf());

		UINT i = 0;
		ComPtr<IDXGIOutput> currentOutput;
		ComPtr<IDXGIOutput> bestOutput;
		int bestIntersectArea = -1;

		// get bounds of window having handle: getWindowHandle()
		RECT m_windowBounds;
		GetWindowRect(getWindowHandle(), &m_windowBounds);

		while (adapter->EnumOutputs(i, currentOutput.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND)
		{
			// Get the retangle bounds of the app window
			GmpiDrawing::RectL appWindowRect = { m_windowBounds.left, m_windowBounds.top, m_windowBounds.right, m_windowBounds.bottom };

			// Get the rectangle bounds of current output
			DXGI_OUTPUT_DESC desc;
			auto hr = currentOutput->GetDesc(&desc);
			RECT r = desc.DesktopCoordinates;
			GmpiDrawing::RectL outputRect = { r.left, r.top, r.right, r.bottom };

			// Compute the intersection
			const auto intersectRect = Intersect(appWindowRect, outputRect);
			int intersectArea = intersectRect.getWidth() * intersectRect.getHeight();
			if (intersectArea > bestIntersectArea)
			{
				bestOutput = currentOutput;
				bestIntersectArea = intersectArea;
			}

			i++;
		}

		// Having determined the output (display) upon which the app is primarily being 
		// rendered, retrieve the HDR capabilities of that display by checking the color space.
		ComPtr<IDXGIOutput6> output6;
		auto hr = bestOutput.As(&output6);

		if (output6)
		{
			DXGI_OUTPUT_DESC1 desc1;
			hr = output6->GetDesc1(&desc1);

			if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)
			{
				_RPT0(_CRT_WARN, "SDR Display\n");
			}
			else
			{
				_RPT0(_CRT_WARN, "HDR Display\n");
			}

			uint32_t numPathArrayElements{};
			uint32_t numModeArrayElements{};

			GetDisplayConfigBufferSizes(
				QDC_ONLY_ACTIVE_PATHS,
				&numPathArrayElements,
				&numModeArrayElements
			);

			std::vector<DISPLAYCONFIG_PATH_INFO> pathInfo;
			std::vector<DISPLAYCONFIG_MODE_INFO> modeInfo;

			pathInfo.resize(numPathArrayElements);
			modeInfo.resize(numModeArrayElements);

			QueryDisplayConfig(
				QDC_ONLY_ACTIVE_PATHS,
				&numPathArrayElements,
				pathInfo.data(),
				&numModeArrayElements,
				modeInfo.data(),
				nullptr
			);

			DISPLAYCONFIG_SDR_WHITE_LEVEL white_level = {};
			white_level.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
			white_level.header.size = sizeof(white_level);
			for (int pathIdx = 0; pathIdx < numPathArrayElements; ++pathIdx)
			{
				white_level.header.adapterId = pathInfo[pathIdx].targetInfo.adapterId;
				white_level.header.id = pathInfo[pathIdx].targetInfo.id;

				if (DisplayConfigGetDeviceInfo(&white_level.header) == ERROR_SUCCESS)
				{
#if	ENABLE_HDR_SUPPORT // proper HDR rendering
					{
						// divide by 1000 to get nits, divide by reference nits (80) to get a factor
						whiteMult = white_level.SDRWhiteLevel / 1000.f;
						DrawingFactory.whiteMult = whiteMult;
					}
#else // fall back to 8-bit rendering and ignore HDR
					{
						const auto whiteMultiplier = white_level.SDRWhiteLevel / 1000.f;
						DX_support_sRGB = DX_support_sRGB && whiteMultiplier == 1.0f; // workarround HDR issues by reverting to 8-bit colour
					}
#endif
				}
			}
		}
	}

	if (m_disable_gpu)
	{
		// release hardware device
		D3D11Device = nullptr;
		r = DXGI_ERROR_UNSUPPORTED;
	}

	// fallback to software rendering.
	if (DXGI_ERROR_UNSUPPORTED == r)
	{
		do {
			r = D3D11CreateDevice(nullptr,
			D3D_DRIVER_TYPE_WARP,
			nullptr,
			flags,
			nullptr, 0,
			D3D11_SDK_VERSION,
			D3D11Device.GetAddressOf(),
			&currentDxFeatureLevel,
			nullptr);

			CLEAR_BITS(flags, D3D11_CREATE_DEVICE_DEBUG);

		} while (r == 0x887a002d); // The application requested an operation that depends on an SDK component that is missing or mismatched. (no DEBUG LAYER).
	}

	// query for the device object�s IDXGIDevice interface
	ComPtr<IDXGIDevice> dxdevice;
	D3D11Device.As(&dxdevice);

	// Retrieve the display adapter
	ComPtr<IDXGIAdapter> adapter;
	dxdevice->GetAdapter(adapter.GetAddressOf());

	// adapter�s parent object is the DXGI factory
	ComPtr<IDXGIFactory2> factory; // Minimum supported client: Windows 8 and Platform Update for Windows 7 
	adapter->GetParent(__uuidof(factory), reinterpret_cast<void **>(factory.GetAddressOf()));

	// query adaptor memory. Assume small integrated graphics cards do not have the capacity for float pixels.
	// Software renderer has no device memory, yet does support float pixels anyhow.
	if (!m_disable_gpu)
	{
		DXGI_ADAPTER_DESC adapterDesc{};
		adapter->GetDesc(&adapterDesc);

		const auto dedicatedRamMB = adapterDesc.DedicatedVideoMemory / 0x100000;

		// Intel HD Graphics on my Kogan has 128MB.
		DX_support_sRGB &= (dedicatedRamMB >= 512); // MB
	}

	/* NOTES:
		DXGI_FORMAT_R10G10B10A2_UNORM - fails in CreateBitmapFromDxgiSurface() don't supprt direct2d says channel9.
		DXGI_FORMAT_R16G16B16A16_FLOAT - 'tends to take up far too much memoryand bandwidth at HD resolutions' - Stack Overlow.

		DXGI ERROR: IDXGIFactory::CreateSwapChain:
			Flip model swapchains (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD)
			only support the following Formats:
			(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM),
			assuming the underlying Device does as well.
	*/

	// https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
	const DXGI_FORMAT bestFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; // Proper gamma-correct blending.
	const DXGI_FORMAT fallbackFormat = DXGI_FORMAT_B8G8R8A8_UNORM; // shitty linear blending.

	{
		UINT driverSrgbSupport = 0;
		auto hr = D3D11Device->CheckFormatSupport(bestFormat, &driverSrgbSupport);

		const UINT srgbflags = D3D11_FORMAT_SUPPORT_DISPLAY | D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_BLENDABLE;

		if (SUCCEEDED(hr))
		{
			DX_support_sRGB &= ((driverSrgbSupport & srgbflags) == srgbflags);
		}

		/* Surface Studio returns only L1
		D3D11_FEATURE_DATA_D3D11_OPTIONS1 Options1{};
		hr = D3D11Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &Options1, sizeof(Options1));
		if (SUCCEEDED(hr))
		{
			// trying to weed out shitty graphics cards like Intel HD Graphics.
			// these tend to have TiledResourceTier < 1
			DX_support_sRGB &= (D3D11_TILED_RESOURCES_TIER_1 <= Options1.TiledResourcesTier);
		}
		*/
/* 
 works to detect Intel HD graphics, but seems a bit random
		D3D11_SHARED_RESOURCE_TIER SharedResourceTier{};
		hr = D3D11Device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS5, &SharedResourceTier, sizeof(SharedResourceTier));
		if (SUCCEEDED(hr))
		{
			// Specifies that DXGI_FORMAT_R11G11B10_FLOAT supports NT handle sharing.
			// trying to weed out shitty graphics cards like Intel HD Graphics.
			DX_support_sRGB &= (D3D11_SHARED_RESOURCE_TIER_3 == SharedResourceTier);
		}
*/
	}

	DX_support_sRGB &= D3D_FEATURE_LEVEL_11_0 <= currentDxFeatureLevel;

	DXGI_SWAP_CHAIN_DESC1 props {};
	props.SampleDesc.Count = 1;
	props.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	props.BufferCount = 2;
	props.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	// Best. Efficient flip.
	props.Format = bestFormat;
	props.Scaling = DXGI_SCALING_NONE; // prevents annoying stretching effect when resizing window.

	if (lowDpiMode)
	{
		RECT temprect;
		GetClientRect(getWindowHandle(), &temprect);

		props.Width = (temprect.right - temprect.left) / 2;
		props.Height = (temprect.bottom - temprect.top) / 2;
		props.Scaling = DXGI_SCALING_STRETCH;
	}

	auto propsFallback = props;
	propsFallback.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;	// Less efficient blit.
	propsFallback.Format = fallbackFormat;
	propsFallback.Scaling = DXGI_SCALING_STRETCH;

	for (int x = 0 ; x < 2 ;++x)
	{
		auto swapchainresult = factory->CreateSwapChainForHwnd(D3D11Device.Get(),
			getWindowHandle(),
			DX_support_sRGB ? &props : &propsFallback,
			nullptr,
			nullptr,
			&m_swapChain);

		if (swapchainresult == S_OK)
			break;

		DX_support_sRGB = false;
	}

	DrawingFactory.setSrgbSupport(DX_support_sRGB);

#if 0
	// By default, a swap chain created with a floating point pixel format is treated as if it uses the
	// DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 color space.
	// That's the same pixel format and color space used by the DWM.

	// from D2d Advanced Color Images sample: https://learn.microsoft.com/en-us/samples/microsoft/windows-universal-samples/d2dadvancedcolorimages/
	ComPtr<IDXGISwapChain3> advancedSwapChain;
	m_swapChain->QueryInterface(advancedSwapChain.ReleaseAndGetAddressOf());
	if (DX_support_sRGB && advancedSwapChain)
	{
		constexpr auto colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709; // linear sRGB
		UINT colorSpaceSupport = 0;
		advancedSwapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport);

		if ((colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) == DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
		{
			// Set the swap chain's color space.
			auto resd = advancedSwapChain->SetColorSpace1(colorSpace);
		}
	}
#endif

	// Creating the Direct2D Device
	ComPtr<ID2D1Device> device;
	DrawingFactory.getD2dFactory()->CreateDevice(dxdevice.Get(),
		device.GetAddressOf());

	// and context.
	device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &mpRenderTarget);

	float dpiX, dpiY;

	// disable DPI for testing.
	if (lowDpiMode)
	{
		dpiX = dpiY = 96.0f;
	}
	else
	{
		DrawingFactory.getD2dFactory()->GetDesktopDpi(&dpiX, &dpiY);
	}

	mpRenderTarget->SetDpi(dpiX, dpiY);

	DipsToWindow = GmpiDrawing::Matrix3x2::Scale(dpiX / 96.0f, dpiY / 96.0f); // was dpiScaleInverse
	WindowToDips = DipsToWindow;
	WindowToDips.Invert();

	// A little jagged on small fonts
	//	mpRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE); // "The quality of rendering grayscale text is comparable to ClearType but is much faster."}

	CreateDeviceSwapChainBitmap();

	if (DrawingFactory.getPlatformPixelFormat() == GmpiDrawing_API::IMpBitmapPixels::kBGRA_SRGB) // DX_support_sRGB)
	{
		context.reset(new gmpi::directx::GraphicsContext2(mpRenderTarget, &DrawingFactory));
	}
	else
	{
		context.reset(new gmpi::directx::GraphicsContext_Win7(mpRenderTarget, &DrawingFactory));
	}
}

void DrawingFrameBase::CreateDeviceSwapChainBitmap()
{
//	_RPT0(_CRT_WARN, "\n\nCreateDeviceSwapChainBitmap()\n");

	ComPtr<IDXGISurface> surface;
	m_swapChain->GetBuffer(0, // buffer index
		__uuidof(surface),
		reinterpret_cast<void **>(surface.GetAddressOf()));

	// Get the swapchain pixel format.
	DXGI_SURFACE_DESC sufaceDesc;
	surface->GetDesc(&sufaceDesc);

	auto props2 = BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		PixelFormat(sufaceDesc.Format, D2D1_ALPHA_MODE_IGNORE)
		);

	ComPtr<ID2D1Bitmap1> bitmap;
	mpRenderTarget->CreateBitmapFromDxgiSurface(surface.Get(),
		props2,
		bitmap.GetAddressOf());

	const auto bitmapsize = bitmap->GetSize();
	swapChainSize.width = static_cast<int32_t>(bitmapsize.width);
	swapChainSize.height = static_cast<int32_t>(bitmapsize.height);

//_RPT2(_CRT_WARN, "%x B[%f,%f]\n", this, bitmapsize.width, bitmapsize.height);

	// Now attach Device Context to swapchain bitmap.
	mpRenderTarget->SetTarget(bitmap.Get());

	// Initial present() moved here in order to ensure it happens before first Timer() tries to draw anything.
//	HRESULT hr = m_swapChain->Present(0, 0);
	firstPresent = true;

	InvalidateRect(getWindowHandle(), nullptr, false);
}

void DrawingFrameBase::CreateRenderTarget()
{
	CreateDevice();
}

void DrawingFrame::ReSize(int left, int top, int right, int bottom)
{
	const auto width = right - left;
	const auto height = bottom - top;

	if (mpRenderTarget && (swapChainSize.width != width || swapChainSize.height != height))
	{
		SetWindowPos(
			windowHandle
			, NULL
			, 0
			, 0
			, width
			, height
			, SWP_NOZORDER
		);

		// Note: This method can fail, but it's okay to ignore the
		// error here, because the error will be returned again
		// the next time EndDraw is called.
/*
		UINT Width = 0; // Auto size
		UINT Height = 0;

		if (lowDpiMode)
		{
			RECT r;
			GetClientRect(&r);

			Width = (r.right - r.left) / 2;
			Height = (r.bottom - r.top) / 2;
		}
*/
		mpRenderTarget->SetTarget(nullptr);
		if (S_OK == m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))
		{
			CreateDeviceSwapChainBitmap();
		}
		else
		{
			ReleaseDevice();
		}
	}
}

// Convert to an integer rect, ensuring it surrounds all partial pixels.
inline GmpiDrawing::RectL RectToIntegerLarger(GmpiDrawing_API::MP1_RECT f)
{
	GmpiDrawing::RectL r;
	r.left = FastRealToIntTruncateTowardZero(f.left);
	r.top = FastRealToIntTruncateTowardZero(f.top);
	r.right = FastRealToIntTruncateTowardZero(f.right) + 1;
	r.bottom = FastRealToIntTruncateTowardZero(f.bottom) + 1;

	return r;
}

void MP_STDCALL DrawingFrameBase::invalidateRect(const GmpiDrawing_API::MP1_RECT* invalidRect)
{
	GmpiDrawing::RectL r;
	if (invalidRect)
	{
		//_RPT4(_CRT_WARN, "invalidateRect r[ %d %d %d %d]\n", (int)invalidRect->left, (int)invalidRect->top, (int)invalidRect->right, (int)invalidRect->bottom);
		r = RectToIntegerLarger( DipsToWindow.TransformRect(*invalidRect) );
	}
	else
	{
		GetClientRect(getWindowHandle(), reinterpret_cast<RECT*>( &r ));
	}

	auto area1 = r.getWidth() * r.getHeight();

	for (auto& dirtyRect : backBufferDirtyRects )
	{
		auto area2 = dirtyRect.getWidth() * dirtyRect.getHeight();

		GmpiDrawing::RectL unionrect(dirtyRect);

		unionrect.top = (std::min)(unionrect.top, r.top);
		unionrect.bottom = (std::max)(unionrect.bottom, r.bottom);
		unionrect.left = (std::min)(unionrect.left, r.left);
		unionrect.right = (std::max)(unionrect.right, r.right);

		auto unionarea = unionrect.getWidth() * unionrect.getHeight();

		if (unionarea <= area1 + area2)
		{
			// replace existing rect with combined rect
			dirtyRect = unionrect;
			return;
			break;
		}
	}

	// no optimisation found, add new rect.
	backBufferDirtyRects.push_back(r);
}

void MP_STDCALL DrawingFrameBase::invalidateMeasure()
{
}

int32_t DrawingFrameBase::setCapture()
{
	::SetCapture(getWindowHandle());
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::getCapture(int32_t & returnValue)
{
	returnValue = ::GetCapture() == getWindowHandle();
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::releaseCapture()
{
	::ReleaseCapture();

	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::createPlatformMenu(GmpiDrawing_API::MP1_RECT* rect, gmpi_gui::IMpPlatformMenu** returnMenu)
{
	auto nativeRect = DipsToWindow.TransformRect(*rect);
	*returnMenu = new GmpiGuiHosting::PGCC_PlatformMenu(getWindowHandle(), &nativeRect, DipsToWindow._22);
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::createPlatformTextEdit(GmpiDrawing_API::MP1_RECT* rect, gmpi_gui::IMpPlatformText** returnTextEdit)
{
	auto nativeRect = DipsToWindow.TransformRect(*rect);
	*returnTextEdit = new GmpiGuiHosting::PGCC_PlatformTextEntry(getWindowHandle(), &nativeRect, DipsToWindow._22);

	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::createFileDialog(int32_t dialogType, gmpi_gui::IMpFileDialog** returnFileDialog)
{
	*returnFileDialog = new Gmpi_Win_FileDialog(dialogType, getWindowHandle());
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::createOkCancelDialog(int32_t dialogType, gmpi_gui::IMpOkCancelDialog** returnDialog)
{
	*returnDialog = new Gmpi_Win_OkCancelDialog(dialogType, getWindowHandle());
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::pinTransmit(int32_t pinId, int32_t size, const void * data, int32_t voice)
{
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::createPinIterator(gmpi::IMpPinIterator** returnIterator)
{
	return gmpi::MP_FAIL;
}

int32_t DrawingFrameBase::getHandle(int32_t & returnValue)
{
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::sendMessageToAudio(int32_t id, int32_t size, const void * messageData)
{
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::ClearResourceUris()
{
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::RegisterResourceUri(const char * resourceName, const char * resourceType, gmpi::IString* returnString)
{
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::OpenUri(const char * fullUri, gmpi::IProtectedFile2** returnStream)
{
	return gmpi::MP_OK;
}

int32_t DrawingFrameBase::FindResourceU(const char * resourceName, const char * resourceType, gmpi::IString* returnString)
{
	return gmpi::MP_OK;
}

} //namespace

#endif // skip compilation on macOS


/*
*  child window DPI awareness, needs to be TOP level window I think, not the frame.
*     // Store the current thread's DPI-awareness context. Windows 10 only
	// DPI_AWARENESS_CONTEXT previousDpiContext = SetThreadDpiAwarenessContext(context);
	//HINSTANCE h_kernal = GetModuleHandle(_T("Kernel32.DLL"));
	HMODULE hUser32 = GetModuleHandleW(L"user32");
	typedef int32_t DPI_AWARENESS_CONTEXT;
	typedef int32_t DPI_HOSTING_BEHAVIOR;
	typedef DPI_AWARENESS_CONTEXT ( WINAPI * PFN_SETTHREADDPIAWARENESS)(DPI_AWARENESS_CONTEXT param);
	PFN_SETTHREADDPIAWARENESS p_SetThreadDpiAwarenessContext = (PFN_SETTHREADDPIAWARENESS)GetProcAddress(hUser32, "SetThreadDpiAwarenessContext");
	PFN_SETTHREADDPIAWARENESS p_SetThreadDpiHostingBehavior = (PFN_SETTHREADDPIAWARENESS)GetProcAddress(hUser32, "SetThreadDpiHostingBehavior");
	DPI_AWARENESS_CONTEXT previousDpiContext = {};
	if(p_SetThreadDpiAwarenessContext)
	{
		// DPI_AWARENESS_CONTEXT_SYSTEM_AWARE - System DPI aware. This window does not scale for DPI changes. It will query for the DPI once and use that value for the lifetime of the process.
		const auto DPI_AWARENESS_CONTEXT_SYSTEM_AWARE = (DPI_AWARENESS_CONTEXT) -1;
		previousDpiContext = p_SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
	}
	struct CreateParams
	{
		BOOL bEnableNonClientDpiScaling;
		BOOL bChildWindowDpiIsolation;
	};
	CreateParams createParams{TRUE, TRUE};

	// Windows 10 (1803) supports child-HWND DPI-mode isolation. This enables
	// child HWNDs to run in DPI-scaling modes that are isolated from that of
	// their parent (or host) HWND. Without child-HWND DPI isolation, all HWNDs
	// in an HWND tree must have the same DPI-scaling mode.
	DPI_HOSTING_BEHAVIOR previousDpiHostingBehavior = {};
	if (p_SetThreadDpiAwarenessContext) // &&bChildWindowDpiIsolation)
	{
		const int32_t DPI_HOSTING_BEHAVIOR_MIXED = 2;
		previousDpiHostingBehavior = p_SetThreadDpiHostingBehavior(DPI_HOSTING_BEHAVIOR_MIXED);
	}

	windowHandle = CreateWindowEx(extended_style, gClassName, L"",
		style, 0, 0, r.right - r.left, r.bottom - r.top,
		parentWnd, NULL, local_GetDllHandle_randomshit(), &createParams);// NULL);

	if (windowHandle)
	{
		SetWindowLongPtr(windowHandle, GWLP_USERDATA, (__int3264)(LONG_PTR)this);
		//		RegisterDragDrop(windowHandle, new CDropTarget(this));

		// Restore the current thread's DPI awareness context
		if(p_SetThreadDpiAwarenessContext)
		{
			p_SetThreadDpiAwarenessContext(previousDpiContext);

			// Restore the current thread DPI hosting behavior, if we changed it.
			//if(bChildWindowDpiIsolation)
			{
				p_SetThreadDpiHostingBehavior(previousDpiHostingBehavior);
			}
		}

		CreateRenderTarget();

		initTooltip();
	}
*
*
*/
