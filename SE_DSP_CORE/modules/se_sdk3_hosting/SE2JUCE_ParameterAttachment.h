#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <span>
#include "RawConversions.h"
#include "IGuiHost2.h"
#include "it_enum_list.h"

/*
#include "SE2JUCE_ParameterAttachment.h"
*/

#ifdef _DEBUG
namespace SeParameters
{
    const char* getParameterName(int32_t param);
}
#endif

struct SeParameterAttachment : gmpi::IMpParameterObserver
{
    int32_t parameterHandle = -1;
    IGuiHost2* controller = {};

    SeParameterAttachment(IGuiHost2* pcontroller, int32_t pparameterHandle) :
        parameterHandle(pparameterHandle)
        , controller(pcontroller)
    {
//        controller->RegisterGui2(this);
    }

    virtual ~SeParameterAttachment()
    {
//        controller->UnRegisterGui2(this);
    }

//    void Init(gmpi::FieldType FieldId = gmpi::MP_FT_VALUE)
//    {
//        controller->initializeGui(this, parameterHandle, FieldId);
//    }

    GMPI_QUERYINTERFACE1(gmpi::MP_IID_PARAMETER_OBSERVER, gmpi::IMpParameterObserver);
    GMPI_REFCOUNT;
};

// same but with std::function callback
struct SeParameterAttachment2 : SeParameterAttachment
{
    std::function<void(const void* data, int32_t size)> onChanged;

    SeParameterAttachment2(
        IGuiHost2* pcontroller,
        int32_t pparameterHandle,
        std::function<void(const void* data, int32_t size)> pOnChanged
    ) :
        SeParameterAttachment(pcontroller, pparameterHandle)
        , onChanged(pOnChanged)
    {
    }

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
        if (/*parameterHandle == pparameterHandle &&*/ gmpi::MP_FT_VALUE == fieldId)
        {
            onChanged(data, size);
        }

        return gmpi::MP_OK;
    }
};

// same but generic
template< typename T>
struct SeParameterAttachment3 : SeParameterAttachment
{
    std::function<void(const T&)> onChanged;
    std::function<void(float)> onChangedNormalized;

    SeParameterAttachment3(
        IGuiHost2* pcontroller,
        int32_t pparameterHandle,
        std::function<void(const T&)> pOnChanged = {},
        std::function<void(float)> ponChangedNormalized = {}
    ) :
        SeParameterAttachment(pcontroller, pparameterHandle)
        , onChanged(pOnChanged)
        , onChangedNormalized(ponChangedNormalized)
    {
    }

    void setValue(const T&& value)
    {
        controller->setParameterValue(value, parameterHandle);
    }

    void setNormalized(float value)
    {
        controller->setParameterValue(value, parameterHandle, gmpi::FieldType::MP_FT_NORMALIZED);
    }

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
//        if (parameterHandle == pparameterHandle)
        {
            RawView raw(data, size);

            if (onChanged && gmpi::MP_FT_VALUE == fieldId)
            {
                onChanged((T)raw);
            }

            if(onChangedNormalized && gmpi::MP_FT_NORMALIZED == fieldId)
			{
				onChangedNormalized((float) raw);
			}
        }

        return gmpi::MP_OK;
    }
};

struct SeParameterAttachmentSlider : SeParameterAttachment
{
    juce::Slider& slider;

    SeParameterAttachmentSlider(IGuiHost2* pcontroller, juce::Slider& pslider, int32_t pparameterHandle) :
        SeParameterAttachment(pcontroller, pparameterHandle)
        , slider(pslider)
    {
        slider.onValueChange = [this] {
            controller->setParameterValue({ (float)slider.getValue() }, parameterHandle);
        };

        slider.onDragStart = [this] {
            controller->setParameterValue(
                { true },
                parameterHandle,
                gmpi::MP_FT_GRAB);
        };

        slider.onDragEnd = [this] {
            controller->setParameterValue(
                { false },
                parameterHandle,
                gmpi::MP_FT_GRAB);
        };
    }

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
        if (/*parameterHandle == pparameterHandle &&*/ gmpi::MP_FT_VALUE == fieldId && size == sizeof(float))
        {
            slider.setValue(RawToValue<float>(data, size));
        }

        return gmpi::MP_OK;
    }
};

// Attach JUCE button to a GMPI enum or int parameter
template <typename T = int32_t>
struct SeParameterAttachmentButton : SeParameterAttachment
{
    juce::Button& button;
    T enumVal = -1;
    T onVal = 1;
    T offVal = 0;

    SeParameterAttachmentButton(
        IGuiHost2* pcontroller
        , juce::Button& pbutton
        , int32_t pparameterHandle
        , T penumVal = 1
        , T poffVal = 0
        , T ponVal = 1
    )
        : SeParameterAttachment(pcontroller, pparameterHandle)
        , button(pbutton)
        , enumVal(penumVal)
        , onVal(ponVal)
        , offVal(poffVal)
    {
        /* no, this just returns defaults 0/10
        Min = (int32_t)controller->getParameterValue(pparameterHandle, gmpi::MP_FT_RANGE_LO);
        Max = (int32_t)controller->getParameterValue(pparameterHandle, gmpi::MP_FT_RANGE_HI);
        */

        button.onClick = [this] {
            if (enumVal > -1)
            {
                // we're assuming switch is wired to an enum parameter as part of a radio-group
                if (button.getToggleState())
                {
                    controller->setParameterValue(enumVal, parameterHandle, gmpi::MP_FT_VALUE);
                }
            }
            else
            {
                // we're assuming switch is wired to an enum parameter, where the first two values are (0 1)
                controller->setParameterValue({ button.getToggleState() ? onVal : offVal }, parameterHandle, gmpi::MP_FT_VALUE);
            }
        };
    }

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t /*pparameterHandle*/, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
        if (/*parameterHandle == pparameterHandle &&*/ gmpi::MP_FT_VALUE == fieldId && size == sizeof(int32_t))
        {
            const auto newVal = RawToValue<T>(data, size);
            if (enumVal > -1)
            {
                // we're assuming switch is wired to an enum parameter as part of a radio-group
                button.setToggleState(newVal == enumVal, juce::NotificationType::dontSendNotification);
            }
            else
            {
                // we're assuming switch is wired to an enum parameter, where the first two values are (0 1)
                button.setToggleState(newVal != offVal, juce::NotificationType::dontSendNotification);
            }
        }

        return gmpi::MP_OK;
    }
};

// Attach JUCE button to a GMPI bool parameter
struct SeParameterAttachmentBoolButton : SeParameterAttachment
{
    juce::Button& button;
    bool isInverted = false;

    SeParameterAttachmentBoolButton(
        IGuiHost2* pcontroller
        , juce::Button& pbutton
        , int32_t pparameterHandle
        , bool pisInverted = false
    )
        : SeParameterAttachment(pcontroller, pparameterHandle)
        , button(pbutton)
        , isInverted(pisInverted)
    {
        if (button.getClickingTogglesState())
        {
            // set up a handler to toggle the param
            button.onClick = [this] {
                UpdateParameter(button.getToggleState() != isInverted);
                };
        }
        else
        {
            // set up a handler to set the param only while mouse is down.
            button.onStateChange = [this]() {
                UpdateParameter((button.getState() == juce::Button::buttonDown) != isInverted);
		    };
        }
    }

	void UpdateParameter(bool isToggled)
	{
        controller->setParameterValue(isToggled ? 1.0f : 0.0f, parameterHandle, gmpi::MP_FT_NORMALIZED);
	}

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
        if (/*parameterHandle == pparameterHandle &&*/ gmpi::MP_FT_VALUE == fieldId && size == sizeof(bool))
        {
            const auto newVal = RawToValue<bool>(data, size);

            // _RPTN(_CRT_WARN, "setParameter %d\n", (int)newVal);

            // if the button is a toggle, we need to update the button
            if (button.getClickingTogglesState())
            {
                button.setToggleState(newVal != isInverted, juce::NotificationType::dontSendNotification);
            }
        }

        return gmpi::MP_OK;
    }
};

#if 0
// For Mode control
struct SeParameterAttachmentButtonWithConfirmation : SeParameterAttachmentButton
{
    AiLookAndFeel5 AiMasterLookandFeel5; // reset symbol and preset combo

    SeParameterAttachmentButtonWithConfirmation(
        IGuiHost2* pcontroller
        , juce::Button& pbutton
        , int32_t pparameterHandle
        , int32_t penumVal = 1
        , int32_t poffVal = 0
        , int32_t ponVal = 1
    );

//    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override;
};
#endif

#if 0
struct SeParameterAttachmentMeter : SeParameterAttachment
{
    std::function<void(void)> onChanged;
    float value = {};

    SeParameterAttachmentMeter(IGuiHost2* pcontroller, int32_t pparameterHandle, std::function<void(void)> pOnChanged) :
        SeParameterAttachment(pcontroller, pparameterHandle)
        , onChanged(pOnChanged)
    {
    }

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
        if (parameterHandle == pparameterHandle && gmpi::MP_FT_VALUE == fieldId && size == sizeof(float))
        {
            value = RawToValue<float>(data, size);
            onChanged();
        }

        return gmpi::MP_OK;
    }
};
#endif

// enables/disables Analze/Master button based on the state of two parameters
struct SeParameterAttachmentButtonDisabler : SeParameterAttachment
{
    std::function<void(bool)> callback;
    int32_t paramEnableHandle = -1;
    bool isMastering = false;
    bool shouldEnable = false;

    SeParameterAttachmentButtonDisabler(IGuiHost2* pcontroller, int32_t paramButton, int32_t paramEnable, std::function <void(bool)> pcallback) :
        SeParameterAttachment(pcontroller, paramButton)
        , callback(pcallback)
        , paramEnableHandle(paramEnable)
    {}

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
        if(gmpi::MP_FT_VALUE != fieldId)
            return gmpi::MP_OK;

        if (pparameterHandle == parameterHandle && size == sizeof(int32_t))
        {
            isMastering = 0 != RawToValue<int32_t>(data, size);
//            button.setEnabled(isMastering || shouldEnable);
            callback(isMastering || shouldEnable);
        }

        if (pparameterHandle == paramEnableHandle && size == sizeof(bool))
        {
            shouldEnable = RawToValue<bool>(data, size);
 //           button.setEnabled(isMastering || shouldEnable);
            callback(isMastering || shouldEnable);
        }

        return gmpi::MP_OK;
    }
};

// enable undo/redo buttons depending on bool parameter
struct SeParameterAttachmentButtonDisabler2 : SeParameterAttachment
{
    juce::Button& button;

    SeParameterAttachmentButtonDisabler2(IGuiHost2* pcontroller, juce::Button& pbutton, int32_t paramEnable) :
        SeParameterAttachment(pcontroller, paramEnable)
        , button(pbutton)
    {}

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
        if (gmpi::MP_FT_VALUE != fieldId)
            return gmpi::MP_OK;

        if (/*pparameterHandle == parameterHandle &&*/ size == sizeof(bool))
        {
            const auto enable = RawToValue<bool>(data, size);
            button.setEnabled(enable);

//            _RPTN(_CRT_WARN, "%s setEnabled(%d)\n", button.getName().getCharPointer(), (int)enable);
        }

        return gmpi::MP_OK;
    }
};

// handles a button that only changes color.
struct ButtonStateManager : SeParameterAttachment, public juce::MouseListener
{
    int32_t enumVal = -1;

    std::function<void(juce::Colour)> onChangedColour;
    bool pressed = false;
    bool hovered = false;
    bool on = false;

    juce::Colour color;

    ButtonStateManager(
        IGuiHost2* pcontroller,
        int32_t pparameterHandle,
        std::function<void(juce::Colour)> ponChangedColour,
        int32_t penumVal = -1
    ) :
        SeParameterAttachment(pcontroller, pparameterHandle)
        , onChangedColour(ponChangedColour)
        , enumVal(penumVal)
    {
    }

    void onChanged()
    {
        juce::Colour newColor;

        if (pressed)
            newColor = juce::Colours::white.withBrightness(0.8f);
        else
        {
            if (on)
            {
                newColor = juce::Colours::red;
            }
            else
            {
                if (hovered)
                    newColor = juce::Colours::white.darker();
                else
                    newColor = juce::Colours::grey;
            }
        }

        if(newColor != color)
        {
            color = newColor;
            onChangedColour(color);
        }
    }

    void onClicked()
    {
        on = !on; //toggle

        if (enumVal > -1)
        {
            // we're assuming switch is wired to an enum parameter as part of a radio-group
            if (on)
            {
                controller->setParameterValue(enumVal, parameterHandle, gmpi::MP_FT_VALUE);
            }
        }
        else
        {
            // we're assuming switch is wired to an enum parameter, where the first two values are (0 1)
            controller->setParameterValue({ on ? (int32_t)1 : (int32_t)0 }, parameterHandle, gmpi::MP_FT_VALUE);
        }
    }

    // gmpi::IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t pparameterHandle, int32_t fieldId, int32_t /*voice*/, const void* data, int32_t size) override
    {
        if (/*parameterHandle == pparameterHandle &&*/ gmpi::MP_FT_VALUE == fieldId && size == sizeof(int32_t))
        {
            if (enumVal > -1)
            {
                // we're assuming switch is wired to an enum parameter as part of a radio-group
                on = RawToValue<int32_t>(data, size) == enumVal;
            }
            else
            {
                // we're assuming switch is wired to an enum parameter, where the first two values are (0 1)
                on = RawToValue<int32_t>(data, size) != 0;
            }

            onChanged();
        }

        return gmpi::MP_OK;
    }

    // juce::MouseListener
    void mouseEnter(const juce::MouseEvent& /*event*/) override
    {
        hovered = true;
        onChanged();
    }
    void mouseExit(const juce::MouseEvent& /*event*/) override
    {
        hovered = false;
        onChanged();
    }

    void mouseDown(const juce::MouseEvent& /*event*/) override
    {
        // capture?

        pressed = true;
        onChanged();
    }

    void mouseUp(const juce::MouseEvent& /*event*/) override
    {
        pressed = false;

        onClicked();
    }
};

template<typename T>
struct Pin
{
    T value = {};
    std::function<void(const T&)> onChanged = {};

    void setValue(T newValue)
    {
        if (value == newValue)
            return;

        value = newValue;

        if (onChanged)
            onChanged(value);
    }
};

struct HasParameterAttachments : gmpi::IMpParameterObserver
{
private:
	std::unordered_map<int32_t, std::vector< std::unique_ptr<SeParameterAttachment> > > parameterAttachments;

public:
    class IGuiHost2& controller;

    HasParameterAttachments(IGuiHost2& controller) : controller(controller)
    {
        controller.RegisterGui2(this);
    }
    
    ~HasParameterAttachments()
    {
        controller.UnRegisterGui2(this);
    }

    template<typename T>
    void attach(Pin<T>& pin, int32_t handle)
    {
        parameterAttachments[handle].push_back(std::make_unique<SeParameterAttachment3<T> >(
            &controller, handle,
            [&pin](T value) -> void {
                pin.setValue(value);
            }));
    }

    void attach(juce::Slider& slider, int32_t handle)
    {
        // set slider range from parameter
        const auto minreal = (float)controller.getParameterValue(handle, gmpi::MP_FT_RANGE_LO);
        const auto maxreal = (float)controller.getParameterValue(handle, gmpi::MP_FT_RANGE_HI);
        slider.setRange((std::min)(minreal, maxreal), (std::max)(minreal, maxreal));

        parameterAttachments[handle].push_back(
            std::make_unique<SeParameterAttachmentSlider>(
                &controller,
                slider,
                handle
            )
        );
    }

    // on/off button
    void attach(juce::Button& button, int32_t handle, bool isInverted = false)
    {
        parameterAttachments[handle].push_back(
            std::make_unique<SeParameterAttachmentBoolButton>(
                &controller,
                button,
                handle,
                isInverted
            )
        );
    }

    // one radio button
    void attach(juce::Button& button, int32_t handle, int enumValue)
    {
        button.setClickingTogglesState(true);

        parameterAttachments[handle].push_back(
            std::make_unique<SeParameterAttachmentButton<int32_t> >(
                &controller,
                button,
                handle,
                enumValue
            )
        );
    }

    void attachRadioButtons(int32_t handle, int32_t radioGroupId, std::span<juce::Button*> buttons)
    {
        it_enum_list it((std::wstring)controller.getParameterValue(handle, gmpi::FieldType::MP_FT_ENUM_LIST));

        int i = 0;
        for (it.First(); !it.IsDone(); it.Next())
        {
            auto e = it.CurrentItem();
            auto& button = *buttons[i++];
            const juce::String name(e->text.c_str());

            button.setButtonText(name);
            button.setRadioGroupId(radioGroupId, juce::dontSendNotification);
            attach(button, handle, e->value);
        }
    }

	void addAttachment(std::unique_ptr<SeParameterAttachment> attachment)
	{
		parameterAttachments[attachment->parameterHandle].push_back(std::move(attachment));
	}

    void initAttachments()
    {
        for (auto& param : parameterAttachments)
        {
			auto handle = param.first;
            controller.initializeGui(this, handle, gmpi::MP_FT_NORMALIZED);
            controller.initializeGui(this, handle, gmpi::MP_FT_VALUE);
        }
    }

    // IMpParameterObserver
    int32_t MP_STDCALL setParameter(int32_t parameterHandle, int32_t fieldId, int32_t voice, const void* data, int32_t size) override
    {
        if (auto it = parameterAttachments.find(parameterHandle); it != parameterAttachments.end())
        {
            for (auto& attachment : it->second)
            {
#if 0 // def _DEBUG
                _RPTN(0, "setParameter: %d %s\n", parameterHandle, SeParameters::getParameterName(parameterHandle));
#endif
                attachment->setParameter(parameterHandle, fieldId, voice, data, size);
            }
        }

		return gmpi::MP_OK;
    }

    GMPI_QUERYINTERFACE1(gmpi::MP_IID_PARAMETER_OBSERVER, gmpi::IMpParameterObserver);
    GMPI_REFCOUNT;
};
