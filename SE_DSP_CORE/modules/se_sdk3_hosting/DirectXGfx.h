#pragma once

/*
#include "DirectXGfx.h"
*/

#include <d2d1_2.h>
#include <dwrite.h>
#include <codecvt>
#include <Wincodec.h>
#include "./gmpi_gui_hosting.h"

// #define LOG_DIRECTX_CALLS

#define ENABLE_HDR_SUPPORT 0

namespace gmpi
{
	namespace directx
	{
		// Helper for managing lifetime of Direct2D interface pointers
		template<class wrappedObjT>
		class ComWrapper
		{
			mutable wrappedObjT* obj = {};

		public:
			ComWrapper() {}

			explicit ComWrapper(wrappedObjT* newobj)
			{
				Assign(newobj);
			}
			ComWrapper(const ComWrapper<wrappedObjT>& value)
			{
				Assign(value.obj);
			}
			// Attach object without incrementing ref count. For objects created with new.
			void Attach(wrappedObjT* newobj)
			{
				wrappedObjT* old = obj;
				obj = newobj;

				if (old)
				{
					old->Release();
				}
			}

			~ComWrapper()
			{
				if (obj)
				{
					obj->Release();
				}
			}
			inline operator wrappedObjT* ()
			{
				return obj;
			}
			const wrappedObjT* operator=(wrappedObjT* value)
			{
				Assign(value);
				return value;
			}
			ComWrapper<wrappedObjT>& operator=(ComWrapper<wrappedObjT>& value)
			{
				Assign(value.get());
				return *this;
			}
			bool operator==(const wrappedObjT* other) const
			{
				return obj == other;
			}
			bool operator==(const ComWrapper<wrappedObjT>& other) const
			{
				return obj == other.obj;
			}
			wrappedObjT* operator->() const
			{
				return obj;
			}

			wrappedObjT*& get()
			{
				return obj;
			}

			wrappedObjT** getAddressOf()
			{
				assert(obj == 0); // Free it before you re-use it!
				return &obj;
			}
			wrappedObjT** put()
			{
				if (obj)
				{
					obj->Release();
					obj = {};
				}

				return &obj;
			}

			bool isNull() const
			{
				return obj == nullptr;
			}

		private:
			// Attach object and increment ref count.
			inline void Assign(wrappedObjT* newobj)
			{
				Attach(newobj);
				if (newobj)
				{
					newobj->AddRef();
				}
			}
		};


		inline void SafeRelease(IUnknown* object)
		{
			if (object)
				object->Release();
		}

		// Classes without GetFactory()
		template<class MpInterface, class DxType>
		class GmpiDXWrapper : public MpInterface
		{
		protected:
			DxType* native_;

			~GmpiDXWrapper()
			{
				if (native_)
				{
					native_->Release();
//					_RPT1(_CRT_WARN, "Release() -> %x\n", (int)native_);
				}
			}

		public:
			GmpiDXWrapper(DxType* native = nullptr) : native_(native) {}

			inline DxType* native() const
			{
				return native_;
			}
		};

		// Classes with GetFactory()
		template<class MpInterface, class DxType>
		class GmpiDXResourceWrapper : public GmpiDXWrapper<MpInterface, DxType>
		{
		protected:
			GmpiDrawing_API::IMpFactory* factory_;

		public:
			GmpiDXResourceWrapper(DxType* native, GmpiDrawing_API::IMpFactory* factory) : GmpiDXWrapper<MpInterface, DxType>(native), factory_(factory) {}
			GmpiDXResourceWrapper(GmpiDrawing_API::IMpFactory* factory) : factory_(factory) {}

			void MP_STDCALL GetFactory(GmpiDrawing_API::IMpFactory **factory) override
			{
				*factory = factory_;
			}
		};

		class Brush : /* public GmpiDrawing_API::IMpBrush,*/ public GmpiDXResourceWrapper<GmpiDrawing_API::IMpBrush, ID2D1Brush> // Resource
		{
		public:
			Brush(ID2D1Brush* native, GmpiDrawing_API::IMpFactory* factory) : GmpiDXResourceWrapper(native, factory) {}

#ifdef LOG_DIRECTX_CALLS
			~Brush()
			{
				_RPT1(_CRT_WARN, "brush%x->Release();\n", (int)this);
				_RPT1(_CRT_WARN, "brush%x = nullptr;\n", (int)this);
			}
#endif

			inline ID2D1Brush* nativeBrush()
			{
				return (ID2D1Brush*)native_;
			}
		};

		class SolidColorBrush final : /* Simulated: public GmpiDrawing_API::IMpSolidColorBrush,*/ public Brush
		{
#if	ENABLE_HDR_SUPPORT
			float whiteMult = 1.0f;
#endif
		public:
			SolidColorBrush(ID2D1SolidColorBrush* b, GmpiDrawing_API::IMpFactory *factory
#if	ENABLE_HDR_SUPPORT
				, float pwhiteMult
#endif
			) : Brush(b, factory)
#if	ENABLE_HDR_SUPPORT
				, whiteMult(pwhiteMult)
#endif
			{}

			inline ID2D1SolidColorBrush* nativeSolidColorBrush()
			{
				return (ID2D1SolidColorBrush*)native_;
			}

			// IMPORTANT: Virtual functions must 100% match simulated interface (GmpiDrawing_API::IMpSolidColorBrush)
			virtual void MP_STDCALL SetColor(const GmpiDrawing_API::MP1_COLOR* color) // simulated: override
			{
//				D2D1::ConvertColorSpace(D2D1::ColorF*) color);
#if	ENABLE_HDR_SUPPORT
				const D2D1_COLOR_F c
				{
					color->r * whiteMult,
					color->g * whiteMult,
					color->b * whiteMult,
					color->a
				};
#else
				const D2D1_COLOR_F c
				{
					color->r,
					color->g,
					color->b,
					color->a
				};
#endif

				nativeSolidColorBrush()->SetColor(c);
			}
			virtual GmpiDrawing_API::MP1_COLOR MP_STDCALL GetColor() // simulated:  override
			{
				auto b = nativeSolidColorBrush()->GetColor();

#if	ENABLE_HDR_SUPPORT
				GmpiDrawing_API::MP1_COLOR c;
				c.a = b.a;
				c.r = b.r / whiteMult;
				c.g = b.g / whiteMult;
				c.b = b.b / whiteMult;
#else
				GmpiDrawing_API::MP1_COLOR c;
				c.a = b.a;
				c.r = b.r;
				c.g = b.g;
				c.b = b.b;
#endif
				return c;
			}

			//	GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_SOLIDCOLORBRUSH_MPGUI, GmpiDrawing_API::IMpSolidColorBrush);

			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				*returnInterface = 0;
				if (iid == GmpiDrawing_API::SE_IID_SOLIDCOLORBRUSH_MPGUI || iid == gmpi::MP_IID_UNKNOWN)
				{
					// non-standard. Forcing this class (which has the correct vtable) to pretend it's the emulated interface.
					*returnInterface = reinterpret_cast<GmpiDrawing_API::IMpSolidColorBrush*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				return gmpi::MP_NOSUPPORT;
			}

			GMPI_REFCOUNT;
		};

		class SolidColorBrush_Win7 final : /* Simulated: public GmpiDrawing_API::IMpSolidColorBrush,*/ public Brush
		{
		public:
			SolidColorBrush_Win7(ID2D1RenderTarget* context, const GmpiDrawing_API::MP1_COLOR* color, GmpiDrawing_API::IMpFactory* factory) : Brush(nullptr, factory)
			{
				GmpiDrawing::Color modified;
				modified.a = color->a;
				modified.r = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color->r));
				modified.g = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color->g));
				modified.b = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color->b));
//				modified = GmpiDrawing::Color::Orange;

				/*HRESULT hr =*/ context->CreateSolidColorBrush(*(D2D1_COLOR_F*)&modified, (ID2D1SolidColorBrush**) &native_);
			}

			inline ID2D1SolidColorBrush* nativeSolidColorBrush()
			{
				return (ID2D1SolidColorBrush*)native_;
			}

			// IMPORTANT: Virtual functions must 100% match simulated interface (GmpiDrawing_API::IMpSolidColorBrush)
			virtual void MP_STDCALL SetColor(const GmpiDrawing_API::MP1_COLOR* color) // simulated: override
			{
				//				D2D1::ConvertColorSpace(D2D1::ColorF*) color);
				GmpiDrawing::Color modified;
				modified.a = color->a;
				modified.r = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color->r));
				modified.g = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color->g));
				modified.b = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color->b));
				nativeSolidColorBrush()->SetColor((D2D1::ColorF*) &modified);
			}

			virtual GmpiDrawing_API::MP1_COLOR MP_STDCALL GetColor() // simulated:  override
			{
				auto b = nativeSolidColorBrush()->GetColor();
				//		return GmpiDrawing::Color(b.r, b.g, b.b, b.a);
				GmpiDrawing_API::MP1_COLOR c;
				c.a = b.a;
				c.r = b.r;
				c.g = b.g;
				c.b = b.b;
				return c;
			}

			//	GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_SOLIDCOLORBRUSH_MPGUI, GmpiDrawing_API::IMpSolidColorBrush);

			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				*returnInterface = 0;
				if (iid == GmpiDrawing_API::SE_IID_SOLIDCOLORBRUSH_MPGUI || iid == gmpi::MP_IID_UNKNOWN)
				{
					// non-standard. Forcing this class (which has the correct vtable) to pretend it's the emulated interface.
					*returnInterface = reinterpret_cast<GmpiDrawing_API::IMpSolidColorBrush*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				return gmpi::MP_NOSUPPORT;
			}

			GMPI_REFCOUNT;
		};

		class GradientStopCollection final : public GmpiDXResourceWrapper<GmpiDrawing_API::IMpGradientStopCollection, ID2D1GradientStopCollection>
		{
		public:
			GradientStopCollection(ID2D1GradientStopCollection* native, GmpiDrawing_API::IMpFactory* factory) : GmpiDXResourceWrapper(native, factory) {}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_GRADIENTSTOPCOLLECTION_MPGUI, GmpiDrawing_API::IMpGradientStopCollection);
			GMPI_REFCOUNT;
		};

		class GradientStopCollection1 final : public GmpiDXResourceWrapper<GmpiDrawing_API::IMpGradientStopCollection, ID2D1GradientStopCollection1>
		{
		public:
			GradientStopCollection1(ID2D1GradientStopCollection1* native, GmpiDrawing_API::IMpFactory* factory) : GmpiDXResourceWrapper(native, factory) {}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_GRADIENTSTOPCOLLECTION_MPGUI, GmpiDrawing_API::IMpGradientStopCollection);
			GMPI_REFCOUNT;
		};

		class LinearGradientBrush final : /* Simulated: public GmpiDrawing_API::IMpLinearGradientBrush,*/ public Brush
		{
		public:
			LinearGradientBrush(GmpiDrawing_API::IMpFactory *factory, ID2D1RenderTarget* context, const GmpiDrawing_API::MP1_LINEAR_GRADIENT_BRUSH_PROPERTIES* linearGradientBrushProperties, const GmpiDrawing_API::MP1_BRUSH_PROPERTIES* brushProperties, const  GmpiDrawing_API::IMpGradientStopCollection* gradientStopCollection)
			 : Brush(nullptr, factory)
			{
				[[maybe_unused]] HRESULT hr = context->CreateLinearGradientBrush((D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES*)linearGradientBrushProperties, (D2D1_BRUSH_PROPERTIES*)brushProperties, ((GradientStopCollection*)gradientStopCollection)->native(), (ID2D1LinearGradientBrush **)&native_);
				assert(hr == 0);
			}

			inline ID2D1LinearGradientBrush* native()
			{
				return (ID2D1LinearGradientBrush*)native_;
			}

			// IMPORTANT: Virtual functions must 100% match simulated interface (GmpiDrawing_API::IMpLinearGradientBrush)
			virtual void MP_STDCALL SetStartPoint(GmpiDrawing_API::MP1_POINT startPoint) // simulated: override
			{
				native()->SetStartPoint(*reinterpret_cast<D2D1_POINT_2F*>(&startPoint));
			}
			virtual void MP_STDCALL SetEndPoint(GmpiDrawing_API::MP1_POINT endPoint) // simulated: override
			{
				native()->SetEndPoint(*reinterpret_cast<D2D1_POINT_2F*>(&endPoint));
			}

			//	GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_LINEARGRADIENTBRUSH_MPGUI, GmpiDrawing_API::IMpLinearGradientBrush);
			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				*returnInterface = 0;
				if (iid == GmpiDrawing_API::SE_IID_LINEARGRADIENTBRUSH_MPGUI || iid == gmpi::MP_IID_UNKNOWN)
				{
					// non-standard. Forcing this class (which has the correct vtable) to pretend it's the emulated interface.
					*returnInterface = reinterpret_cast<GmpiDrawing_API::IMpLinearGradientBrush*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				return gmpi::MP_NOSUPPORT;
			}

			GMPI_REFCOUNT;
		};

		class RadialGradientBrush final : /* Simulated: public GmpiDrawing_API::IMpRadialGradientBrush,*/ public Brush
		{
		public:
			RadialGradientBrush(GmpiDrawing_API::IMpFactory *factory, ID2D1RenderTarget* context, const GmpiDrawing_API::MP1_RADIAL_GRADIENT_BRUSH_PROPERTIES* linearGradientBrushProperties, const GmpiDrawing_API::MP1_BRUSH_PROPERTIES* brushProperties, const GmpiDrawing_API::IMpGradientStopCollection* gradientStopCollection)
			 : Brush(nullptr, factory)
			{
				[[maybe_unused]] HRESULT hr = context->CreateRadialGradientBrush((D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES*)linearGradientBrushProperties, (D2D1_BRUSH_PROPERTIES*)brushProperties, ((GradientStopCollection*)gradientStopCollection)->native(), (ID2D1RadialGradientBrush **)&native_);
				assert(hr == 0);
			}

			inline ID2D1RadialGradientBrush* native()
			{
				return (ID2D1RadialGradientBrush*)native_;
			}

			// IMPORTANT: Virtual functions must 100% match simulated interface.
			virtual void MP_STDCALL SetCenter(GmpiDrawing_API::MP1_POINT center)  // simulated: override
			{
				native()->SetCenter(*reinterpret_cast<D2D1_POINT_2F*>(&center));
			}

			virtual void MP_STDCALL SetGradientOriginOffset(GmpiDrawing_API::MP1_POINT gradientOriginOffset) // simulated: override
			{
				native()->SetGradientOriginOffset(*reinterpret_cast<D2D1_POINT_2F*>(&gradientOriginOffset));
			}

			virtual void MP_STDCALL SetRadiusX(float radiusX) // simulated: override
			{
				native()->SetRadiusX(radiusX);
			}

			virtual void MP_STDCALL SetRadiusY(float radiusY) // simulated: override
			{
				native()->SetRadiusY(radiusY);
			}

			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				*returnInterface = 0;
				if (iid == GmpiDrawing_API::SE_IID_RADIALGRADIENTBRUSH_MPGUI || iid == gmpi::MP_IID_UNKNOWN)
				{
					// non-standard. Forcing this class (which has the correct vtable) to pretend it's the emulated interface.
					*returnInterface = reinterpret_cast<GmpiDrawing_API::IMpRadialGradientBrush*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				return gmpi::MP_NOSUPPORT;
			}

			GMPI_REFCOUNT;
		};

		class StrokeStyle final : public GmpiDXResourceWrapper<GmpiDrawing_API::IMpStrokeStyle, ID2D1StrokeStyle>
		{
		public:
			StrokeStyle(ID2D1StrokeStyle* native, GmpiDrawing_API::IMpFactory* factory) : GmpiDXResourceWrapper(native, factory) {}

			virtual GmpiDrawing_API::MP1_CAP_STYLE MP_STDCALL GetStartCap() override
			{
				return (GmpiDrawing_API::MP1_CAP_STYLE) native()->GetStartCap();
			}

			virtual GmpiDrawing_API::MP1_CAP_STYLE MP_STDCALL GetEndCap() override
			{
				return (GmpiDrawing_API::MP1_CAP_STYLE) native()->GetEndCap();
			}

			virtual GmpiDrawing_API::MP1_CAP_STYLE MP_STDCALL GetDashCap() override
			{
				return (GmpiDrawing_API::MP1_CAP_STYLE) native()->GetDashCap();
			}

			virtual float MP_STDCALL GetMiterLimit() override
			{
				return native()->GetMiterLimit();
			}

			virtual GmpiDrawing_API::MP1_LINE_JOIN MP_STDCALL GetLineJoin() override
			{
				return (GmpiDrawing_API::MP1_LINE_JOIN) native()->GetLineJoin();
			}

			virtual float MP_STDCALL GetDashOffset() override
			{
				return native()->GetDashOffset();
			}

			virtual GmpiDrawing_API::MP1_DASH_STYLE MP_STDCALL GetDashStyle() override
			{
				return (GmpiDrawing_API::MP1_DASH_STYLE) native()->GetDashStyle();
			}

			virtual uint32_t MP_STDCALL GetDashesCount() override
			{
				return native()->GetDashesCount();
			}

			void MP_STDCALL GetDashes(float* dashes, uint32_t dashesCount) override
			{
				return native()->GetDashes(dashes, dashesCount);
			}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_STROKESTYLE_MPGUI, GmpiDrawing_API::IMpStrokeStyle);
			GMPI_REFCOUNT;
		};

		inline ID2D1StrokeStyle* toNative(const GmpiDrawing_API::IMpStrokeStyle* strokeStyle)
		{
			if (strokeStyle)
			{
				return ((StrokeStyle*)strokeStyle)->native();
			}
			return nullptr;
		}

		class TessellationSink final : public GmpiDXWrapper<GmpiDrawing_API::IMpTessellationSink, ID2D1TessellationSink>
		{
		public:
			TessellationSink(ID2D1Mesh* mesh)
			{
				[[maybe_unused]] HRESULT hr = mesh->Open(&native_);
				assert(hr == S_OK);
			}

			void MP_STDCALL AddTriangles(const GmpiDrawing_API::MP1_TRIANGLE* triangles, uint32_t trianglesCount) override
			{
				native_->AddTriangles((const D2D1_TRIANGLE*) triangles, trianglesCount);
			}

			int32_t MP_STDCALL Close() override
			{
				native_->Close();
				return MP_OK;
			}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_TESSELLATIONSINK_MPGUI, GmpiDrawing_API::IMpTessellationSink);
			GMPI_REFCOUNT;
		};
		
		class Mesh final : public GmpiDXResourceWrapper<GmpiDrawing_API::IMpMesh, ID2D1Mesh>
		{
		public:
			Mesh(GmpiDrawing_API::IMpFactory* factory, ID2D1RenderTarget* context) :
				GmpiDXResourceWrapper(factory)
			{
				[[maybe_unused]] HRESULT hr = context->CreateMesh(&native_);
				assert(hr == S_OK);
			}

			// IMpMesh
			int32_t MP_STDCALL Open(GmpiDrawing_API::IMpTessellationSink** returnObject) override
			{
				*returnObject = nullptr;
				gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> wrapper;
				wrapper.Attach(new TessellationSink(native_));
				return wrapper->queryInterface(GmpiDrawing_API::SE_IID_TESSELLATIONSINK_MPGUI, reinterpret_cast<void **>(returnObject));
			}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_MESH_MPGUI, GmpiDrawing_API::IMpMesh);
			GMPI_REFCOUNT;
		};

		class TextFormat final : public GmpiDXWrapper<GmpiDrawing_API::IMpTextFormat, IDWriteTextFormat>
		{
			std::wstring_convert<std::codecvt_utf8<wchar_t>>* stringConverter = {}; // constructed once is much faster.
			bool useLegacyBaseLineSnapping = true;
			float topAdjustment = {};
			float fontMetrics_ascent = {};

			void CalculateTopAdjustment()
			{
				assert(topAdjustment == 0.0f); // else boundingBoxSize calculation will be affected, and won't be actual native size.

				GmpiDrawing_API::MP1_FONT_METRICS fontMetrics;
				GetFontMetrics(&fontMetrics);

				GmpiDrawing_API::MP1_SIZE boundingBoxSize;
				GetTextExtentU("A", 1, &boundingBoxSize);

				topAdjustment = boundingBoxSize.height - (fontMetrics.ascent + fontMetrics.descent);
				fontMetrics_ascent = fontMetrics.ascent;
			}

		public:
			TextFormat(std::wstring_convert<std::codecvt_utf8<wchar_t>>* pstringConverter, IDWriteTextFormat* native) :
				GmpiDXWrapper<GmpiDrawing_API::IMpTextFormat, IDWriteTextFormat>(native)
				, stringConverter(pstringConverter)
			{
				CalculateTopAdjustment();
			}
#ifdef LOG_DIRECTX_CALLS
			~TextFormat()
			{
				_RPT1(_CRT_WARN, "textformat%x->Release();\n", (int)this);
				_RPT1(_CRT_WARN, "textformat%x = nullptr;\n", (int)this);
			}
#endif

			int32_t MP_STDCALL SetTextAlignment(GmpiDrawing_API::MP1_TEXT_ALIGNMENT textAlignment) override
			{
				native()->SetTextAlignment((DWRITE_TEXT_ALIGNMENT)textAlignment);
				return gmpi::MP_OK;
			}

			int32_t MP_STDCALL SetParagraphAlignment(GmpiDrawing_API::MP1_PARAGRAPH_ALIGNMENT paragraphAlignment) override
			{
				native()->SetParagraphAlignment((DWRITE_PARAGRAPH_ALIGNMENT)paragraphAlignment);
				return gmpi::MP_OK;
			}

			int32_t MP_STDCALL SetWordWrapping(GmpiDrawing_API::MP1_WORD_WRAPPING wordWrapping) override
			{
				return native()->SetWordWrapping((DWRITE_WORD_WRAPPING)wordWrapping);
			}

			int32_t MP_STDCALL SetLineSpacing(float lineSpacing, float baseline) override
			{
				// Hack, reuse this method to enable legacy-mode.
				if (static_cast<float>(IMpTextFormat::ImprovedVerticalBaselineSnapping) == lineSpacing)
				{
					useLegacyBaseLineSnapping = false;
					return gmpi::MP_OK;
				}

				// For the default method, spacing depends solely on the content. For uniform spacing, the specified line height overrides the content.
				DWRITE_LINE_SPACING_METHOD method = lineSpacing < 0.0f ? DWRITE_LINE_SPACING_METHOD_DEFAULT : DWRITE_LINE_SPACING_METHOD_UNIFORM;
				return native()->SetLineSpacing(method, fabsf(lineSpacing), baseline);
			}

			int32_t MP_STDCALL GetFontMetrics(GmpiDrawing_API::MP1_FONT_METRICS* returnFontMetrics) override;

			// TODO!!!: Probably needs to accept constraint rect like DirectWrite. !!!
			//	void MP_STDCALL GetTextExtentU(const char* utf8String, int32_t stringLength, GmpiDrawing::Size& returnSize)
			void MP_STDCALL GetTextExtentU(const char* utf8String, int32_t stringLength, GmpiDrawing_API::MP1_SIZE* returnSize) override;

			float getTopAdjustment() const
			{
				return topAdjustment;
			}

			float getAscent() const
			{
				return fontMetrics_ascent;
			}

			bool getUseLegacyBaseLineSnapping() const
			{
				return useLegacyBaseLineSnapping;
			}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_TEXTFORMAT_MPGUI, GmpiDrawing_API::IMpTextFormat);
			GMPI_REFCOUNT;
		};

		class bitmapPixels : public GmpiDrawing_API::IMpBitmapPixels
		{
			bool alphaPremultiplied;
			IWICBitmap* bitmap;
			UINT bytesPerRow{};
			BYTE* ptr{};
			IWICBitmapLock* pBitmapLock;
			ID2D1Bitmap* nativeBitmap_;
			int flags;
			IMpBitmapPixels::PixelFormat pixelFormat = kBGRA; // default to non-SRGB Win7 (not tested)

		public:
			bitmapPixels(ID2D1Bitmap* nativeBitmap, IWICBitmap* inBitmap, bool _alphaPremultiplied, int32_t pflags)
			{
				nativeBitmap_ = nativeBitmap;
				assert(inBitmap);

				UINT w, h;
				inBitmap->GetSize(&w, &h);

				{
					WICPixelFormatGUID formatGuid;
					inBitmap->GetPixelFormat(&formatGuid);

					// premultiplied BGRA (default)
					if (std::memcmp(&formatGuid, &GUID_WICPixelFormat32bppPBGRA, sizeof(formatGuid)) == 0)
					{
						pixelFormat = kBGRA_SRGB;
					}
				}

				bitmap = nullptr;
				pBitmapLock = nullptr;
				WICRect rcLock = { 0, 0, (INT)w, (INT)h };
				flags = pflags;

				if (0 <= inBitmap->Lock(&rcLock, flags, &pBitmapLock))
				{
					pBitmapLock->GetStride(&bytesPerRow);
					UINT bufferSize;
					pBitmapLock->GetDataPointer(&bufferSize, &ptr);

					bitmap = inBitmap;
					bitmap->AddRef();

					alphaPremultiplied = _alphaPremultiplied;
					if (!alphaPremultiplied)
						unpremultiplyAlpha();
				}
				else
				{
					alphaPremultiplied = true; // prevent possible null deference of 'bitmap' in destructor
				}
			}

			~bitmapPixels()
			{
				if (!alphaPremultiplied)
					premultiplyAlpha();

				if (nativeBitmap_)
				{
#if 1
					if (0 != (flags & GmpiDrawing_API::MP1_BITMAP_LOCK_WRITE))
					{
						D2D1_RECT_U r;
						r.left = r.top = 0;
						bitmap->GetSize(&r.right, &r.bottom);

						nativeBitmap_->CopyFromMemory(&r, ptr, bytesPerRow);
					}
#else
					nativeBitmap_->Release();
					nativeBitmap_ = nullptr;
#endif
				}

				SafeRelease(pBitmapLock);
				SafeRelease(bitmap);
			}

			virtual uint8_t* MP_STDCALL getAddress() const { return ptr; }
			virtual int32_t MP_STDCALL getBytesPerRow() const { return bytesPerRow; }
			virtual int32_t MP_STDCALL getPixelFormat() const
			{
				return pixelFormat;
/*
				nativeBitmap_->GetPixelFormat().format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ? kBGRA_SRGB : kRGBA;

				WICPixelFormatGUID pixelFormat = 0;
				bitmap->GetPixelFormat(&pixelFormat);

				return pixelFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB ? kBGRA_SRGB : kRGBA;
*/
			}

			inline uint8_t fast8bitScale(uint8_t a, uint8_t b)
			{
				int t = (int)a * (int)b;
				return (uint8_t)((t + 1 + (t >> 8)) >> 8); // fast way to divide by 255
			}

			void premultiplyAlpha()
			{
				UINT w, h;
				bitmap->GetSize(&w, &h);
				int totalPixels = h * bytesPerRow / sizeof(uint32_t);

				uint8_t* pixel = ptr;

				for (int i = 0; i < totalPixels; ++i)
				{
					if (pixel[3] == 0)
					{
						pixel[0] = 0;
						pixel[1] = 0;
						pixel[2] = 0;
					}
					else
					{
						pixel[0] = fast8bitScale(pixel[0], pixel[3]);
						pixel[1] = fast8bitScale(pixel[1], pixel[3]);
						pixel[2] = fast8bitScale(pixel[2], pixel[3]);
					}

					pixel += sizeof(uint32_t);
				}
			}

			//-----------------------------------------------------------------------------
			void unpremultiplyAlpha()
			{
				UINT w, h;
				bitmap->GetSize(&w, &h);
				int totalPixels = h * bytesPerRow / sizeof(uint32_t);

				uint8_t* pixel = ptr;

				for (int i = 0; i < totalPixels; ++i)
				{
					if (pixel[3] != 0)
					{
						pixel[0] = (uint32_t)(pixel[0] * 255) / pixel[3];
						pixel[1] = (uint32_t)(pixel[1] * 255) / pixel[3];
						pixel[2] = (uint32_t)(pixel[2] * 255) / pixel[3];
					}
					pixel += sizeof(uint32_t);
				}
			}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_BITMAP_PIXELS_MPGUI, GmpiDrawing_API::IMpBitmapPixels);
			GMPI_REFCOUNT;
		};

		class Bitmap : public GmpiDrawing_API::IMpBitmap
		{
		public:
			ID2D1Bitmap* nativeBitmap_;
			ID2D1DeviceContext* nativeContext_;
			IWICBitmap* diBitmap_ = {};
#if	ENABLE_HDR_SUPPORT
			gmpi::directx::ComWrapper<ID2D1Bitmap1> nativeBitmap_HDR_;
#endif
			class Factory* factory;
#ifdef _DEBUG
			std::string debugFilename;
#endif
			Bitmap(Factory* pfactory, IWICBitmap* diBitmap);

			Bitmap(Factory* pfactory, ID2D1DeviceContext* nativeContext, ID2D1Bitmap* nativeBitmap) :
				nativeBitmap_(nativeBitmap)
				, nativeContext_(nativeContext)
				, factory(pfactory)
			{
				nativeBitmap->AddRef();
			}

			~Bitmap()
			{
				if (nativeBitmap_)
				{
					nativeBitmap_->Release();
				}
				if (diBitmap_)
				{
					diBitmap_->Release();
				}
			}

			ID2D1Bitmap* GetNativeBitmap(ID2D1DeviceContext* nativeContext);

			virtual GmpiDrawing_API::MP1_SIZE MP_STDCALL GetSizeF() override
			{
				if (diBitmap_)
				{
					UINT width{}, height{};
					diBitmap_->GetSize(&width, &height);
					return { (float)width, (float)height };
				}
				else if (nativeBitmap_)
				{
					const auto sizef = nativeBitmap_->GetSize();
					return { sizef.width, sizef.height };
				}

				return {};
			}

			int32_t MP_STDCALL GetSize(GmpiDrawing_API::MP1_SIZE_U* returnSize) override
			{
				if (diBitmap_)
				{
					diBitmap_->GetSize(&returnSize->width, &returnSize->height);
				}
				else if (nativeBitmap_)
				{
					const auto sizef = nativeBitmap_->GetSize();
					returnSize->width = (uint32_t)sizef.width;
					returnSize->height = (uint32_t)sizef.height;
				}
				else
				{
					*returnSize = {};
					return gmpi::MP_FAIL;
				}

				return gmpi::MP_OK;
			}

			int32_t MP_STDCALL lockPixelsOld(GmpiDrawing_API::IMpBitmapPixels** returnInterface, bool alphaPremultiplied) override
			{
				*returnInterface = 0;

				gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> b2;
				b2.Attach(new bitmapPixels(nativeBitmap_, diBitmap_, alphaPremultiplied, GmpiDrawing_API::MP1_BITMAP_LOCK_READ | GmpiDrawing_API::MP1_BITMAP_LOCK_WRITE));

				return b2->queryInterface(GmpiDrawing_API::SE_IID_BITMAP_PIXELS_MPGUI, (void**)(returnInterface));
			}

			virtual int32_t MP_STDCALL lockPixels(GmpiDrawing_API::IMpBitmapPixels** returnInterface, int32_t flags);

			void MP_STDCALL ApplyAlphaCorrection() override{} // deprecated
//			void ApplyAlphaCorrection_win7();
			void ApplyPreMultiplyCorrection();

			void MP_STDCALL GetFactory(GmpiDrawing_API::IMpFactory** pfactory) override;

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_BITMAP_MPGUI, GmpiDrawing_API::IMpBitmap);
			GMPI_REFCOUNT;
		};

		class BitmapBrush final : /* Simulated: public GmpiDrawing_API::IMpBitmapBrush,*/ public Brush
		{
		public:
			BitmapBrush(
				GmpiDrawing_API::IMpFactory *factory,
				ID2D1DeviceContext* context,
				const GmpiDrawing_API::IMpBitmap* bitmap,
				const GmpiDrawing_API::MP1_BITMAP_BRUSH_PROPERTIES* bitmapBrushProperties,
				const GmpiDrawing_API::MP1_BRUSH_PROPERTIES* brushProperties
			)
			 : Brush(nullptr, factory)
			{
				auto bm = ((Bitmap*)bitmap);
				auto nativeBitmap = bm->GetNativeBitmap(context);

				[[maybe_unused]] const auto hr = context->CreateBitmapBrush(nativeBitmap, (D2D1_BITMAP_BRUSH_PROPERTIES*)bitmapBrushProperties, (D2D1_BRUSH_PROPERTIES*)brushProperties, (ID2D1BitmapBrush**)&native_);
				assert(hr == 0);
			}

			inline ID2D1BitmapBrush* native()
			{
				return (ID2D1BitmapBrush*)native_;
			}

			// IMPORTANT: Virtual functions must 100% match simulated interface.
			virtual void MP_STDCALL SetExtendModeX(GmpiDrawing_API::MP1_EXTEND_MODE extendModeX)
			{
				native()->SetExtendModeX((D2D1_EXTEND_MODE)extendModeX);
			}

			virtual void MP_STDCALL SetExtendModeY(GmpiDrawing_API::MP1_EXTEND_MODE extendModeY)
			{
				native()->SetExtendModeY((D2D1_EXTEND_MODE)extendModeY);
			}

			virtual void MP_STDCALL SetInterpolationMode(GmpiDrawing_API::MP1_BITMAP_INTERPOLATION_MODE interpolationMode)
			{
				native()->SetInterpolationMode((D2D1_BITMAP_INTERPOLATION_MODE)interpolationMode);
			}

			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				*returnInterface = 0;
				if (iid == GmpiDrawing_API::SE_IID_BITMAPBRUSH_MPGUI || iid == gmpi::MP_IID_UNKNOWN)
				{
					// non-standard. Forcing this class (which has the correct vtable) to pretend it's the emulated interface.
					*returnInterface = reinterpret_cast<GmpiDrawing_API::IMpLinearGradientBrush*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				return gmpi::MP_NOSUPPORT;
			}

			GMPI_REFCOUNT;
		};

		class GeometrySink : public GmpiDrawing_API::IMpGeometrySink2
		{
			ID2D1GeometrySink* geometrysink_;

		public:
			GeometrySink(ID2D1GeometrySink* context) : geometrysink_(context) {}
			~GeometrySink()
			{
				if (geometrysink_)
				{
					geometrysink_->Release();

#ifdef LOG_DIRECTX_CALLS
					_RPT1(_CRT_WARN, "sink%x->Release();\n", (int)this);
					_RPT1(_CRT_WARN, "sink%x = nullptr;\n", (int)this);
#endif
				}
			}
			void MP_STDCALL SetFillMode(GmpiDrawing_API::MP1_FILL_MODE fillMode) override
			{
				geometrysink_->SetFillMode((D2D1_FILL_MODE)fillMode);
			}
#if 0
			void MP_STDCALL SetSegmentFlags(GmpiDrawing_API::MP1_PATH_SEGMENT vertexFlags) override
			{
				geometrysink_->SetSegmentFlags((D2D1_PATH_SEGMENT)vertexFlags);
			}
#endif
			void MP_STDCALL BeginFigure(GmpiDrawing_API::MP1_POINT startPoint, GmpiDrawing_API::MP1_FIGURE_BEGIN figureBegin) override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT4(_CRT_WARN, "sink%x->BeginFigure(D2D1::Point2F(%f,%f), (D2D1_FIGURE_BEGIN)%d);\n", (int)this, startPoint.x, startPoint.y, figureBegin);
#endif
				geometrysink_->BeginFigure(*reinterpret_cast<D2D1_POINT_2F*>(&startPoint), (D2D1_FIGURE_BEGIN)figureBegin);
			}
			void MP_STDCALL AddLines(const GmpiDrawing_API::MP1_POINT* points, uint32_t pointsCount) override
			{
				geometrysink_->AddLines(reinterpret_cast<const D2D1_POINT_2F*>(points), pointsCount);
			}
			void MP_STDCALL AddBeziers(const GmpiDrawing_API::MP1_BEZIER_SEGMENT* beziers, uint32_t beziersCount) override
			{
				geometrysink_->AddBeziers(reinterpret_cast<const D2D1_BEZIER_SEGMENT*>(beziers), beziersCount);
			}
			void MP_STDCALL EndFigure(GmpiDrawing_API::MP1_FIGURE_END figureEnd) override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT2(_CRT_WARN, "sink%x->EndFigure((D2D1_FIGURE_END)%d);\n", (int)this, figureEnd);
#endif
				geometrysink_->EndFigure((D2D1_FIGURE_END)figureEnd);
			}
			int32_t MP_STDCALL Close() override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT1(_CRT_WARN, "sink%x->Close();\n", (int)this);
#endif
				auto hr = geometrysink_->Close();
				return hr == 0 ? (gmpi::MP_OK) : (gmpi::MP_FAIL);
			}
			void MP_STDCALL AddLine(GmpiDrawing_API::MP1_POINT point) override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT4(_CRT_WARN, "sink%x->AddLine(D2D1::Point2F(%f,%f));\n", (int)this, point.x, point.y);
#endif
				geometrysink_->AddLine(*reinterpret_cast<D2D1_POINT_2F*>(&point));
			}
			void MP_STDCALL AddBezier(const GmpiDrawing_API::MP1_BEZIER_SEGMENT* bezier) override
			{
				geometrysink_->AddBezier(reinterpret_cast<const D2D1_BEZIER_SEGMENT*>(bezier));
			}
			void MP_STDCALL AddQuadraticBezier(const GmpiDrawing_API::MP1_QUADRATIC_BEZIER_SEGMENT* bezier) override
			{
				geometrysink_->AddQuadraticBezier(reinterpret_cast<const D2D1_QUADRATIC_BEZIER_SEGMENT*>(bezier));
			}
			void MP_STDCALL AddQuadraticBeziers(const GmpiDrawing_API::MP1_QUADRATIC_BEZIER_SEGMENT* beziers, uint32_t beziersCount) override
			{
				geometrysink_->AddQuadraticBeziers(reinterpret_cast<const D2D1_QUADRATIC_BEZIER_SEGMENT*>(beziers), beziersCount);
			}
			void MP_STDCALL AddArc(const GmpiDrawing_API::MP1_ARC_SEGMENT* arc) override
			{
				geometrysink_->AddArc(reinterpret_cast<const D2D1_ARC_SEGMENT*>(arc));
			}

			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				if (iid == GmpiDrawing_API::SE_IID_GEOMETRYSINK2_MPGUI || iid == GmpiDrawing_API::SE_IID_GEOMETRYSINK_MPGUI || iid == gmpi::MP_IID_UNKNOWN)
				{
					*returnInterface = reinterpret_cast<void*>(static_cast<GmpiDrawing_API::IMpGeometrySink2*>(this));
					addRef();
					return gmpi::MP_OK;
				}

				*returnInterface = 0;
				return gmpi::MP_NOSUPPORT;
			}
			GMPI_REFCOUNT;
		};


		class Geometry : public GmpiDrawing_API::IMpPathGeometry
		{
			friend class GraphicsContext;

			ID2D1PathGeometry* geometry_;

		public:
			Geometry(ID2D1PathGeometry* context) : geometry_(context)
			{}
			~Geometry()
			{
				if (geometry_)
				{
					geometry_->Release();
#ifdef LOG_DIRECTX_CALLS
					_RPT1(_CRT_WARN, "geometry%x->Release();\n", (int)this);
					_RPT1(_CRT_WARN, "geometry%x = nullptr;\n", (int)this);
#endif
				}
			}

			ID2D1PathGeometry* native()
			{
				return geometry_;
			}

			int32_t MP_STDCALL Open(GmpiDrawing_API::IMpGeometrySink** geometrySink) override;
			void MP_STDCALL GetFactory(GmpiDrawing_API::IMpFactory** factory) override
			{
				//		native_->GetFactory((ID2D1Factory**)factory);
			}

			int32_t MP_STDCALL StrokeContainsPoint(GmpiDrawing_API::MP1_POINT point, float strokeWidth, GmpiDrawing_API::IMpStrokeStyle* strokeStyle, const GmpiDrawing_API::MP1_MATRIX_3X2* worldTransform, bool* returnContains) override
			{
				BOOL result = FALSE;
				geometry_->StrokeContainsPoint(*(D2D1_POINT_2F*)&point, strokeWidth, toNative(strokeStyle), (const D2D1_MATRIX_3X2_F *)worldTransform, &result);
				*returnContains = result == TRUE;

				return gmpi::MP_OK;
			}
			int32_t MP_STDCALL FillContainsPoint(GmpiDrawing_API::MP1_POINT point, const GmpiDrawing_API::MP1_MATRIX_3X2* worldTransform, bool* returnContains) override
			{
				BOOL result = FALSE;
				geometry_->FillContainsPoint(*(D2D1_POINT_2F*)&point, (const D2D1_MATRIX_3X2_F *)worldTransform, &result);
				*returnContains = result == TRUE;

				return gmpi::MP_OK;
			}
			int32_t MP_STDCALL GetWidenedBounds(float strokeWidth, GmpiDrawing_API::IMpStrokeStyle* strokeStyle, const GmpiDrawing_API::MP1_MATRIX_3X2* worldTransform, GmpiDrawing_API::MP1_RECT* returnBounds) override
			{
				geometry_->GetWidenedBounds(strokeWidth, toNative(strokeStyle), (const D2D1_MATRIX_3X2_F *)worldTransform, (D2D_RECT_F*)returnBounds);
				return gmpi::MP_OK;
			}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_PATHGEOMETRY_MPGUI, GmpiDrawing_API::IMpPathGeometry);
			GMPI_REFCOUNT;
		};


		class Factory : public GmpiDrawing_API::IMpFactory2
		{
			ID2D1Factory1* m_pDirect2dFactory;
			IDWriteFactory* writeFactory;
			IWICImagingFactory* pIWICFactory;
			std::vector<std::wstring> supportedFontFamiliesLowerCase;
			std::vector<std::string> supportedFontFamilies;
			std::map<std::wstring, std::wstring> GdiFontConversions;
			bool DX_support_sRGB;

		public:
			static std::wstring_convert<std::codecvt_utf8<wchar_t>> stringConverter; // cached, as constructor is super-slow.

#if	ENABLE_HDR_SUPPORT
			float whiteMult = 1.0f;
			bool isHdr() const
			{
				return whiteMult != 1.0f;
			}
#endif

			// for diagnostics only.
			auto getDirectWriteFactory()
			{
				return writeFactory;
			}
			auto getWicFactory()
			{
				return pIWICFactory;
			}
			auto getFactory()
			{
				return m_pDirect2dFactory;
			}

			void setSrgbSupport(bool s)
			{
				DX_support_sRGB = s;
			}
			
			GmpiDrawing_API::IMpBitmapPixels::PixelFormat getPlatformPixelFormat()
			{
				return DX_support_sRGB ? GmpiDrawing_API::IMpBitmapPixels::kBGRA_SRGB : GmpiDrawing_API::IMpBitmapPixels::kBGRA;
			}

			Factory();
			void Init(ID2D1Factory1* existingFactory = nullptr);
			~Factory();

			ID2D1Factory1* getD2dFactory()
			{
				return m_pDirect2dFactory;
			}
			std::wstring fontMatch(std::wstring fontName, GmpiDrawing_API::MP1_FONT_WEIGHT fontWeight, float fontSize);

			int32_t MP_STDCALL CreatePathGeometry(GmpiDrawing_API::IMpPathGeometry** pathGeometry) override;
			int32_t MP_STDCALL CreateTextFormat(const char* fontFamilyName, void* unused /* fontCollection */, GmpiDrawing_API::MP1_FONT_WEIGHT fontWeight, GmpiDrawing_API::MP1_FONT_STYLE fontStyle, GmpiDrawing_API::MP1_FONT_STRETCH fontStretch, float fontSize, void* unused2 /* localeName */, GmpiDrawing_API::IMpTextFormat** textFormat) override;
			int32_t MP_STDCALL CreateImage(int32_t width, int32_t height, GmpiDrawing_API::IMpBitmap** returnDiBitmap) override;
			int32_t MP_STDCALL LoadImageU(const char* utf8Uri, GmpiDrawing_API::IMpBitmap** returnDiBitmap) override;
			int32_t CreateStrokeStyle(const GmpiDrawing_API::MP1_STROKE_STYLE_PROPERTIES* strokeStyleProperties, float* dashes, int32_t dashesCount, GmpiDrawing_API::IMpStrokeStyle** returnValue) override
			{
				*returnValue = nullptr;

				ID2D1StrokeStyle* b = nullptr;

				auto hr = m_pDirect2dFactory->CreateStrokeStyle((const D2D1_STROKE_STYLE_PROPERTIES*) strokeStyleProperties, dashes, dashesCount, &b);

				if (hr == 0)
				{
					gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> wrapper;
					wrapper.Attach(new StrokeStyle(b, this));

					return wrapper->queryInterface(GmpiDrawing_API::SE_IID_STROKESTYLE_MPGUI, reinterpret_cast<void**>(returnValue));
				}

				return hr == 0 ? (gmpi::MP_OK) : (gmpi::MP_FAIL);
			}

			IWICBitmap* CreateDiBitmapFromNative(ID2D1Bitmap* D2D_Bitmap);

			// IMpFactory2
			int32_t MP_STDCALL GetFontFamilyName(int32_t fontIndex, gmpi::IString* returnString) override;

			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				*returnInterface = 0;
				if ( iid == GmpiDrawing_API::SE_IID_FACTORY2_MPGUI || iid == GmpiDrawing_API::SE_IID_FACTORY_MPGUI || iid == gmpi::MP_IID_UNKNOWN)
				{
					*returnInterface = reinterpret_cast<GmpiDrawing_API::IMpFactory2*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				return gmpi::MP_NOSUPPORT;
			}

			GMPI_REFCOUNT_NO_DELETE;
		};

		class GraphicsContext : public GmpiDrawing_API::IMpDeviceContext
		{
		protected:
			ID2D1DeviceContext* context_;

			Factory* factory;
			std::vector<GmpiDrawing_API::MP1_RECT> clipRectStack;
			std::wstring_convert<std::codecvt_utf8<wchar_t>>* stringConverter; // cached, as constructor is super-slow.
#if	ENABLE_HDR_SUPPORT
			float whiteMult = 1.0f; // cached for speed.
#endif
			void Init()
			{
				stringConverter = &(factory->stringConverter);
#if	ENABLE_HDR_SUPPORT
				whiteMult = factory->whiteMult;
#endif
			}

		public:
			GraphicsContext(ID2D1DeviceContext* deviceContext, Factory* pfactory) :
				context_(deviceContext)
				, factory(pfactory)
			{
				context_->AddRef();
				Init();
			}

			GraphicsContext(Factory* pfactory) :
				context_(nullptr)
				, factory(pfactory)
			{
				Init();
			}

			~GraphicsContext()
			{
				context_->Release();
			}

#if	ENABLE_HDR_SUPPORT
			bool isHdr() const
			{
				return factory->isHdr();
			}
#endif

			ID2D1DeviceContext* native()
			{
				return context_;
			}

			void MP_STDCALL GetFactory(GmpiDrawing_API::IMpFactory** pfactory) override
			{
				*pfactory = factory;
			}

			void MP_STDCALL DrawRectangle(const GmpiDrawing_API::MP1_RECT* rect, const GmpiDrawing_API::IMpBrush* brush, float strokeWidth, const GmpiDrawing_API::IMpStrokeStyle* strokeStyle) override
			{
				context_->DrawRectangle(D2D1::RectF(rect->left, rect->top, rect->right, rect->bottom), ((Brush*)brush)->nativeBrush(), strokeWidth, toNative(strokeStyle) );
			}

			void MP_STDCALL FillRectangle(const GmpiDrawing_API::MP1_RECT* rect, const GmpiDrawing_API::IMpBrush* brush) override
			{
				context_->FillRectangle((D2D1_RECT_F*)rect, (ID2D1Brush*)((Brush*)brush)->nativeBrush());
			}

			void MP_STDCALL Clear(const GmpiDrawing_API::MP1_COLOR* clearColor) override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT0(_CRT_WARN, "{\n");
				_RPT4(_CRT_WARN, "auto c = D2D1::ColorF(%.3ff, %.3ff, %.3ff, %.3ff);\n", clearColor->r, clearColor->g, clearColor->b, clearColor->a);
				_RPT0(_CRT_WARN, "context_->Clear(c);\n");
				_RPT0(_CRT_WARN, "}\n");
#endif
#if	ENABLE_HDR_SUPPORT
				const D2D1_COLOR_F c
				{
					clearColor->r * whiteMult,
					clearColor->g * whiteMult,
					clearColor->b * whiteMult,
					clearColor->a
				};
#else
				const D2D1_COLOR_F c
				{
					clearColor->r,
					clearColor->g,
					clearColor->b,
					clearColor->a
				};
#endif
				context_->Clear(&c);
			}

			void MP_STDCALL DrawLine(GmpiDrawing_API::MP1_POINT point0, GmpiDrawing_API::MP1_POINT point1, const GmpiDrawing_API::IMpBrush* brush, float strokeWidth, const GmpiDrawing_API::IMpStrokeStyle* strokeStyle) override
			{
				context_->DrawLine(*((D2D_POINT_2F*)&point0), *((D2D_POINT_2F*)&point1), ((Brush*)brush)->nativeBrush(), strokeWidth, toNative(strokeStyle));
			}

			void MP_STDCALL DrawGeometry(const GmpiDrawing_API::IMpPathGeometry* geometry, const GmpiDrawing_API::IMpBrush* brush, float strokeWidth = 1.0f, const GmpiDrawing_API::IMpStrokeStyle* strokeStyle = 0) override;

			void MP_STDCALL FillGeometry(const GmpiDrawing_API::IMpPathGeometry* geometry, const GmpiDrawing_API::IMpBrush* brush, const GmpiDrawing_API::IMpBrush* opacityBrush) override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT3(_CRT_WARN, "context_->FillGeometry(geometry%x, brush%x, nullptr);\n", (int)geometry, (int)brush);
#endif
				auto d2d_geometry = ((Geometry*)geometry)->geometry_;

				ID2D1Brush* opacityBrushNative;
				if (opacityBrush)
				{
					opacityBrushNative = ((Brush*)brush)->nativeBrush();
				}
				else
				{
					opacityBrushNative = nullptr;
				}

				context_->FillGeometry(d2d_geometry, ((Brush*)brush)->nativeBrush(), opacityBrushNative);
			}

			//void MP_STDCALL FillMesh(const GmpiDrawing_API::IMpMesh* mesh, const GmpiDrawing_API::IMpBrush* brush) override
			//{
			//	auto nativeMesh = ((Mesh*)mesh)->native();
			//	context_->FillMesh(nativeMesh, ((Brush*)brush)->nativeBrush());
			//}

			void MP_STDCALL DrawTextU(const char* utf8String, int32_t stringLength, const GmpiDrawing_API::IMpTextFormat* textFormat, const GmpiDrawing_API::MP1_RECT* layoutRect, const GmpiDrawing_API::IMpBrush* brush, int32_t flags) override;

			//	void MP_STDCALL DrawBitmap( GmpiDrawing_API::IMpBitmap* mpBitmap, GmpiDrawing::Rect destinationRectangle, float opacity, int32_t interpolationMode, GmpiDrawing::Rect sourceRectangle) override
			void MP_STDCALL DrawBitmap(const GmpiDrawing_API::IMpBitmap* mpBitmap, const GmpiDrawing_API::MP1_RECT* destinationRectangle, float opacity, /* MP1_BITMAP_INTERPOLATION_MODE*/ int32_t interpolationMode, const GmpiDrawing_API::MP1_RECT* sourceRectangle) override
			{
				auto bm = (Bitmap*) mpBitmap;
				if (!bm)
					return;

				auto bitmap = bm->GetNativeBitmap(context_);
				if (!bitmap)
					return;

				context_->DrawBitmap(
					bitmap,
					(D2D1_RECT_F*)destinationRectangle,
					opacity,
					(D2D1_BITMAP_INTERPOLATION_MODE)interpolationMode,
					(D2D1_RECT_F*)sourceRectangle
				);
			}

			void MP_STDCALL SetTransform(const GmpiDrawing_API::MP1_MATRIX_3X2* transform) override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT0(_CRT_WARN, "{\n");
				_RPT4(_CRT_WARN, "auto t = D2D1::Matrix3x2F(%.3f, %.3f, %.3f, %.3f, ", transform->_11, transform->_12, transform->_21, transform->_22);
				_RPT4(_CRT_WARN, "%.3f, %.3f);\n", transform->_31, transform->_32);
				_RPT0(_CRT_WARN, "context_->SetTransform(t);\n");
				_RPT0(_CRT_WARN, "}\n");
#endif
				context_->SetTransform(reinterpret_cast<const D2D1_MATRIX_3X2_F*>(transform));
			}

			void MP_STDCALL GetTransform(GmpiDrawing_API::MP1_MATRIX_3X2* transform) override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT0(_CRT_WARN, "{\n");
				_RPT0(_CRT_WARN, "D2D1_MATRIX_3X2_F t;\n");
				_RPT0(_CRT_WARN, "context_->GetTransform(&t);\n");
				_RPT0(_CRT_WARN, "}\n");
#endif
				context_->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
			}

			int32_t MP_STDCALL CreateSolidColorBrush(const GmpiDrawing_API::MP1_COLOR* color, GmpiDrawing_API::IMpSolidColorBrush **solidColorBrush) override;

			int32_t MP_STDCALL CreateGradientStopCollection(const GmpiDrawing_API::MP1_GRADIENT_STOP* gradientStops, uint32_t gradientStopsCount, /* GmpiDrawing_API::MP1_GAMMA colorInterpolationGamma, GmpiDrawing_API::MP1_EXTEND_MODE extendMode,*/ GmpiDrawing_API::IMpGradientStopCollection** gradientStopCollection) override;

			template <typename T>
			int32_t make_wrapped(gmpi::IMpUnknown* object, const gmpi::MpGuid& iid, T** returnObject)
			{
				*returnObject = nullptr;
				gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> b2;
				b2.Attach(object);
				return b2->queryInterface(iid, reinterpret_cast<void**>(returnObject));
			};

			int32_t MP_STDCALL CreateLinearGradientBrush(const GmpiDrawing_API::MP1_LINEAR_GRADIENT_BRUSH_PROPERTIES* linearGradientBrushProperties, const GmpiDrawing_API::MP1_BRUSH_PROPERTIES* brushProperties, const  GmpiDrawing_API::IMpGradientStopCollection* gradientStopCollection, GmpiDrawing_API::IMpLinearGradientBrush** linearGradientBrush) override
			{
				//*linearGradientBrush = nullptr;
				//gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> b2;
				//b2.Attach(new LinearGradientBrush(factory, context_, linearGradientBrushProperties, brushProperties, gradientStopCollection));
				//return b2->queryInterface(GmpiDrawing_API::SE_IID_LINEARGRADIENTBRUSH_MPGUI, reinterpret_cast<void **>(linearGradientBrush));

				return make_wrapped(
					new LinearGradientBrush(factory, context_, linearGradientBrushProperties, brushProperties, gradientStopCollection),
					GmpiDrawing_API::SE_IID_LINEARGRADIENTBRUSH_MPGUI,
					linearGradientBrush);
			}

			int32_t MP_STDCALL CreateBitmapBrush(const GmpiDrawing_API::IMpBitmap* bitmap, const GmpiDrawing_API::MP1_BITMAP_BRUSH_PROPERTIES* bitmapBrushProperties, const GmpiDrawing_API::MP1_BRUSH_PROPERTIES* brushProperties, GmpiDrawing_API::IMpBitmapBrush** returnBrush) override
			{
				*returnBrush = nullptr;
				gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> b2;
				b2.Attach(new BitmapBrush(factory, context_, bitmap, bitmapBrushProperties, brushProperties));
				return b2->queryInterface(GmpiDrawing_API::SE_IID_BITMAPBRUSH_MPGUI, reinterpret_cast<void **>(returnBrush));
			}
			int32_t MP_STDCALL CreateRadialGradientBrush(const GmpiDrawing_API::MP1_RADIAL_GRADIENT_BRUSH_PROPERTIES* radialGradientBrushProperties, const GmpiDrawing_API::MP1_BRUSH_PROPERTIES* brushProperties, const GmpiDrawing_API::IMpGradientStopCollection* gradientStopCollection, GmpiDrawing_API::IMpRadialGradientBrush** radialGradientBrush) override
			{
				*radialGradientBrush = nullptr;
				gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> b2;
				b2.Attach(new RadialGradientBrush(factory, context_, radialGradientBrushProperties, brushProperties, gradientStopCollection));
				return b2->queryInterface(GmpiDrawing_API::SE_IID_RADIALGRADIENTBRUSH_MPGUI, reinterpret_cast<void **>(radialGradientBrush));
			}

			int32_t MP_STDCALL CreateCompatibleRenderTarget(const GmpiDrawing_API::MP1_SIZE* desiredSize, GmpiDrawing_API::IMpBitmapRenderTarget** bitmapRenderTarget) override;

			void MP_STDCALL DrawRoundedRectangle(const GmpiDrawing_API::MP1_ROUNDED_RECT* roundedRect, const GmpiDrawing_API::IMpBrush* brush, float strokeWidth, const GmpiDrawing_API::IMpStrokeStyle* strokeStyle) override
			{
				context_->DrawRoundedRectangle((D2D1_ROUNDED_RECT*)roundedRect, (ID2D1Brush*)((Brush*)brush)->nativeBrush(), (FLOAT)strokeWidth, toNative(strokeStyle));
			}

//			int32_t MP_STDCALL CreateMesh(GmpiDrawing_API::IMpMesh** returnObject) override;

			void MP_STDCALL FillRoundedRectangle(const GmpiDrawing_API::MP1_ROUNDED_RECT* roundedRect, const GmpiDrawing_API::IMpBrush* brush) override
			{
				context_->FillRoundedRectangle((D2D1_ROUNDED_RECT*)roundedRect, (ID2D1Brush*)((Brush*)brush)->nativeBrush());
			}

			void MP_STDCALL DrawEllipse(const GmpiDrawing_API::MP1_ELLIPSE* ellipse, const GmpiDrawing_API::IMpBrush* brush, float strokeWidth, const GmpiDrawing_API::IMpStrokeStyle* strokeStyle) override
			{
				context_->DrawEllipse((D2D1_ELLIPSE*)ellipse, (ID2D1Brush*)((Brush*)brush)->nativeBrush(), (FLOAT)strokeWidth, toNative(strokeStyle));
			}

			void MP_STDCALL FillEllipse(const GmpiDrawing_API::MP1_ELLIPSE* ellipse, const GmpiDrawing_API::IMpBrush* brush) override
			{
				context_->FillEllipse((D2D1_ELLIPSE*)ellipse, (ID2D1Brush*)((Brush*)brush)->nativeBrush());
			}

			void MP_STDCALL PushAxisAlignedClip(const GmpiDrawing_API::MP1_RECT* clipRect/*, GmpiDrawing_API::MP1_ANTIALIAS_MODE antialiasMode*/) override;

			void MP_STDCALL PopAxisAlignedClip() override
			{
//				_RPT0(_CRT_WARN, "                 PopAxisAlignedClip()\n");
#ifdef LOG_DIRECTX_CALLS
				_RPT0(_CRT_WARN, "context_->PopAxisAlignedClip();\n");
#endif
				context_->PopAxisAlignedClip();
				clipRectStack.pop_back();
			}

			void MP_STDCALL GetAxisAlignedClip(GmpiDrawing_API::MP1_RECT* returnClipRect) override;

			void MP_STDCALL BeginDraw() override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT0(_CRT_WARN, "\n\n// ==================================================\n");
				_RPT0(_CRT_WARN, "context_->BeginDraw();\n");
#endif
				context_->BeginDraw();
			}

			int32_t MP_STDCALL EndDraw() override
			{
#ifdef LOG_DIRECTX_CALLS
				_RPT0(_CRT_WARN, "context_->EndDraw();\n");
#endif
				auto hr = context_->EndDraw();

				return hr == S_OK ? (gmpi::MP_OK) : (gmpi::MP_FAIL);
			}

//			int32_t MP_STDCALL GetUpdateRegion(GmpiDrawing_API::IUpdateRegion** returnUpdateRegion) override;

			//	void MP_STDCALL InsetNewMethodHere(){}

			bool SupportSRGB()
			{
				return factory->getPlatformPixelFormat() == GmpiDrawing_API::IMpBitmapPixels::kBGRA_SRGB;
			}

			GMPI_QUERYINTERFACE1(GmpiDrawing_API::SE_IID_DEVICECONTEXT_MPGUI, GmpiDrawing_API::IMpDeviceContext);
			GMPI_REFCOUNT_NO_DELETE;
		};

		class GraphicsContext2 : public GraphicsContext, public GmpiDrawing_API::IMpDeviceContextExt
		{
		public:
			GraphicsContext2(ID2D1DeviceContext* deviceContext, Factory* pfactory) : GraphicsContext(deviceContext, pfactory) {}
			GraphicsContext2(Factory* pfactory) : GraphicsContext(pfactory) {}

			int32_t MP_STDCALL CreateBitmapRenderTarget(GmpiDrawing_API::MP1_SIZE_L desiredSize, bool enableLockPixels, GmpiDrawing_API::IMpBitmapRenderTarget** returnObject) override;

			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				*returnInterface = 0;
				if (iid == GmpiDrawing_API::SE_IID_DEVICECONTEXT_MPGUI || iid == MP_IID_UNKNOWN)
				{
					*returnInterface = static_cast<GmpiDrawing_API::IMpDeviceContext*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				if (iid == GmpiDrawing_API::IMpDeviceContextExt::guid)
				{
					*returnInterface = static_cast<GmpiDrawing_API::IMpDeviceContextExt*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				return MP_NOSUPPORT;
			}

			GMPI_REFCOUNT_NO_DELETE;
		};

		class BitmapRenderTarget : public GraphicsContext
		{
			ID2D1BitmapRenderTarget* gpuBitmapRenderTarget = {};

			IWICBitmap* wicBitmap{};
			ID2D1RenderTarget* wikBitmapRenderTarget{};

		public:
#if 0
			// Create on GPU only
			BitmapRenderTarget(GraphicsContext* g, const GmpiDrawing_API::MP1_SIZE* desiredSize, Factory* pfactory) :
				GraphicsContext(pfactory)
			{
#if 1
				// Create a render target on the GPU. Not modifyable by CPU.
				/* auto hr = */ g->native()->CreateCompatibleRenderTarget(*(D2D1_SIZE_F*)desiredSize, &gpuBitmapRenderTarget);
				gpuBitmapRenderTarget->QueryInterface(IID_ID2D1DeviceContext, (void**)&context_);
#else
				// Create a WIC render target. Modifyable by CPU (lock pixels).

				// First create the WIC bitmap
				D2D1_RENDER_TARGET_PROPERTIES props{
					D2D1_RENDER_TARGET_TYPE_DEFAULT,
					{DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
					{},
					{},
					D2D1_RENDER_TARGET_USAGE_NONE,
					D2D1_FEATURE_LEVEL_DEFAULT
				};

				auto hr = pfactory->getWicFactory()->CreateBitmap((UINT)desiredSize->width, (UINT)desiredSize->height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &wicBitmap);
				pfactory->getD2dFactory()->CreateWicBitmapRenderTarget(wicBitmap, props, &wikBitmapRenderTarget);
				wikBitmapRenderTarget->QueryInterface(IID_ID2D1DeviceContext, (void**)&context_);
#endif

				clipRectStack.push_back({ 0, 0, desiredSize->width, desiredSize->height });
			}
#endif
			// Create on GPU only
			BitmapRenderTarget(GraphicsContext* g, GmpiDrawing_API::MP1_SIZE desiredSize,Factory* pfactory, bool enableLockPixels = false) :
				GraphicsContext(pfactory)
			{
				if (enableLockPixels)
				{
					// Create a WIC render target. Modifyable by CPU (lock pixels). More expensive.

					// First create the WIC bitmap
					D2D1_RENDER_TARGET_PROPERTIES props{
						D2D1_RENDER_TARGET_TYPE_DEFAULT,
						{DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED },
						{},
						{},
						D2D1_RENDER_TARGET_USAGE_NONE,
						D2D1_FEATURE_LEVEL_DEFAULT
					};

					[[maybe_unused]] auto hr = pfactory->getWicFactory()->CreateBitmap((UINT)desiredSize.width, (UINT)desiredSize.height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &wicBitmap);
					pfactory->getD2dFactory()->CreateWicBitmapRenderTarget(wicBitmap, props, &wikBitmapRenderTarget);
					wikBitmapRenderTarget->QueryInterface(IID_ID2D1DeviceContext, (void**)&context_);
				}
				else
				{
					// Create a render target on the GPU. Not modifyable by CPU.
					/* auto hr = */ g->native()->CreateCompatibleRenderTarget(*(D2D1_SIZE_F*)&desiredSize, &gpuBitmapRenderTarget);
					gpuBitmapRenderTarget->QueryInterface(IID_ID2D1DeviceContext, (void**)&context_);
				}

				clipRectStack.push_back({ 0, 0, desiredSize.width, desiredSize.height });
			}

			~BitmapRenderTarget()
			{
				if (gpuBitmapRenderTarget)
					gpuBitmapRenderTarget->Release();

				if (wikBitmapRenderTarget)
					wikBitmapRenderTarget->Release();
			}

			virtual int32_t MP_STDCALL GetBitmap(GmpiDrawing_API::IMpBitmap** returnBitmap);

			int32_t MP_STDCALL queryInterface(const gmpi::MpGuid& iid, void** returnInterface) override
			{
				*returnInterface = 0;
				if (iid == GmpiDrawing_API::SE_IID_BITMAP_RENDERTARGET_MPGUI)
				{
					// non-standard. Forcing this class (which has the correct vtable) to pretend it's the emulated interface.
					*returnInterface = reinterpret_cast<GmpiDrawing_API::IMpBitmapRenderTarget*>(this);
					addRef();
					return gmpi::MP_OK;
				}
				else
				{
					return GraphicsContext::queryInterface(iid, returnInterface);
				}
			}

			GMPI_REFCOUNT;
		};

		// Direct2D context tailored to devices without sRGB high-color support. i.e. Windows 7.
		class GraphicsContext_Win7 : public GraphicsContext
		{
		public:

			GraphicsContext_Win7(ID2D1DeviceContext* context, Factory* pfactory) :
				GraphicsContext(context, pfactory)
			{}

			int32_t MP_STDCALL CreateSolidColorBrush(const GmpiDrawing_API::MP1_COLOR* color, GmpiDrawing_API::IMpSolidColorBrush **solidColorBrush) override
			{
				*solidColorBrush = nullptr;
				gmpi_sdk::mp_shared_ptr<gmpi::IMpUnknown> b;
				b.Attach(new SolidColorBrush_Win7(context_, color, factory));
				return b->queryInterface(GmpiDrawing_API::SE_IID_SOLIDCOLORBRUSH_MPGUI, reinterpret_cast<void **>(solidColorBrush));
			}

			int32_t MP_STDCALL CreateGradientStopCollection(const GmpiDrawing_API::MP1_GRADIENT_STOP* gradientStops, uint32_t gradientStopsCount, /* GmpiDrawing_API::MP1_GAMMA colorInterpolationGamma, GmpiDrawing_API::MP1_EXTEND_MODE extendMode,*/ GmpiDrawing_API::IMpGradientStopCollection** gradientStopCollection) override
			{
				// Adjust gradient gamma.
				std::vector<GmpiDrawing_API::MP1_GRADIENT_STOP> stops;
				stops.assign(gradientStopsCount, GmpiDrawing_API::MP1_GRADIENT_STOP());

				for(uint32_t i = 0 ; i < gradientStopsCount ; ++i)
				{
					auto& srce = gradientStops[i];
					auto& dest = stops[i];
					dest.position = srce.position;
					dest.color.a = srce.color.a;
					dest.color.r = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(srce.color.r));
					dest.color.g = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(srce.color.g));
					dest.color.b = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(srce.color.b));
				}

				return GraphicsContext::CreateGradientStopCollection(stops.data(), gradientStopsCount, gradientStopCollection);
			}

			void MP_STDCALL Clear(const GmpiDrawing_API::MP1_COLOR* clearColor) override
			{
				GmpiDrawing_API::MP1_COLOR color(*clearColor);
				color.r = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color.r));
				color.g = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color.g));
				color.b = se_sdk::FastGamma::pixelToNormalised(se_sdk::FastGamma::float_to_sRGB(color.b));
				context_->Clear((D2D1_COLOR_F*)&color);
			}
		};
	} // Namespace
} // Namespace