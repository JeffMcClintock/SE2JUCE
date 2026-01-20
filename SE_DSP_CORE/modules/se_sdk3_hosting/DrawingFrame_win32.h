#pragma once

/*
#include "modules/se_sdk3_hosting/DrawingFrame_win32.h"
using namespace GmpiGuiHosting;
*/

#if defined(_WIN32) // skip compilation on macOS

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <dxgi1_6.h>
#include <d3d11_1.h>
#include <dwmapi.h>
#include "DirectXGfx.h"
#include "TimerManager.h"
#include "../../conversion.h"
#include "../shared/xplatform.h"
#include "../se_sdk3/mp_sdk_gui2.h"
#include "../se_sdk3_hosting/gmpi_gui_hosting.h"
#include "../se_sdk3_hosting/GraphicsRedrawClient.h"

namespace SynthEdit2
{
	class IPresenter;
}

namespace GmpiGuiHosting
{
// helper
inline float calcWhiteLevelForHwnd(HWND windowHandle)
{
	auto parent = ::GetAncestor(windowHandle, GA_ROOT);
	RECT m_windowBoundsRoot;
	GetWindowRect(parent, &m_windowBoundsRoot);

	RECT m_windowBounds;
	DwmGetWindowAttribute(parent, DWMWA_EXTENDED_FRAME_BOUNDS, &m_windowBounds, sizeof(m_windowBounds));

	/*
	// get bounds of window. dosn't handle DPI
	GetWindowRect(windowHandle, &m_windowBounds);
	*/
	GmpiDrawing::RectL appWindowRect{ m_windowBounds.left, m_windowBounds.top, m_windowBounds.right, m_windowBounds.bottom };

	// get all the monitor paths.
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

	DISPLAYCONFIG_TARGET_DEVICE_NAME targetName{};
	targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
	targetName.header.size = sizeof(targetName);

	DISPLAYCONFIG_SDR_WHITE_LEVEL white_level{};
	white_level.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
	white_level.header.size = sizeof(white_level);

	// check against each path (monitor)
	float whiteMult{ 1.0f };
	int bestIntersectArea = -1;
	for (auto& path : pathInfo)
	{
		const int idx = path.sourceInfo.modeInfoIdx;

		if (idx == DISPLAYCONFIG_PATH_MODE_IDX_INVALID)
			continue;

		const auto& sourceMode = modeInfo[idx].sourceMode;

		const GmpiDrawing::RectL outputRect
		{
			  sourceMode.position.x
			, sourceMode.position.y
			, static_cast<int32_t>(sourceMode.position.x + sourceMode.width)
			, static_cast<int32_t>(sourceMode.position.y + sourceMode.height)
		};

		const auto intersectRect = Intersect(appWindowRect, outputRect);
		const int intersectArea = intersectRect.getWidth() * intersectRect.getHeight();
		if (intersectArea <= bestIntersectArea)
			continue;

		// Get monitor handle for this path's target
		targetName.header.adapterId = path.targetInfo.adapterId;
		white_level.header.adapterId = path.targetInfo.adapterId;
		targetName.header.id = path.targetInfo.id;
		white_level.header.id = path.targetInfo.id;

		if (DisplayConfigGetDeviceInfo(&targetName.header) != ERROR_SUCCESS)
			continue;

		if (DisplayConfigGetDeviceInfo(&white_level.header) != ERROR_SUCCESS)
			continue;

		// divide by 1000 to get a factor
		const auto lwhiteMult = white_level.SDRWhiteLevel / 1000.f;

//		_RPTWN(0, L"Monitor %s [%d, %d] white level %f\n", targetName.monitorFriendlyDeviceName, outputRect.left, outputRect.top, lwhiteMult);

		bestIntersectArea = intersectArea;
		whiteMult = lwhiteMult;
	}
//	_RPTWN(0, L"Best white level %f\n", whiteMult);

	return whiteMult;
}

	// Base class for DrawingFrame (VST3 Plugins) and MyFrameWndDirectX (SynthEdit 1.4+ Panel View).
	class DrawingFrameBase : public gmpi_gui::IMpGraphicsHost, public gmpi::IMpUserInterfaceHost2, public TimerClient
	{
		std::chrono::time_point<std::chrono::steady_clock> frameCountTime;
		bool firstPresent = false;
		UpdateRegionWinGdi updateRegion_native;
		std::unique_ptr<gmpi::directx::GraphicsContext> context;

		// HDR support. Above swapChain so these get released first.
		gmpi::directx::ComPtr<ID2D1Effect> hdrWhiteScaleEffect;
		gmpi::directx::ComPtr<::ID2D1DeviceContext> hdrRenderTargetDC;
		gmpi::directx::ComPtr<ID2D1BitmapRenderTarget> hdrRenderTarget;
		gmpi::directx::ComPtr<ID2D1Bitmap> hdrBitmap;

		float windowWhiteLevel{}; // swapchain may choose to use a different white level than the monitor in 8-bit mode.

		// https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
		inline static const DXGI_FORMAT bestFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; // Proper gamma-correct blending.
		inline static const DXGI_FORMAT fallbackFormat = DXGI_FORMAT_B8G8R8A8_UNORM; // shitty linear blending.

	protected:
		bool monitorChanged = false;

		gmpi_sdk::mp_shared_ptr<IGraphicsRedrawClient> frameUpdateClient;

		GmpiDrawing::SizeL swapChainSize = {};

		ID2D1DeviceContext* mpRenderTarget = {};
		IDXGISwapChain1* m_swapChain = {};

		gmpi_sdk::mp_shared_ptr<gmpi_gui_api::IMpGraphics4> gmpi_gui_client; // usually a ContainerView at the topmost level
		gmpi_sdk::mp_shared_ptr<gmpi_gui_api::IMpKeyClient> gmpi_key_client;

		// Paint() uses Direct-2d which block on vsync. Therefore all invalid rects should be applied in one "hit", else windows message queue chokes calling WM_PAINT repeately and blocking on every rect.
		std::vector<GmpiDrawing::RectL> backBufferDirtyRects;
		GmpiDrawing::Matrix3x2 viewTransform;
		GmpiDrawing::Matrix3x2 DipsToWindow;
		GmpiDrawing::Matrix3x2 WindowToDips;
		int pollHdrChangesCount = 100;
		int toolTiptimer = 0;
		bool toolTipShown;
		HWND tooltipWindow;
		static const int toolTiptimerInit = 40; // x/60 Hz
		std::wstring toolTipText;
		bool reentrant;
		bool lowDpiMode = {};
		bool isTrackingMouse = false;
		GmpiDrawing::Point cubaseBugPreviousMouseMove = { -1,-1 };

	public:
		std::function<void()> clientInvalidated; // called from Paint if monitor white-level changed since last check.
		float calcWhiteLevel();
		void recreateSwapChainAndClientAsync();

		static const int viewDimensions = 7968; // DIPs (divisible by grids 60x60 + 2 24 pixel borders)
		inline static bool m_disable_gpu = false;
		inline static bool m_disable_deep_color = false;
		
		gmpi::directx::Factory DrawingFactory;

		DrawingFrameBase() :
			mpRenderTarget(nullptr)
			,m_swapChain(nullptr)
			, toolTipShown(false)
			, tooltipWindow(0)
			, reentrant(false)
		{
			DrawingFactory.Init();
		}

		virtual ~DrawingFrameBase()
		{
			StopTimer();

			// Free GUI objects first so they can release fonts etc before releasing factorys.
			gmpi_gui_client = nullptr;

			ReleaseDevice();
		}

		// provids a default message handler. Note that some clients provide their own. e.g. MyFrameWndDirectX
		LRESULT WindowProc(
			HWND hwnd,
			UINT message,
			WPARAM wParam,
			LPARAM lParam);

		// to help re-create device when lost.
		void ReleaseDevice()
		{
			if (hdrWhiteScaleEffect)
			{
				hdrWhiteScaleEffect->SetInput(0, nullptr);
				hdrWhiteScaleEffect = nullptr;
			}
			hdrBitmap = nullptr;
			hdrRenderTargetDC = nullptr;
			hdrRenderTarget = nullptr;

			if (mpRenderTarget)
				mpRenderTarget->Release();
			if (m_swapChain)
				m_swapChain->Release();

			mpRenderTarget = nullptr;
			m_swapChain = nullptr;
		}

		void InitClientSize();

		void CreateDevice();
		void CreateDeviceSwapChainBitmap();

		void AddView(gmpi_gui_api::IMpGraphics4* pcontainerView) // aka addClient
		{
			gmpi_gui_client = pcontainerView;
			pcontainerView->queryInterface(IGraphicsRedrawClient::guid, frameUpdateClient.asIMpUnknownPtr());
			pcontainerView->queryInterface(gmpi_gui_api::IMpKeyClient::guid, gmpi_key_client.asIMpUnknownPtr());

			gmpi_sdk::mp_shared_ptr<gmpi::IMpUserInterface2> pinHost;
			gmpi_gui_client->queryInterface(gmpi::MP_IID_GUI_PLUGIN2, pinHost.asIMpUnknownPtr());

			if(pinHost)
				pinHost->setHost(static_cast<gmpi_gui::IMpGraphicsHost*>(this));

			InitClientSize();
		}

		void detachAndRecreate();

		void OnPaint();
		virtual HWND getWindowHandle() = 0;
		void CreateRenderTarget();

		// Inherited via IMpUserInterfaceHost2
		virtual int32_t MP_STDCALL pinTransmit(int32_t pinId, int32_t size, const void * data, int32_t voice = 0) override;
		virtual int32_t MP_STDCALL createPinIterator(gmpi::IMpPinIterator** returnIterator) override;
		virtual int32_t MP_STDCALL getHandle(int32_t & returnValue) override;
		virtual int32_t MP_STDCALL sendMessageToAudio(int32_t id, int32_t size, const void * messageData) override;
		virtual int32_t MP_STDCALL ClearResourceUris() override;
		virtual int32_t MP_STDCALL RegisterResourceUri(const char * resourceName, const char * resourceType, gmpi::IString* returnString) override;
		virtual int32_t MP_STDCALL OpenUri(const char * fullUri, gmpi::IProtectedFile2** returnStream) override;
		virtual int32_t MP_STDCALL FindResourceU(const char * resourceName, const char * resourceType, gmpi::IString* returnString) override;
		int32_t MP_STDCALL LoadPresetFile_DEPRECATED(const char* presetFilePath) override
		{
//			Presenter()->LoadPresetFile(presetFilePath);
			return gmpi::MP_FAIL;
		}

		// IMpGraphicsHost
		virtual void MP_STDCALL invalidateRect(const GmpiDrawing_API::MP1_RECT * invalidRect) override;
		virtual void MP_STDCALL invalidateMeasure() override;
		virtual int32_t MP_STDCALL setCapture() override;
		virtual int32_t MP_STDCALL getCapture(int32_t & returnValue) override;
		virtual int32_t MP_STDCALL releaseCapture() override;
		virtual int32_t MP_STDCALL GetDrawingFactory(GmpiDrawing_API::IMpFactory ** returnFactory) override
		{
			*returnFactory = &DrawingFactory;
			return gmpi::MP_OK;
		}

		virtual int32_t MP_STDCALL createPlatformMenu(GmpiDrawing_API::MP1_RECT* rect, gmpi_gui::IMpPlatformMenu** returnMenu) override;
		virtual int32_t MP_STDCALL createPlatformTextEdit(GmpiDrawing_API::MP1_RECT* rect, gmpi_gui::IMpPlatformText** returnTextEdit) override;
		virtual int32_t MP_STDCALL createFileDialog(int32_t dialogType, gmpi_gui::IMpFileDialog** returnFileDialog) override;
		virtual int32_t MP_STDCALL createOkCancelDialog(int32_t dialogType, gmpi_gui::IMpOkCancelDialog** returnDialog) override;

		// IUnknown methods
		virtual int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface)
		{
			if (iid == gmpi::MP_IID_UI_HOST2)
			{
				// important to cast to correct vtable (ug_plugin3 has 2 vtables) before reinterpret cast
				*returnInterface = reinterpret_cast<void*>(static_cast<IMpUserInterfaceHost2*>(this));
				addRef();
				return gmpi::MP_OK;
			}

			if (iid == gmpi_gui::SE_IID_GRAPHICS_HOST || iid == gmpi_gui::SE_IID_GRAPHICS_HOST_BASE || iid == gmpi::MP_IID_UNKNOWN)
			{
				// important to cast to correct vtable (ug_plugin3 has 2 vtables) before reinterpret cast
				*returnInterface = reinterpret_cast<void*>(static_cast<IMpGraphicsHost*>(this));
				addRef();
				return gmpi::MP_OK;
			}

			*returnInterface = 0;
			return gmpi::MP_NOSUPPORT;
		}

		void initTooltip();
		void TooltipOnMouseActivity();
		void ShowToolTip();
		void HideToolTip();
		void OnSize(UINT width, UINT height);
		virtual bool OnTimer() override;
		virtual void autoScrollStart() {}
		virtual void autoScrollStop() {}

		auto getClient() {
			return gmpi_gui_client;
		}

		GMPI_REFCOUNT_NO_DELETE;
	};

	// This is used in VST3. Native HWND window frame.
	class DrawingFrame : public DrawingFrameBase
	{
		HWND windowHandle = {};
		HWND parentWnd = {};

	public:
		
		HWND getWindowHandle() override
		{
			return windowHandle;
		}

		void open(void* pParentWnd, const GmpiDrawing_API::MP1_SIZE_L* overrideSize = {});
		void ReSize(int left, int top, int right, int bottom);
		virtual void DoClose() {}
	};
} // namespace.

#endif // skip compilation on macOS
