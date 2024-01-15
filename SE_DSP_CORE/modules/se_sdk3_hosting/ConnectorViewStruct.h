#pragma once
#include "ConnectorView.h"

namespace SynthEdit2
{
	struct sharedGraphicResources_connectors
	{
		GmpiDrawing::SolidColorBrush brushes[13];
		GmpiDrawing::SolidColorBrush errorBrush;
		GmpiDrawing::SolidColorBrush emphasiseBrush;
		GmpiDrawing::SolidColorBrush selectedBrush;
		GmpiDrawing::SolidColorBrush draggingBrush;

		sharedGraphicResources_connectors(GmpiDrawing::Graphics& g)
		{
			using namespace GmpiDrawing;
			brushes[0] = g.CreateSolidColorBrush(Color::Black);

			errorBrush = g.CreateSolidColorBrush(Color::Red);
			emphasiseBrush = g.CreateSolidColorBrush(Color::Lime);
			selectedBrush = g.CreateSolidColorBrush(Color::DodgerBlue);
			draggingBrush = g.CreateSolidColorBrush(Color::Orange);

			uint32_t datatypeColors[] = {
				0x00A800, // ENUM green
				0xbc0000, // TEXT red
				0xD9D900, //0xbcbc00, // MIDI2 yellow
				0x00bcbc, // DOUBLE
				0x000000, // BOOL - black.
				0x0000bc, // float blue
				0x00A8A8,//0x00bcbc, // FLOAT green-blue
				0x008989, // unused
				0xbc5c00, // INT orange
				0xb45e00, // INT64 orange
				0xbc00bc, // BLOB -purple
				0xbc00bc, // Class -purple
				0xbcbcbc, // Spare - white.
			};

			assert(std::size(brushes) <= std::size(datatypeColors));

			for(size_t i = 0; i < std::size(brushes); ++i)
			{
				brushes[i] = g.CreateSolidColorBrush(datatypeColors[i]);
			}
		}
	};

	// Plain connections on structure view only.
	class ConnectorView2 : public ConnectorViewBase
	{
		static const int highlightWidth = 4;
		static const int NodeRadius = 4;
		const float arrowLength = 8;
		const float arrowWidth = 4;
		const float hitTestWidth = 5;

		enum ELineType { CURVEY, ANGLED };
		ELineType lineType_ = ANGLED;

		std::vector<GmpiDrawing::Point> nodes;
		std::vector<GmpiDrawing::PathGeometry> segmentGeometrys;

		int draggingNode = -1;
		int hoverNode = -1;
		int hoverSegment = -1;
		GmpiDrawing::Point arrowPoint;
		Gmpi::VectorMath::Vector2D arrowDirection;

		std::shared_ptr<sharedGraphicResources_connectors> drawingResources;
		sharedGraphicResources_connectors* getDrawingResources(GmpiDrawing::Graphics& g);
		bool mouseHover = {};
		static GmpiDrawing::Point pointPrev; // for dragging nodes
		
	public:
		// Dynamic patch-cables.
		ConnectorView2(Json::Value* pDatacontext, ViewBase* pParent) :
			ConnectorViewBase(pDatacontext, pParent)
		{
			auto& object_json = *datacontext;

			type = CableType::StructureCable;

			lineType_ = (ELineType) object_json.get("lineType", (int)ANGLED).asInt();

			auto& nodes_json = object_json["nodes"];

			if (!nodes_json.empty())
			{
				for (auto& node_json : nodes_json)
				{
					nodes.push_back(GmpiDrawing::Point(node_json["x"].asFloat(), node_json["y"].asFloat()));
				}
			}
		}

		ConnectorView2(ViewBase* pParent, int32_t pfromUgHandle, int fromPin, int32_t ptoUgHandle, int toPin) :
			ConnectorViewBase(pParent, pfromUgHandle, fromPin, ptoUgHandle, toPin)
		{
			type = CableType::StructureCable;
		}

		void CalcArrowGeometery(GmpiDrawing::GeometrySink & sink, GmpiDrawing::Point ArrowTip, Gmpi::VectorMath::Vector2D v1);

		void CreateGeometry() override;
		std::vector<GmpiDrawing::PathGeometry>& GetSegmentGeometrys();
		void CalcBounds() override;
		void OnRender(GmpiDrawing::Graphics& g) override;
		bool hitTest(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
		bool hitTestR(int32_t flags, GmpiDrawing_API::MP1_RECT selectionRect) override;

		int32_t onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
		int32_t onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
		int32_t onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point) override;
		int32_t onMouseWheel(int32_t flags, int32_t delta, GmpiDrawing_API::MP1_POINT point) override
		{
			return gmpi::MP_UNHANDLED;
		}
		int32_t populateContextMenu(GmpiDrawing_API::MP1_POINT point, GmpiGuiHosting::ContextItemsSink2* contextMenuItemsSink) override
		{
			return gmpi::MP_UNHANDLED;
		}
		void setHover(bool mouseIsOverMe) override;

		void OnMoved(GmpiDrawing::Rect& newRect) override
		{
		}
		void OnNodesMoved(std::vector<GmpiDrawing::Point>& newNodes) override;
	};
}
