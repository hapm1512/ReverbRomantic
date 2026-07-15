#pragma once

#include <JuceHeader.h>

namespace Parameters
{
namespace IDs
{
inline constexpr auto algorithm  = "algorithm";
inline constexpr auto engine     = "engine";
inline constexpr auto mix        = "mix";
inline constexpr auto decay      = "decay";
inline constexpr auto time       = "time";
inline constexpr auto preDelay   = "preDelay";
inline constexpr auto size       = "size";
inline constexpr auto width      = "width";
inline constexpr auto warmth     = "warmth";
inline constexpr auto brightness = "brightness";
inline constexpr auto diffusion  = "diffusion";
inline constexpr auto density    = "density";
inline constexpr auto modulation = "modulation";
inline constexpr auto bloom      = "bloom";
inline constexpr auto ducking    = "ducking";
inline constexpr auto duckingEnable    = "duckingEnable";
inline constexpr auto duckThreshold   = "duckThreshold";
inline constexpr auto duckDepth       = "duckDepth";
inline constexpr auto duckAttack      = "duckAttack";
inline constexpr auto duckRelease     = "duckRelease";
inline constexpr auto duckKnee        = "duckKnee";
inline constexpr auto shimmerEnable   = "shimmerEnable";
inline constexpr auto shimmerMix      = "shimmerMix";
inline constexpr auto shimmerPitch    = "shimmerPitch";
inline constexpr auto shimmerFeedback = "shimmerFeedback";
inline constexpr auto shimmerTone     = "shimmerTone";
inline constexpr auto sidechainEnable    = "sidechainEnable";
inline constexpr auto sidechainThreshold = "sidechainThreshold";
inline constexpr auto sidechainAmount    = "sidechainAmount";
inline constexpr auto sidechainAttack    = "sidechainAttack";
inline constexpr auto sidechainRelease   = "sidechainRelease";
inline constexpr auto sidechainHPF       = "sidechainHPF";
inline constexpr auto lowCut     = "lowCut";
inline constexpr auto highCut    = "highCut";
inline constexpr auto output     = "output";
inline constexpr auto quality    = "quality";
inline constexpr auto freeze     = "freeze";
inline constexpr auto freezeMix  = "freezeMix";
inline constexpr auto freezeFade = "freezeFade";
inline constexpr auto freezeDamp = "freezeDamp";
inline constexpr auto bypass     = "bypass";
}

inline const juce::StringArray algorithmNames
{
    "Romantic Hall",
    "Vocal Plate",
    "Studio Room",
    "Chamber",
    "Cathedral",
    "Ambient"
};

inline const juce::StringArray engineNames { "JUCE Core", "FDN16 Core" };

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}
