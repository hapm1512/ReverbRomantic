#include "ParameterLayout.h"

namespace Parameters
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    const auto addFloat = [&parameters] (const char* id,
                                         const char* name,
                                         float minimum,
                                         float maximum,
                                         float interval,
                                         float defaultValue,
                                         float skew = 1.0f)
    {
        parameters.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 },
            name,
            juce::NormalisableRange<float> { minimum, maximum, interval, skew },
            defaultValue));
    };

    // Hall remains the default to preserve the sound of existing sessions.
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::algorithm, 1 },
        "Algorithm",
        algorithmNames,
        0));

    addFloat (IDs::mix,        "Mix",        0.0f,    100.0f,   0.1f,   35.0f);
    addFloat (IDs::decay,      "Decay",      0.2f,     20.0f,  0.01f,    4.2f, 0.45f);
    addFloat (IDs::time,       "Time",       0.25f,     2.0f, 0.001f,    1.0f);
    addFloat (IDs::preDelay,   "Pre Delay",  0.0f,    250.0f,   0.1f,   38.0f);
    addFloat (IDs::size,       "Size",      25.0f,    200.0f,   0.1f,  110.0f);
    addFloat (IDs::width,      "Width",      0.0f,    200.0f,   0.1f,  125.0f);
    addFloat (IDs::warmth,     "Warmth",   -12.0f,     12.0f,   0.1f,    3.0f);
    addFloat (IDs::brightness, "Brightness",-12.0f,     12.0f,   0.1f,   -1.0f);
    addFloat (IDs::diffusion,  "Diffusion",  0.0f,    100.0f,   0.1f,   88.0f);
    addFloat (IDs::density,    "Density",    0.0f,    100.0f,   0.1f,   94.0f);
    addFloat (IDs::modulation, "Modulation", 0.0f,    100.0f,   0.1f,   22.0f);
    addFloat (IDs::bloom,      "Bloom",      0.0f,    100.0f,   0.1f,   45.0f);
    addFloat (IDs::ducking,    "Ducking",    0.0f,    100.0f,   0.1f,   18.0f);

    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { IDs::duckingEnable, 1 },
        "Ducking Enable",
        false));

    addFloat (IDs::duckThreshold, "Duck Threshold",
              -60.0f, 0.0f, 0.1f, -24.0f);
    addFloat (IDs::duckDepth, "Duck Depth",
              0.0f, 100.0f, 0.1f, 50.0f);
    addFloat (IDs::duckAttack, "Duck Attack",
              1.0f, 100.0f, 0.1f, 10.0f, 0.45f);
    addFloat (IDs::duckRelease, "Duck Release",
              50.0f, 1000.0f, 1.0f, 250.0f, 0.45f);
    addFloat (IDs::duckKnee, "Duck Knee",
              0.0f, 12.0f, 0.1f, 6.0f);

    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { IDs::shimmerEnable, 1 },
        "Shimmer Enable",
        false));

    addFloat (IDs::shimmerMix, "Shimmer Mix",
              0.0f, 100.0f, 0.1f, 35.0f);

    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::shimmerPitch, 1 },
        "Shimmer Pitch",
        juce::StringArray { "+7 semitones", "+12 semitones" },
        1));

    addFloat (IDs::shimmerFeedback, "Shimmer Feedback",
              0.0f, 70.0f, 0.1f, 30.0f);
    addFloat (IDs::shimmerTone, "Shimmer Tone",
              500.0f, 12000.0f, 1.0f, 6000.0f, 0.35f);

    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { IDs::sidechainEnable, 1 },
        "Sidechain Enable",
        false));

    addFloat (IDs::sidechainThreshold, "Sidechain Threshold",
              -60.0f, 0.0f, 0.1f, -24.0f);
    addFloat (IDs::sidechainAmount, "Sidechain Amount",
              0.0f, 100.0f, 0.1f, 50.0f);
    addFloat (IDs::sidechainAttack, "Sidechain Attack",
              0.1f, 200.0f, 0.1f, 8.0f, 0.45f);
    addFloat (IDs::sidechainRelease, "Sidechain Release",
              10.0f, 3000.0f, 1.0f, 220.0f, 0.40f);
    addFloat (IDs::sidechainHPF, "Sidechain HPF",
              20.0f, 1000.0f, 1.0f, 120.0f, 0.35f);
    addFloat (IDs::lowCut,     "Low Cut",   20.0f,   1000.0f,   1.0f,  120.0f, 0.35f);
    addFloat (IDs::highCut,    "High Cut",1000.0f,  20000.0f,   1.0f,12000.0f, 0.35f);
    addFloat (IDs::output,     "Output",    -24.0f,     12.0f,   0.1f,    0.0f);

    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { IDs::quality, 1 },
        "Quality",
        juce::StringArray { "Eco", "Standard", "High", "Ultra" },
        1));

    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { IDs::freeze, 1 }, "Freeze", false));

    addFloat (IDs::freezeMix,  "Freeze Mix",  0.0f, 100.0f, 0.1f, 100.0f);
    addFloat (IDs::freezeFade, "Freeze Fade", 20.0f, 500.0f, 1.0f, 80.0f, 0.55f);
    addFloat (IDs::freezeDamp, "Freeze Damp", 0.0f, 100.0f, 0.1f, 35.0f);

    parameters.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { IDs::bypass, 1 }, "Bypass", false));

    return { parameters.begin(), parameters.end() };
}
}
