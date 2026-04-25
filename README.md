# Rimell Anamorphic

Rimell Anamorphic is an OFX filter plugin for making spherical digital footage feel like finished, desqueezed anamorphic footage.

In its main mode, the plugin assumes normal spherical source footage and applies a combined anamorphic-style optical treatment as one OFX effect.

A separate utility path is available for real anamorphic source footage that genuinely needs desqueeze and framing handling.

Status: active development / experimental.

This README reflects the current implementation in code.

## Current Functionality

Rimell Anamorphic is currently focused on converting normal spherical footage into a stylized, already-desqueezed anamorphic-looking image. It also includes utility controls for genuine anamorphic source footage, but the main creative path assumes the input was shot spherical.

The plugin currently implements the following behavior:

1. Input and geometry mode architecture
- Input mode options:
	- Spherical -> Anamorphic Look (default)
	- Real Anamorphic Utility
	- Creative Warp
- Geometry controls include:
	- Squeeze mode options (Off, Squeeze, Desqueeze) for the real anamorphic utility and creative warp paths
	- Squeeze ratio control
	- Virtual horizontal expansion behavior (formerly horizontal FOV boost), influenced by virtual focal length

2. Virtual anamorphic geometry and axis-specific warping
- Barrel and mustache distortion
- Vertical compensation
- Edge compression
- Close-focus center squeeze variation (mumps-style shaping)
- Center protection / width compensation behavior to limit over-widening in key subjects
- Breathing response based on focus distance

3. Edge and field character
- Edge focus falloff behavior (formerly field curvature contribution)
- Edge blur
- Tangential smear and horizontal smear
- Radial falloff shaping
- Vertical sharpness compensation

4. Chromatic behavior
- Lateral chromatic aberration with pixel scaling
- Edge-only CA option
- Longitudinal CA approximation (focus/edge dependent channel softness)

5. Highlight and flare response
- Highlight-driven response controls (threshold and shaping) used for flare/bloom systems
- Directional streak flare with angle, length, intensity, color, threshold, density, span, and falloff
- Oval highlight bloom and veil shaping controls (bokeh-style highlight behavior)
- Bloom/veil controls include stretch, rotation, edge falloff, threshold scaling, ring/sampling controls, and highlight cream

6. Ghosting and coating-inspired response
- Ghost reflections with count, spread, tint, intensity
- Coating tint response (Warm, Neutral, Cool) with warm/cool scaling
- Center veil contribution and black-lift protection behavior

7. Vignette and highlight-edge shaping
- Oval vignette
- Asymmetry and corner bias
- Edge highlight vignette (cat-eye style dimming)
- Bloom vignette dimming

8. Scope framing, aspect tools, and guides
- Aspect guides on/off
- Output aspect choices: 2.00:1, 2.39:1, 2.66:1, Custom
- Safe area guide
- Letterbox preview with opacity
- Adjustable guide strengths for aspect and safe lines

9. Blend and performance controls
- Global Mix control
- Render Quality modes: Draft, Preview, Final
- Quality mode scales expensive sampling paths for flare, bloom, blur, ghosts, and chromatic work

## Conceptual Model and Limits

- Main path intent: spherical input -> virtual anamorphic presentation.
- Main path geometry does not desqueeze normal circular-lens footage; it emulates anamorphic character on top of the original plate.
- Real anamorphic desqueeze remains available as a utility path for genuinely squeezed source footage.
- The plugin can simulate anamorphic traits, but it cannot reconstruct horizontal scene information that was never captured in-frame.
- Oval highlight behavior in this workflow is an approximation of anamorphic highlight response, not a physically complete depth-aware bokeh reconstruction.

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
