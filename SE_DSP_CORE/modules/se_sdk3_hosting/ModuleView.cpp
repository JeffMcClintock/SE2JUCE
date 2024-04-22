#include "pch.h"
#include <vector>
#include <sstream>
#include <iomanip>

#include "ModuleView.h"
                                                                                       #include "ModuleViewStruct.h"
#include "ContainerView.h"
#include "ConnectorView.h"
#include "UgDatabase.h"
#include "modules/shared/xplatform.h"
#include "modules/shared/xplatform_modifier_keys.h"
#include "UgDatabase2.h"
#include "ProtectedFile.h"
#include "../SE_DSP_CORE/RawConversions.h"
#include "DragLine.h"
#include "SubViewPanel.h"
#if defined(SE_EDIT_SUPPORT)
#include "SubViewCadmium.h"
#include "../SynthEdit/cpu_accumulator.h"
#include "../SynthEdit/MfcDocPresenter.h"
#endif
#include "ResizeAdorner.h"
#include "modules/shared/xp_simd.h"
#include "modules/shared/GraphHelpers.h"
#include "../SE_DSP_CORE/IGuiHost2.h"
#include "modules/se_sdk3_hosting/PresenterCommands.h"

using namespace gmpi;
using namespace std;
using namespace GmpiDrawing_API;
using namespace GmpiDrawing;


namespace SynthEdit2
{
	ModuleView::ModuleView(const wchar_t* typeId, ViewBase* pParent, int handle) : ViewChild(pParent, handle)
		, recursionStopper_(0)
		, initialised_(false)
		, ignoreMouse(false)
	{
		moduleInfo = CModuleFactory::Instance()->GetById(typeId);
	}

	ModuleView::ModuleView(Json::Value* context, ViewBase* pParent) : ViewChild(context, pParent)
		, recursionStopper_(0)
		, initialised_(false)
		, ignoreMouse(false)
	{
		auto& module_element = *context;
		if (pParent->getViewType() == CF_PANEL_VIEW)
		{
			bounds_.left = module_element["pl"].asFloat();
			bounds_.top = module_element["pt"].asFloat();
			bounds_.right = module_element["pr"].asFloat();
			bounds_.bottom = module_element["pb"].asFloat();
		}
		else
		{
			bounds_.left = module_element["sl"].asFloat();
			bounds_.top = module_element["st"].asFloat();
			bounds_.right = module_element["sr"].asFloat();
			bounds_.bottom = module_element["sb"].asFloat();
		}
		setSelected(module_element["selected"].asBool() );

		static std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
		auto typeName = module_element["type"].asString();
		const auto typeId = convert.from_bytes(typeName);
		moduleInfo = CModuleFactory::Instance()->GetById(typeId);
        assert( moduleInfo != nullptr );
	}

	ModuleViewPanel::ModuleViewPanel(const wchar_t* typeId, ViewBase* pParent, int handle) : ModuleView(typeId, pParent, handle)
	{
		assert(moduleInfo != nullptr && moduleInfo->UniqueId() != L"Container");
		Build();
	}

	ModuleViewPanel::ModuleViewPanel(Json::Value* context, class ViewBase* pParent, std::map<int, class ModuleView*>& guiObjectMap) : ModuleView(context, pParent)
	{
		auto& module_element = *context;

		ignoreMouse = !module_element["ignoremouse"].empty();

		if(moduleInfo->UniqueId() == L"Container")
		{
			isRackModule_ = !module_element["isRackModule"].empty();

			const auto isCadmiumView = !module_element["isCadmiumView"].empty();
			if (isCadmiumView)
			{
				BuildContainerCadmium(context, guiObjectMap);
			}
			else
			{
				BuildContainer(context, guiObjectMap);
			}
		}
		else
		{
			Build();
		}
	}

	void ModuleView::BuildContainer(Json::Value* context, std::map<int, class ModuleView*>& guiObjectMap)
	{
		assert(moduleInfo->UniqueId() == L"Container");

		auto subView = new SubView(parent->getViewType());
		gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> object;
		object.Attach(static_cast<gmpi::IMpUserInterface2B*>(subView));
		assert(object != nullptr);
		{
			auto r = object->queryInterface(gmpi::MP_IID_GUI_PLUGIN2, pluginParameters.asIMpUnknownPtr());
			r = object->queryInterface(gmpi::MP_IID_GUI_PLUGIN2B, pluginParameters2B.asIMpUnknownPtr());
			r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI3, pluginGraphics3.asIMpUnknownPtr());
			r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI4, pluginGraphics4.asIMpUnknownPtr());
			r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI2, pluginGraphics2.asIMpUnknownPtr());
			r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI, pluginGraphics.asIMpUnknownPtr());

			pluginParameters->setHost(static_cast<gmpi::IMpUserInterfaceHost2*>(this));
		}

		auto subPresenter = Presenter()->CreateSubPresenter(handle);
		subView->Init(subPresenter);
		subView->BuildModules(context, guiObjectMap);

		if (Presenter()->GetPatchManager() != subPresenter->GetPatchManager())
		{
			subView->BuildPatchCableNotifier(guiObjectMap);
		}
	}

	void ModuleView::BuildContainerCadmium(Json::Value* context, std::map<int, class ModuleView*>& guiObjectMap)
	{
#if defined(SE_EDIT_SUPPORT)
		assert(moduleInfo->UniqueId() == L"Container");

		gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> object;
		object.Attach(static_cast<gmpi::IMpUserInterface2B*>(new SubViewCadmium(parent->getViewType())));
		assert(object != nullptr);
		{
			auto r = object->queryInterface(gmpi::MP_IID_GUI_PLUGIN2, pluginParameters.asIMpUnknownPtr());
			r = object->queryInterface(gmpi::MP_IID_GUI_PLUGIN2B, pluginParameters2B.asIMpUnknownPtr());
			r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI, pluginGraphics.asIMpUnknownPtr());
			r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI2, pluginGraphics2.asIMpUnknownPtr());
			r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI3, pluginGraphics3.asIMpUnknownPtr());
			r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI4, pluginGraphics4.asIMpUnknownPtr());

			pluginParameters->setHost(static_cast<gmpi::IMpUserInterfaceHost2*>(this));
		}

		auto subView = dynamic_cast<SubViewCadmium*>(pluginGraphics.get());
		auto subPresenter = Presenter()->CreateSubPresenter(handle);
		subView->Init(subPresenter);

#ifdef _DEBUG
		{
			Json::StyledWriter writer;
			auto factoryXml = writer.write(*context);
			auto s = factoryXml;
		}
#endif

		subView->BuildView(context); // , guiObjectMap);

		if (Presenter()->GetPatchManager() != subPresenter->GetPatchManager())
		{
			subView->BuildPatchCableNotifier(guiObjectMap);
		}
#endif
	}

	void ModuleView::Build()
	{
		auto& mi = moduleInfo;

		gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> object;
		object.Attach(mi->Build(MP_SUB_TYPE_GUI2, true));

		if (!object)
		{
			if (mi->getWindowType() == MP_WINDOW_TYPE_NONE) // can't support legacy graphics, but can support invisible legacy sub-controls.
			{
				object.Attach(mi->Build(MP_SUB_TYPE_GUI, true));
			}
		}

		if (!object)
		{
#if defined( SE_EDIT_SUPPORT ) && defined( _DEBUG)
			if (mi->hasGuiModule())
			{
				_RPTN(0, "FAILED TO LOAD: %S\n", mi->UniqueId().c_str());
			}
			mi->load_failed_gui = true;
#endif

			return;
		}

		auto r = object->queryInterface(gmpi::MP_IID_GUI_PLUGIN2, pluginParameters.asIMpUnknownPtr());
		r = object->queryInterface(gmpi::MP_IID_GUI_PLUGIN2B, pluginParameters2B.asIMpUnknownPtr());
		r = object->queryInterface(gmpi::MP_IID_GUI_PLUGIN, pluginParametersLegacy.asIMpUnknownPtr());
		r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI, pluginGraphics.asIMpUnknownPtr());
		r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI2, pluginGraphics2.asIMpUnknownPtr());
		r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI3, pluginGraphics3.asIMpUnknownPtr());
		r = object->queryInterface(gmpi_gui_api::SE_IID_GRAPHICS_MPGUI4, pluginGraphics4.asIMpUnknownPtr());

		if (!pluginParameters.isNull())
		{
			pluginParameters->setHost(static_cast<gmpi::IMpUserInterfaceHost2*>(this));
		}
		else
		{
			gmpi_sdk::mp_shared_ptr<gmpi::IMpLegacyInitialization> legacyInitMethod;
			r = object->queryInterface(gmpi::MP_IID_LEGACY_INITIALIZATION, legacyInitMethod.asIMpUnknownPtr());
			if (!legacyInitMethod.isNull())
			{
				legacyInitMethod->setHost(static_cast<IMpUserInterfaceHost*>(this));
			}
			else
			{
				// last gasp
				// CAN'T/SHOULDN"T DYNAMIC CAST INTO DLL!!!! (but we have to support old 3rd-party modules)
				auto oldSchool = dynamic_cast<IoldSchoolInitialisation*>(object.get());
				if (oldSchool)
				{
					oldSchool->setHost(static_cast<IMpUserInterfaceHost*>(this));
				}
			}
		}
	}
	
	void ModuleView::CreateGraphicsResources()
	{
	}

	int32_t ModuleViewPanel::measure(GmpiDrawing::Size availableSize, GmpiDrawing::Size* returnDesiredSize)
	{
		if (pluginGraphics)
		{
			return pluginGraphics->measure(availableSize, returnDesiredSize);
		}
		else
		{
			*returnDesiredSize = availableSize;
		}
		return gmpi::MP_OK;
	}

	GmpiDrawing::Rect ModuleViewPanel::GetClipRect()
	{
		auto clipArea = ModuleView::GetClipRect();

		if (pluginGraphics4)
		{
			GmpiDrawing::Rect clientClipArea{};
			pluginGraphics4->getClipArea(&clientClipArea);
			clientClipArea.Offset(bounds_.left + pluginGraphicsPos.left, bounds_.top + pluginGraphicsPos.top);
			clipArea.Union(clientClipArea);
		}

		return clipArea;
	}

	int32_t ModuleViewPanel::arrange(GmpiDrawing::Rect finalRect)
	{
		bounds_ = finalRect; // TODO put in base class.

		if (pluginGraphics)
		{
			pluginGraphicsPos = GmpiDrawing::Rect(0, 0, finalRect.right - finalRect.left, finalRect.bottom - finalRect.top);
			pluginGraphics->arrange(pluginGraphicsPos);
		}
		return gmpi::MP_OK;
	}

	int32_t ModuleView::GetDrawingFactory(GmpiDrawing_API::IMpFactory** returnFactory)
	{
		*returnFactory = parent->GetDrawingFactory();
		return gmpi::MP_OK;
	}

	GmpiDrawing::Factory ModuleView::DrawingFactory()
	{
		return GmpiDrawing::Factory(parent->GetDrawingFactory());
	}

	void ModuleViewPanel::OnRender(Graphics& g)
	{
		if (pluginGraphics == nullptr)
		{
			return;
		}

#if 0 // debug layout and clip rects
		g.FillRectangle(GetClipRect(), g.CreateSolidColorBrush(Color::FromArgb(0x200000ff)));
		g.FillRectangle(getLayoutRect(), g.CreateSolidColorBrush(Color::FromArgb(0x2000ff00)));
#endif
/*
		// Transform to module-relative.
		const auto originalTransform = g.GetTransform();
		auto adjustedTransform = Matrix3x2::Translation(bounds_.left , bounds_.top) * originalTransform;
		g.SetTransform(adjustedTransform);
*/
		// Render.
		pluginGraphics->OnRender(g.Get());

#if 0 //def _DEBUG
		// Alignment marks.
		{
			auto brsh = g.CreateSolidColorBrush(Color::Red);
			g.FillRectangle(Rect(0, 0, 1, 1), brsh);
			g.FillRectangle(Rect(64, 64, 65, 65), brsh);
		}
#endif
		// Transform back.
//		g.SetTransform(originalTransform);
	}

	GmpiDrawing::Point ModuleView::getConnectionPoint(CableType cableType, int pinIndex)
	{
		assert(cableType == CableType::PatchCable);

		for (auto& patchpoint : getModuleType()->patchPoints)
		{
			if (patchpoint.dspPin == pinIndex)
			{
				auto modulePosition = getLayoutRect();
//				return GmpiDrawing::Point(patchpoint.x + modulePosition.left, patchpoint.y + modulePosition.top);
				return GmpiDrawing::Point(patchpoint.x + modulePosition.left + pluginGraphicsPos.left, patchpoint.y + modulePosition.top + pluginGraphicsPos.top);
				break;
			}
		}

		return GmpiDrawing::Point();
	}

	int32_t ModuleView::createPlatformTextEdit(GmpiDrawing_API::MP1_RECT* rect, gmpi_gui::IMpPlatformText** returnTextEdit)
	{
		GmpiDrawing::Rect r(*rect);
		r.Offset(bounds_.left + pluginGraphicsPos.left, bounds_.top + pluginGraphicsPos.top);
		return parent->ChildCreatePlatformTextEdit(&r, returnTextEdit);
	}

	int32_t ModuleView::createPlatformMenu(GmpiDrawing_API::MP1_RECT* rect, gmpi_gui::IMpPlatformMenu** returnMenu)
	{
		GmpiDrawing::Rect r(*rect);
		r.Offset(bounds_.left + pluginGraphicsPos.left, bounds_.top + pluginGraphicsPos.top);
		return parent->ChildCreatePlatformMenu(&r, returnMenu);
	}

	int32_t ModuleView::createFileDialog(int32_t dialogType, gmpi_gui::IMpFileDialog** returnFileDialog)
	{
		return parent->getGuiHost()->createFileDialog(dialogType, returnFileDialog);
	}
	int32_t ModuleView::createOkCancelDialog(int32_t dialogType, gmpi_gui::IMpOkCancelDialog** returnFileDialog)
	{
		return parent->getGuiHost()->createOkCancelDialog(dialogType, returnFileDialog);
	}

	void ModuleView::invalidateMeasure()
	{
		if (!initialised_)
			return;

//		parent->getGuiHost()->invalidateMeasure();
#if defined(SE_EDIT_SUPPORT)
		if (auto presentor = dynamic_cast<MfcDocPresenterBase*>(parent->Presenter()); presentor)
		{
			GmpiDrawing::Size current{ bounds_.getWidth(), bounds_ .getHeight()};
			GmpiDrawing::Size desired{};
			measure(current, &desired);

			if (current != desired)
			{
				presentor->DirtyView();
			}
			else
			{
				// if size remains the same, avoid expensive recreation of the entire view
				// just redraw.
				invalidateRect(nullptr);
			}
		}
#endif
	}

	void ModuleView::invalidateRect(const GmpiDrawing_API::MP1_RECT* invalidRect)
	{ 
		if (invalidRect)
		{
			GmpiDrawing::Rect r(*invalidRect);
			r.Offset(bounds_.left + pluginGraphicsPos.left, bounds_.top + pluginGraphicsPos.top);
			parent->ChildInvalidateRect(r);
		}
		else
			parent->ChildInvalidateRect(bounds_);
	}

	int32_t ModuleView::setCapture()
	{
		mouseCaptured = true;
		return parent->setCapture(this); // getGuiHost()->setCapture();
	}

	int32_t ModuleView::getCapture(int32_t& returnValue)
	{
		returnValue = mouseCaptured;
		//	returnValue = WpfHost->IsMouseCaptured;
		return gmpi::MP_OK;
	}

	int32_t ModuleView::releaseCapture()
	{
		mouseCaptured = false;
		return parent->releaseCapture();
	}

	void ModuleView::OnMoved(GmpiDrawing::Rect& newRect)
	{
		GmpiDrawing::Rect invalidRect(GetClipRect());

		// measure/arrange if nesc.
		GmpiDrawing::Size origSize(bounds_.getWidth(), bounds_.getHeight());
		GmpiDrawing::Size newSize(newRect.getWidth(), newRect.getHeight());
		if (newSize != origSize)
		{
			// Note, due to font width diferences, this may result in different size/layout than original GDI graphics. e..g knobs shifting.
			GmpiDrawing::Size desired(newSize);
			measure(newSize, &desired);
			arrange(GmpiDrawing::Rect(newRect.left, newRect.top, newRect.left + desired.width, newRect.top + desired.height));
		}
		else
		{
			arrange(newRect);
		}

		invalidRect.Union(GetClipRect());

		parent->ChildInvalidateRect(invalidRect);

		// update any parent subview
		parent->OnChildMoved();
	}

	int32_t ModuleView::getHandle(int32_t& returnValue)
	{
		returnValue = handle;
		return gmpi::MP_OK;
	}
	
	int32_t ModuleView::sendMessageToAudio(int32_t id, int32_t size, const void* messageData)
	{
//		return parent->getGuiHost2()->sendSdkMessageToAudio( handle,  id,  size,  messageData);
		return Presenter()->GetPatchManager()->sendSdkMessageToAudio(handle, id, size, messageData);
	}

	int32_t ModuleView::RegisterResourceUri(const char* resourceName, const char* resourceType, gmpi::IString* returnString)
	{
		return GmpiResourceManager::Instance()->RegisterResourceUri(handle, parent->getSkinName(), resourceName, resourceType, returnString);
	}

	int32_t ModuleView::FindResourceU(const char* resourceName, const char* resourceType, gmpi::IString* returnString)
	{
		return GmpiResourceManager::Instance()->FindResourceU(handle, parent->getSkinName(), resourceName, resourceType, returnString);
	}

	int32_t ModuleView::LoadPresetFile_DEPRECATED(const char* presetFilePath)
	{
//		Presenter()->LoadPresetFile(presetFilePath);
		return gmpi::MP_OK;
	}

	int32_t ModuleView::setPin(ModuleView* fromModule, int32_t fromPinId, int32_t pinId, int32_t voice, int32_t size, const void* data)
	{
		if (recursionStopper_ < 10)
		{
			++recursionStopper_;
#if 0
			auto c = (const char*)data;
			_RPT3(_CRT_WARN, "ModuleView[%S]::setPin: pin=%d, voice=%d val=", moduleInfo->GetName().c_str(), pinId, voice);
			for (int i = 0; i < size; ++i)
			{
				_RPT1(_CRT_WARN, "%02X", 0xFF & (int)c[i]);
			}
			_RPT0(_CRT_WARN, "\n");
#endif

			// Notify my module.
			if (pluginParameters)
			{
				pluginParameters->setPin(pinId, voice, size, data);
				if (pluginParameters2B)
				{
					pluginParameters2B->notifyPin(pinId, voice);
				}
			}
			else
			{
				if (pluginParametersLegacy)
				{
					pluginParametersLegacy->setPin(pinId, voice, size, (void*) data);
					pluginParametersLegacy->notifyPin(pinId, voice);
				}
			}

			// For outputs, send out notification to other connections (not the incoming one though).
			auto it = connections_.find(pinId);
			while (it != connections_.end() && (*it).first == pinId)
			{
				auto& connection = (*it).second;
				if (connection.otherModule_ != fromModule || connection.otherModulePinIndex_ != fromPinId)
				{
					connection.otherModule_->setPin(this, pinId, connection.otherModulePinIndex_, voice, size, data);
				}
				it++;
			}
			if (!initialised_)
			{
				//_RPT2(0, "m:%d alreadySentDataPins_ <- %d\n", handle, pinId);
				alreadySentDataPins_.push_back(pinId);
			}

			--recursionStopper_;
		}
		return gmpi::MP_OK;
	}

	// works on structure, not on panel, panel sub-controls don't know if they are muted or not.
	bool ModuleView::isPinConnectionActive(int pinIndex)const
	{
		for (auto it = connections_.find(pinIndex) ; it != connections_.end() && (*it).first == pinIndex ; ++it)
		{
			auto& connection = (*it).second;
			if(!connection.otherModule_->isMuted())
			{
				return true;
			}
		}

		return false;
	}

	std::string ModuleView::getToolTip(GmpiDrawing_API::MP1_POINT point)
	{
		if (pluginGraphics2)
		{
			auto local = PointToPlugin(point);

			gmpi_sdk::MpString s;
			if( MP_OK == pluginGraphics2->getToolTip(local, &s) )
			{
				return s.str();
			}
			return string();
		}

		return string();
	}

	void ModuleView::receiveMessageFromAudio(void* msg)
	{
		struct DspMsgInfo
		{
			int id;
			int size;
			void* data;
		};
		const DspMsgInfo* nfo = (DspMsgInfo*)msg;

		if (pluginParameters)
		{
			pluginParameters->receiveMessageFromAudio(nfo->id, nfo->size, nfo->data);
		}
		if (pluginParametersLegacy)
		{
			pluginParametersLegacy->receiveMessageFromAudio(nfo->id, nfo->size, nfo->data);
		}
	}
	
	int32_t ModuleView::populateContextMenu(GmpiDrawing_API::MP1_POINT point, GmpiGuiHosting::ContextItemsSink2* contextMenuItemsSink)
	{
		if (pluginParameters)
		{
			auto local = PointToPlugin(point);

			contextMenuItemsSink->currentCallback = [this](int32_t idx, GmpiDrawing_API::MP1_POINT point) { return onContextMenu(idx); };

			return contextMenuItemsSink->Populate(pluginParameters, local);
		}
		else
		{
			return gmpi::MP_UNHANDLED;
		}
	}

	int32_t ModuleView::onContextMenu(int32_t idx)
	{
		if (pluginParameters)
		{
			return pluginParameters->onContextMenu(idx);
		}

		return gmpi::MP_UNHANDLED;
	}

	bool ModuleViewPanel::hitTest(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (!ModuleView::hitTest(flags, point))
			return false;

		if (!pluginGraphics2)
			return false;

		auto local = PointToPlugin(point);

		gmpi_sdk::mp_shared_ptr<ISubView> subView;
		if (pluginGraphics)
		{
			pluginGraphics->queryInterface(SE_IID_SUBVIEW, subView.asIMpUnknownPtr());
		}

		if (subView)
		{
			return subView->hitTest(flags, &local);
		}
		else
		{
			if (pluginGraphics3)
			{
				// TODO!! use editEnabled to somehow ignore click on knob titles when no editing.
				// e.g. List entry and knobs on PD303 have blank area at top that blocks anything above from being clicked.
				// either add a flag, or a host-control ("is editor") to allow plugin to know if it's in edit mode.
				return pluginGraphics3->hitTest2(flags, local) == gmpi::MP_OK;
			}

			return pluginGraphics2->hitTest(local) == gmpi::MP_OK;
		}
	}

	bool ModuleView::hitTest(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (isVisable())
		{
			auto bounds = getLayoutRect();
			if (bounds.ContainsPoint(point))
			{
				return true;
			}
		}
		return false;
	}

	void ModuleView::setHover(bool mouseIsOverMe)
	{
		if (pluginGraphics3) // TODO: implement a static dummy pluginGraphics2 to avoid all the null tests.
		{
			pluginGraphics3->setHover(mouseIsOverMe);
		}
	}

	std::vector<patchpoint_description>* ModuleView::getPatchPoints()
	{
		return &moduleInfo->patchPoints;
	}

	int32_t ModuleView::onPointerDown(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (ignoreMouse) // background image.
			return gmpi::MP_UNHANDLED;

		// pluginGraphics2 supports hit-testing, else need to call onPointerDown() to determin hit on client.
		if (pluginGraphics2 || parent->getViewType() == CF_STRUCTURE_VIEW) // Since Structure view is "behind" client, it always gets selected.
		{
			Presenter()->ObjectClicked(handle, gmpi::modifier_keys::getHeldKeys());
		}

		GmpiDrawing::Point moduleLocal(point);
		moduleLocal.x -= bounds_.left;
		moduleLocal.y -= bounds_.top;

		bool clientHit = false;

		// Mouse over client area?
		if (pluginGraphics && pluginGraphicsPos.ContainsPoint(moduleLocal))
		{
			Point local = PointToPlugin(point);

			// Patch-Points. Initiate drag on left-click.
			if ((flags & gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON) != 0)
			{
				auto patchPoints = getPatchPoints();
				if (patchPoints != nullptr)
				{
					for (auto& p : *patchPoints)
					{
						float distanceSquared = (local.x - p.x) * (local.x - p.x) + (local.y - p.y) * (local.y - p.y);
						if (distanceSquared <= p.radius * p.radius)
						{
							Point dragStartPoint = Point(static_cast<float>(p.x), static_cast<float>(p.y)) + Size(bounds_.left + pluginGraphicsPos.left, bounds_.top + pluginGraphicsPos.top);
							parent->StartCableDrag(this, p.dspPin, dragStartPoint, 0 != (flags & gmpi_gui_api::GG_POINTER_KEY_ALT));
							return gmpi::MP_HANDLED;
						}
					}
				}
			}

			int32_t res = MP_UNHANDLED;

			if (pluginGraphics2) // Client supports proper hit testing.
			{
				// In Panel-view, we can assume mouse already hit-tested against client. On Structure-view it could be a click on client OR on pins.
				clientHit = parent->getViewType() == CF_PANEL_VIEW;

				if (!clientHit)
				{
					gmpi_sdk::mp_shared_ptr<ISubView> subView;
					if (pluginGraphics)
					{
						pluginGraphics->queryInterface(SE_IID_SUBVIEW, subView.asIMpUnknownPtr());
					}

					if (subView)
					{
						clientHit = subView->hitTest(flags, &local);
					}
					else
					{
						clientHit = gmpi::MP_OK == pluginGraphics2->hitTest(local);
					}
				}

				// In Panel-view, we can assume mouse already hit-tested against client. On Structure-view it could be a click on client OR on pins.
				// clientHit = parent->getViewType() == CF_PANEL_VIEW || MP_OK == pluginGraphics2->hitTest(local);

				if(clientHit)
					res = pluginGraphics->onPointerDown(flags, local);// older modules indicate hit via return value.
			}
			else
			{
				// Old system: Hit-testing infered from onPointerDown() return value;
				res = pluginGraphics->onPointerDown(flags, local);// older modules indicate hit via return value.

				clientHit = (res == gmpi::MP_OK || res == gmpi::MP_HANDLED);

				if (clientHit && parent->getViewType() == CF_PANEL_VIEW)
				{
					Presenter()->ObjectClicked(handle, gmpi::modifier_keys::getHeldKeys());
				}
			}

			if (MP_HANDLED == res) // Client indicates no further processing needed.
				return MP_HANDLED;
		}

		// Right-click (context menu).
		if ((flags & gmpi_gui_api::GG_POINTER_FLAG_SECONDBUTTON) != 0)
		{
			GmpiGuiHosting::ContextItemsSink2 contextMenu;

			// Cut, Copy, Paste etc.
			Presenter()->populateContextMenu(&contextMenu, handle);
			contextMenu.AddSeparator();

			// Add custom entries e.g. "MIDI Learn".
			if (clientHit)
				[[maybe_unused]] auto result = populateContextMenu(point, &contextMenu);

			contextMenu.ShowMenuAsync(this, PointToPlugin(point));

			return gmpi::MP_HANDLED; // Indicate menu already shown.
		}

//?		return parent->getViewType() == CF_STRUCTURE_VIEW || clientHit ? gmpi::MP_OK : gmpi::MP_UNHANDLED;
		return clientHit ? gmpi::MP_OK : gmpi::MP_UNHANDLED;
	}

	void ModuleView::OnCableDrag(ConnectorViewBase* dragline, GmpiDrawing::Point dragPoint, float& bestDistanceSquared, IViewChild*& bestModule, int& bestPinIndex)
	{
		if (dragline->type != CableType::PatchCable)
			return;

		auto point = dragPoint;
		if (hitTest(0, point))
		{
			GmpiDrawing::Point local(point);
			local -= OffsetToClient();

			gmpi_sdk::mp_shared_ptr<ISubView> subView;
			if (pluginGraphics)
			{
				pluginGraphics->queryInterface(SE_IID_SUBVIEW, subView.asIMpUnknownPtr());
			}

			if (subView)
			{
				subView->OnCableDrag(dragline, local, bestDistanceSquared, bestModule, bestPinIndex);
			}
			else
			{
				std::vector<patchpoint_description>* patchPoints;
				patchPoints = &getModuleType()->patchPoints;

				for (auto& patchpoint : *patchPoints)
				{
					float distanceSquared = (local.x - patchpoint.x) * (local.x - patchpoint.x) + (local.y - patchpoint.y) * (local.y - patchpoint.y);
					if (distanceSquared < bestDistanceSquared && (dragline->fixedEndModule() != handle || dragline->fixedEndPin() != patchpoint.dspPin))
					{
						if (Presenter()->CanConnect(dragline->type, dragline->fixedEndModule(), dragline->fixedEndPin(), handle, patchpoint.dspPin))
						{
							bestDistanceSquared = distanceSquared;
							bestModule = this;
							bestPinIndex = patchpoint.dspPin;
						}
					}
				}
			}
		}
	}
	bool ModuleViewPanel::EndCableDrag(GmpiDrawing_API::MP1_POINT point, ConnectorViewBase* dragline)
	{
		if (!hitTest(0, point))
			return false;

		GmpiDrawing::Point local(point);
		auto modulePosition = getLayoutRect();
		local.x -= modulePosition.left;
		local.y -= modulePosition.top;

		for (auto& patchpoint : getModuleType()->patchPoints)
		{
			float distanceSquared = (local.x - patchpoint.x) * (local.x - patchpoint.x) + (local.y - patchpoint.y) * (local.y - patchpoint.y);
			if (distanceSquared <= patchpoint.radius * patchpoint.radius)
			{
				auto toPin = patchpoint.dspPin;

				int colorIndex = 0;
				if (auto patchcable = dynamic_cast<PatchCableView*>(dragline); patchcable)
				{
					colorIndex = patchcable->getColorIndex();
				}

				Presenter()->AddPatchCable(dragline->fromModuleHandle(), dragline->fromPin(), getModuleHandle(), toPin, colorIndex);
				return true;
				break;
			}
		}

		return false;
	}

	bool ModuleViewPanel::isShown()
	{
		return parent->isShown();
	}

	bool ModuleViewPanel::hitTestR(int32_t flags, GmpiDrawing_API::MP1_RECT selectionRect)
	{
		if (!isVisable())
			return false;

		if(!ModuleView::hitTestR(flags, selectionRect))
			return false;

		gmpi_sdk::mp_shared_ptr<ISubView> subView;
		if (pluginGraphics)
		{
			pluginGraphics->queryInterface(SE_IID_SUBVIEW, subView.asIMpUnknownPtr());
		}

		// ignore hidden panels when selecting by lasso
		if (subView)
		{
			return subView->isVisible();
		}

		return true;
	}

	bool ModuleViewPanel::isDraggable(bool editEnabled)
	{
		// default is that anything can be dragged in the editor.
		return editEnabled || isRackModule();
	}

	int32_t ModuleView::onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (pluginGraphics)
		{
			auto local = PointToPlugin(point);
			return pluginGraphics->onPointerMove(flags, local);
		}
		else
		{
			return gmpi::MP_UNHANDLED;
		}
	}

	int32_t ModuleView::onPointerUp(int32_t flags, GmpiDrawing_API::MP1_POINT point)
	{
		if (pluginGraphics)
		{
			auto local = PointToPlugin(point);
			return pluginGraphics->onPointerUp(flags, local);
		}
		else
		{
			return gmpi::MP_UNHANDLED;
		}
	}

	int32_t ModuleView::onMouseWheel(int32_t flags, int32_t delta, GmpiDrawing_API::MP1_POINT point)
	{
		if (!pluginGraphics3)
			return gmpi::MP_UNHANDLED;

		const auto local = PointToPlugin(point);
		return pluginGraphics3->onMouseWheel(flags, delta, local);
	}

	// legacy crap forwarded to new members..
	int32_t ModuleView::pinTransmit(int32_t pinId, int32_t size, /*const*/ void* data, int32_t voice)
	{
		return pinTransmit(pinId, size, (const void*) data, voice);
	}
	int32_t ModuleView::sendMessageToAudio(int32_t id, int32_t size, /*const*/ void* messageData)
	{
		return sendMessageToAudio(id, size, (const void*) messageData);
	}

	// These legacy functions are supported asthey are simple.
	int32_t ModuleView::getHostId(int32_t maxChars, wchar_t* returnString)
	{
		const wchar_t* hostName = L"SynthEdit UWP";
#if defined( _MSC_VER )
		wcscpy_s(returnString, maxChars, hostName);
#else
		wcscpy(returnString, hostName);
#endif
		return gmpi::MP_OK;
	}

	int32_t ModuleView::getHostVersion(int32_t& returnValue)
	{
		returnValue = 1000;
		return gmpi::MP_OK;
	}

	// We don't support these legacy functions from older SDK.
	int32_t ModuleView::setIdleTimer(int32_t active)
	{
		return gmpi::MP_FAIL;
	}
	int32_t ModuleView::resolveFilename(const wchar_t* shortFilename, int32_t maxChars, wchar_t* returnFullFilename)
	{
//		return parent->getGuiHost2()->resolveFilename(shortFilename, maxChars, returnFullFilename);
		return Presenter()->GetPatchManager()->resolveFilename(shortFilename, maxChars, returnFullFilename);
	}
	int32_t ModuleView::addContextMenuItem( /*const*/ wchar_t* menuText, int32_t index, int32_t flags)
	{
		return gmpi::MP_FAIL;
	}
	int32_t ModuleView::getPinCount(int32_t& returnCount)
	{
		if (totalPins_ > -1)
		{
			returnCount = totalPins_;
			return gmpi::MP_OK;
		}

		return gmpi::MP_FAIL;
	}

	int32_t ModuleView::openProtectedFile(const wchar_t* shortFilename, class IProtectedFile **file)
	{
		*file = nullptr; // If anything fails, return null.

		// new way, disk files only. (and no MFC).
//		std::wstring filename(shortFilename);
//		std::wstring l_filename = AudioMaster()->getShell()->ResolveFilename(filename, L"");

		const int maxChars = 512;
		wchar_t temp[maxChars];
		resolveFilename(shortFilename, maxChars, temp);

		wstring l_filename(temp);
		auto filename_utf8 = WStringToUtf8(l_filename);

		auto pf2 = ProtectedFile2::FromUri(filename_utf8.c_str());

		if (pf2)
		{
			*file = pf2;
			return gmpi::MP_OK;
		}

		return gmpi::MP_FAIL;
	}

	std::unique_ptr<IViewChild> ModuleViewPanel::createAdorner(ViewBase* pParent)
	{
		return std::make_unique<ResizeAdorner>(pParent, this);
	}


	} // namespace.

#if 0 //def SE_TAR GET_PURE_UWP
	void ModuleView::OnPointerPressed(float x, float y, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e)
	{
		float childLeft = position_.left + ((position_.right - position_.left) - pluginGraphicsSize.width) * 0.5f;

		GmpiDrawing::Point gmpiPoint(x - childLeft, y - (position_.bottom - pluginGraphicsSize.height));

		Windows::UI::Xaml::Input::Pointer^ ptr = e->Pointer;

		int32_t flags = 0;

		if (ptr->PointerDeviceType == Windows::Devices::Input::PointerDeviceType::Mouse)
		{
			flags |= gmpi_gui_api::GG_POINTER_FLAG_INCONTACT;
			// To get mouse state, we need extended pointer details.
			// We get the pointer info through the getCurrentPoint method
			// of the event argument. 
			Windows::UI::Input::PointerPoint^ ptrPt = e->GetCurrentPoint(nullptr);
			if (ptrPt->Properties->IsLeftButtonPressed)
			{
				flags |= gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON | gmpi_gui_api::GG_POINTER_FLAG_PRIMARY;
				//				eventLog.Text += "\nLeft button: " + ptrPt.PointerId;
			}
			if (ptrPt->Properties->IsMiddleButtonPressed)
			{
				//				eventLog.Text += "\nWheel button: " + ptrPt.PointerId;
				flags |= gmpi_gui_api::GG_POINTER_FLAG_THIRDBUTTON;
			}
			if (ptrPt->Properties->IsRightButtonPressed)
			{
				flags |= gmpi_gui_api::GG_POINTER_FLAG_SECONDBUTTON;
			}
		}
		else
		{
			// pen or touch.

			// Guess.
			flags |= gmpi_gui_api::GG_POINTER_FLAG_INCONTACT;
			flags |= gmpi_gui_api::GG_POINTER_FLAG_FIRSTBUTTON | gmpi_gui_api::GG_POINTER_FLAG_PRIMARY;
		}

		pluginGraphics->onPointerDown(flags, gmpiPoint);

		/* todo
		if (mouseCaptured)
		{
		canvas->CapturePointer(ptr);
		}
		*/
	}

	void ModuleView::OnPointerMoved(float x, float y, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e)
	{
		float childLeft = position_.left + ((position_.right - position_.left) - pluginGraphicsSize.width) * 0.5f;
		GmpiDrawing::Point gmpiPoint(x - childLeft, y - (position_.bottom - pluginGraphicsSize.height));
		int32_t flags = 0;
		pluginGraphics->onPointerMove(flags, gmpiPoint);
	}

	void ModuleView::OnPointerReleased(float x, float y, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^e)
	{
		float childLeft = position_.left + ((position_.right - position_.left) - pluginGraphicsSize.width) * 0.5f;
		GmpiDrawing::Point gmpiPoint(x - childLeft, y - (position_.bottom - pluginGraphicsSize.height));
		int32_t flags = 0;
		pluginGraphics->onPointerUp(flags, gmpiPoint);
	}
#endif
