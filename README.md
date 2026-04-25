# Rimell Anamorphic

**Anamorphic lens character for DaVinci Resolve.**

Rimell Anamorphic is an OFX video plugin designed to add controlled anamorphic-style optical character to digital footage. It is not just a streak flare effect. The aim is to recreate the broader language of anamorphic glass: horizontal squeeze behaviour, edge fall-off, oval highlight rendering, subtle optical distortion, chromatic separation, veiling glare, and controlled flare response.

The plugin is intended for editors, colourists, and filmmakers who want the feeling of anamorphic optics without needing to shoot every project on real anamorphic lenses.

> Status: early development / experimental.

---

## What Cylindra does

Rimell Anamorphic simulates several families of anamorphic lens behaviour:

### 1. Geometry and squeeze

Anamorphic lenses compress the horizontal field of view during capture and are later desqueezed into a wider image. Rimell Anamorphic includes tools for working with this behaviour creatively or technically.

Planned controls:

* Squeeze / desqueeze ratio: 1.25x, 1.33x, 1.5x, 1.8x, 2x
* Output aspect preview: 2.00:1, 2.39:1, 2.66:1, custom
* Horizontal field-of-view expansion
* Barrel distortion
* Mustache distortion
* Edge compression
* Centre-safe correction

This module is the foundation of the plugin. Without geometry controls, an “anamorphic” effect quickly becomes just a flare preset.

---

### 2. Edge rendering and field fall-off

A lot of anamorphic character comes from how the image behaves away from the centre of the frame. Rimell Anamorphic aims to emulate this through directional blur, edge softness, and asymmetric optical fall-off.

Planned controls:

* Edge softness
* Horizontal smear
* Tangential smear
* Radial fall-off
* Field curvature approximation
* Centre sharpness protection
* Edge-only masking

The goal is to make the frame feel optically shaped rather than uniformly sharpened or blurred.

---

### 3. Oval bokeh and highlight shaping

One of the most recognisable anamorphic traits is oval out-of-focus highlights. In real lenses, this is tied to the cylindrical optical design and the squeeze/desqueeze process.

Planned controls:

* Oval highlight stretch
* Bokeh verticality
* Highlight isolation threshold
* Bokeh softness
* Edge bokeh compression
* Cat-eye edge behaviour
* Optional depth-map input for better results later

This is one of the hardest parts to emulate properly in post. Without depth information, Rimell Anamorphic can only approximate oval bokeh by detecting and reshaping bright defocused regions. A later version may support external depth maps or matte inputs for more convincing focus-dependent behaviour.

---

### 4. Flares, ghosting, and veiling glare

Rimell Anamorphic includes flare tools, but they are treated as one part of the anamorphic look rather than the whole point of the plugin.

Planned controls:

* Horizontal streak flares
* Flare length
* Flare intensity
* Flare colour
* Flare threshold
* Ghost reflections
* Veiling glare
* Highlight bloom
* Coating style presets
* Light-source masking

The default look should be restrained. Overdone blue streaks tend to make footage look like a cheap plugin rather than real glass.

---

### 5. Chromatic and coating character

Real lenses do not render every wavelength perfectly. Rimell Anamorphic adds optional colour separation and coating behaviour to support a more optical image.

Planned controls:

* Lateral chromatic aberration
* Edge-only chromatic separation
* Warm/cool coating bias
* Highlight tint
* Shadow protection
* Skin-tone protection

These effects should usually be subtle. The plugin is designed to add character, not destroy image quality.

---

### 6. Vintage defects

Some anamorphic behaviours are technically flaws, but can be visually useful. Rimell Anamorphic keeps these optional rather than building them into every preset.

Planned controls:

* Anamorphic mumps / close-focus face widening
* Focus breathing approximation
* Uneven illumination
* Asymmetric vignetting
* Corner warp
* Vintage softness

Modern anamorphic lenses often try to remove these issues, so Cylindra will treat them as creative choices, not defaults.

---

## What Rimell Anamorphic is not

Rimell Anamorphic is not a physically perfect replacement for real anamorphic lenses.

It cannot fully recreate:

* True depth-dependent bokeh without depth information
* Real focus breathing driven by lens mechanics
* Exact lens-specific behaviour without measured lens profiles
* Complex internal reflections from a real optical path
* The full interaction between aperture, focus distance, focal length, sensor size, and lens design

The plugin is designed to create useful, controllable anamorphic-style image character in post. It should be judged as a creative optical tool, not as a scientific lens simulator.

---

## Intended workflow

A typical node or effect chain might look like this:

1. Primary colour correction
2. Noise reduction, if needed
3. Rimell Anamorphic geometry / edge character
4. Rimell Anamorphic highlight behaviour / flare
5. Film emulation, halation, or grain
6. Final contrast and output transform

For best results, apply Rimell Anamorphic before final grain and after basic exposure balancing. Extremely clipped highlights may produce less natural flare and bloom behaviour.

---

## Suggested controls

### Core

| Control          | Purpose                                       |
| ---------------- | --------------------------------------------- |
| `Amount`         | Global strength of the effect                 |
| `Squeeze Ratio`  | Simulated anamorphic squeeze/desqueeze amount |
| `Aspect Preview` | Optional framing guide / crop preview         |
| `Lens Character` | Clean, modern, vintage, heavy, custom         |

### Geometry

| Control             | Purpose                                        |
| ------------------- | ---------------------------------------------- |
| `Barrel`            | Adds or removes barrel distortion              |
| `Mustache`          | Adds complex wide-angle-style distortion       |
| `Edge Compression`  | Compresses or stretches the edges of the image |
| `Centre Protection` | Keeps the centre of the frame more stable      |

### Edge and focus

| Control            | Purpose                                                  |
| ------------------ | -------------------------------------------------------- |
| `Edge Softness`    | Softens the outer frame                                  |
| `Horizontal Smear` | Adds directional anamorphic-style blur                   |
| `Field Fall-off`   | Controls how quickly the image degrades toward the edges |
| `Astigmatism`      | Separates horizontal and vertical detail behaviour       |

### Highlight behaviour

| Control           | Purpose                                   |
| ----------------- | ----------------------------------------- |
| `Flare Threshold` | Defines which highlights generate flares  |
| `Flare Length`    | Controls streak size                      |
| `Flare Intensity` | Controls streak brightness                |
| `Flare Colour`    | Blue, neutral, amber, custom              |
| `Veiling Glare`   | Adds soft wash around bright areas        |
| `Ghosting`        | Adds secondary internal reflection shapes |

### Bokeh approximation

| Control          | Purpose                                                |
| ---------------- | ------------------------------------------------------ |
| `Oval Stretch`   | Reshapes bright defocused regions                      |
| `Bokeh Softness` | Controls blur around isolated highlights               |
| `Cat-eye Edge`   | Shapes edge highlights into clipped ovals              |
| `Depth Matte`    | Optional future input for better focus-aware behaviour |

---

## Presets

Possible preset direction:

* `Modern 2x Clean`
* `Vintage 2x Soft Edge`
* `Subtle Scope`
* `Blue Streak Controlled`
* `Warm Coating`
* `Low Distortion CinemaScope`
* `Heavy Mumps / Vintage Close Focus`
* `Edge Smear Experimental`

Presets should be starting points, not finished looks. The plugin should encourage subtle adjustment per shot.

---

## Development goals

The first usable version should focus on:

* Stable OFX integration
* GPU-accelerated image processing where possible
* Geometry and edge rendering
* Subtle chromatic aberration
* Basic streak flare generation
* Sensible defaults
* Resolve-friendly controls

Later versions can add:

* Depth-map input
* Lens profile presets
* Per-channel distortion
* More realistic ghosting
* Better bokeh isolation
* Animation-aware focus breathing
* Batch-safe utility mode for desqueeze/crop workflows

---

## Naming

Working name: **Rimell Anamorphic**

The name uses the Rimell identity directly and keeps the product clear: this is an anamorphic lens-character plugin, not a vague cinematic filter pack. It can still support a broader suite of tools:

* `Rimell Anamorphic FX` — creative anamorphic character
* `Rimell Anamorphic Utility` — desqueeze, crop, and framing tools
* `Rimell Anamorphic Warp` — geometry-only correction/creative distortion
* `Rimell Anamorphic Flare` — dedicated flare and ghosting module

---

## License

To be decided.

---

## Disclaimer

Rimell Anamorphic is an independent plugin concept and is not affiliated with ARRI, ZEISS, Panavision, Blackmagic Design, DaVinci Resolve, or any other lens or software manufacturer.
