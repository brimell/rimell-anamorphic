# Rimell Anamorphic

Rimell Anamorphic is an OFX image effect that applies a synthetic anamorphic look to spherical footage and also includes a utility path for real anamorphic material that needs desqueeze and framing help.

The plugin is built around a procedural lens-map model and an OpenLensIO-informed naming/coordinate approach. It is not a full OpenLensIO interchange implementation.

Current project version: `0.2.0`

Status: active development, experimental.

## What It Does

The main creative path starts with normal spherical source footage, builds a virtual anamorphic coordinate field, samples the source through that field, and derives lens character from the same mapped space.

The output is intended to feel like finished anamorphic footage rather than a simple crop or letterbox.

```text
spherical source -> virtual anamorphic transfer -> desqueezed-looking output
```

For real anamorphic plates, the utility path supports squeeze/desqueeze and framing-related handling.

```text
real anamorphic source -> utility geometry / framing handling
```

## Key Features

- Synthetic anamorphic transfer from spherical footage
- Real anamorphic utility mode
- OpenLensIO-informed coordinate and profile model
- Geometry controls for squeeze, axis warp, edge compression, breathing, and distortion
- Highlight and flare shaping, including bloom, streaks, ghosts, and coating response
- Edge treatment, chromatic aberration, vignette, and field-curvature controls
- Framing guides, safe areas, letterbox preview, and optional auto edge crop
- CPU render path on non-Apple platforms
- Metal render path on macOS marked experimental

## Repository Layout

- `src/` plugin source, render code, parameter definitions, and Metal kernels
- `tests/` sanity executables for lens mapping and render logic
- `resources/Info.plist.in` OFX bundle metadata template
- `CMakeLists.txt` build, package, and install setup

## Build Requirements

- CMake 3.20 or newer
- A C++17 compiler
- On macOS, Xcode command line tools for `xcrun`, `metal`, and `metallib`
- An OFX host for manual plugin testing

The build fetches OpenFX from GitHub during configuration.

## Build

From the repository root:

```bash
cmake -S . -B build
cmake --build build
```

On macOS, the build also compiles the Metal kernel into the plugin resources.

## Test

The CMake build produces two sanity executables:

- `build/lens-map-sanity`
- `build/render-sanity`

Run them after building:

```bash
./build/lens-map-sanity
./build/render-sanity
```

## Install

The plugin is packaged as an OFX bundle named `RimellAnamorphic.ofx.bundle`.

To install into the default OFX location for the current platform:

```bash
cmake --install build
```

You can override the install directory with the `OFX_PLUGIN_INSTALL_DIR` CMake cache variable.

## Package

To create a tarball of the built bundle:

```bash
cmake --build build --target package-ofx
```

## Controls

The OFX UI is grouped into these sections:

- Core: mix, debug view, backend selection, render quality, look preset, input mode
- Geometry: squeeze ratio, axis warp, anamorphic transfer, distortion, breathing, focus distance
- Highlights / Flares: bloom, streak flare, veil, ghosts, coating style
- Edge / CA: edge blur, smear, chromatic aberration, vignette, field curvature
- Framing: guides, output aspect, safe area, letterbox preview, auto edge crop

## Notes

- `Look Preset` provides starting points, but `Manual` leaves the detailed controls unchanged.
- `Processing Backend` defaults to CPU-safe behavior, with an experimental Metal option on macOS.
- The plugin is designed as a render-time OFX effect, not as a standalone desktop application.

## License

Add license details here if the project adopts one.
