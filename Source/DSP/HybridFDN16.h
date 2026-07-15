#pragma once
#include <JuceHeader.h>
#include <array>
#include "DelayLine.h"
#include "Matrix16.h"
#include "Diffuser.h"
#include "EarlyReflection.h"
#include "Damping.h"
#include "Modulation.h"
#include "TailBloom.h"
#include "Ducking.h"
#include "StereoWidth.h"
#include "Limiter.h"

class HybridFDN16
{
public:
    enum class RoomModel : int
    {
        room = 0,
        studio,
        chamber,
        hall,
        cathedral,
        plate
    };

    struct Parameters
    {
        float decaySeconds = 4.2f;
        float preDelayMs = 38.0f;
        float sizePercent = 110.0f;
        float widthPercent = 125.0f;
        float diffusionPercent = 88.0f;
        float densityPercent = 94.0f;
        float modulationPercent = 22.0f;
        float bloomPercent = 45.0f;
        float duckingPercent = 18.0f;

        // External sidechain ducking. Disabled by default for compatibility.
        bool sidechainEnabled = false;
        float sidechainThresholdDb = -24.0f;
        float sidechainAmountPercent = 50.0f;
        float sidechainAttackMs = 8.0f;
        float sidechainReleaseMs = 220.0f;
        float sidechainHighPassHz = 120.0f;

        float lowCutHz = 120.0f;
        float highCutHz = 12000.0f;
        float warmthDb = 3.0f;
        float brightnessDb = -1.0f;
        int quality = 1;
        bool freeze = false;
        bool processOutputStage = true;
        RoomModel roomModel = RoomModel::hall;
    };

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();
    void setParameters (const Parameters& newParameters) noexcept;

    // Legacy path: no external sidechain signal.
    void processStereo (float inputL, float inputR,
                        float& outputL, float& outputR) noexcept;

    // External sidechain path used by the processor in Epic 4E.3.
    void processStereo (float inputL, float inputR,
                        float sidechainL, float sidechainR,
                        float& outputL, float& outputR) noexcept;

    float getSidechainGainReductionDb() const noexcept
    {
        return sidechainGainReductionDb;
    }

private:
    struct RoomProfile
    {
        float delayScale;
        float decayScale;
        float preDelayScale;
        float diffusionBias;
        float densityBias;
        float dampingScale;
        float modulationScale;
        float bloomBias;
        float widthScale;
        float earlyScale;
        float lateScale;
        float injectionGain;
        float tapGain;
    };

    static RoomProfile getRoomProfile (RoomModel model) noexcept;
    static bool parametersEqual (const Parameters&, const Parameters&) noexcept;
    void updateFDNCoefficients() noexcept;
    float processHighPass (float input, float& inputState, float& outputState) noexcept;
    float processLowPass (float input, float& state) noexcept;
    float processSidechainDetector (float sidechainL, float sidechainR) noexcept;

    std::array<RomanticDelayLine, 16> delays;
    std::array<RomanticDelayLine, 2> preDelay;
    alignas (64) Matrix16::Vector feedback {};
    alignas (64) Matrix16::Vector dampingState {};
    alignas (64) Matrix16::Vector bassState {};
    alignas (64) Matrix16::Vector presenceState {};
    alignas (64) Matrix16::Vector baseDelaySamples {};
    alignas (64) Matrix16::Vector feedbackGains {};

    Diffuser diffuser;
    EarlyReflection earlyReflection;
    Damping outputDamping;
    Modulation modulationSource;
    TailBloom tailBloom;
    Ducking ducking;
    StereoWidth stereoWidth;
    Limiter limiter;

    static constexpr std::array<int, 16> primeDelaySamples48k {
        1429, 1607, 1811, 2029,
        2287, 2557, 2879, 3209,
        3583, 4001, 4441, 4937,
        5483, 6089, 6761, 7507
    };

    Parameters parameters;
    bool parametersInitialised = false;
    RoomProfile roomProfile { getRoomProfile (RoomModel::hall) };
    double sampleRate = 44100.0;
    float sizeScale = 1.0f;
    float effectiveDecaySeconds = 4.2f;
    float densityScale = 0.95f;
    float effectiveDiffusionMix = 0.88f;
    float injectionGain = 0.105f;
    float tapGain = 0.115f;
    float lowPassCoefficient = 0.5f;
    float highPassCoefficient = 0.99f;
    float dampingCoefficient = 0.5f;
    float bassSplitCoefficient = 0.97f;
    float presenceSplitCoefficient = 0.55f;
    float lowDecayMultiplier = 1.0f;
    float midDecayMultiplier = 0.985f;
    float highDecayMultiplier = 0.94f;
    float bassResonanceControl = 1.0f;
    float toneGain = 1.0f;
    float modulationDepthSamples = 0.0f;
    float earlyOutputGain = 0.32f;
    float lateOutputGain = 1.0f;
    float energyCompensation = 1.0f;
    float hpInputL = 0.0f, hpInputR = 0.0f;
    float hpOutputL = 0.0f, hpOutputR = 0.0f;
    float lpStateL = 0.0f, lpStateR = 0.0f;

    // External sidechain detector state.
    float sidechainHpfCoefficient = 0.0f;
    float sidechainHpfInput = 0.0f;
    float sidechainHpfOutput = 0.0f;
    float sidechainEnvelope = 0.0f;
    float sidechainGain = 1.0f;
    float sidechainGainReductionDb = 0.0f;
    float sidechainAttackCoefficient = 0.0f;
    float sidechainReleaseCoefficient = 0.0f;
    float sidechainThresholdLinear = 0.0630957f;
    float sidechainMinimumGain = 1.0f;
};
