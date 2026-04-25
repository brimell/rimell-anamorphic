# Rimell Anamorphic

Rimell Anamorphic is an OFX filter plugin that applies a combined anamorphic-style optical treatment in a single pass.

Status: active development / experimental.

This README reflects the current implementation in code.

## Current Functionality

The plugin currently implements the following behavior:

1. Geometry and squeeze pipeline
- Squeeze mode: Off, Squeeze, Desqueeze
- Squeeze ratio control
- Horizontal FOV boost influenced by virtual focal length
- Barrel and mustache distortion
- Vertical compensation
- Edge compression
- Close-focus mumps shaping with face-width compensation
- Breathing response based on focus distance

2. Chromatic behavior
- Lateral chromatic aberration with pixel scaling
- Edge-only CA option
- Longitudinal CA approximation (focus/edge dependent channel softness)

3. Edge and field character
- Edge blur
- Field curvature contribution
- Tangential smear and horizontal smear
- Radial falloff shaping
- Vertical sharpness compensation

4. Lens additives and highlight response
- Directional streak flare with angle, length, intensity, color, threshold, density, span, and falloff
- Anamorphic-shaped bloom/veil system with stretch, rotation, edge falloff, threshold scaling, ring/sampling controls, and highlight cream
- Ghost reflections with count, spread, tint, intensity
- Coating style response (Warm, Neutral, Cool) with warm/cool scaling
- Center veil contribution and black-lift protection behavior

5. Vignette and highlight-edge shaping
- Oval vignette
- Asymmetry and corner bias
- Edge highlight vignette (cat-eye style dimming)
- Bloom vignette dimming

6. Aspect tools and guides
- Aspect guides on/off
- Output aspect choices: 2.00:1, 2.39:1, 2.66:1, Custom
- Safe area guide
- Letterbox preview with opacity
- Adjustable guide strengths for aspect and safe lines

7. Blend and performance controls
- Global Mix control
- Render Quality modes: Draft, Preview, Final
- Quality mode scales expensive sampling paths for flare, bloom, blur, ghosts, and chromatic work

## Host / Technical Behavior

- OFX context: Filter
- Source clip: RGBA
- Output clip: RGBA
- Supported bit depths: 8-bit, 16-bit, 32-bit float
- Frame threading enabled
- Tiled rendering disabled
- Temporal clip access disabled
- Identity optimization: if Mix is 0, the effect reports source identity
- ROI expansion is implemented to account for flare, bloom, blur, and CA sampling footprint

## What Is Not Implemented Yet

The current version does not include:

- Depth-map or matte input for depth-aware bokeh
- True physically based lens simulation
- Lens profile system
- GPU render path
- Built-in preset browser
- Multi-clip workflows beyond source/output filter processing

## Practical Use Notes

- The effect is designed as a stacked optical look, not only a flare generator.
- Best results usually come from subtle settings and a non-maximum Mix.
- Highly clipped highlights can reduce nuance in bloom/flare response.

## Project Name

Plugin label: Rimell Anamorphic

## Build

Configure and build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Install:

```sh
cmake --install build
```

The built bundle is written to:

```text
build/RimellAnamorphic.ofx.bundle
```

After installation, restart your host application. The effect is labeled Rimell Anamorphic in the Rimell/Lens group.

## License

To be decided.

## Disclaimer

Rimell Anamorphic is an independent plugin project and is not affiliated with ARRI, ZEISS, Panavision, Blackmagic Design, DaVinci Resolve, or other lens/software manufacturers.
