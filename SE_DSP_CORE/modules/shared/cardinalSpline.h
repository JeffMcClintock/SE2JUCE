#pragma once
#include "Drawing.h"
#include "VectorMath.h"

namespace gmpi
{
namespace drawing
{
namespace utils
{

inline GmpiDrawing::Point InterpolatePoint(double t, GmpiDrawing::Point a, GmpiDrawing::Point b)
{
	return GmpiDrawing::Point(static_cast<float>(a.x + (b.x - a.x) * t), static_cast<float>(a.y + (b.y - a.y) * t));
}

inline void CalcCurve(GmpiDrawing::Point pts0, GmpiDrawing::Point pts1, GmpiDrawing::Point pts2, float tension, GmpiDrawing::Point& p1, GmpiDrawing::Point& p2)
{
	// tangent from 1st to last point, ignoring center point.
	Gmpi::VectorMath::Vector2D tangent = Gmpi::VectorMath::Vector2D::FromPoints(pts0, pts2);

	// ORIGINAL TEXTBOOK:Control points equal distance from node. Over-shoots on uneven node spaceing.
	//p1 = pts[1] - tangent * tension;
	//p2 = pts[1] + tangent * tension;

		// Jeff - Control points scaled along tangent depending on node spacing. Good/expensive.
	auto l1 = Gmpi::VectorMath::Vector2D::FromPoints(pts0, pts1).Length();
	auto l2 = Gmpi::VectorMath::Vector2D::FromPoints(pts2, pts1).Length();
	auto tot = l1 + l2;
	tot = (std::max)(1.0f, tot); // prevent divide-by-zero.
	auto tension1 = 2.0f * tension * l1 / tot;
	auto tension2 = 2.0f * tension * l2 / tot;
	p1 = pts1 - tangent * tension1;
	p2 = pts1 + tangent * tension2;
}

inline std::vector<GmpiDrawing::Point> cardinalSpline(std::vector<GmpiDrawing::Point>& pts, float tension_adj = 0.5f)
{
	int i, nrRetPts;
	GmpiDrawing::Point p1, p2;
	const float tension = tension_adj * (1.0f / 3.0f); // We are calculating control points. Tension is 0.5, smaller value has sharper corners.

	//if (closed)
	//    nrRetPts = (pts.Count + 1) * 3 - 2;
	//else
	nrRetPts = static_cast<int>(pts.size()) * 3 - 2;

	std::vector<GmpiDrawing::Point> retPnt;
	retPnt.assign(nrRetPts, GmpiDrawing::Point{ 0.0f,0.0f });
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

		GmpiDrawing::Point mirror(x, y);
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

		GmpiDrawing::Point mirror(x, y);
		CalcCurve(pts[i - 1], pts[i], mirror, tension, p1, p2);

		retPnt[nrRetPts - 2] = p1;
		retPnt[nrRetPts - 1] = pts[pts.size() - 1];
	}
	return retPnt;
}
}
}
}