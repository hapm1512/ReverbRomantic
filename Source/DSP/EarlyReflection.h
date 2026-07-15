#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

class EarlyReflection
{
public:
    enum class Shape : int
    {
        room = 0,
        studio,
        chamber,
        hall,
        cathedral,
        plate
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setSize (float newSize) noexcept;
    void setShape (Shape newShape) noexcept;
    void setHighCut (float frequencyHz) noexcept;
    void processStereo (float inputL, float inputR,
                        float& outputL, float& outputR) noexcept;

private:
    static constexpr size_t numTaps = 32;

    struct ShapeProfile
    {
        float width;
        float depth;
        float height;
        float absorption;
        float diffusion;
        float crossfeed;
        float output;
    };

    static ShapeProfile profileFor (Shape shape) noexcept;
    float readInterpolated (const std::vector<float>& buffer,
                            float delaySamples) const noexcept;
    void updateTapGeometry() noexcept;
    void updateAbsorption() noexcept;

    std::vector<float> bufferL;
    std::vector<float> bufferR;

    alignas (64) std::array<float, numTaps> delaySamplesL {};
    alignas (64) std::array<float, numTaps> delaySamplesR {};
    alignas (64) std::array<float, numTaps> secondaryDelayL {};
    alignas (64) std::array<float, numTaps> secondaryDelayR {};
    alignas (64) std::array<float, numTaps> gainL {};
    alignas (64) std::array<float, numTaps> gainR {};
    alignas (64) std::array<float, numTaps> secondaryGain {};
    alignas (64) std::array<float, numTaps> absorptionCoefficient {};
    alignas (64) std::array<float, numTaps> filterStateL {};
    alignas (64) std::array<float, numTaps> filterStateR {};
    alignas (64) std::array<float, numTaps> diffusionStateL {};
    alignas (64) std::array<float, numTaps> diffusionStateR {};

    size_t writeIndex = 0;
    double sampleRate = 44100.0;
    float size = 1.0f;
    float highCutHz = 12000.0f;
    Shape shape = Shape::hall;
    ShapeProfile profile { profileFor (Shape::hall) };
};
