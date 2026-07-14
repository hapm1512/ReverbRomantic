cmake_minimum_required(VERSION 3.22)
project(ReverbRomantic VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory("D:/JUCE8" JUCE)
juce_add_plugin(ReverbRomantic
    COMPANY_NAME "Hai Pham Audio"
    BUNDLE_ID com.haiphamaudio.reverbromantic
    PLUGIN_MANUFACTURER_CODE HaiP
    PLUGIN_CODE RvRm
    FORMATS VST3 Standalone
    PRODUCT_NAME "Reverb Romantic"
    DESCRIPTION "Romantic vocal reverb"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Reverb Stereo)
juce_generate_juce_header(ReverbRomantic)
file(GLOB_RECURSE REVERB_SOURCES CONFIGURE_DEPENDS Source/*.cpp Source/*.h)
target_sources(ReverbRomantic PRIVATE ${REVERB_SOURCES})
target_compile_definitions(ReverbRomantic PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0)
target_link_libraries(ReverbRomantic PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
