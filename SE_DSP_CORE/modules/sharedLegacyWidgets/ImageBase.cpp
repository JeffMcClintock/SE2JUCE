#include "./ImageBase.h"
#include <algorithm>
#include "../shared/unicode_conversion.h"
#include "../shared/xplatform_modifier_keys.h"
#include "../shared/string_utilities.h"
#include "../shared/it_enum_list.h"
#include "../shared/ImageCache.h"
#include "../shared/xp_simd.h"

using namespace std;
using namespace gmpi;
using namespace gmpi_gui;
using namespace JmUnicodeConversions;
using namespace GmpiDrawing;
using namespace GmpiDrawing_API;

// handle pin updates.
void ImageBase::reDraw()
{
	invalidateRect();
}

void ImageBase::calcDrawAt()
{
	if(skinBitmap::calcDrawAt(getAnimationPos()))
		invalidateRect();
}

int32_t ImageBase::OnRender(GmpiDrawing_API::IMpDeviceContext* drawingContext)
{
	GmpiDrawing::Graphics dc2(drawingContext);

	renderBitmap(dc2, SizeU(0, 0));

	return gmpi::MP_OK;
}

void ImageBase::onSetFilename()
{
	string imageFile = WStringToUtf8(pinFilename);
	Load(imageFile);
}

void ImageBase::Load(const string& imageFile)
{
	if (MP_OK == skinBitmap::Load(getHost(), getGuiHost(), imageFile.c_str()))
	{
		onLoaded();

		getGuiHost()->invalidateMeasure();
		calcDrawAt();
		reDraw();
	}
}

int32_t ImageBase::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
{
	//	_RPT2(_CRT_WARN, "onPointerUp (%f,%f)\n", point.x, point.y);

	if( !getCapture() )
	{
		return gmpi::MP_UNHANDLED;
	}

	releaseCapture();
	pinMouseDown = false;
	pinMouseDownLegacy = false;

	return gmpi::MP_OK;
}

int32_t ImageBase::populateContextMenu(float x, float y, gmpi::IMpUnknown* contextMenuItemsSink)
{
	/*
	if (!hitTest(Point(x,y))) // hit test should be communicated already during On PointerDown via return code.
		return MP_UNHANDLED;
	*/

	gmpi::IMpContextItemSink* sink;
	contextMenuItemsSink->queryInterface(gmpi::MP_IID_CONTEXT_ITEMS_SINK, reinterpret_cast<void**>( &sink));

	it_enum_list itr(pinMenuItems);

	for (itr.First(); !itr.IsDone(); ++itr)
	{
		int32_t flags = 0;

		// Special commands (sub-menus)
		switch (itr.CurrentItem()->getType())
		{
			case enum_entry_type::Separator:
			case enum_entry_type::SubMenu:
				flags |= gmpi_gui::MP_PLATFORM_MENU_SEPARATOR;
				break;

			case enum_entry_type::SubMenuEnd:
			case enum_entry_type::Break:
				continue;

			default:
				break;
		}

		sink->AddItem(WStringToUtf8(itr.CurrentItem()->text).c_str(), itr.CurrentItem()->value, flags);
	}
	sink->release();
	return gmpi::MP_OK;
}

int32_t ImageBase::onContextMenu(int32_t selection)
{
	pinMenuSelection = selection; // send menu momentarily, then reset.
	pinMenuSelection = -1;

	return gmpi::MP_OK;
}

int32_t ImageBase::getToolTip(float x, float y, gmpi::IMpUnknown* returnToolTipString)
{
	IString* returnValue = 0;

	auto hint = getHint();

	if(hint.empty() || MP_OK != returnToolTipString->queryInterface(gmpi::MP_IID_RETURNSTRING, reinterpret_cast<void**>( &returnValue)) )
	{
		return gmpi::MP_NOSUPPORT;
	}

	auto utf8ToolTip = WStringToUtf8(hint);

	returnValue->setData(utf8ToolTip.data(), (int32_t)utf8ToolTip.size());

	return gmpi::MP_OK;
}

