#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstddef>
#include <vector>

// Dual-stage stereo diffusion network.
// All memory is allocated by prepare(); processStereo() is realtime-safe.
class Diffuser
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setAmount (float newAmount) noexcept;
    void processStereo (float& left, float& right) noexcept;

private:
    struct AllPass
    {
        void prepare (double sampleRate, float milliseconds, float stageGain);
        void reset();
        void setGain (float newGain) noexcept;
        [[nodiscard]] float process (float input) noexcept;

        std::vector<float> buffer;
        std::size_t writePosition = 0;
        float gain = 0.65f;
    };

    static constexpr std::size_t stagesPerChannel = 8;
    static constexpr std::size_t stageBoundary = 4;

    std::array<AllPass, stagesPerChannel> leftStages;
    std::array<AllPass, stagesPerChannel> rightStages;

    float amount = 0.88f;
};
