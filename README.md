# Rimell Anamorphic

Rimell Anamorphic is an OFX filter plugin for making spherical digital footage feel like finished, desqueezed anamorphic footage through a procedural lens-mapping system called **AxiScope Mapping** / **Virtual Anamorphic Transfer**.

The plugin is now organised around an **OpenLensIO-informed coordinate and profile model**. Rimell Anamorphic does not yet implement full OpenLensIO interchange, but its internal model, naming, metadata structure, and future profile direction are aligned with the OpenLensIO lens-model concepts where they are useful for an OFX image effect.

In its main mode, the plugin assumes normal spherical source footage, builds a synthetic anamorphic view map, samples the original image through that map, then drives optical artefacts from the same mapped coordinate field.

A separate utility path is available for real anamorphic source footage that genuinely needs desqueeze and framing handling.

Status: active development / experimental.

This README reflects the current implementation direction and the planned OpenLensIO-compatible profile architecture.

---

## What the Plugin Is For

Rimell Anamorphic is designed around a practical problem: most digital footage is shot with spherical lenses, but a lot of cinema language associated with widescreen images comes from anamorphic optics.

A basic crop to 2.39:1 can make footage wider, but it does not make the footage behave as though it passed through anamorphic glass. Real anamorphic lenses do more than change aspect ratio. They map the scene differently in the horizontal and vertical directions, introduce axis-dependent distortion, change how focus fall-off behaves, reshape highlights, and often create distinctive flares, bokeh, edge softness, chromatic behaviour, vignetting, and breathing.

Rimell Anamorphic is therefore not just a letterbox tool and not just a flare generator. It is a shared virtual lens-mapping effect intended to approximate several visual consequences of anamorphic image formation in a controllable post-production plugin.

The main creative path is:

```text
spherical source footage -> virtual anamorphic optical treatment -> already-desqueezed anamorphic-looking output
```

The separate utility path is:

```text
real anamorphic source footage -> desqueeze / framing / guide handling
```

---

## OpenLensIO-Informed Design

OpenLensIO provides a useful reference model for how lens metadata, camera coordinates, distortion, overscan, aperture, and vignetting can be represented in a production-friendly way. Rimell Anamorphic uses OpenLensIO as a conceptual anchor, but it should not be described as a complete OpenLensIO implementation yet.

The current plugin uses an **OpenLensIO-informed profile layer** rather than a strict OpenLensIO file interchange layer.

### What Rimell Anamorphic currently borrows from OpenLensIO

* image-space coordinates centred on the sensor/screen centre
* horizontal-positive-right and vertical-positive-down image axes
* millimetre-style normalised screen-space thinking, even when the live OFX render path works in pixels
* separation between undistorted and distorted coordinate spaces
* distinction between centre-of-distortion offset and perspective offset
* radial and decentering distortion concepts
* overscan / ROI expansion thinking for distortion, blur, flare, and bloom sampling footprints
* dynamic lens-state thinking, where focus, iris, zoom, and user controls may change the effective profile
* optical vignetting as a polynomial-style falloff model
* future support for per-channel distortion parameters for chromatic aberration

### What Rimell Anamorphic does not currently implement

* full OpenLensIO JSON parsing
* full OpenLensIO serialisation
* measured lens profile import
* measured lens profile export
* strict projection-matrix or field-of-view camera interchange
* physically complete aperture and circle-of-confusion calculation
* measured entrance pupil behaviour
* true calibrated anamorphic lens distortion solving

The current implementation is best described as:

```text
OpenLensIO-informed synthetic anamorphic profile -> OFX render-time virtual lens map
```

not:

```text
complete OpenLensIO lens interchange implementation
```

---

## OpenLensIO Concepts Used by the Plugin

### Image-space coordinate frame

OpenLensIO describes image/screen coordinates relative to the centre of the sensor or active screen area, with the horizontal axis positive to the right and the vertical axis positive downwards. Rimell Anamorphic adopts the same convention internally for its virtual lens map.

In implementation terms:

```text
pixel coordinates -> centred normalised image coordinates -> virtual OpenLensIO-style screen coordinates -> lens map -> source sample coordinates
```

The OFX render path still receives image bounds in pixels, but the lens mapping layer should treat those pixels as samples on a centred image plane.

### Undistorted and distorted spaces

OpenLensIO separates undistorted coordinates from distorted coordinates. Rimell Anamorphic uses the same broad separation:

| Space                     | Meaning in Rimell Anamorphic                                                |
| ------------------------- | --------------------------------------------------------------------------- |
| `epsilon_u` / undistorted | the idealised spherical or virtual camera coordinate before lens distortion |
| `epsilon_d` / distorted   | the image coordinate after synthetic lens distortion / anamorphic transfer  |
| `epsilon_mapped`          | the final mapped source coordinate used by the OFX sampler                  |

For real lens correction workflows, undistortion usually means removing lens distortion. Rimell Anamorphic usually does the opposite in its main creative mode: it intentionally adds a controlled synthetic distortion to spherical footage.

### Centre offset and perspective offset

OpenLensIO distinguishes:

| Offset                              | Purpose                                                |
| ----------------------------------- | ------------------------------------------------------ |
| `deltaC` / distortion centre offset | moves the centre of distortion independently           |
| `deltaP` / perspective offset       | moves the centre of projection and distortion together |

Rimell Anamorphic should expose these ideas carefully. In the creative UI, they do not need to be called `deltaC` and `deltaP`, but the profile layer should preserve that distinction.

Suggested user-facing names:

| Internal/OpenLensIO-informed name | User-facing name     |
| --------------------------------- | -------------------- |
| `deltaC.x`                        | Distortion Centre X  |
| `deltaC.y`                        | Distortion Centre Y  |
| `deltaP.x`                        | Perspective Offset X |
| `deltaP.y`                        | Perspective Offset Y |

For most users, these should remain advanced controls or profile parameters, not headline creative sliders.

### Radial and decentering distortion

OpenLensIO uses a Brown-Conrady-style distortion model with radial and decentering terms. Rimell Anamorphic should gradually move its distortion layer towards a compatible parameter naming scheme:

| OpenLensIO concept                  | Rimell Anamorphic control / profile parameter           |
| ----------------------------------- | ------------------------------------------------------- |
| `k1...k6` radial coefficients       | barrel, pincushion, mustache, higher-order radial shape |
| `p1`, `p2` decentering coefficients | asymmetric distortion / decentered lens behaviour       |
| `deltaC`                            | distortion centre offset                                |
| `deltaP`                            | perspective offset                                      |
| `Omega`, `OmegaPrime`               | overscan / render expansion                             |
| `a1...a3`                           | optical vignetting polynomial                           |

The public controls can stay artist-friendly, but saved profiles should use stable OpenLensIO-informed names where possible.

### Overscan and ROI expansion

OpenLensIO treats overscan as necessary when distortion would otherwise leave unrendered areas. Rimell Anamorphic uses the same idea in OFX terms through ROI expansion.

In this plugin, overscan is relevant to:

* barrel / mustache distortion
* virtual horizontal expansion
* edge compression
* flare span
* bloom radius
* ghost spread
* chromatic aberration offsets

The plugin’s ROI expansion should be treated as the OFX equivalent of an OpenLensIO-style overscan requirement.

### Aperture and focus

OpenLensIO includes aperture/focus concepts related to circle of confusion, but Rimell Anamorphic does not currently solve a physically complete depth-of-field model.

The current plugin uses:

* `focusDistance`
* oval highlight bloom
* longitudinal CA approximation

These should be described as **focus-aware image-space approximations**, not as a physical aperture solve.

### Vignetting

OpenLensIO models optical vignetting with polynomial radial falloff terms. Rimell Anamorphic currently uses a more creative oval/asymmetric vignette model.

Future profile work should separate:

| Vignette type                 | Use                                                        |
| ----------------------------- | ---------------------------------------------------------- |
| OpenLensIO optical vignetting | profile-driven radial intensity falloff                    |
| natural vignetting            | angle-dependent falloff, future work                       |
| mechanical vignetting         | physical obstruction / cat-eye edge behaviour, future work |
| creative oval vignette        | user-facing anamorphic look control                        |

---

## OpenLensIO-Informed Profile Format

Rimell Anamorphic should use a profile file that separates measured/model-style lens data from creative look controls.

A proposed profile structure:

```json
{
  "schema": "com.rimell.anamorphic.profile",
  "schemaVersion": "0.1.0",
  "profileType": "synthetic_anamorphic_look",
  "profileName": "Classic 2x Soft Edge",
  "description": "Synthetic anamorphic look inspired by vintage 2x scope lenses. Not a measured real lens profile.",

  "openLensIO": {
    "modelVersion": "1.0.0-informed",
    "implementationStatus": "partial_informed_model",
    "coordinateUnits": "normalised_sensor_mm_equivalent",
    "usesOpenLensIOInterchange": false,
    "notes": "This block follows OpenLensIO concepts and naming where practical, but is not a strict OpenLensIO payload."
  },

  "sensor": {
    "activeWidthMm": 36.0,
    "activeHeightMm": 20.25,
    "normalisation": "width_based"
  },

  "lensState": {
    "focalLengthMm": 50.0,
    "focusDistance": 0.5,
    "iris": null,
    "zoom": null,
    "entrancePupilDistance": 0.0
  },

  "centreOffsets": {
    "deltaC": [0.0, 0.0],
    "deltaP": [0.0, 0.0]
  },

  "distortion": {
    "model": "openlensio_brown_conrady_informed",
    "k": [-0.08, 0.0, 0.018, 0.0, 0.0, 0.0],
    "p": [0.0, 0.0],
    "creativeBarrel": -0.08,
    "creativeMustache": 0.018
  },

  "anamorphic": {
    "mode": "spherical_to_anamorphic_look",
    "asq": 2.0,
    "anamorphicTransfer": 0.75,
    "axisWarp": 0.25,
    "horizontalExpansion": 0.22,
    "edgeCompression": 0.18,
    "edgeCompressionStart": 0.55,
    "verticalCompensation": 0.08,
    "centreProtection": 0.65,
    "mumps": 0.05,
    "mumpsCompensation": 0.35,
    "breathingX": 0.04,
    "breathingY": 0.01
  },

  "chromaticAberration": {
    "model": "rimell_per_channel_offset",
    "lateralAmountPixels": 0.8,
    "edgeOnly": true,
    "longitudinalAmount": 0.03,
    "futureOpenLensIOMode": "three_sets_of_distortion_parameters"
  },

  "vignetting": {
    "openLensIOOptical": {
      "enabled": false,
      "a": [0.0, 0.0, 0.0]
    },
    "creativeOval": {
      "enabled": true,
      "amount": 0.18,
      "asymmetry": 0.0,
      "cornerBias": 0.0,
      "catEyeEdge": 0.25
    }
  },

  "edgeCharacter": {
    "edgeFocusFalloff": 0.25,
    "edgeBlur": 0.22,
    "horizontalSmear": 0.16,
    "tangentialSmear": 0.18,
    "radialFalloff": 0.65,
    "verticalSharpness": 0.04
  },

  "highlightResponse": {
    "threshold": 0.82,
    "thresholdSoftness": 0.15,
    "ovalHighlightShape": 0.65,
    "ovalHighlightRotation": 0.0,
    "bloomRadius": 0.24,
    "highlightCream": 0.25,
    "veil": 0.08,
    "blackLiftProtection": 0.65
  },

  "flare": {
    "enabled": true,
    "angleDegrees": 0.0,
    "length": 0.45,
    "intensity": 0.25,
    "colour": [0.35, 0.75, 1.0],
    "density": 1.0,
    "span": 1.0,
    "falloff": 3.0
  },

  "ghosting": {
    "enabled": true,
    "count": 2,
    "spread": 0.35,
    "intensity": 0.12,
    "tint": [0.55, 0.8, 1.0]
  },

  "framing": {
    "outputAspect": 2.39,
    "letterboxPreview": true,
    "safeArea": 0.9
  }
}
```

This is not a strict OpenLensIO file. It is a Rimell profile format with an OpenLensIO-informed lens block. That distinction matters.

---

## OpenLensIO Mapping Table

| OpenLensIO parameter / concept | Meaning                                   | Rimell Anamorphic usage                                            |
| ------------------------------ | ----------------------------------------- | ------------------------------------------------------------------ |
| `w`                            | active sensor width in mm                 | profile sensor width / normalisation basis                         |
| `F`                            | focal length in mm                        | virtual focal length / horizontal expansion reference              |
| `deltaC`                       | distortion centre offset                  | advanced distortion centre controls                                |
| `deltaP`                       | perspective offset                        | advanced perspective offset controls                               |
| `k1...k6`                      | radial distortion coefficients            | profile-level radial distortion, maps to barrel/mustache controls  |
| `p1`, `p2`                     | decentering distortion                    | asymmetric distortion / lens decentering                           |
| `Omega`, `OmegaPrime`          | overscan factors                          | OFX ROI expansion / render overscan planning                       |
| `OmegaMax`, `OmegaPrimeMax`    | maximum overscan                          | future profile metadata for safe render bounds                     |
| `zepd`                         | entrance pupil distance                   | future focus/breathing metadata; currently not physically modelled |
| `Phi`                          | focal distance                            | focus distance / depth-map focus value, approximate                |
| `a1...a3`                      | optical vignetting coefficients           | future profile-driven vignetting; current vignette is creative     |
| anamorphic `Asq`               | anamorphic squeeze                        | real desqueeze utility and synthetic anamorphic transfer strength  |
| anamorphic `Ax`, `Ay`          | asymmetric anamorphic distortion terms    | future axis-specific distortion coefficients                       |
| per-channel distortion sets    | lateral chromatic aberration future model | future RGB distortion profiles; current CA is pixel-offset based   |

---

## AxiScope Mapping / Virtual Anamorphic Transfer

The current implementation is built around one shared procedural lens map instead of a set of unrelated look sliders.

For each output pixel, Rimell Anamorphic:

* converts the pixel into centred image coordinates
* maps those coordinates into an OpenLensIO-informed virtual screen space
* builds a procedural virtual lens coordinate field
* blends from spherical coordinates into an axis-dependent anamorphic coordinate using `Anamorphic Transfer`
* applies centre protection, edge compression, axis-weighted barrel/mustache distortion, mumps, breathing, and virtual horizontal expansion
* samples the source image through the final map
* derives chromatic separation, edge softness, bloom shape, flare length, ghost bias, and vignette behaviour from the same lens identity

The output of the main spherical mode is already desqueezed-looking. The plugin does not literally desqueeze normal spherical footage unless the utility squeeze/desqueeze path is selected for real anamorphic material.

The v1 map is procedural and real-time. It is STMap-like internally, but there is not yet an import/export workflow for measured maps.

---

## Spherical vs Anamorphic Lenses

### The short version

A spherical lens is rotationally symmetric. Its optical power is the same around the optical axis, so horizontal and vertical scene information is mapped with the same basic geometry.

An anamorphic lens is not rotationally symmetric. It has different optical power in two orthogonal planes, usually created with cylindrical lens elements. This means the lens maps horizontal and vertical information differently. The image is compressed in one axis during capture and expanded again during projection or post-production.

That difference is the heart of the plugin.

Rimell Anamorphic does not claim to turn spherical footage into real anamorphic capture. It cannot restore horizontal field of view that was never recorded. Instead, it uses post-processing to approximate the visual signatures that come from anamorphic image mapping and its associated aberrations.

---

### Spherical lenses

A conventional spherical cinema or photography lens is built around rotational symmetry. In simple terms, the lens behaves similarly no matter which direction you rotate around the centre of the image.

This means:

* horizontal and vertical magnification are normally the same
* circular points of light tend to remain circular when defocused, ignoring aperture blade shape and ordinary lens defects
* distortion is usually radially organised around the optical centre
* barrel, pincushion, and mustache distortion generally act as circular/radial image-space deformations
* focus behaviour is usually not intentionally different between horizontal and vertical directions
* a widescreen frame from a spherical lens is usually created by cropping the top and bottom of a larger image

For example, when spherical footage is cropped to 2.39:1, the frame becomes wide, but the lens behaviour remains spherical. You gain a widescreen composition, but you lose vertical image area. The crop does not create anamorphic bokeh, anamorphic squeeze, axis-specific distortion, horizontal streak flare, or differential focus behaviour.

---

### Anamorphic lenses

An anamorphic lens has different imaging behaviour in two perpendicular axes. The common cinema version uses cylindrical optical elements to compress a wide horizontal field of view onto a narrower recording format.

The important point is that an anamorphic lens is not simply a wide lens. It is an optical system with unequal power in the horizontal and vertical planes.

A real anamorphic capture pipeline looks roughly like this:

```text
wide scene -> anamorphic lens compresses image horizontally -> squeezed image recorded -> image desqueezed in post/projection -> wide final image
```

A spherical crop pipeline looks more like this:

```text
normal spherical image -> top and bottom cropped away -> wide final image
```

Those two workflows can produce the same output aspect ratio, but they do not produce the same lens behaviour.

---

## Visual Differences Between Spherical and Anamorphic Images

### 1. Widescreen framing

The most obvious anamorphic association is widescreen framing, usually around 2.39:1.

However, widescreen framing alone is not anamorphic. A spherical lens can be cropped to 2.39:1. The difference is that anamorphic capture historically used more of the recording area by squeezing a wide field onto the frame, while spherical widescreen often discards image area through cropping.

Rimell Anamorphic includes aspect guides, safe area guides, letterbox preview, and output aspect choices because framing is part of the anamorphic workflow, even though framing alone is not the whole effect.

### 2. Elliptical bokeh

Anamorphic lenses are associated with oval or elliptical out-of-focus highlights. A circular point of light can become vertically oval in the final desqueezed image.

Rimell Anamorphic approximates this through:

* oval highlight bloom
* bokeh-style highlight shaping
* stretch and rotation controls
* edge falloff
* threshold scaling
* bloom vignette dimming
* cat-eye style edge dimming

This remains an approximation rather than a physical lens solve. The plugin detects and reshapes highlight regions from the plate, with focus-style response controlled by image-space lens and edge behaviour.

### 3. Differential depth of field

Because an anamorphic lens has different optical behaviour in the horizontal and vertical planes, depth-of-field behaviour can feel different from a spherical lens.

Rimell Anamorphic does not implement a physically complete depth-of-field model. Instead, it approximates the perceived result with edge falloff, focus distance response, edge blur, directional smear, oval highlight bloom, and vertical sharpness compensation.

### 4. Horizontal flares

Horizontal streak flares are one of the most recognisable anamorphic traits. They are caused by the way bright light interacts with the optical system, coatings, and cylindrical elements.

They are also overused. A blue horizontal line is the cheapest shorthand for “anamorphic”, but it is not enough by itself.

Rimell Anamorphic includes directional streak flares, but the plugin is built so flare is only one component of the look. The effect is more convincing when flare works with geometry, edge softness, highlight bloom, CA, and vignette rather than sitting on top as an obvious graphic overlay.

### 5. Veiling glare and bloom

Anamorphic images often show a soft wash around bright sources, depending on the lens, coatings, exposure, and lighting. This can lift the local blacks, soften contrast around highlights, and create a creamy or atmospheric image.

Rimell Anamorphic models this through:

* oval highlight bloom
* veil shaping
* highlight cream
* centre veil contribution
* black-lift protection
* threshold scaling
* sampling/ring controls

### 6. Ghosting and internal reflections

Bright lights inside or near the frame can bounce between optical elements, producing secondary reflections or ghost images.

Rimell Anamorphic includes ghost reflections with count, spread, tint, and intensity. The goal is controlled optical suggestion, not a physically perfect multi-element ray-traced ghost model.

### 7. Barrel, pincushion, mustache, and axis-specific distortion

Spherical lenses often show radial distortion. Anamorphic lenses can include those behaviours, but the distortion is often more complex because the horizontal and vertical axes do not behave the same.

Rimell Anamorphic currently includes:

* OpenLensIO-informed radial distortion coefficients at profile level
* barrel distortion
* mustache distortion
* decentering/asymmetric distortion direction for future profile work
* vertical compensation
* edge compression
* centre protection
* width compensation behaviour
* creative warp mode

### 8. Edge softness and field falloff

Many lenses are sharper in the centre than the edges. Anamorphic lenses can have especially distinctive edge behaviour because aberrations differ across the two axes.

Rimell Anamorphic includes:

* edge focus falloff
* edge blur
* tangential smear
* horizontal smear
* radial falloff shaping
* vertical sharpness compensation

Centre protection is important here. If the entire image is softened evenly, it does not feel like lens character. It just looks blurred.

### 9. Chromatic aberration

Chromatic aberration occurs when different wavelengths of light do not align perfectly. It can produce coloured fringing around high-contrast edges or near the edge of the frame.

Rimell Anamorphic includes:

* lateral chromatic aberration
* pixel-scaled CA
* edge-only CA option
* longitudinal CA approximation
* focus/edge-dependent channel softness

Future OpenLensIO-style profile work should model lateral CA with three separate distortion parameter sets, one for each colour channel.

### 10. Vignetting, light falloff, and cat-eye edge behaviour

Real lenses often darken toward the edges. Wide apertures and optical/mechanical vignetting can also reshape out-of-focus highlights near the frame edge into cat-eye forms.

Rimell Anamorphic includes:

* oval vignette
* asymmetry
* corner bias
* edge highlight vignette
* cat-eye style dimming
* bloom vignette dimming

Future profile work should separate OpenLensIO-style optical vignetting from creative oval vignetting.

### 11. Anamorphic mumps

“Anamorphic mumps” is a close-focus distortion where faces or central subjects can appear wider or oddly swollen. It is associated with changes in anamorphic behaviour at close focus, particularly in older or less corrected designs.

Modern high-end anamorphic lenses often try to avoid this. For a plugin, mumps should therefore be optional, not a default.

Rimell Anamorphic includes close-focus centre squeeze variation and width compensation behaviour so this can be used as a controlled vintage defect rather than an unavoidable problem.

### 12. Focus breathing

Focus breathing is a change in framing or magnification as focus distance changes. It can occur in both spherical and anamorphic lenses, but in anamorphic systems it may also interact with squeeze behaviour and axis-specific geometry.

Rimell Anamorphic includes breathing response based on focus distance. This is not a measured mechanical lens model; it is a creative approximation intended to support animated or keyframed focus-style behaviour.

---

## Why This Matters for a Post-Production Plugin

A convincing anamorphic-style plugin should not treat anamorphic as one isolated effect. The look is cumulative.

Weak approach:

```text
crop to 2.39:1 + add blue flare
```

Better approach:

```text
wide framing + axis-specific geometry + edge falloff + oval highlight shaping + subtle CA + flare/veil/ghosting + controlled vignette
```

Rimell Anamorphic is built around a stronger version of the second approach: the geometry, edge behaviour, chromatic aberration, bloom, flare, ghosts, and vignette all reference a shared virtual lens map where practical.

Depth-map input is disabled for now so Apple hosts can stay on the Metal render path.

The plugin still infers lens calibration from controls rather than measured lens profiles, OpenLensIO payloads, or STMaps.

---

## Current Functionality

Rimell Anamorphic is currently focused on converting normal spherical footage into a stylised, already-desqueezed anamorphic-looking image. It also includes utility controls for genuine anamorphic source footage, but the main creative path assumes the input was shot spherical.

The plugin currently implements the following behaviour:

### 0. Creative look presets

* `Manual`, which leaves the individual controls as set
* `Subtle Modern`, a restrained 1.33x-style starting point
* `Classic 2x`, a stronger vintage-scope starting point
* `Night Flare`, biased toward bright-source streaks, bloom, and ghosts
* `Geometry Only`, which disables additive optical effects and keeps the synthetic mapping path

These presets are exposed as starting points through the normal OFX parameter set. The individual controls remain public, so a host project can still keyframe or override the underlying look controls.

### 1. Input and geometry mode architecture

* Input mode options:

  * `Spherical -> Anamorphic Look` default
  * `Real Anamorphic Utility`
  * `Creative Warp`
* Geometry controls include:

  * lens identity choices: `Custom`, `Modern 1.33x`, `Classic 2x`, `Scope Soft Edge`
  * `Anamorphic Transfer`, which blends from spherical mapping to synthetic anamorphic mapping
  * `Axis Warp`, which adds user-controlled horizontal/vertical separation on top of the selected identity
  * `Center Protection`, which protects the middle of the frame from strong remapping
  * `Edge Compression Start`, which controls where horizontal edge compression begins
  * squeeze mode options: `Off`, `Squeeze`, `Desqueeze` for the real anamorphic utility and creative warp paths
  * squeeze ratio control
  * virtual horizontal expansion behaviour, formerly horizontal FOV boost, influenced by virtual focal length
* OpenLensIO-informed internal geometry parameters include:

  * sensor width / height normalisation
  * virtual focal length `F`
  * distortion centre offset `deltaC`
  * perspective offset `deltaP`
  * radial distortion coefficients `k1...k6`
  * decentering coefficients `p1`, `p2`
  * overscan / ROI expansion values
### 2. Virtual anamorphic geometry and axis-specific warping

* Barrel and mustache distortion
* OpenLensIO-informed radial distortion model direction
* Decentering/asymmetric distortion direction for future profile use
* Vertical compensation
* Edge compression
* Close-focus centre squeeze variation, mumps-style shaping
* Centre protection / width compensation behaviour to limit over-widening in key subjects
* Breathing response based on focus distance

### 3. Edge and field character

* Edge focus falloff behaviour, formerly field curvature contribution
* Edge blur
* Tangential smear and horizontal smear
* Radial falloff shaping
* Vertical sharpness compensation

### 4. Chromatic behaviour

* Lateral chromatic aberration with pixel scaling
* Edge-only CA option
* Longitudinal CA approximation, using focus/edge dependent channel softness
* Future profile direction: per-channel distortion parameter sets for red, green, and blue

### 5. Highlight and flare response

* Highlight-driven response controls, threshold and shaping, used for flare/bloom systems
* Directional streak flare with angle, length, intensity, colour, threshold, density, span, and falloff
* Oval highlight bloom and veil shaping controls, bokeh-style highlight behaviour
* Bloom/veil controls include stretch, rotation, edge falloff, threshold scaling, ring/sampling controls, and highlight cream

### 6. Ghosting and coating-inspired response

* Ghost reflections with count, spread, tint, and intensity
* Coating tint response: `Warm`, `Neutral`, `Cool`, with warm/cool scaling
* Centre veil contribution and black-lift protection behaviour

### 7. Vignette and highlight-edge shaping

* Oval vignette
* Asymmetry and corner bias
* Edge highlight vignette, cat-eye style dimming
* Bloom vignette dimming
* Future OpenLensIO-style optical vignetting coefficients `a1...a3`

### 8. Scope framing, aspect tools, and guides

* Aspect guides on/off
* Output aspect choices:

  * `2.00:1`
  * `2.39:1`
  * `2.66:1`
  * `Custom`
* Safe area guide
* Letterbox preview with opacity
* Adjustable guide strengths for aspect and safe lines

### 9. Blend and performance controls

* Global Mix control
* Render Quality modes:

  * `Draft`
  * `Preview`
  * `Final`
* Quality mode scales expensive sampling paths for flare, bloom, blur, ghosts, and chromatic work
* Control groups are organised as Core, Geometry, Highlights / Flares, Edge / CA, and Framing where the host exposes OFX parameter groups

---

## Conceptual Model and Limits

* Main path intent: spherical input -> virtual anamorphic presentation.
* Main path geometry does not desqueeze normal circular-lens footage; it builds a synthetic anamorphic transfer map and samples the original plate through that map.
* Real anamorphic desqueeze remains available as a utility path for genuinely squeezed source footage.
* The plugin can simulate anamorphic traits, but it cannot reconstruct horizontal scene information that was never captured in-frame.
* Oval highlight behaviour in this workflow is an approximation of anamorphic highlight response, not a physically complete depth-aware bokeh reconstruction.
* Flare, ghosting, glare, CA, vignette, and blur are image-processing approximations rather than calibrated ray-traced simulations of a measured optical assembly.
* The plugin does not currently know the real lens, aperture, subject distance, or sensor size unless those values are provided through a future profile layer.
* External depth-map focus separation is temporarily disabled while the render path is kept Metal-first.
* The plugin does not currently import measured OpenLensIO lens payloads, export OpenLensIO profiles, import STMaps, or export STMaps.
* The profile model is OpenLensIO-informed, not OpenLensIO-complete.

---

## Implementation Notes: How the Effect Maps to Lens Concepts

| Lens concept                  | Real optical behaviour                                               | Rimell Anamorphic approximation                                                                                     |
| ----------------------------- | -------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| OpenLensIO image coordinates  | centred sensor/screen coordinates in mm                              | centred normalised image-space mapping layer                                                                        |
| OpenLensIO radial distortion  | Brown-Conrady radial terms `k1...k6`                                 | barrel/mustache controls and future profile coefficients                                                            |
| OpenLensIO decentering        | `p1`, `p2` distortion terms                                          | future asymmetric/decentred distortion profile support                                                              |
| OpenLensIO centre offsets     | `deltaC`, `deltaP`                                                   | future distortion centre / perspective offset controls                                                              |
| OpenLensIO overscan           | render larger image to cover distortion                              | OFX ROI expansion and render overscan planning                                                                      |
| OpenLensIO vignetting         | polynomial optical vignetting                                        | future profile coefficients; current creative oval vignette                                                         |
| Anamorphic squeeze            | horizontal compression during capture, desqueeze later               | real utility mode supports squeeze/desqueeze; creative mode avoids blunt stretching spherical footage by default    |
| Axis-dependent mapping        | different power in horizontal and vertical planes                    | shared procedural lens map, Anamorphic Transfer, Axis Warp, edge compression, vertical compensation                 |
| Cylindrical optical character | one-axis optical power and axis-dependent aberration                 | horizontal/tangential smear, vertical sharpness compensation, oval highlight shaping                                |
| Elliptical bokeh              | defocused points become vertically oval after desqueeze              | highlight isolation, oval bloom stretch, rotation, edge falloff, cat-eye dimming                                    |
| Depth of field                | focus separation depends on subject distance, aperture, focal length | focus distance, edge falloff, blur, smear, bloom shaping, and longitudinal CA approximations |
| Linear flare                  | bright sources produce horizontal streaks                            | directional streak flare system with angle, span, threshold, density, colour, falloff, and identity scale           |
| Veiling glare                 | optical wash and contrast loss around strong light                   | bloom/veil, centre veil, highlight cream, black-lift protection                                                     |
| Ghosting                      | internal element reflections                                         | count/spread/tint/intensity ghost model with anamorphic axis bias                                                   |
| Chromatic aberration          | wavelength-dependent misregistration                                 | lateral CA, edge-only CA, longitudinal CA approximation; future per-channel distortion sets                         |
| Mumps                         | close-focus anamorphic widening / astigmatic behaviour               | close-focus centre squeeze variation and width compensation                                                         |
| Breathing                     | framing/magnification changes during focus pulls                     | focus-distance based breathing response                                                                             |

---

## Host / Technical Behaviour

* OFX context: Filter
* Source clip: RGBA
* Output clip: RGBA
* Supported bit depths:

  * 8-bit
  * 16-bit
  * 32-bit float
* Frame threading enabled
* Tiled rendering disabled
* Temporal clip access disabled
* Identity optimisation: if Mix is 0, the effect reports source identity
* ROI expansion is implemented to account for flare, bloom, blur, and CA sampling footprint
* On Apple builds, Metal render is advertised for 8-bit, 16-bit, and 32-bit float RGBA when the host supplies an OFX Metal command queue
* Apple builds advertise Metal-only rendering; CPU rendering remains available on non-Apple builds

---

## Current Limitations

The current version does not include:

* strict OpenLensIO file import/export
* true physically based lens simulation
* measured lens profile system
* lens calibration workflow
* STMap import/export workflow
* built-in preset browser beyond the current `Look Preset` parameter
* multi-clip workflows beyond source/output filter processing
* real measured lens model interchange
* physically accurate internal reflection path tracing
* calibrated aperture / circle-of-confusion modelling
* measured entrance pupil behaviour
* practical evaluation of multiple anamorphic distortion models

---

## Possible Future Direction

The current plugin is an OFX filter effect, not a full lens distortion interchange system. A future advanced version could expand towards a more profile-driven workflow.

Possible future features:

* OpenLensIO JSON import for measured/synthetic lens profiles
* Rimell profile export with OpenLensIO-informed lens blocks
* lens profile presets for common anamorphic styles
* STMap export for geometry-only workflows
* STMap import for measured lens distortion
* spherical-to-anamorphic distortion transitions
* calibrated view maps for specific real lenses
* depth/matte input for stronger bokeh behaviour
* GPU render support for non-Metal platforms
* separate utility plugin for desqueeze/framing only
* real-time preview optimisations

One useful long-term model would be to separate the plugin into three layers:

```text
1. Creative optical character: flare, bloom, ghosting, CA, edge softness, bokeh approximation
2. Distortion/profile mapping: squeeze, desqueeze, STMap-style remapping, measured or synthetic lens profiles
3. OpenLensIO interchange: profile import/export, lens-state metadata, overscan, vignetting, CA, and calibrated distortion
```

That would keep the current plugin usable as a creative look tool while making room for a more technical lens-mapping workflow later.

---

## Practical Use Notes

* The effect is designed as a shared virtual lens-map look, not only a flare generator.
* Best results usually come from subtle settings and a non-maximum Mix.
* Highly clipped highlights can reduce nuance in bloom/flare response.
* Heavy flare without geometry or edge treatment can look fake.
* Heavy CA can quickly look like a bad filter rather than lens character.
* If the input is normal spherical footage, use `Spherical -> Anamorphic Look`, not utility desqueeze.
* If the input was actually shot with an anamorphic adapter or lens, use `Real Anamorphic Utility` for desqueeze/framing handling.
* For dialogue or face-heavy footage, use centre protection and width compensation carefully. Too much close-focus shaping can create an ugly mumps effect.
* For night footage, lower flare thresholds and stronger veil may work well, but clipped practicals can produce harsh results.
* For daylight footage, edge falloff, subtle distortion, vignette, and mild CA may be more useful than obvious streak flare.
* Treat OpenLensIO support as a technical/profile direction, not a guarantee that the current look is a measured real lens.

---

## Suggested Starting Points

### Subtle modern anamorphic look

Use this when the footage should still feel clean.

* low-to-medium global Mix
* mild edge compression
* mild oval vignette
* subtle horizontal smear
* low CA
* restrained flare threshold
* minimal ghosting
* centre protection enabled

### Vintage anamorphic look

Use this when the image can feel more imperfect.

* stronger edge blur
* stronger tangential smear
* more visible barrel/mustache distortion
* warmer coating tint
* increased veil
* subtle ghosting
* mild close-focus centre variation
* stronger oval bloom

### Night flare look

Use this when practical lights are visible in-frame.

* lower flare threshold
* longer flare span
* moderate flare intensity
* stronger highlight cream
* controlled black-lift protection
* oval bloom enabled
* ghosting used carefully

### Geometry-only look

Use this when the user wants lens shape without obvious flares.

* flare intensity low or off
* bloom subtle
* edge compression active
* mild barrel/mustache distortion
* edge falloff active
* oval vignette active
* CA very subtle

---

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

After installation, restart your host application. The effect is labelled Rimell Anamorphic in the Rimell/Lens group.

---

## Project Name

Plugin label: Rimell Anamorphic

OFX grouping: Rimell / Lens

Internal profile direction: OpenLensIO-informed Rimell Anamorphic Profile

---

## License

To be decided.

---

## Disclaimer

Rimell Anamorphic is an independent plugin project and is not affiliated with ARRI, ZEISS, Panavision, Blackmagic Design, DaVinci Resolve, SMPTE, SPIE, FrancoAngeli, or other lens/software manufacturers.

The plugin approximates visual characteristics associated with anamorphic optics. It is not a physically complete replacement for shooting with real anamorphic lenses.

OpenLensIO references in this README describe an implementation direction and profile-alignment strategy. They do not imply that the current plugin is a complete, validated, or officially conformant OpenLensIO implementation.
