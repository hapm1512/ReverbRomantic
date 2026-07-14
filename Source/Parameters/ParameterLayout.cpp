#include "ParameterLayout.h"
namespace Parameters {
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto addFloat=[&](auto id, auto name, float lo, float hi, float step, float def, float skew=1.0f){
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1}, name,
            juce::NormalisableRange<float>{lo,hi,step,skew}, def));};
    addFloat(IDs::mix,"Mix",0,100,0.1f,35); addFloat(IDs::decay,"Decay",0.2f,20,0.01f,4.2f,0.45f);
    addFloat(IDs::time,"Time",0.25f,2,0.001f,1); addFloat(IDs::preDelay,"Pre Delay",0,250,0.1f,38);
    addFloat(IDs::size,"Size",25,200,0.1f,110); addFloat(IDs::width,"Width",0,200,0.1f,125);
    addFloat(IDs::warmth,"Warmth",-12,12,0.1f,3); addFloat(IDs::brightness,"Brightness",-12,12,0.1f,-1);
    addFloat(IDs::diffusion,"Diffusion",0,100,0.1f,88); addFloat(IDs::density,"Density",0,100,0.1f,94);
    addFloat(IDs::modulation,"Modulation",0,100,0.1f,22); addFloat(IDs::bloom,"Bloom",0,100,0.1f,45);
    addFloat(IDs::ducking,"Ducking",0,100,0.1f,18); addFloat(IDs::lowCut,"Low Cut",20,1000,1,120,0.35f);
    addFloat(IDs::highCut,"High Cut",1000,20000,1,12000,0.35f); addFloat(IDs::output,"Output",-24,12,0.1f,0);
    p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{IDs::quality,1},"Quality",juce::StringArray{"Eco","Standard","High","Ultra"},1));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{IDs::freeze,1},"Freeze",false));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{IDs::bypass,1},"Bypass",false));
    return {p.begin(),p.end()};
}}
