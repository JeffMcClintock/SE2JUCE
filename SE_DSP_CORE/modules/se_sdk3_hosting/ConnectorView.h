#pragma once
#include "IViewChild.h"
#include "modules/se_sdk2/se_datatypes.h"
#include "../shared/VectorMath.h"

namespace SynthEdit2
{
	class ConnectorViewBase : public ViewChild
	{
	protected:
		const int cableDiameter = 6;
		static const int lineWidth_ = 3;
		
		int32_t fromModuleH;
		int32_t toModuleH;
		int fromModulePin;
		int toModulePin;
		char datatype = DT_FSAMPLE;
		bool isGuiConnection = false;
		int highlightFlags = 0;
#if defined( _DEBUG )
		float cancellation = 0.0f;
#endif

		GmpiDrawing::PathGeometry geometry;
		GmpiDrawing::StrokeStyle strokeStyle;

	public:

		int draggingFromEnd = -1;
		GmpiDrawing::Point from_;
		GmpiDrawing::Point to_;

		CableType type = CableType::PatchCable;

		// Fixed connections.
		ConnectorViewBase(Json::Value* pDatacontext, ViewBase* pParent);

		// Dynamic patch-cables.
		ConnectorViewBase(ViewBase* pParent, int32_t pfromUgHandle, int fromPin, int32_t ptoUgHandle, int toPin) :
			ViewChild(pParent, -1)
			, fromModuleH(pfromUgHandle)
			, toModuleH(ptoUgHandle)
			, fromModulePin(fromPin)
			, toModulePin(toPin)
		{
		}

		void setHighlightFlags(int flags);

		virtual bool useDroop()
		{
			return false;
		}

		void pickup(int draggingFromEnd, GmpiDrawing_API::MP1_POINT pMousePos);

		int32_t toModuleHandle()
		{
			return toModuleH;
		}

		int32_t fromModuleHandle()
		{
			return fromModuleH;
		}
		int32_t fromPin()
		{
			return fromModulePin;
		}
		int32_t toPin()
		{
			return toModulePin;
		}

		int32_t measure(GmpiDrawing::Size availableSize, GmpiDrawing::Size* returnDesiredSize) override;
		int32_t arrange(GmpiDrawing::Rect finalRect) override;

		void OnModuleMoved();
		virtual void CalcBounds() = 0;
		virtual void CreateGeometry() = 0;

		// IViewChild
		int32_t onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
		int32_t onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
		int32_t onMouseWheel(int32_t flags, int32_t delta, GmpiDrawing_API::MP1_POINT point) override
		{
			return gmpi::MP_UNHANDLED;
		}

		void OnMoved(GmpiDrawing::Rect& newRect) override {}
		void OnNodesMoved(std::vector<GmpiDrawing::Point>& newNodes) override {}

		int32_t onContextMenu(int32_t idx) override;

		int32_t fixedEndModule()
		{
			return draggingFromEnd == 1 ? fromModuleH : toModuleH;
		}
		int32_t fixedEndPin()
		{
			return draggingFromEnd == 1 ? fromModulePin : toModulePin;
		}
		GmpiDrawing::Point dragPoint()
		{
			return draggingFromEnd == 1 ? to_ : from_;
		}
	};

	struct sharedGraphicResources_patchcables
	{
		static const int numColors = 5;
		GmpiDrawing::SolidColorBrush brushes[numColors][2]; // 5 colors, 2 opacities.
		GmpiDrawing::SolidColorBrush highlightBrush;
		GmpiDrawing::SolidColorBrush outlineBrush;

		sharedGraphicResources_patchcables(GmpiDrawing::Graphics& g)
		{
			using namespace GmpiDrawing;
			highlightBrush = g.CreateSolidColorBrush(Color::White);
			outlineBrush = g.CreateSolidColorBrush(Color::Black);

			uint32_t datatypeColors[numColors] = {
				0x00B56E, // 0x00A800, // green
				0xED364A, // 0xbc0000, // red
				0xFFB437, // 0xD9D900, // yellow
				0x3695EF, // 0x0000bc, // blue
//				0x000000, // black 0x00A8A8, // green-blue
				0x8B4ADE, // 0xbc00bc, // purple
			};

			for(int i = 0; i < numColors; ++i)
			{
				brushes[i][0] = g.CreateSolidColorBrush(datatypeColors[i]);					// Visible
				brushes[i][1] = g.CreateSolidColorBrush(Color(datatypeColors[i], 0.5f));	// Faded
			}
		}
	};

	// Dynamic patch-cables.
	// Fancy end-user-connected Patch-Cables.
	class PatchCableView : public ConnectorViewBase
	{
		bool isHovered = false;
		bool isShownCached = true;
		std::shared_ptr<sharedGraphicResources_patchcables> drawingResources;
		int colorIndex = 0;

		inline constexpr static float mouseNearEndDist = 20.0f; // should be more than mouseNearWidth, else looks glitchy.

	public:

		PatchCableView(ViewBase* pParent, int32_t pfromUgHandle, int fromPin, int32_t ptoUgHandle, int toPin, int pColor = -1) :
			ConnectorViewBase(pParent, pfromUgHandle, fromPin, ptoUgHandle, toPin)
		{
			if (pColor >= 0)
			{
				colorIndex = pColor;
			}
			else
			{
				// index -1 use next color cyclically
				// index -2 use current color

				static int nextColorIndex = 0;
				if (pColor == -1 && ++nextColorIndex >= sharedGraphicResources_patchcables::numColors)
					nextColorIndex = 0;

				colorIndex = nextColorIndex;
			}

			datatype = DT_FSAMPLE;
			isGuiConnection = false;
		}

		~PatchCableView();

		bool useDroop() override
		{
			return true;
		}

		int getColorIndex()
		{
			return colorIndex;
		}

		virtual void CreateGeometry() override;
		virtual void CalcBounds() override;

		bool isShown() override; // Indicates if module should be drawn or not (because of 'Show on Parent' state).
		void OnVisibilityUpdate();
		void OnRender(GmpiDrawing::Graphics& g) override;
		void setHover(bool) override;
		int32_t onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
		bool hitTest(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
		virtual int32_t populateContextMenu(GmpiDrawing_API::MP1_POINT point, GmpiGuiHosting::ContextItemsSink2* contextMenuItemsSink) override;

		sharedGraphicResources_patchcables* getDrawingResources(GmpiDrawing::Graphics& g);
	};
}
