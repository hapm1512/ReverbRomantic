#pragma once
#include <JuceHeader.h>
#include <array>
#include "DelayLine.h"
#include "Matrix16.h"
class HybridFDN16 { public: void prepare(const juce::dsp::ProcessSpec&); void reset(); void setParameters(float,float,float,float,float,float,bool); void processStereo(float,float,float&,float&) noexcept; private: std::array<RomanticDelayLine,16> delays; Matrix16::Vector feedback{},dampingState{},phases{},rates{}; std::array<float,16> baseMs{31.1f,37.7f,41.3f,43.9f,47.3f,53.1f,59.3f,61.7f,67.9f,71.3f,73.9f,79.7f,83.9f,89.3f,97.1f,101.9f}; double sampleRate=44100; float decay=4,size=1,damping=12000,diffusion=.88f,density=.94f,modulation=.22f; bool freeze=false; };
