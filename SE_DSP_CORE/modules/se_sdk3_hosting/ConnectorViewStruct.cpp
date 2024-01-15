#include "pch.h"
#include "ConnectorViewStruct.h"
#include "ModuleView.h"
#include "ContainerView.h"
#include "modules/shared/xplatform_modifier_keys.h"
#include "modules/shared/VectorMath.h"

using namespace GmpiDrawing;
using namespace Gmpi::VectorMath;

namespace SynthEdit2
{
	GmpiDrawing::Point ConnectorView2::pointPrev{}; // for dragging nodes

	Point InterpolatePoint(double t, Point a, Point b)
	{
		return Point(static_cast<float>(a.x + (b.x - a.x) * t), static_cast<float>(a.y + (b.y - a.y) * t));
	}

	void CalcCurve(Point pts0, Point pts1, Point pts2, float tension, Point& p1, Point& p2)
	{
		// tangent from 1st to last point, ignoring center point.
		Vector2D tangent = Vector2D::FromPoints(pts0, pts2);

		// ORIGINAL TEXTBOOK:Control points equal distance from node. Over-shoots on uneven node spaceing.
		//p1 = pts[1] - tangent * tension;
		//p2 = pts[1] + tangent * tension;

		 // Jeff - Control points scaled along tangent depending on node spacing. Good/expensive.
		auto l1 = Vector2D::FromPoints(pts0, pts1).Length();
		auto l2 = Vector2D::FromPoints(pts2, pts1).Length();
		auto tot = l1 + l2;
		tot = (std::max)(1.0f, tot); // prevent divide-by-zero.
		auto tension1 = 2.0f * tension * l1 / tot;
		auto tension2 = 2.0f * tension * l2 / tot;
		p1 = pts1 - tangent * tension1;
		p2 = pts1 + tangent * tension2;
	}

	std::vector<Point> cardinalSpline(std::vector<Point>& pts)
	{
		int i, nrRetPts;
		Point p1, p2;
		constexpr float tension = 0.5f * (1.0f / 3.0f); // Te are calculating contolpoints. Tension is 0.5, smaller value has sharper corners.

		//if (closed)
		//    nrRetPts = (pts.Count + 1) * 3 - 2;
		//else
		nrRetPts = static_cast<int>(pts.size()) * 3 - 2;

		std::vector<Point> retPnt;
		retPnt.assign(nrRetPts, Point{ 0.0f,0.0f });
		//for (i = 0; i < nrRetPts; i++)
		//	retPnt[i] = new Point();

		// first node.
		//if (!closed)
		{
			/*
			// Orig textbook.
			CalcCurveEnd(pts[0], pts[1], tension, out p1);
			retPnt[0] = pts[0];
			retPnt[1] = p1;
			*/
			// Jeff
			i = 0;
			float x = pts[i].x + pts[i].x - pts[i + 1].x; // reflected horiz.
			float y;
			if (x > pts[i].x) // comming out from left (wrong side of module) reflect y too.
			{
				y = pts[i].y + pts[i].y - pts[i + 1].y;
			}
			else
			{
				y = pts[i + 1].y;
			}

			Point mirror(x, y);
			CalcCurve(mirror, pts[i], pts[i + 1], tension, p1, p2);

			retPnt[0] = pts[i];
			retPnt[1] = p2;

		}
		for (i = 0; i < pts.size() - 2; ++i) //(closed ? 1 : 2); i++)
		{
			CalcCurve(pts[i], pts[i + 1], pts[(i + 2) % pts.size()], tension, p1, p2);
			retPnt[3 * i + 2] = p1;
			retPnt[3 * i + 3] = pts[i + 1];
			retPnt[3 * i + 4] = p2;
		}
		//if (closed)
		//{
		//    CalcCurve(new Point[] { pts[pts.Count - 1], pts[0], pts[1] }, tension, out p1, out p2);
		//    retPnt[nrRetPts - 2] = p1;
		//    retPnt[0] = pts[0];
		//    retPnt[1] = p2;
		//    retPnt[nrRetPts - 1] = retPnt[0];
		//}
		//else
		{
			/* textbook
			// Last node
			CalcCurveEnd(pts[pts.Count - 1], pts[pts.Count - 2], tension, out p1);
			*/

			// Jeff. come in at a horizontal by mirroring 2nd to last point horiz.
			i = static_cast<int>(pts.size()) - 1;
			float x = pts[i].x + pts[i].x - pts[i - 1].x; // reflected horiz.
			float y;
			if (x < pts[i].x) // comming in from right (wrong side of module) reflect y too.
			{
				y = pts[i].y + pts[i].y - pts[i - 1].y;
			}
			else
			{
				y = pts[i - 1].y;
			}

			Point mirror(x, y);
			CalcCurve(pts[i - 1], pts[i], mirror, tension, p1, p2);

			retPnt[nrRetPts - 2] = p1;
			retPnt[nrRetPts - 1] = pts[pts.size() - 1];
		}
		return retPnt;
	}

	void ConnectorView2::CalcArrowGeometery(GeometrySink& sink, Point ArrowTip, Vector2D v1)
	{
		auto vn = Perpendicular(v1);
		vn *= arrowWidth * 0.5f;

		Point ArrowBase = ArrowTip + v1 * arrowLength;
		auto arrowBaseLeft = ArrowBase + vn;
		auto arrowBaseRight = ArrowBase - vn;
#if 0
		// draw base line
		if (beginFigure)
		{
			sink.BeginFigure(ArrowBase, FigureBegin::Hollow);
		}
		sink.AddLine(arrowBaseLeft);
		sink.AddLine(ArrowTip);
		sink.AddLine(arrowBaseRight);
		sink.AddLine(ArrowBase);
#else
		// don't draw base line. V-shape.
		sink.BeginFigure(arrowBaseLeft, FigureBegin::Hollow);
		sink.AddLine(ArrowTip);
		sink.AddLine(arrowBaseRight);
#endif
	}

	// calc 'elbow' when line doubles back to the left.
	inline float calcEndFolds(int draggingFromEnd, const Point& from, const Point& to)
	{
		const float endAdjust = 10.0f;

		if (draggingFromEnd < 0 && (from.x > to.x - endAdjust) && fabs(from.y - to.y) > endAdjust)
		{
			return endAdjust;
		}

		return 0.0f;
	}

	void ConnectorView2::CreateGeometry()
	{
		GmpiDrawing::Factory factory(parent->GetDrawingFactory());

		StrokeStyleProperties strokeStyleProperties;
		strokeStyleProperties.setCapStyle(draggingFromEnd != -1 ? CapStyle::Round : CapStyle::Flat);
		strokeStyleProperties.setLineJoin(LineJoin::Round);
		strokeStyle = factory.CreateStrokeStyle(strokeStyleProperties);

		geometry = factory.CreatePathGeometry();
		auto sink = geometry.Open();

		//			_RPT4(_CRT_WARN, "Calc [%.3f,%.3f] -> [%.3f,%.3f]\n", from_.x, from_.y, to_.x, to_.y );
		// No curve when dragging.
		// straight line.

		// 'back' going lines need extra curve
		const auto endAdjust = calcEndFolds(draggingFromEnd, from_, to_);

		std::vector<Point> nodesInclusive; // of start and end point.
		nodesInclusive.push_back(from_);

		if (endAdjust > 0.0f)
		{
			auto p = from_;
			p.x += endAdjust;
			nodesInclusive.push_back(p);
		}

		nodesInclusive.insert(std::end(nodesInclusive), std::begin(nodes), std::end(nodes));

		if (endAdjust > 0.0f)
		{
			auto p = to_;
			p.x -= endAdjust;
			nodesInclusive.push_back(p);
		}

		nodesInclusive.push_back(to_);

		if (lineType_ == CURVEY)
		{
			// Transform on-line nodes into Bezier Spline control points.
			auto splinePoints = cardinalSpline(nodesInclusive);

			sink.BeginFigure(splinePoints[0], FigureBegin::Hollow);

			for (int i = 1; i < splinePoints.size(); i += 3)
			{
				sink.AddBezier(BezierSegment(splinePoints[i], splinePoints[i + 1], splinePoints[i + 2]));
			}

			if (!isGuiConnection)//&& drawArrows)
			{
				int splineCount = (static_cast<int>(splinePoints.size()) - 1) / 3;
				int middleSplineIdx = splineCount / 2;
				if (splineCount & 0x01) // odd number
				{
					// Arrow
					constexpr float t = 0.5f;
					constexpr auto oneMinusT = 1.0f - t;
					Point* p = splinePoints.data() + middleSplineIdx * 3;
					{
						// calulate mid-point on cubic bezier curve.
						arrowPoint.x = oneMinusT * oneMinusT * oneMinusT * p[0].x + 3 * oneMinusT * oneMinusT * t * p[1].x + 3 * oneMinusT * t * t * p[2].x + t * t * t * p[3].x;
						arrowPoint.y = oneMinusT * oneMinusT * oneMinusT * p[0].y + 3 * oneMinusT * oneMinusT * t * p[1].y + 3 * oneMinusT * t * t * p[2].y + t * t * t * p[3].y;
					}
					{
						// calulate tangent at mid-point.
						arrowDirection = 3.f * oneMinusT * oneMinusT * Vector2D::FromPoints(p[0], p[1])
							+ 6.f * t * oneMinusT * Vector2D::FromPoints(p[1], p[2])
							+ 3.f * t * t * Vector2D::FromPoints(p[2], p[3]);
					}
				}
				else
				{
					arrowPoint = splinePoints[middleSplineIdx * 3];
					arrowDirection = Vector2D::FromPoints(splinePoints[middleSplineIdx * 3 - 1], splinePoints[middleSplineIdx * 3 + 1]);
				}
				arrowDirection.Normalize();

				const auto arrowCenter = arrowPoint + 0.5f * arrowLength * arrowDirection;
				sink.EndFigure(FigureEnd::Open);
				CalcArrowGeometery(sink, arrowCenter, -arrowDirection);
				sink.EndFigure(FigureEnd::Closed);
			}
			else
			{
				sink.EndFigure(FigureEnd::Open);
			}
		}
		else
		{
			Vector2D v1 = Vector2D::FromPoints(nodesInclusive.front(), nodesInclusive.back());
			if (v1.LengthSquared() < 0.01f) // fix coincident points.
			{
				v1.x = 0.1f;
			}

			//_RPT1(_CRT_WARN, "v1.Length() %f\n", v1.Length() );
//			constexpr float minDrawLengthSquared = 40.0f * 40.0f;
//			bool drawArrows = v1.LengthSquared() > minDrawLengthSquared && draggingFromEnd < 0;

			v1.Normalize();

			// vector normal.
			auto vn = Perpendicular(v1);
			vn *= arrowWidth * 0.5f;

#if 0
			// left end arrow.
			if (isGuiConnection && drawArrows)
			{
				Point ArrowTip = from_ + v1 * (3 + 0.5f * ModuleViewStruct::plugDiameter);

				CalcArrowGeometery(sink, ArrowTip, v1);
				sink.EndFigure(FigureEnd::Open);
				Point lineStart = ArrowTip + v1 * arrowLength * 0.5f;
				sink.BeginFigure(lineStart, FigureBegin::Hollow);
			}
			else
#endif
			{
				//				sink.BeginFigure(from_, FigureBegin::Hollow);
			}

			bool first = true;
			for (auto& n : nodesInclusive)
			{
				if (first)
				{
					sink.BeginFigure(n, FigureBegin::Hollow);
					first = false;
				}
				else
				{
					sink.AddLine(n);
				}
			}

			if (!nodes.empty())
			{
				Point from = nodes.back();

				v1 = Vector2D::FromPoints(from, to_);
				if (v1.LengthSquared() < 0.01f) // fix coincident points.
				{
					v1.x = 0.1f;
				}

//				drawArrows = v1.LengthSquared() > minDrawLengthSquared;

				v1.Normalize();

				// vector normal.
				vn = Perpendicular(v1);
				vn *= arrowWidth * 0.5f;
			}
#if 0
			// right end.
			{
				Point ArrowTip = to_ - v1 * (3 + 0.5f * ModuleViewStruct::plugDiameter);
				if (drawArrows)
				{
					Point ArrowBase = ArrowTip - v1 * arrowLength * 0.5f;
					sink.AddLine(ArrowBase);
					sink.EndFigure(FigureEnd::Open);
					CalcArrowGeometery(sink, ArrowTip, -v1);
				}
				else
				{
					sink.AddLine(to_);
				}
			}
			sink.EndFigure(FigureEnd::Open);
#else
			//			sink.AddLine(to_);
			sink.EndFigure(FigureEnd::Open); // complete line

			// center arrow
			if (!isGuiConnection)
			{
				int splineCount = (static_cast<int>(nodesInclusive.size()) + 1) / 2;
				int middleSplineIdx = splineCount / 2;
				//				if (splineCount & 0x01) // odd number
				{
					//arrowPoint.x = 0.5f * (nodesInclusive[middleSplineIdx].x + nodesInclusive[middleSplineIdx + 1].x);
					//arrowPoint.y = 0.5f * (nodesInclusive[middleSplineIdx].y + nodesInclusive[middleSplineIdx + 1].y);

					const auto segmentVector = Vector2D::FromPoints(nodesInclusive[middleSplineIdx], nodesInclusive[middleSplineIdx + 1]);
					const auto segmentLength = segmentVector.Length();
					const auto distanceToArrowPoint = (std::min)(segmentLength, 0.5f * (segmentLength + arrowLength));
					arrowDirection = Vector2D::FromPoints(nodesInclusive[middleSplineIdx], nodesInclusive[middleSplineIdx + 1]);
					arrowPoint = nodesInclusive[middleSplineIdx] + (distanceToArrowPoint / segmentLength) * arrowDirection;
				}
				/*
				else
				{
				// ugly on node
					arrowPoint = nodesInclusive[middleSplineIdx];
					arrowDirection = Vector2D::FromPoints(nodesInclusive[middleSplineIdx], nodesInclusive[middleSplineIdx + 1]);
				}
				*/
				arrowDirection.Normalize();

				// Add arrow figure.
				CalcArrowGeometery(sink, arrowPoint, -arrowDirection);
				sink.EndFigure(FigureEnd::Closed);
			}
#endif
		}

		sink.Close();
		segmentGeometrys.clear();
	}

	// as individual segments
	std::vector<GmpiDrawing::PathGeometry>& ConnectorView2::GetSegmentGeometrys()
	{
		if (segmentGeometrys.empty())
		{
			GmpiDrawing::Factory factory(parent->GetDrawingFactory());

			std::vector<Point> nodesInclusive; // of start and end point plus 'elbows'.

			nodesInclusive.push_back(from_);

			// exclude the elbows, else inserting nodes gets confused.
			const auto endAdjust = calcEndFolds(draggingFromEnd, from_, to_);
			const bool hasElbows = endAdjust > 0.0f;
			if (hasElbows)
			{
				auto p = from_;
				p.x += endAdjust;
				nodesInclusive.push_back(p);
			}

			nodesInclusive.insert(std::end(nodesInclusive), std::begin(nodes), std::end(nodes));

			if (hasElbows)
			{
				auto p = to_;
				p.x -= endAdjust;
				nodesInclusive.push_back(p);
			}

			nodesInclusive.push_back(to_);

			if (lineType_ == CURVEY)
			{
				// Transform on-line nodes into Bezier Spline control points.
				auto splinePoints = cardinalSpline(nodesInclusive);

				const size_t fromIdx = hasElbows ? 3 : 0;
				const size_t toIdx = hasElbows ? splinePoints.size() - 4 : splinePoints.size() - 1;

				for (size_t i = fromIdx; i < toIdx; i += 3)
				{
					auto segment = factory.CreatePathGeometry();
					auto sink = segment.Open();
					sink.BeginFigure(splinePoints[i], FigureBegin::Hollow);
					sink.AddBezier(BezierSegment(splinePoints[i + 1], splinePoints[i + 2], splinePoints[i + 3]));
					sink.EndFigure(FigureEnd::Open);
					sink.Close();
					segmentGeometrys.push_back(segment);
				}
			}
			else
			{
				bool first = true;
				PathGeometry segment;
				GeometrySink sink;

				const size_t fromIdx = hasElbows ? 1 : 0;
				const size_t toIdx = hasElbows ? nodesInclusive.size() - 1 : nodesInclusive.size();

				for (size_t i = fromIdx; i < toIdx; ++i)
				{
					if (!first)
					{
						sink.AddLine(nodesInclusive[i]);

//						if (hasElbows && (i == 1 || i == nodesInclusive.size() - 2))
//							continue;
						
						sink.EndFigure(FigureEnd::Open);
						sink.Close();
						segmentGeometrys.push_back(segment);
					}
					first = false;

					if (i == toIdx - 1) // last.
						continue;

					segment = factory.CreatePathGeometry();
					sink = segment.Open();
					sink.BeginFigure(nodesInclusive[i], FigureBegin::Hollow);
				}
			}
		}
		return segmentGeometrys;
	}

	void ConnectorView2::CalcBounds()
	{
		CreateGeometry();

		auto oldBounds = bounds_;

		float expand = getSelected() ? (float)NodeRadius * 2 + 1 : (float)cableDiameter;

		bounds_ = geometry.GetWidenedBounds(expand, strokeStyle);

		if (oldBounds != bounds_)
		{
			oldBounds.Union(bounds_);
			parent->ChildInvalidateRect(oldBounds);
		}
	}

	GraphicsResourceCache<sharedGraphicResources_connectors> drawingResourcesCache;

	sharedGraphicResources_connectors* ConnectorView2::getDrawingResources(GmpiDrawing::Graphics& g)
	{
		if (!drawingResources)
		{
			drawingResources = drawingResourcesCache.get(g);
		}

		return drawingResources.get();
	}

	void ConnectorView2::OnRender(GmpiDrawing::Graphics& g)
	{
		if (geometry.isNull())
			return;

		auto resources = getDrawingResources(g);
		float width = 2.0f;
		SolidColorBrush* brush3 = {};
#if defined( _DEBUG )
		auto pinkBrush = g.CreateSolidColorBrush(Color::Pink);
		if (cancellation != 0.0f)
		{
			brush3 = &pinkBrush;
			width = 9.0f * cancellation;
		}
		else
#endif 
		{
			if (draggingFromEnd < 0)
			{
				if ((highlightFlags & 3) != 0)
				{
					if ((highlightFlags & 1) != 0) // error
					{
						brush3 = &resources->errorBrush;
					}
					else
					{
						// Emphasise
						brush3 = &resources->emphasiseBrush;
					}
				}
				else
				{
					brush3 = &resources->brushes[datatype];
				}
			}
			else
			{
				// highlighted (dragging).
				brush3 = &resources->draggingBrush;
			}
		}

		if (getSelected())
		{
			brush3 = &resources->selectedBrush;
		}

		if (getSelected() || mouseHover)
		{
			width = 3.f;
		}

		assert(brush3);
		g.DrawGeometry(geometry, *brush3, width, strokeStyle);

		if (getSelected() && hoverSegment != -1)
		{
			auto segments = GetSegmentGeometrys();
			g.DrawGeometry(segments[hoverSegment], *brush3, width + 1);
		}

		// Nodes
		if (getSelected())
		{
			auto outlineBrush = g.CreateSolidColorBrush(Color::DodgerBlue);
			auto fillBrush = g.CreateSolidColorBrush(Color::White);

			for (auto& n : nodes)
			{
				g.FillCircle(n, static_cast<float>(NodeRadius), fillBrush);
				g.DrawCircle(n, static_cast<float>(NodeRadius), outlineBrush);
			}
		}
#ifdef _DEBUG
		//		g.DrawCircle(arrowPoint, static_cast<float>(NodeRadius), g.CreateSolidColorBrush(Color::DodgerBlue));
		//		g.DrawLine(arrowPoint - arrowDirection * 10.0f, arrowPoint + arrowDirection * 10.0f, g.CreateSolidColorBrush(Color::Black));
#endif
	}

	int32_t ConnectorView2::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		// dragging something.
		if (parent->getCapture())
		{
			// dragging a node.
			if (draggingNode != -1)
			{
				const auto snapGridSize = Presenter()->GetSnapSize();
				GmpiDrawing::Size delta(point.x - pointPrev.x, point.y - pointPrev.y);
				if (delta.width != 0.0f || delta.height != 0.0f) // avoid false snap on selection
				{
					const float halfGrid = snapGridSize * 0.5f;
					
					GmpiDrawing::Point snapReference = nodes[draggingNode];

					// nodes snap to center of grid, not lines of grid like modules do
					GmpiDrawing::Point newPoint = snapReference + delta;
					newPoint.x = halfGrid + floorf((newPoint.x) / snapGridSize) * snapGridSize;
					newPoint.y = halfGrid + floorf((newPoint.y) / snapGridSize) * snapGridSize;
					GmpiDrawing::Size snapDelta = newPoint - snapReference;

					pointPrev += snapDelta;

					if (snapDelta.width != 0.0 || snapDelta.height != 0.0)
					{
						Presenter()->DragNode(getModuleHandle(), draggingNode, pointPrev);
						nodes[draggingNode] = pointPrev;
						CalcBounds();

						parent->ChildInvalidateRect(bounds_);
					}
				}

				return gmpi::MP_OK;
			}

			// dragging new line
			return ConnectorViewBase::onPointerMove(flags, point);
		}

		return gmpi::MP_UNHANDLED;
	}

	int32_t ConnectorView2::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (!parent->getCapture() || draggingNode == -1)
		{
			return ConnectorViewBase::onPointerUp(flags, point);
		}

		parent->releaseCapture();
		return gmpi::MP_OK;
	}

	bool ConnectorView2::hitTest(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		hoverNode = -1;
		hoverSegment = -1;

		if (!GetClipRect().ContainsPoint(point))
		{
			return false;
		}
		if (geometry.isNull())
		{
			return false;
		}

		GmpiDrawing::Point local(point);
		local.x -= bounds_.left;
		local.y -= bounds_.top;

		if (!geometry.StrokeContainsPoint(point, 3.0f))
			return false;

		// hit test individual node points.
		int i = 0;
		for (auto& n : nodes)
		{
			float dx = n.x - point.x;
			float dy = n.y - point.y;

			if ((dx * dx + dy * dy) <= (float)((1 + NodeRadius) * (1 + NodeRadius)))
			{
				hoverNode = i;
				break;
			}
			++i;
		}

		// hit test individual segments.
		if (hoverNode == -1)
		{
			hoverSegment = -1;

			auto segments = GetSegmentGeometrys();
			int j = 0;
			for (auto& s : segments)
			{
				if (s.StrokeContainsPoint(point, hitTestWidth))
				{
					hoverSegment = j;
					break;
				}
				++j;
			}

			if (hoverSegment == -1)
			{
				// handle mouse over elbows and minor hit test glitches by defaulting to nearest end.
				auto d1 = Vector2D::FromPoints(from_, point).LengthSquared();
				auto d2 = Vector2D::FromPoints(to_, point).LengthSquared();
				if (d1 < d2)
				{
					hoverSegment = 0;
				}
				else
				{
					hoverSegment = static_cast<int>(segments.size()) - 1;
				}
			}
		}

		return true;
	}

	bool ConnectorView2::hitTestR(int32_t flags, GmpiDrawing_API::MP1_RECT selectionRect)
	{
		if (!isOverlapped(GetClipRect(), GmpiDrawing::Rect(selectionRect)) || geometry.isNull())
		{
			return false;
		}

#ifdef _WIN32
		// TODO !! auto relation = geometry.CompareWithGeometry(otherGeometry, otherGeometryTransform);

		auto d2dGeometry = dynamic_cast<gmpi::directx::Geometry*>(geometry.Get())->native();

		ID2D1Factory* factory = {};
		d2dGeometry->GetFactory(&factory);

		// TODO!!! this really only needs computing once, not on every single line
		ID2D1RectangleGeometry* rectangleGeometry = {};
		factory->CreateRectangleGeometry(
			{
				selectionRect.left,
				selectionRect.top,
				selectionRect.right,
				selectionRect.bottom,
			},
			&rectangleGeometry
			);

		Matrix3x2 inputGeometryTransform;

		D2D1_GEOMETRY_RELATION relation = {};

		d2dGeometry->CompareWithGeometry(
			rectangleGeometry,
			(D2D1_MATRIX_3X2_F*)&inputGeometryTransform,
			&relation
		);

		rectangleGeometry->Release();
		factory->Release();

		return D2D1_GEOMETRY_RELATION_DISJOINT != relation;
#else
		// !! TODO provide on mac (used only in editor anyhow)
		return true;
#endif
	}

	void ConnectorView2::setHover(bool mouseIsOverMe)
	{
		mouseHover = mouseIsOverMe;

		if (!mouseHover)
		{
			hoverNode = hoverSegment = -1;
		}

		const auto redrawRect = GetClipRect();
		parent->getGuiHost()->invalidateRect(&redrawRect);
	}

	void ConnectorView2::OnNodesMoved(std::vector<GmpiDrawing::Point>& newNodes)
	{
		nodes = newNodes;
		parent->getGuiHost()->invalidateMeasure();
	}

	// TODO: !!! hit-testing lines should be 'fuzzy' and return the closest line when more than 1 is hittable (same as plugs).
	// This allows a bigger hit radius without losing accuracy. maybe hit tests return a 'confidence (0.0 - 1.0).
	int32_t ConnectorView2::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
//		_RPT0(_CRT_WARN, "ConnectorView2::onPointerDown\n");

		if (parent->getCapture()) // then we are *already* draging.
		{
			parent->autoScrollStop();
			parent->releaseCapture();
			parent->EndCableDrag(point, this);
			// I am now DELETED!!!
			return gmpi::MP_HANDLED;
		}
		else
		{
			if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) != 0)
			{
				// Clicked a node?
				if (hoverNode >= 0)
				{
					draggingNode = hoverNode;
					pointPrev = point;
					parent->setCapture(this);
					return gmpi::MP_OK;
				}
				else
				{
					// When already selected, clicks add new nodes.
					if (getSelected())
					{
						assert(hoverSegment >= 0); // shouldn't get mouse-down without previously calling hit-test

						Presenter()->InsertNode(handle, hoverSegment + 1, point);

						nodes.insert(nodes.begin() + hoverSegment, point);

						draggingNode = hoverSegment;
						parent->setCapture(this);
						CalcBounds();
						parent->ChildInvalidateRect(bounds_); // sometimes bounds don't change, but still need to draw new node.

						hitTest(flags, point); // re-hit-test to get new hoverNode.

						return gmpi::MP_OK;
					}

					int hitEnd = -1;
					// Is hit at line end?
					GmpiDrawing::Size delta = from_ - Point(point);
					float lengthSquared = delta.width * delta.width + delta.height * delta.height;
					float hitRadiusSquared = 100;
					if (lengthSquared < hitRadiusSquared)
					{
						hitEnd = 0;
					}
					else
					{
						delta = to_ - Point(point);
						lengthSquared = delta.width * delta.width + delta.height * delta.height;
						if (lengthSquared < hitRadiusSquared)
						{
							hitEnd = 1;
						}
					}

					// Select Object.
					Presenter()->ObjectClicked(handle, gmpi::modifier_keys::getHeldKeys());

					if (hitEnd == -1)
						return gmpi::MP_OK; // normal hit.
					// TODO pickup from end, mayby when <ALT> held.
				}
				return gmpi::MP_OK;
			}
			else
			{
				// Context menu.
				if ((flags & gmpi_gui_api::GG_POINTER_FLAG_SECONDBUTTON) != 0)
				{
					GmpiGuiHosting::ContextItemsSink2 contextMenu;
					Presenter()->populateContextMenu(&contextMenu, handle, hoverNode);

					// Cut, Copy, Paste etc.
					contextMenu.AddSeparator();

					// Add custom entries e.g. "Remove Cable".
					/*auto result =*/ populateContextMenu(point, &contextMenu);

					GmpiDrawing::Rect r(point.x, point.y, point.x, point.y);

					GmpiGui::PopupMenu nativeMenu;
					parent->ChildCreatePlatformMenu(&r, nativeMenu.GetAddressOf());
					contextMenu.ShowMenuAsync2(nativeMenu, point);

					return gmpi::MP_HANDLED; // Indicate menu already shown.
				}
			}
		}
		return gmpi::MP_HANDLED;
	}
} // namespace

