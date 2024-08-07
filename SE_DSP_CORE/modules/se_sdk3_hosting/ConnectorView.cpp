#include "pch.h"
#include "ConnectorView.h"
#include "ContainerView.h"
#include "ModuleView.h"
#include "ModuleViewStruct.h"
#include "modules/shared/xplatform_modifier_keys.h"
#include "modules/shared/VectorMath.h"

using namespace GmpiDrawing;
using namespace Gmpi::VectorMath;

// perceptualy even colors. (rainbow). Might need the one with purple instead (for BLOBs).
// https://bokeh.github.io/colorcet/

const float rainbow_bgyr_35_85_c72[256][3] = {
{0, 0.20387, 0.96251},
{0, 0.21524, 0.9514},
{0, 0.22613, 0.94031},
{0, 0.23654, 0.92923},
{0, 0.24654, 0.91817},
{0, 0.2562, 0.90712},
{0, 0.26557, 0.89608},
{0, 0.27465, 0.88506},
{0, 0.28348, 0.87405},
{0, 0.29209, 0.86305},
{0, 0.30047, 0.85206},
{0, 0.3087, 0.84109},
{0, 0.31672, 0.83013},
{0, 0.32458, 0.81917},
{0, 0.33232, 0.80823},
{0, 0.3399, 0.7973},
{0, 0.34736, 0.78638},
{0, 0.3547, 0.77546},
{0, 0.36191, 0.76456},
{0, 0.36902, 0.75367},
{0, 0.37602, 0.7428},
{0, 0.38291, 0.73193},
{0, 0.38969, 0.72108},
{0, 0.39636, 0.71026},
{0, 0.40292, 0.69946},
{0, 0.40934, 0.68872},
{0, 0.41561, 0.67802},
{0, 0.42172, 0.66738},
{0, 0.42768, 0.65684},
{0, 0.43342, 0.64639},
{0, 0.43896, 0.63605},
{0, 0.44432, 0.62583},
{0, 0.44945, 0.61575},
{0, 0.45438, 0.60579},
{0, 0.45911, 0.59597},
{0.0043377, 0.46367, 0.58627},
{0.029615, 0.46807, 0.57668},
{0.055795, 0.47235, 0.56717},
{0.077065, 0.47652, 0.55774},
{0.095292, 0.48061, 0.54837},
{0.11119, 0.48465, 0.53903},
{0.1253, 0.48865, 0.52971},
{0.13799, 0.49262, 0.5204},
{0.14937, 0.49658, 0.5111},
{0.15963, 0.50055, 0.50179},
{0.169, 0.50452, 0.49244},
{0.17747, 0.50849, 0.48309},
{0.18517, 0.51246, 0.4737},
{0.19217, 0.51645, 0.46429},
{0.19856, 0.52046, 0.45483},
{0.20443, 0.52448, 0.44531},
{0.20974, 0.52851, 0.43577},
{0.21461, 0.53255, 0.42616},
{0.21905, 0.53661, 0.41651},
{0.22309, 0.54066, 0.40679},
{0.22674, 0.54474, 0.397},
{0.23002, 0.54883, 0.38713},
{0.233, 0.55292, 0.3772},
{0.23568, 0.55703, 0.36716},
{0.23802, 0.56114, 0.35704},
{0.24006, 0.56526, 0.34678},
{0.24185, 0.56939, 0.3364},
{0.24334, 0.57354, 0.32588},
{0.24458, 0.57769, 0.31523},
{0.24556, 0.58185, 0.30439},
{0.2463, 0.58603, 0.29336},
{0.2468, 0.59019, 0.28214},
{0.24707, 0.59438, 0.27067},
{0.24714, 0.59856, 0.25896},
{0.24703, 0.60275, 0.24696},
{0.24679, 0.60693, 0.23473},
{0.24647, 0.61109, 0.22216},
{0.24615, 0.61523, 0.20936},
{0.24595, 0.61936, 0.19632},
{0.246, 0.62342, 0.18304},
{0.24644, 0.62742, 0.16969},
{0.24748, 0.63135, 0.1563},
{0.24925, 0.63518, 0.14299},
{0.25196, 0.6389, 0.13001},
{0.2557, 0.64249, 0.11741},
{0.26057, 0.64594, 0.10557},
{0.26659, 0.64926, 0.094696},
{0.27372, 0.65242, 0.084904},
{0.28182, 0.65545, 0.076489},
{0.29078, 0.65835, 0.069753},
{0.30043, 0.66113, 0.064513},
{0.31061, 0.66383, 0.060865},
{0.32112, 0.66642, 0.058721},
{0.33186, 0.66896, 0.057692},
{0.34272, 0.67144, 0.057693},
{0.35356, 0.67388, 0.058443},
{0.36439, 0.67628, 0.059738},
{0.37512, 0.67866, 0.061142},
{0.38575, 0.68102, 0.062974},
{0.39627, 0.68337, 0.064759},
{0.40666, 0.68571, 0.066664},
{0.41692, 0.68803, 0.068644},
{0.42707, 0.69034, 0.070512},
{0.43709, 0.69266, 0.072423},
{0.44701, 0.69494, 0.074359},
{0.45683, 0.69723, 0.076211},
{0.46657, 0.6995, 0.07809},
{0.47621, 0.70177, 0.079998},
{0.48577, 0.70403, 0.081943},
{0.49527, 0.70629, 0.083778},
{0.5047, 0.70853, 0.085565},
{0.51405, 0.71076, 0.087502},
{0.52335, 0.71298, 0.089316},
{0.53259, 0.7152, 0.091171},
{0.54176, 0.7174, 0.092931},
{0.5509, 0.7196, 0.094839},
{0.55999, 0.72178, 0.096566},
{0.56902, 0.72396, 0.098445},
{0.57802, 0.72613, 0.10023},
{0.58698, 0.72828, 0.10204},
{0.5959, 0.73044, 0.10385},
{0.60479, 0.73258, 0.10564},
{0.61363, 0.73471, 0.10744},
{0.62246, 0.73683, 0.10925},
{0.63125, 0.73895, 0.11102},
{0.64001, 0.74104, 0.11282},
{0.64874, 0.74315, 0.11452},
{0.65745, 0.74523, 0.11636},
{0.66613, 0.74731, 0.11813},
{0.67479, 0.74937, 0.11986},
{0.68343, 0.75144, 0.12161},
{0.69205, 0.75348, 0.12338},
{0.70065, 0.75552, 0.12517},
{0.70923, 0.75755, 0.12691},
{0.71779, 0.75957, 0.12868},
{0.72633, 0.76158, 0.13048},
{0.73487, 0.76359, 0.13221},
{0.74338, 0.76559, 0.13396},
{0.75188, 0.76756, 0.13568},
{0.76037, 0.76954, 0.13747},
{0.76884, 0.77151, 0.13917},
{0.77731, 0.77346, 0.14097},
{0.78576, 0.77541, 0.14269},
{0.7942, 0.77736, 0.14444},
{0.80262, 0.77928, 0.14617},
{0.81105, 0.7812, 0.14791},
{0.81945, 0.78311, 0.14967},
{0.82786, 0.78502, 0.15138},
{0.83626, 0.78691, 0.15311},
{0.84465, 0.7888, 0.15486},
{0.85304, 0.79066, 0.15662},
{0.86141, 0.79251, 0.15835},
{0.86978, 0.79434, 0.16002},
{0.87814, 0.79612, 0.16178},
{0.88647, 0.79786, 0.16346},
{0.89477, 0.79952, 0.16507},
{0.90301, 0.80106, 0.1667},
{0.91115, 0.80245, 0.16819},
{0.91917, 0.80364, 0.16964},
{0.92701, 0.80456, 0.1709},
{0.93459, 0.80514, 0.172},
{0.94185, 0.80532, 0.17289},
{0.94869, 0.80504, 0.17355},
{0.95506, 0.80424, 0.17392},
{0.96088, 0.80289, 0.17399},
{0.96609, 0.80097, 0.17375},
{0.97069, 0.7985, 0.17319},
{0.97465, 0.79549, 0.17234},
{0.97801, 0.79201, 0.17121},
{0.98082, 0.7881, 0.16986},
{0.98314, 0.78384, 0.16825},
{0.98504, 0.77928, 0.16652},
{0.9866, 0.7745, 0.16463},
{0.98789, 0.76955, 0.16265},
{0.98897, 0.76449, 0.16056},
{0.9899, 0.75932, 0.15848},
{0.99072, 0.75411, 0.15634},
{0.99146, 0.74885, 0.15414},
{0.99214, 0.74356, 0.15196},
{0.99279, 0.73825, 0.14981},
{0.9934, 0.73293, 0.1476},
{0.99398, 0.72759, 0.14543},
{0.99454, 0.72224, 0.1432},
{0.99509, 0.71689, 0.14103},
{0.99562, 0.71152, 0.1388},
{0.99613, 0.70614, 0.13659},
{0.99662, 0.70075, 0.13444},
{0.9971, 0.69534, 0.13223},
{0.99755, 0.68993, 0.13006},
{0.998, 0.6845, 0.12783},
{0.99842, 0.67906, 0.12564},
{0.99883, 0.67361, 0.1234},
{0.99922, 0.66815, 0.12119},
{0.99959, 0.66267, 0.11904},
{0.99994, 0.65717, 0.11682},
{1, 0.65166, 0.11458},
{1, 0.64613, 0.11244},
{1, 0.64059, 0.11024},
{1, 0.63503, 0.10797},
{1, 0.62945, 0.1058},
{1, 0.62386, 0.1036},
{1, 0.61825, 0.10135},
{1, 0.61261, 0.099135},
{1, 0.60697, 0.096882},
{1, 0.6013, 0.094743},
{1, 0.59561, 0.092465},
{1, 0.58989, 0.090257},
{1, 0.58416, 0.088032},
{1, 0.5784, 0.085726},
{1, 0.57263, 0.083542},
{1, 0.56682, 0.081316},
{1, 0.56098, 0.079004},
{1, 0.55513, 0.076745},
{1, 0.54925, 0.07453},
{1, 0.54333, 0.072245},
{1, 0.53739, 0.070004},
{1, 0.53141, 0.067732},
{1, 0.52541, 0.065424},
{1, 0.51937, 0.06318},
{1, 0.5133, 0.06081},
{1, 0.50718, 0.058502},
{1, 0.50104, 0.056232},
{1, 0.49486, 0.053826},
{1, 0.48863, 0.051494},
{1, 0.48236, 0.049242},
{1, 0.47605, 0.046828},
{1, 0.46969, 0.044447},
{1, 0.46327, 0.042093},
{1, 0.45681, 0.039648},
{1, 0.45031, 0.037261},
{1, 0.44374, 0.034882},
{1, 0.43712, 0.032495},
{1, 0.43043, 0.030303},
{1, 0.42367, 0.02818},
{1, 0.41686, 0.026121},
{1, 0.40997, 0.024126},
{1, 0.40299, 0.022194},
{1, 0.39595, 0.020325},
{1, 0.38882, 0.018517},
{0.99994, 0.38159, 0.016771},
{0.99961, 0.37428, 0.015085},
{0.99927, 0.36685, 0.013457},
{0.99892, 0.35932, 0.011916},
{0.99855, 0.35167, 0.010169},
{0.99817, 0.3439, 0.0087437},
{0.99778, 0.336, 0.0073541},
{0.99738, 0.32796, 0.0060199},
{0.99696, 0.31976, 0.0047429},
{0.99653, 0.31138, 0.0035217},
{0.99609, 0.30282, 0.0023557},
{0.99563, 0.29407, 0.0012445},
{0.99517, 0.2851, 0.00018742},
{0.99469, 0.27591, 0},
{0.9942, 0.26642, 0},
{0.99369, 0.25664, 0},
{0.99318, 0.24652, 0},
{0.99265, 0.23605, 0},
{0.99211, 0.22511, 0},
{0.99155, 0.2137, 0},
{0.99099, 0.20169, 0},
{0.99041, 0.18903, 0},
};

namespace SynthEdit2
{
	ConnectorViewBase::ConnectorViewBase(Json::Value* pDatacontext, ViewBase* pParent) : ViewChild(pDatacontext, pParent)
	{
		auto& object_json = *datacontext;

		fromModuleH = object_json["fMod"].asInt();
		toModuleH = object_json["tMod"].asInt();

		fromModulePin = object_json["fPlg"].asInt();
		toModulePin = object_json["tPlg"].asInt();

		setSelected(object_json["selected"].asBool());
		highlightFlags = object_json["highlightFlags"].asInt();

#if defined( _DEBUG )
//		if(highlightFlags)
		{
//			_RPTN(0, "ConnectorViewBase::ConnectorViewBase highlightFlags =  %d\n", highlightFlags);
		}

		cancellation = object_json["Cancellation"].asFloat();
#endif
	}

	int32_t ConnectorViewBase::measure(GmpiDrawing::Size availableSize, GmpiDrawing::Size* returnDesiredSize)
	{
		// Measure/Arrange not really applicable to lines.
		returnDesiredSize->height = 10;
		returnDesiredSize->width = 10;

		if (type == CableType::StructureCable)
		{
			auto module1 = dynamic_cast<ModuleViewStruct*>(Presenter()->HandleToObject(fromModuleH));
			if (module1)
			{
				datatype = static_cast<char>(module1->getPinDatatype(fromModulePin));
				isGuiConnection = module1->getPinGuiType(fromModulePin);
			}
		}

		return gmpi::MP_OK;
	}

	int32_t ConnectorViewBase::arrange(GmpiDrawing::Rect finalRect)
	{
		// Measure/Arrange not applicable to lines. Determines it's own bounds during arrange phase.
		// don't overwright.
		// ViewChild::arrange(finalRect);

		OnModuleMoved();

		return gmpi::MP_OK;
	}

	void ConnectorViewBase::setHighlightFlags(int flags)
	{
//		_RPTN(0, "ConnectorViewBase::setHighlightFlags %d\n", flags);
		highlightFlags = flags;

		if (flags == 0)
		{
			parent->MoveToBack(this);
		}
		else
		{
			parent->MoveToFront(this);
		}

		const auto r = GetClipRect();
		parent->invalidateRect(&r);
	}

	void ConnectorViewBase::pickup(int pdraggingFromEnd, GmpiDrawing_API::MP1_POINT pMousePos)
	{
		if (pdraggingFromEnd == 0)
			from_ = pMousePos;
		else
			to_ = pMousePos;

		draggingFromEnd = pdraggingFromEnd;
		parent->setCapture(this);

		parent->invalidateRect(); // todo bounds only. !!!

		CalcBounds();
		parent->invalidateRect(&bounds_);
	}

	void ConnectorViewBase::OnModuleMoved()
	{
		auto module1 = Presenter()->HandleToObject(fromModuleH);
		auto module2 = Presenter()->HandleToObject(toModuleH);

		if (module1 == nullptr || module2 == nullptr)
			return;

		auto from = module1->getConnectionPoint(type, fromModulePin);
		auto to = module2->getConnectionPoint(type, toModulePin);

		from = module1->parent->MapPointToView(parent, from);
		to = module2->parent->MapPointToView(parent, to);

		if (from != from_ || to != to_)
		{
			from_ = from;
			to_ = to;
			CalcBounds();
		}
	}

	void PatchCableView::CreateGeometry()
	{
		GmpiDrawing::Factory factory(parent->GetDrawingFactory());
		strokeStyle = factory.CreateStrokeStyle(useDroop() ? CapStyle::Round : CapStyle::Flat);
		geometry = factory.CreatePathGeometry();
		auto sink = geometry.Open();

		//			_RPT4(_CRT_WARN, "Calc [%.3f,%.3f] -> [%.3f,%.3f]\n", from_.x, from_.y, to_.x, to_.y );

		// No curve when dragging.
		if (/*draggingFromEnd >= 0 ||*/ !useDroop())
		{
			// straight line.
			sink.BeginFigure(from_, FigureBegin::Hollow);
			sink.AddLine(to_);

			//			_RPT4(_CRT_WARN, "FRom[%.3f,%.3f] -> [%.3f,%.3f]\n", from_.x, from_.y, to_.x, to_.y );
		}
		else
		{
			sink.BeginFigure(from_, FigureBegin::Hollow);
			// sagging curve.
			GmpiDrawing::Size droopOffset(0, 20);
			sink.AddBezier(BezierSegment(from_ + droopOffset, to_ + droopOffset, to_));
		}

		sink.EndFigure(FigureEnd::Open);
		sink.Close();
	}

	void PatchCableView::CalcBounds()
	{
		OnVisibilityUpdate();
		CreateGeometry();

		auto oldBounds = bounds_;
		bounds_ = geometry.GetWidenedBounds((float)cableDiameter, strokeStyle);

		if (oldBounds != bounds_)
		{
			oldBounds.Union(bounds_);
			parent->ChildInvalidateRect(oldBounds);
		}
	}

	bool PatchCableView::isShown() // Indicates if cable should be drawn/clickable or not (because of 'Show on Parent' state).
	{
		if (draggingFromEnd >= 0)
			return true;

		auto module1 = Presenter()->HandleToObject(fromModuleH);
		auto module2 = Presenter()->HandleToObject(toModuleH);

		return module1 && module2 && module1->isShown() && module2->isShown();
	}

	void PatchCableView::OnVisibilityUpdate()
	{
		bool newIsShown = isShown();
		bool changed = newIsShown != isShownCached;
		isShownCached = newIsShown;
		if (changed)
		{
			auto r = GetClipRect();
			parent->invalidateRect(&r);
		}
	}

	GraphicsResourceCache<sharedGraphicResources_patchcables> drawingResourcesCachePatchCables;

	sharedGraphicResources_patchcables* PatchCableView::getDrawingResources(GmpiDrawing::Graphics& g)
	{
		if (!drawingResources)
		{
			drawingResources = drawingResourcesCachePatchCables.get(g);
		}

		return drawingResources.get();
	}


	void PatchCableView::OnRender(GmpiDrawing::Graphics& g)
	{
		if (geometry.isNull() || !isShownCached)
			return;

		auto resources = getDrawingResources(g);

//		g.FillRectangle(bounds_, g.CreateSolidColorBrush(Color::AliceBlue));

		const bool drawSolid = isHovered || draggingFromEnd >= 0;

		if (drawSolid)
		{
			g.DrawGeometry(geometry, resources->outlineBrush, 6.0f, strokeStyle);
		}

		// Colored fill.
		g.DrawGeometry(geometry, resources->brushes[colorIndex][1 - static_cast<int>(drawSolid)], 4.0f, strokeStyle);

		if (!drawSolid || draggingFromEnd >= 0)
			return;

		// draw white highlight on cable.
		Matrix3x2 originalTransform = g.GetTransform();

		auto adjustedTransform = Matrix3x2::Translation(-1, -1) * originalTransform;

		g.SetTransform(adjustedTransform);

		g.DrawGeometry(geometry, resources->highlightBrush, 1.0f, strokeStyle);

		g.SetTransform(originalTransform);
	}

	void PatchCableView::setHover(bool mouseIsOverMe)
	{
		if(isHovered != mouseIsOverMe)
		{
			isHovered = mouseIsOverMe;

			const auto r = GetClipRect();
			parent->invalidateRect(&r);
		}
	}

	// Mis-used as a global mouse tracker.
	bool PatchCableView::hitTest(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (!isShownCached)
			return false;

		// <ctrl> or <shift> click ignores patch cables (so patch point can spawn new cable)
		if ((flags & (gmpi_gui_api::GG_POINTER_KEY_CONTROL| gmpi_gui_api::GG_POINTER_KEY_SHIFT)) != 0)
			return false;

		if (!bounds_.ContainsPoint(point) || geometry.isNull()) // FM-Lab has null geometries for hidden patch cables.
			return false;

		// Hits ignored, except at line ends. So cables don't interfere with knobs.
		float distanceToendSquared = {};
		constexpr float lineHitWidth = 7.0f;

		{
			GmpiDrawing::Size delta = from_ - Point(point);
			distanceToendSquared = delta.width * delta.width + delta.height * delta.height;
			constexpr float hitRadiusSquared = mouseNearEndDist * mouseNearEndDist;
			if (distanceToendSquared > hitRadiusSquared)
			{
				delta = to_ - Point(point);
				distanceToendSquared = delta.width * delta.width + delta.height * delta.height;
				if (distanceToendSquared > hitRadiusSquared)
				{
					return false;
				}
			}
		}

		// Do proper hit testing.
		return geometry.StrokeContainsPoint(point, lineHitWidth, strokeStyle.Get());
	}

	PatchCableView::~PatchCableView()
	{
		parent->OnChildDeleted(this);
		parent->autoScrollStop();
	}

	int32_t PatchCableView::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (imCaptured()) //parent->getCapture()) // dragging?
		{
			parent->releaseCapture();
			parent->EndCableDrag(point, this);
			// I am now DELETED!!!
			return gmpi::MP_HANDLED;
		}

		if (!isHovered)
			return gmpi::MP_UNHANDLED;

		// Select Object.
		Presenter()->ObjectClicked(handle, gmpi::modifier_keys::getHeldKeys());

		if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) != 0)
		{
/* confusing
			// <SHIFT> deletes cable.
			if ((flags & gmpi_gui_api::GG_POINTER_KEY_SHIFT) != 0)
			{
				parent->RemoveCables(this);
				return gmpi::MP_HANDLED;
			}
*/

			// Left-click
			GmpiDrawing::Size delta = from_ - Point(point);
			const float lengthSquared = delta.width * delta.width + delta.height * delta.height;
			constexpr float hitRadiusSquared = mouseNearEndDist * mouseNearEndDist;
			const int hitEnd = (lengthSquared < hitRadiusSquared) ? 0 : 1;

			pickup(hitEnd, point);
		}
		else
		{
			// Right-click (context menu).
			GmpiGuiHosting::ContextItemsSink2 contextMenu;

			/*	correct, but confusing on a cable.

							// Cut, Copy, Paste etc.
							Presenter()->populateContextMenu(&contextMenu);
							contextMenu.AddSeparator();
			*/
			// Add custom entries e.g. "MIDI Learn".
			populateContextMenu(point, &contextMenu);

			GmpiDrawing::Rect r(point.x, point.y, point.x, point.y);

			GmpiGui::PopupMenu nativeMenu;
			parent->ChildCreatePlatformMenu(&r, nativeMenu.GetAddressOf());
			contextMenu.ShowMenuAsync2(nativeMenu, point);
		}
		return gmpi::MP_HANDLED; // Indicate menu already shown.
	}

	int32_t ConnectorViewBase::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (imCaptured()) //parent->getCapture())
		{
			if (draggingFromEnd == 0)
				from_ = point;
			else
				to_ = point;

			parent->OnCableMove(this);

			CalcBounds();

			return gmpi::MP_OK;
		}

		return gmpi::MP_UNHANDLED;
	}

	int32_t ConnectorViewBase::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (imCaptured()) //if (parent->getCapture())
		{
			// detect single clicks on pin, continue dragging.
			const float dragThreshold = 6;
			if (abs(from_.x - to_.x) < dragThreshold && abs(from_.y - to_.y) < dragThreshold)
			{
				return gmpi::MP_HANDLED;
			}

			parent->autoScrollStop();
			parent->releaseCapture();
			parent->EndCableDrag(point, this);
			// I am now DELETED!!!
		}

		return gmpi::MP_OK;
	}

	int32_t PatchCableView::populateContextMenu(GmpiDrawing_API::MP1_POINT point, GmpiGuiHosting::ContextItemsSink2* contextMenuItemsSink)
	{
		contextMenuItemsSink->currentCallback = [this](int32_t idx, GmpiDrawing_API::MP1_POINT point) { return onContextMenu(idx); };
		contextMenuItemsSink->AddItem("Remove Cable", 0);

		return gmpi::MP_OK;
	}

	int32_t ConnectorViewBase::onContextMenu(int32_t idx)
	{
		if (idx == 0)
		{
			//!!! Probably should just use selection and deletion like all else!!!
			// might need some special-case handling, since objects don't exist as docobs.
			parent->RemoveCables(this);
		}
		return gmpi::MP_OK;
	}

} // namespace

