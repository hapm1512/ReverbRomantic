# RC2 - Realtime Optimization and Audio Polish

## Completed

- Avoid repeated FDN coefficient rebuilds for unchanged parameters.
- Preserve all existing DSP algorithms and factory presets.
- Smooth Mix automation over 25 milliseconds.
- Smooth Output automation over 25 milliseconds.
- Guard the final output against non-finite samples.
- Retain `juce::ScopedNoDenormals` in the audio callback.
- Reduce the GUI status timer from 30 Hz to 20 Hz.
- Keep all DSP memory allocation inside `prepareToPlay()`.

## Validation matrix

- Sample rates: 44.1, 48 and 96 kHz.
- Buffer sizes: 64, 128, 256 and 512 samples.
- Hosts: Cubase, Reaper and Studio One.
- Formats: VST3 and Standalone.
- Verify bypass, automation, mono, stereo and sidechain.
