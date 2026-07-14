#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

class EarlyReflection
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setSize (float newSize) noexcept;
    void processStereo (float inputL, float inputR, float& outputL, float& outputR) noexcept;

private:
    static constexpr size_t numTaps = 32;

    float readInterpolated (const std::vector<float>& buffer, float delaySamples) const noexcept;
    void updateTapDelays() noexcept;

    std::vector<float> bufferL;
    std::vector<float> bufferR;

    std::array<float, numTaps> delayMs
    {
         4.3f,  5.9f,  7.7f,  9.8f, 12.1f, 14.7f, 17.4f, 20.6f,
        23.9f, 27.5f, 31.2f, 35.1f, 39.4f, 43.9f, 48.7f, 53.8f,
        59.2f, 64.9f, 70.8f, 77.1f, 83.7f, 90.6f, 97.8f,105.3f,
       113.1f,121.2f,129.6f,138.3f,147.3f,156.6f,166.2f,176.1f
    };

    std::array<float, numTaps> delaySamplesL {};
    std::array<float, numTaps> delaySamplesR {};

    std::array<float, numTaps> gainL
    {
         0.250f,-0.215f, 0.193f, 0.174f,-0.158f, 0.145f,-0.133f, 0.123f,
         0.114f,-0.106f, 0.099f,-0.092f, 0.086f, 0.080f,-0.075f, 0.070f,
        -0.066f, 0.062f, 0.058f,-0.055f, 0.052f,-0.049f, 0.046f, 0.043f,
        -0.041f, 0.039f,-0.037f, 0.035f, 0.033f,-0.031f, 0.029f,-0.027f
    };

    std::array<float, numTaps> gainR
    {
        -0.205f, 0.238f, 0.169f,-0.187f, 0.151f,-0.139f, 0.128f,-0.118f,
         0.109f, 0.101f,-0.094f, 0.088f,-0.082f, 0.077f, 0.072f,-0.068f,
         0.064f,-0.060f, 0.057f, 0.054f,-0.051f, 0.048f,-0.045f, 0.043f,
         0.040f,-0.038f, 0.036f,-0.034f, 0.032f, 0.030f,-0.028f, 0.026f
    };

    size_t writeIndex = 0;
    double sampleRate = 44100.0;
    float size = 1.0f;
    float outputGain = 0.86f;
};
