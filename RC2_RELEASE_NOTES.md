# RC2 - Realtime Optimization and Audio Polish

## Completed

- Avoid repeated FDN coefficient rebuilds for unchanged parameters.
- Skip pitch-shifting DSP while Shimmer is fully bypassed.
- Skip frozen-loop DSP while Freeze is fully bypassed.
- Preserve all existing DSP algorithms and factory presets.
- Smooth Mix automation over 25 milliseconds.
- Smooth Output automation over 25 milliseconds.
- Guard the final output against non-finite samples.
- Retain `juce::ScopedNoDenormals` in the audio callback.
- Reduce the GUI status timer from 30 Hz to 20 Hz.
- Remove the decorative background waveform.
- Correct Density so it no longer shortens RT60.
- Raise wet injection and late-tail output gain.
- Open the Hall and Cathedral high-frequency damping.
- Keep all DSP memory allocation inside `prepareToPlay()`.

## Validation matrix

- Sample rates: 44.1, 48 and 96 kHz.
- Buffer sizes: 64, 128, 256 and 512 samples.
- Hosts: Cubase, Reaper and Studio One.
- Formats: VST3 and Standalone.
- Verify bypass, automation, mono, stereo and sidechain.
