#include "mp_sdk_gui2.h"
#include "../shared/FontCache.h"
#include "../shared/expression_evaluate.h"
#include "../shared/unicode_conversion.h"
#include "Drawing.h"
#include <iomanip>
#include <sstream>
#include "../sharedLegacyWidgets/WidgetHost.h"
#include "../sharedLegacyWidgets/EditWidget.h"
#include "../sharedLegacyWidgets/TextWidget.h"

using namespace std;
using namespace gmpi;
using namespace GmpiDrawing;

const float scaleFactor = 2.15f;
const float scaleFactorB = scaleFactor * 0.5f;

class SeWaveshaperGuiBase : public gmpi_gui::MpGuiGfxBase, public FontCacheClient
{
protected:
	StringGuiPin pinShape;
	GmpiDrawing::TextFormat dtextFormat;
	FontMetadata* typeface_ = nullptr;

public:
	void DrawScale(Graphics g)
	{
		const float snapToPixelOffset = 0.5f;

		auto r2 = getRect();

		auto background_brush = g.CreateSolidColorBrush(typeface_->getBackgroundColor());
		g.FillRectangle(r2, background_brush);

		int width = r2.right - r2.left;
		int height = r2.bottom - r2.top;
		float v_scale = height / scaleFactor;
		float h_scale = width / scaleFactor;
		int mid_x = width / 2;
		int mid_y = height / 2;

		// create a green pen
		auto darked_col = typeface_->getColor();
		darked_col.r *= 0.5f;
		darked_col.g *= 0.5f;
		darked_col.b *= 0.5f;

		auto brush2 = g.CreateSolidColorBrush(darked_col);

		// BACKGROUND LINES
		// horizontal line
		const float penWidth = 1.0f;
		g.DrawLine(GmpiDrawing::Point(0, mid_y + snapToPixelOffset), GmpiDrawing::Point(width, mid_y + snapToPixelOffset), brush2, penWidth);

		// horiz center line
		g.DrawLine(GmpiDrawing::Point(mid_x + snapToPixelOffset, 0), GmpiDrawing::Point(mid_x + snapToPixelOffset, height), brush2, penWidth);

		// diagonal
		g.DrawLine(GmpiDrawing::Point(snapToPixelOffset, height), GmpiDrawing::Point(width + snapToPixelOffset, 0), brush2, penWidth);

		int tick_width;
		for (int v = -10; v < 11; v += 2)
		{
			float y = v * v_scale / 10.f;
			float x = v * h_scale / 10.f;

			if (v % 5 == 0)
				tick_width = 4;
			else
				tick_width = 2;

			// X-Axis ticks
			g.DrawLine(GmpiDrawing::Point(snapToPixelOffset + mid_x - tick_width, snapToPixelOffset + mid_y + (int)y), GmpiDrawing::Point(snapToPixelOffset + mid_x + tick_width, snapToPixelOffset + mid_y + (int)y), brush2, penWidth);

			// Y-Axis ticks
			g.DrawLine(GmpiDrawing::Point(snapToPixelOffset + mid_x + (int)x, snapToPixelOffset + mid_y - tick_width), GmpiDrawing::Point(snapToPixelOffset + mid_x + (int)x, snapToPixelOffset + mid_y + tick_width), brush2, penWidth);
		}

		// labels
		if (height > 30)
		{
			// Set up the font
			int fontHeight = 10;

			auto brush = g.CreateSolidColorBrush(typeface_->getColor());// Color::FromArgb(0xFF00FA00));

																		//SetTextColor(hDC, RGB(0, 250, 0));
																		//SetBkMode(hDC, TRANSPARENT);
																		//SetTextAlign(hDC, TA_LEFT);
			dtextFormat.SetTextAlignment(TextAlignment::Leading);

			char txt[10];
			// Y-Axis text
			for (float fv = -5; fv < 5.1; fv += 2.0)
			{
				float y = fv * v_scale / 5.f;
				if (fv != -1.f)
				{
					sprintf(txt, "%2.0f", fv);
					//TextOut(hDC, mid_x + tick_width, mid_y - (int)y - fontHeight / 2, txt, (int)wcslen(txt));
					int tx = mid_x + tick_width;
					int ty = mid_y - (int)y - fontHeight / 2;
					GmpiDrawing::Rect textRect(tx, ty, tx + 100, ty + typeface_->pixelHeight_);
					g.DrawTextU(txt, dtextFormat, textRect, brush2);
				}
			}

			//			int orig_ta = SetTextAlign(hDC, TA_CENTER);
			//TODO			auto originalAlignment = dtextFormat.GetTextAlignment();
			dtextFormat.SetTextAlignment(TextAlignment::Center);
			// X-Axis text
			for (float fv = -4; fv < 4.1; fv += 2.0)
			{
				if (fv != -1.f)
				{
					sprintf(txt, "%2.0f", fv);
					//	TextOut(hDC, mid_x + (int)y, mid_y + tick_width, txt, (int)wcslen(txt));
					float x = fv * h_scale / 5.f;
					int tx = mid_x + (int)x;
					int ty = mid_y + tick_width;
					GmpiDrawing::Rect textRect(tx - 50, ty, tx + 50, ty + typeface_->pixelHeight_);
					g.DrawTextU(txt, dtextFormat, textRect, brush2);
				}
			}
		}
	}
};

class SeWaveshaper3XpGui : public SeWaveshaperGuiBase
{
	static const int WS_NODE_COUNT = 11;
	static const int NODE_SIZE = 6;
	Point nodes[WS_NODE_COUNT]; // x,y co-ords of control points
	Point m_ptPrev;
	int drag_node;

	void onSetshape()
	{
		auto s = pinShape.getValue();

		// convert CString of numbers to array of screen co-ords
		int i = 0;
		while (s.length() > 0 && i < WS_NODE_COUNT)
		{
			int p1 = s.find(L"(");
			int p2 = s.find(L",");
			int p3 = s.find(L")");
			if (p3 < 1)
				return;
			
			wchar_t* unused;

			float x = wcstof(s.substr(p1 + 1, p2 - p1 - 1).c_str(), &unused);
			float y = wcstof(s.substr(p2 + 1, p3 - p2 - 1).c_str(), &unused);
			x = (x + 5.f) * 10.f; // convert to screen co-ords
			y = (-y + 5.f) * 10.f;

			nodes[i].x = x;
			nodes[i++].y = y;

			p3++;
			s = s.substr(p3, s.length() - p3);
		}

		if (i == 0) // if string empty, set defaults
		{
			const wchar_t* DEFAULT_VALUE = L"(-5.0,-5.0)(-4.0,-4.0)(-3.0,-3.0)(-2.0,-2.0)(-1.0,-1.0)(0.0,0.0)(1.0,1.0)(2.0,2.0)(3.0,3.0)(4.0,4.0)(5.0,5.0)";
			pinShape = wstring(DEFAULT_VALUE);
		}

		invalidateRect();
	}

public:
	SeWaveshaper3XpGui() :
		drag_node(-1)
	{
		initializePin(pinShape, static_cast<MpGuiBaseMemberPtr2>(&SeWaveshaper3XpGui::onSetshape));
	}

	virtual int32_t MP_STDCALL onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		auto r2 = getRect();

		int width = r2.right - r2.left;
		int height = r2.bottom - r2.top;

		float vscale = r2.getHeight() * 0.01f / scaleFactorB;
		float hscale = r2.getWidth() * 0.01f / scaleFactorB;
		float midX = r2.getWidth() * 0.5f;
		float midY = r2.getHeight() * 0.5f;

		for (int i = WS_NODE_COUNT - 1; i >= 0; i--)
		{
			float x = midX + (nodes[i].x - 50) * hscale - NODE_SIZE / 2;
			float y = midY + (nodes[i].y - 50) * vscale - NODE_SIZE / 2;
			Rect nodeRect(x - 0.5, y - 0.5, x + NODE_SIZE + 1, y + NODE_SIZE + 1);
			if(nodeRect.ContainsPoint(point))
			{
				drag_node = i;
				m_ptPrev = point;
				setCapture();
				return MP_HANDLED;
			}
		}
		
		return MP_OK;
	}

	virtual int32_t MP_STDCALL onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if ( !getCapture() || drag_node < 0)
			return MP_OK;

		{
			auto r2 = getRect();

			int width = r2.right - r2.left;
			int height = r2.bottom - r2.top;

			float vscale = r2.getHeight() * 0.01f;
			float hscale = r2.getWidth() * 0.01f;

			float left = 0.f;
			float right = 100.f;

			if (drag_node > 0)
				left = nodes[drag_node - 1].x + 1;

			if (drag_node < WS_NODE_COUNT - 1)
				right = nodes[drag_node + 1].x - 1;

			if (drag_node == 0)
				right = 0.f;

			if (drag_node == WS_NODE_COUNT - 1)
				left = 100.f;

			Point p(nodes[drag_node]);
			p.x += (float)(point.x - m_ptPrev.x) / hscale;
			p.y += (float)(point.y - m_ptPrev.y) / vscale;
			// constain
			p.x = max(p.x, left);
			p.x = min(p.x, right);
			p.y = max(p.y, 0.f);
			p.y = min(p.y, 100.f);
			nodes[drag_node] = p;
			m_ptPrev = point;
		}

		//	std::wstring v;
		std::wostringstream oss;

		for (int i = 0 ; i < WS_NODE_COUNT; ++i)
		{
			float x = nodes[i].x / 10.f - 5.f;
			float y = 5.f - nodes[i].y / 10.f;

			oss << setiosflags(ios_base::fixed) << L"(" << setprecision(1) << x << L"," << setiosflags(ios_base::fixed) << setprecision(1) << y << L")";
		}

		pinShape = oss.str();

		return MP_HANDLED;
	}

	virtual int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if (getCapture())
			releaseCapture();

		return MP_OK;
	}

	int32_t MP_STDCALL OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext) override
	{
		Graphics g(drawingContext);

		auto r2 = getRect();
		g.PushAxisAlignedClip(r2);

		DrawScale(g);

		float vscale = r2.getHeight() * 0.01f / scaleFactorB;
		float hscale = r2.getWidth() * 0.01f / scaleFactorB;
		float midX = r2.getWidth() * 0.5f;
		float midY = r2.getHeight() * 0.5f;

		auto brush = g.CreateSolidColorBrush(Color::FromArgb(0xFF00FF00));

		for(int i = 1 ; i < WS_NODE_COUNT ; ++i )
		{
			float x1 = midX + (nodes[i-1].x - 50) * hscale;
			float y1 = midY + (nodes[i-1].y - 50) * vscale;
			float x2 = midX + (nodes[i].x - 50) * hscale;
			float y2 = midY + (nodes[i].y - 50) * vscale;
			g.DrawLine(x1, y1, x2, y2, brush);
		}

		for (int i = WS_NODE_COUNT - 1; i >= 0; i--)
		{
			float x = midX + (nodes[i].x - 50) * hscale - NODE_SIZE / 2;
			float y = midY + (nodes[i].y - 50) * vscale - NODE_SIZE / 2;
			g.DrawRectangle(Rect(x, y, x + NODE_SIZE, y + NODE_SIZE), brush);
		}

		if (drag_node > -1)
		{
			Point p(nodes[drag_node].x, nodes[drag_node].y);
			float x = midX + (nodes[drag_node].x - 50) * hscale - NODE_SIZE;
			float y = midY + (nodes[drag_node].y - 50) * vscale - NODE_SIZE;
			std::ostringstream oss;
			oss << setiosflags(ios_base::fixed) << setprecision(1) << x << setiosflags(ios_base::fixed) << setprecision(1) << y;
			g.DrawTextU(oss.str().c_str(), dtextFormat, x, y, brush);
		}

		g.PopAxisAlignedClip();

		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL initialize() override
	{
		// Create a custom, cached, TextFormat based on the "tty" Style.
		const char* style = "tty";
		const char* customStyleName = "Waveshaper3:text";
		if (!TextFormatExists(getHost(), getGuiHost(), customStyleName))
		{
			// Get the specified font style.
			FontMetadata* fontmetadata = nullptr;
			GetTextFormat(getHost(), getGuiHost(), style, &fontmetadata);

			auto textFormat = AddCustomTextFormat(getHost(), getGuiHost(), customStyleName, fontmetadata);

			// Apply customisation.
			textFormat.SetTextAlignment(TextAlignment::Leading); // Left
			textFormat.SetParagraphAlignment(ParagraphAlignment::Center);
			textFormat.SetWordWrapping(WordWrapping::NoWrap); // prevent word wrapping into two lines that don't fit box.
		}

		// retrieve cached font data (color etc).
		auto textFormat_readonly = GetTextFormat(getHost(), getGuiHost(), customStyleName, &typeface_);
		// convert to mutable (so we can change alignment while drawing).
		*dtextFormat.GetAddressOf() = textFormat_readonly.Get();
		dtextFormat.Get()->addRef();

		return MpGuiGfxBase::initialize();
	}
};

class WsGraphWidget : public Widget, public FontCacheClient
{
public:
	string shape;

	WsGraphWidget()
//	: typeface_(nullptr)
	{}
	//~EditWidget();

	void DrawScale(Graphics g)
	{
		FontMetadata* typeface = nullptr;
		GetTextFormat(patchMemoryHost_, guiHost_, "tty", &typeface); // get ttyp font color etc (ignore actual font becuase we need to create a custom one to modify_
		auto dtextFormat = g.GetFactory().CreateTextFormat(10);

		const float snapToPixelOffset = 0.5f;

		auto r2 = getRect();
		int width = r2.right - r2.left;
		int height = r2.bottom - r2.top;

		auto background_brush = g.CreateSolidColorBrush(typeface->getBackgroundColor());
		g.FillRectangle(Rect(0,0, width, height), background_brush);

		float v_scale = height / scaleFactor;
		float h_scale = width / scaleFactor;
		int mid_x = width / 2;
		int mid_y = height / 2;

		// create a green pen
		auto darked_col = typeface->getColor();
		darked_col.r *= 0.5f;
		darked_col.g *= 0.5f;
		darked_col.b *= 0.5f;

		auto brush2 = g.CreateSolidColorBrush(darked_col);

		// BACKGROUND LINES
		// horizontal line
		const float penWidth = 1.0f;
		g.DrawLine(GmpiDrawing::Point(0, mid_y + snapToPixelOffset), GmpiDrawing::Point(width, mid_y + snapToPixelOffset), brush2, penWidth);

		// horiz center line
		g.DrawLine(GmpiDrawing::Point(mid_x + snapToPixelOffset, 0), GmpiDrawing::Point(mid_x + snapToPixelOffset, height), brush2, penWidth);

		// diagonal
		g.DrawLine(GmpiDrawing::Point(snapToPixelOffset, height), GmpiDrawing::Point(width + snapToPixelOffset, 0), brush2, penWidth);

		int tick_width;
		for (int v = -10; v < 11; v += 2)
		{
			float y = v * v_scale / 10.f;
			float x = v * h_scale / 10.f;

			if (v % 5 == 0)
				tick_width = 4;
			else
				tick_width = 2;

			// X-Axis ticks
			g.DrawLine(GmpiDrawing::Point(snapToPixelOffset + mid_x - tick_width, snapToPixelOffset + mid_y + (int)y), GmpiDrawing::Point(snapToPixelOffset + mid_x + tick_width, snapToPixelOffset + mid_y + (int)y), brush2, penWidth);

			// Y-Axis ticks
			g.DrawLine(GmpiDrawing::Point(snapToPixelOffset + mid_x + (int)x, snapToPixelOffset + mid_y - tick_width), GmpiDrawing::Point(snapToPixelOffset + mid_x + (int)x, snapToPixelOffset + mid_y + tick_width), brush2, penWidth);
		}

		// labels
		if (height > 30)
		{
			// Set up the font
			int fontHeight = 10;

			auto brush = g.CreateSolidColorBrush(typeface->getColor());// Color::FromArgb(0xFF00FA00));

																		//SetTextColor(hDC, RGB(0, 250, 0));
																		//SetBkMode(hDC, TRANSPARENT);
																		//SetTextAlign(hDC, TA_LEFT);
			dtextFormat.SetTextAlignment(TextAlignment::Leading);

			char txt[10];
			// Y-Axis text
			for (float fv = -5; fv < 5.1; fv += 2.0)
			{
				float y = fv * v_scale / 5.f;
				if (fv != -1.f)
				{
					sprintf(txt, "%2.0f", fv);
					//TextOut(hDC, mid_x + tick_width, mid_y - (int)y - fontHeight / 2, txt, (int)wcslen(txt));
					int tx = mid_x + tick_width;
					int ty = mid_y - (int)y - fontHeight / 2;
					GmpiDrawing::Rect textRect(tx, ty, tx + 100, ty + typeface->pixelHeight_);
					g.DrawTextU(txt, dtextFormat, textRect, brush2);
				}
			}

			//			int orig_ta = SetTextAlign(hDC, TA_CENTER);
			//TODO			auto originalAlignment = dtextFormat.GetTextAlignment();
			dtextFormat.SetTextAlignment(TextAlignment::Center);
			// X-Axis text
			for (float fv = -4; fv < 4.1; fv += 2.0)
			{
				if (fv != -1.f)
				{
					sprintf(txt, "%2.0f", fv);
					//	TextOut(hDC, mid_x + (int)y, mid_y + tick_width, txt, (int)wcslen(txt));
					float x = fv * h_scale / 5.f;
					int tx = mid_x + (int)x;
					int ty = mid_y + tick_width;
					GmpiDrawing::Rect textRect(tx - 50, ty, tx + 50, ty + typeface->pixelHeight_);
					g.DrawTextU(txt, dtextFormat, textRect, brush2);
				}
			}
		}
	}

	void OnRender(GmpiDrawing::Graphics& g) override
	{
		const auto originalTransform = g.GetTransform();
		auto adjustedTransform = Matrix3x2::Translation(position.left, position.top) * originalTransform;
		g.SetTransform(adjustedTransform);

		auto r2 = getRect();
//		g.PushAxisAlignedClip(r2);

		DrawScale(g);

		float vscale = r2.getHeight() * 0.01f / scaleFactorB;
		float hscale = r2.getWidth() * 0.01f / scaleFactorB;
		int mid_x = r2.getWidth() / 2;
		int mid_y = r2.getHeight() / 2;

		auto brush = g.CreateSolidColorBrush(Color::FromArgb(0xFF00FF00));

		Evaluator ee;
		auto formula_ascii = shape; //;

		int flags = 0;

		const float ten_over_scale = scaleFactorB * 10.f / r2.getWidth();

		auto geometry = g.GetFactory().CreatePathGeometry();
		auto sink = geometry.Open();

		for (int x = 0; x < r2.getWidth(); x++)
		{
			double xf = ten_over_scale * (float)(x - mid_x);
			ee.SetValue("x", &xf);
			double yf;
			ee.Evaluate(formula_ascii.c_str(), &yf, &flags);
			if (isnan(yf))
			{
				yf = 0.0;
			}

			yf = mid_y - 10.0f * yf * vscale; // 0.1 * yf * r2.getHeight() / scaleFactorB;

			if (x == 0)
			{
				sink.BeginFigure(x, yf);
			}
			else
			{
				sink.AddLine(GmpiDrawing::Point(x, yf));
			}
		}
		sink.EndFigure(FigureEnd::Open);
		sink.Close();

		g.DrawGeometry(geometry, brush, 1.0f);

		// Transform back.
		g.SetTransform(originalTransform);
	}

	void Init(const char* style)
	{
//		FontCacheClient::Init(style);
	}

	virtual GmpiDrawing::Size getSize() override
	{
		return Size(100, 100);
	}
};

class SeWaveshaper2XpGuiB : public WidgetHost
{
	//GmpiDrawing::TextFormat dtextFormat;
	//FontMetadata* typeface_;

public:
	SeWaveshaper2XpGuiB()// :
//		typeface_(nullptr)
	{
		initializePin(pinShape, static_cast<MpGuiBaseMemberPtr2>(&SeWaveshaper2XpGuiB::onSetShape));
	}

	// IMpGraphics overrides.
	virtual int32_t MP_STDCALL onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		if (!getCapture())
		{
			return gmpi::MP_UNHANDLED;
		}

		releaseCapture();

		if (captureWidget)
		{
			captureWidget->onPointerUp(flags, point);

			for (auto& w : widgets)
			{
				if (w->ClearDirty())
				{
					invalidateRect();
					break;
				}
			}
		}

		captureWidget = nullptr;

		return gmpi::MP_OK;
	}

	virtual int32_t MP_STDCALL initialize() override
	{
		// Header (just a spacer)
		auto header = make_shared<TextWidget>();
		widgets.push_back(header);

		// Graph
		auto g = make_shared<WsGraphWidget>();
		widgets.push_back(g);

		auto e = make_shared<EditWidget>();
		e->OnChangedEvent = [this](const char* newvalue) -> void { this->OnWidgetUpdate(newvalue); };

		widgets.push_back(e);
/*
		dtextFormat.SetTextAlignment(TextAlignment::Leading); // Left
		dtextFormat.SetParagraphAlignment(ParagraphAlignment::Center);
		dtextFormat.SetWordWrapping(WordWrapping::NoWrap); // prevent word wrapping into two lines that don't fit box.
 */
		for( auto& w : widgets )
			w->setHost(getHost());

		e->Init("control_edit");
		e->SetText(pinShape);

		header->Init("control_label", true);

		onSetShape();

		return MpGuiGfxBase::initialize();
	}

	int32_t MP_STDCALL measure(GmpiDrawing_API::MP1_SIZE availableSize, GmpiDrawing_API::MP1_SIZE* returnDesiredSize) override
	{
		const float minSize = 15;

		returnDesiredSize->width = (std::max)(minSize, availableSize.width);
		returnDesiredSize->height = (std::max)(minSize, availableSize.height);

		return gmpi::MP_OK;
	}

	int32_t MP_STDCALL arrange(GmpiDrawing_API::MP1_RECT finalRect) override
	{
		WidgetHost::arrange(finalRect);

		Rect r(finalRect);
		
		// Header
		r.bottom = widgets[0]->getSize().height;
		widgets[0]->setPosition(r);

		// Graph
		r.top = r.bottom;
		r.bottom = finalRect.bottom - widgets[2]->getSize().height;
		widgets[1]->setPosition(r);

		// Edit box
		r.top = r.bottom;
		r.bottom = finalRect.bottom;
		widgets[2]->setPosition(r);

		return MP_OK;
	}

private:
	void OnWidgetUpdate(const char* newValue)
	{
		pinShape = newValue;
	}

	void onSetShape()
	{
		if( widgets.size() > 0 )
		{
			const auto textU = JmUnicodeConversions::WStringToUtf8(pinShape.getValue());
			dynamic_cast<WsGraphWidget*>(widgets[1].get())->shape = textU;
			dynamic_cast<EditWidget*>(widgets[2].get())->SetText(textU);
		}

        invalidateRect(); // required on mac because update is async
	}

	StringGuiPin pinShape;
};

namespace
{
	auto r1 = Register<SeWaveshaper3XpGui>::withId(L"SE Waveshaper3 XP");
	auto r2 = Register<SeWaveshaper2XpGuiB>::withId(L"SE Waveshaper2 XP");
}
