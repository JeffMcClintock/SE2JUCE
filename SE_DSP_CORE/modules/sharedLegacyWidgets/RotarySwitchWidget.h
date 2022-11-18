#pragma once
#include "math.h"
#include "../shared/ImageMetadata.h"
#include "../shared/ImageCache.h"
#include "Widget.h"
#include "../shared/unicode_conversion.h"
#include "../shared/it_enum_list.h"

class RotarySwitchWidget : public BitmapWidget, public FontCacheClient
{
	GmpiDrawing::TextFormat_readonly dtextFormat;
	FontMetadata* typeface_;
	std::wstring itemList_;
	bool drawLabels_;
	GmpiDrawing::Size drawOffset;
	int CountPerSide;
	float yInc;
	int itemCount;
	float startAngle;
	float AngleInc;
	float radius;
	bool bothSides;

	int currentQuantizedValue;

public:
	std::function<void(int32_t)> OnChangedEvent;

	RotarySwitchWidget() {}

	RotarySwitchWidget(gmpi::IMpUnknown* host, const char* imageFile) :BitmapWidget(host, imageFile)
		, drawLabels_(true)
		, drawOffset(0, 0)
		,currentQuantizedValue(0)
	{
	}

	void UpdateNormalised()
	{
		it_enum_list itr(itemList_);
		itr.FindValue(currentQuantizedValue);
		if (!itr.IsDone())
		{
			float norm = ((float)(*itr)->index) / (std::max)(1.0f, itemCount - 1.f);
			//_RPT2(_CRT_WARN, "%f -> %f\n", GetNormalised(), norm);
			SetNormalised(norm);
		}
	}

	void setValueInt(int v)
	{
		if (v != currentQuantizedValue)
		{
			//_RPT1(_CRT_WARN, "setValueInt %d\n", v);
			currentQuantizedValue = v;
			UpdateNormalised();
		}
	}

	void init(std::wstring itemList, bool drawLabels)
	{
		itemList_ = itemList;
		drawLabels_ = drawLabels;

		dtextFormat = GetTextFormat(patchMemoryHost_, guiHost_, "switch_label", &typeface_);

		// SE 1.1 used font 'size', SE 1.4 used pixelHeight_, which resulted in different (more spaced-out) results.
		float fontSize = static_cast<float>(typeface_->verticalSnapBackwardCompatibilityMode ? typeface_->pixelHeight_ : typeface_->size_);

		it_enum_list itr(itemList);
		
		if (itemCount != itr.size())
		{
			itemCount = itr.size();

			radius = 35.0f;
			yInc = fontSize;

			float height_of_stack = (itemCount - 1.0f) * yInc;
			float highest_item = height_of_stack / 2.0f;
			CountPerSide = itemCount;
			bothSides = false;

			if (highest_item > radius) // then need to use both sides
			{
				bothSides = true;
				CountPerSide = (int)floorf(itemCount * 0.5f);
				height_of_stack = (CountPerSide - 1.0f) * yInc;
				highest_item = height_of_stack / 2.0f; // - radius;

				if (highest_item > radius - yInc) // then too many to fit (expand radius)
				{
					radius = highest_item + yInc / 2.0f;
				}
			}

			float natural_y_inc = (2.0f * radius) / CountPerSide;

			if (natural_y_inc > yInc)
			{
				yInc = natural_y_inc;
				height_of_stack = (CountPerSide - 1.0f) * yInc;
				highest_item = height_of_stack / 2.0f; // - radius;
			}

			if (bothSides)
			{
				// how may 'ticks' between 3 oclock and 9 oclock
				float count_items_lower_half = (float)CountPerSide;

				if (CountPerSide * 2.0f != itemCount) // additional tick at center bottom?
				{
					count_items_lower_half += 1.0f;
				}

				//					AngleInc = (PI + 2.0 * startAngle) / (itemCount - 1.0);
				AngleInc = (float)M_PI / count_items_lower_half;
				startAngle = -AngleInc * (CountPerSide - 1.0f) / 2.0f;
			}
			else
			{
				startAngle = -asin(highest_item / radius);// - PI / 2.0;
				AngleInc = -2.0f * startAngle / (CountPerSide - 1.0f);

				if (CountPerSide == 1.0f)
				{
					AngleInc = 0.0f; // handle div by zero
					startAngle = 0.0f;
				}
			}

			setRotarySwitch((int)itemCount, startAngle / (2.0f * (float)M_PI), (itemCount - 1.0f) * AngleInc / (2.0f * (float)M_PI));

			// As itemCount and rotary switch range changed, normalised will need to be updated. Else widget appears stuck at 3 oclock.
			UpdateNormalised();
		}
	}

	virtual void OnRender(GmpiDrawing::Graphics& dc) override
	{
		GmpiDrawing::Size os(drawOffset.width - position.left, drawOffset.height - position.top);
		
		GmpiDrawing::Rect r;
		DrawRotarySwitch(&dc, os, r);
	}

	virtual void onPointerMove(int32_t flags, GmpiDrawing_API::MP1_POINT point) override
	{
		BitmapWidget::onPointerMove(flags, point);

		if (dirty)
		{
			/* prone to small errors, better to get precise drawn-at value.
			int index = (int)(animationPosition * (float)itemCount + 0.5f / (float)itemCount);
			index = (std::min)(index, itemCount - 1);
			*/

			auto index = getRotaryCurrentStep();
			assert(index >= 0 && index < itemCount);

			it_enum_list itr(itemList_);
			itr.FindIndex(index);
			assert(!itr.IsDone());

			currentQuantizedValue = itr.CurrentItem()->value;
//			_RPT1(_CRT_WARN, "onPointerMove %d\n", currentQuantizedValue);
			OnChangedEvent(currentQuantizedValue);
		}
	}

	virtual GmpiDrawing::Size getSize() override
	{
		GmpiDrawing::Rect returnBoundRect;
		DrawRotarySwitch(0, GmpiDrawing::Size(0, 0), returnBoundRect);
		drawOffset.width = returnBoundRect.left;
		drawOffset.height = returnBoundRect.top;

		return GmpiDrawing::Size(returnBoundRect.getWidth(), returnBoundRect.getHeight());
	}

	virtual GmpiDrawing::Point getKnobCenter() override
	{
		if (!bitmap_.isNull())
		{
			return GmpiDrawing::Point(-drawOffset.width + bitmapMetadata_->frameSize.width / 2, -drawOffset.height + bitmapMetadata_->frameSize.height / 2);
		}
		else
		{
			return BitmapWidget::getKnobCenter();
		}
	}

	void DrawRotarySwitch(GmpiDrawing::Graphics* dc, GmpiDrawing::Size drawOffset, GmpiDrawing::Rect& returnBoundRect)
	{
		const int ROTARY_LABEL_CLEARANCE = 4;

		if (!bitmapMetadata_)
		{
			return;
		}

		// Lines and text.
		if (drawLabels_)
		{
			GmpiDrawing::SolidColorBrush brush;
			
			if(dc)
				brush = dc->CreateSolidColorBrush(typeface_->color_); //  GmpiDrawing::Color::White);

			it_enum_list itr(itemList_);

			GmpiDrawing::Size offset;
			offset.width = -drawOffset.width + bitmapMetadata_->frameSize.width / 2 ;
			offset.height = -drawOffset.height + bitmapMetadata_->frameSize.height / 2;

			GmpiDrawing::Size switch_size((float)bitmapMetadata_->frameSize.width, (float)bitmapMetadata_->frameSize.height);
			//?		bm->SetRect(CRect(-switch_size.cx * 0.5, -switch_size.cy * 0.5, switch_size.cx * 0.5, switch_size.cy * 0.5));
			float inner_radius = switch_size.width * 0.4f;
			int item_index = 0;

			for (itr.First(); !itr.IsDone(); itr.Next())
			{
				// add label
				auto txt = JmUnicodeConversions::WStringToUtf8(itr.CurrentItem()->text);
//				GmpiDrawing::Size text_size(50, typeface_->pixelHeight_);
				auto text_size = dtextFormat.GetTextExtentU(txt);

				float y;

				if (item_index < CountPerSide)
				{
					y = yInc * (item_index - (CountPerSide - 1.0f) / 2.0f);
				}
				else
				{
					if (item_index > itemCount - CountPerSide - 1.5f)
					{
						y = yInc * ((itemCount - item_index - 1.0f) - (CountPerSide - 1.0f) / 2.0f);
					}
					else
					{
						y = radius; // center bottom
					}
				}

				float x = sqrt(radius * radius - y*y);

				if (y >= radius)
					x = 0;

				if (bothSides && item_index > (itemCount - 1) / 2.0) // switch to other side
				{
					x = -x;
				}

				float angle = startAngle + item_index * AngleInc;
				//				_RPT3(_CRT_WARN, "angle %.2f (count %d/%d) ", angle * 180.0 / PI, (int) item_index,(int)itemCount );
				float ly = sin(angle) * radius;
				float lx = cos(angle) * radius;// - text_size.cy / 2;
											   //				_RPT2(_CRT_WARN, "at (%d,%d)\n", (int) lx,(int)ly );
				float x1 = lx;
				float y1 = ly;
				float x2 = x1 * inner_radius / radius;
				float y2 = y1 * inner_radius / radius;
				float x3 = 0.0;
				float y3 = 0.0;

				// truncate line if nesc
				if ((y1 < (y - 0.8) && y2 > y) || (y1 > (y + 0.8) && y2 < y))
				{
					x1 = x1 * (y / y1);
					y1 = y;
					// additional horiz line
					y3 = y;
					x3 = x;
				}

				// Draw line.
				if (dc)
				{
					const float penWidth = 1;

					auto factory = dc->GetFactory();
					auto geometry = factory.CreatePathGeometry();
					auto sink = geometry.Open();

					// ml->StartLine(x2, y2);
					GmpiDrawing::Point p(x2 + offset.width, y2 + offset.height);

					sink.BeginFigure(p);

					// ml->MoveTo(x1, y1);
					p.x = x1 + offset.width;
					p.y = y1 + offset.height;
					sink.AddLine(p);

					if (x3 != 0.0 && y3 != 0.0)
					{
						//ml->MoveTo(x3, y3);
						p.x = x3 + offset.width;
						p.y = y3 + offset.height;
						sink.AddLine(p);
					}

					sink.EndFigure(GmpiDrawing::FigureEnd::Open);
					sink.Close();
					dc->DrawGeometry(geometry, brush, penWidth);
				}

				// add lable
				if (x > -1 && x < 1) // then it is near center
				{
					if (y > 0)
					{
						y += ROTARY_LABEL_CLEARANCE;
					}
					else
					{
						y -= text_size.height / 2;// looks better without  + ROTARY_LABEL_CLEARANCE;
					}

					x -= text_size.width / 2; // center horiz
				}
				else
				{
					if (x > 0)
					{
						x += ROTARY_LABEL_CLEARANCE;
					}
					else
					{
						x -= ROTARY_LABEL_CLEARANCE + text_size.width;
					}
				}

				y -= text_size.height / 2; // center vert
				y = floor(y);

				// Draw text here.
				GmpiDrawing::Rect textRect;
				textRect.left = x + offset.width;
				textRect.top = y + offset.height;
				textRect.right = textRect.left + text_size.width + 1;
				textRect.bottom = textRect.top + text_size.height;

				if (dc)
				{
					dc->DrawTextU(txt, dtextFormat, textRect, brush);
				}
				else
				{
					returnBoundRect.left = (std::min)(returnBoundRect.left, textRect.left);
					returnBoundRect.right = (std::max)(returnBoundRect.right, textRect.right);
					returnBoundRect.top = (std::min)(returnBoundRect.top, textRect.top);
					returnBoundRect.bottom = (std::max)(returnBoundRect.bottom, textRect.bottom);
				}

				item_index++;
			}

			if (dc == nullptr)
			{
				returnBoundRect.bottom = ceil(returnBoundRect.bottom);
			}

			//	_RPT0(_CRT_WARN, "------------\n" );
			//bm->setRotarySwitch(itemCount, startAngle / (2.0 * M_PI), (itemCount - 1.0) * AngleInc / (2.0 * M_PI));
			//led_group->Normalise(); // center everything
		}

		if (!bitmap_.isNull())
		{
			//float x = (float)bitmapMetadata_->padding_left - bitmapMetadata_->frameSize.width * 0.5f + position.left;
			//float y = (float)bitmapMetadata_->padding_top - bitmapMetadata_->frameSize.height * 0.5f + position.top;

			GmpiDrawing::Rect knob_rect(0.f, (float)drawAt, (float)bitmapMetadata_->frameSize.width, (float)(drawAt + bitmapMetadata_->frameSize.height));
			GmpiDrawing::Size offset;
			offset.width = -drawOffset.width;
			offset.height = -drawOffset.height;

			if (dc)
			{
				dc->DrawBitmap(bitmap_, GmpiDrawing::Point(offset.width, offset.height), knob_rect);
			}
			else
			{
				returnBoundRect.left = (std::min)(returnBoundRect.left, offset.width);
				returnBoundRect.top = (std::min)(returnBoundRect.top, offset.height);
				returnBoundRect.right = (std::max)(returnBoundRect.right, offset.width + bitmapMetadata_->frameSize.width);
				returnBoundRect.bottom = (std::max)(returnBoundRect.bottom, offset.height + bitmapMetadata_->frameSize.height);
			}
		}
	}
};

