#pragma once
#include <JuceHeader.h>
namespace Parameters {
namespace IDs {
inline constexpr auto mix="mix"; inline constexpr auto decay="decay"; inline constexpr auto time="time";
inline constexpr auto preDelay="preDelay"; inline constexpr auto size="size"; inline constexpr auto width="width";
inline constexpr auto warmth="warmth"; inline constexpr auto brightness="brightness";
inline constexpr auto diffusion="diffusion"; inline constexpr auto density="density";
inline constexpr auto modulation="modulation"; inline constexpr auto bloom="bloom";
inline constexpr auto ducking="ducking"; inline constexpr auto lowCut="lowCut";
inline constexpr auto highCut="highCut"; inline constexpr auto output="output";
inline constexpr auto quality="quality"; inline constexpr auto freeze="freeze"; inline constexpr auto bypass="bypass";
}
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}
