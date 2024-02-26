#pragma once
#include <JuceHeader.h>

inline juce::String displayPercentage(float value, int /*maxLen*/)
{
    // whole numbers with percentage sign. see also customizeParameter() in OptimusProcessor.cpp
    const auto txt = std::to_string(static_cast<int>(value)) + " %";
    return juce::String(txt);
}

inline juce::String displayPercentageD(double value)
{
    // whole numbers with percentage sign. see also customizeParameter() in OptimusProcessor.cpp
    const auto txt = std::to_string(static_cast<int>(value)) + " %";
    return juce::String(txt);
}

inline juce::String displayPercentageWithSign(float value, int /*maxLen*/)
{
    // whole numbers with percentage sign and +/-. see also customizeParameter() in OptimusProcessor.cpp
    const auto txt = std::string(value < 0.0f ? "" : "+") + std::to_string(static_cast<int>(value)) + " %";
    return juce::String(txt);
}

inline juce::String displayPercentageWithSignD(double value)
{
    // whole numbers with percentage sign and +/-. see also customizeParameter() in OptimusProcessor.cpp
    const auto txt = std::string(value < 0.0f ? "" : "+") + std::to_string(static_cast<int>(value)) + " %";
    return juce::String(txt);
}

inline juce::String displaydB(float value, int /*maxLen*/)
{
    // real number with a 1 decimal places
    char txt[12];

#if defined(_MSC_VER)
    sprintf_s(txt, "%2.1f dB", value);
#else
    sprintf(txt, "%2.1f dB", value);
#endif

    return juce::String(txt);
}

inline juce::String displaydBD(double value)
{
    // real number with a 1 decimal places
    char txt[12];

#if defined(_MSC_VER)
    sprintf_s(txt, "%2.1f dB", value);
#else
    sprintf(txt, "%2.1f dB", value);
#endif

    return juce::String(txt);
}

inline juce::String displayRealNumber(float value, int /*maxLen*/)
{
    // real number with a sensible number of decimal places
    char txt[12];
    if (fabsf(value) < 20.f)
    {
#if defined(_MSC_VER)
        sprintf_s(txt, "%2.1f", value);
#else
        sprintf(txt, "%2.1f", value);
#endif
    }
    else
    {
#if defined(_MSC_VER)
        sprintf_s(txt, "%3.0f", value);
#else
        sprintf(txt, "%3.0f", value);
#endif
    }

    return juce::String(txt);
}

inline juce::String displayRealNumberD(double value)
{
    // real number with a sensible number of decimal places
    char txt[12];
    if (fabs(value) < 20.)
    {
#if defined(_MSC_VER)
        sprintf_s(txt, "%2.1f", value);
#else
        sprintf(txt, "%2.1f", value);
#endif
    }
    else
    {
#if defined(_MSC_VER)
        sprintf_s(txt, "%3.0f", value);
#else
        sprintf(txt, "%3.0f", value);
#endif
    }

    return juce::String(txt);
}