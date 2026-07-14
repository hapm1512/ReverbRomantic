# ReverbRomantic - Epic 3A DSP Core

## Completed
- Connected complete realtime DSP pipeline.
- Added stereo predelay.
- Added four-stage stereo diffuser.
- Added eight-tap early reflections.
- Integrated Hybrid FDN 16x16 late tail.
- Added damping and low/high cut processing.
- Added modulation source.
- Added tail bloom envelope.
- Added input-driven wet ducking.
- Added energy-normalized stereo width.
- Added soft output limiter.
- Connected all DSP-related APVTS parameters.
- Corrected mono processing path.
- No allocation inside sample processing.

## Pipeline
Input -> Predelay -> Diffuser -> Early Reflections -> FDN16 -> Tone Filters -> Bloom -> Width -> Ducking -> Limiter -> Wet/Dry Mix
