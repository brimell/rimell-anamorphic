# Rimell Anamorphic

Rimell Anamorphic is an OFX filter plugin for making spherical digital footage feel like finished, desqueezed anamorphic footage through a procedural mapping approach called **AxiScope Mapping** / **Virtual Anamorphic Transfer**.

In its main mode, the plugin assumes normal spherical source footage, builds a synthetic anamorphic view map, samples the original image through that map, then drives optical artefacts from the same mapped coordinate field.

A separate utility path is available for real anamorphic source footage that genuinely needs desqueeze and framing handling.

Status: active development / experimental.

This README reflects the current implementation in code.

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

## AxiScope Mapping / Virtual Anamorphic Transfer

The current implementation is built around one shared procedural lens map instead of a set of unrelated look sliders.

For each output pixel, Rimell Anamorphic:

* converts the pixel into normalized source coordinates
* builds a procedural virtual lens coordinate field
* blends from spherical coordinates into an axis-dependent anamorphic coordinate using `Anamorphic Transfer`
* applies center protection, edge compression, axis-weighted barrel/mustache distortion, mumps, breathing, and virtual horizontal expansion
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

In a VFX or post-production context, spherical footage is often easier to work with because its distortions are usually describable with standard radial lens models. These can include barrel distortion, pincushion distortion, mustache distortion, decentering, thin-prism distortion, vignetting, chromatic aberration, flare, and bokeh. Many of these are not unique to spherical lenses, but their behaviour is usually less axis-dependent than with anamorphic glass.

---

### Anamorphic lenses

An anamorphic lens has different imaging behaviour in two perpendicular axes. The common cinema version uses cylindrical optical elements to compress a wide horizontal field of view onto a narrower recording format.

The important point is that an anamorphic lens is not simply a wide lens. It is an optical system with unequal power in the horizontal and vertical planes.

In practical cinema terms, anamorphic lenses were historically used to record widescreen images onto standard film frames. Instead of cropping away the top and bottom of the film frame, the lens squeezed the wider image onto the available frame area. The image was then desqueezed later for projection or post-production.

This allowed a widescreen image to use more of the available film area than a simple spherical crop. That is one of the reasons anamorphic capture became important in cinema: it was both a format solution and an optical aesthetic.

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

### Aspect ratio, storage aspect ratio, display aspect ratio, and pixel aspect ratio

Anamorphic imaging is closely tied to the distinction between the shape of the stored image and the shape of the displayed image.

Useful terms:

| Term                           | Meaning                                                                         |
| ------------------------------ | ------------------------------------------------------------------------------- |
| `Storage Aspect Ratio` / `SAR` | The pixel dimensions of the stored image, such as 1920x1080 or 4096x2160        |
| `Display Aspect Ratio` / `DAR` | The intended displayed shape of the image, such as 2.39:1                       |
| `Pixel Aspect Ratio` / `PAR`   | The shape relationship of each pixel when displayed; square pixels have PAR 1:1 |
| `Squeeze Ratio`                | The anamorphic compression factor, such as 1.33x, 1.5x, or 2x                   |

A simplified relationship is:

```text
PAR = DAR / SAR
```

In ordinary square-pixel footage, the storage shape and display shape match directly. In anamorphic workflows, the recorded image may be stored squeezed and then displayed wider by applying a pixel aspect or desqueeze transform.

Example:

```text
4:3 stored image + 2x anamorphic desqueeze -> roughly 2.66:1 display image
```

Modern workflows may use square pixels and simply apply a software desqueeze, but the conceptual idea is the same: the capture and display geometry are not identical.

Rimell Anamorphic separates this into two different use cases:

1. **Real Anamorphic Utility**: for footage that was genuinely captured squeezed and needs correct desqueeze/framing handling.
2. **Spherical -> Anamorphic Look**: for normal spherical footage where the plugin should not literally desqueeze the image, because there is no squeezed source image to restore.

This distinction matters. Desqueezing normal spherical footage would just stretch it. It would make people wider and distort the entire image in a blunt way. The default creative mode instead keeps the plate fundamentally usable while adding virtual anamorphic characteristics on top.

---

### Object-to-image mapping

At the simplest level, a lens maps object space to image space.

A spherical lens usually has the same effective focal behaviour around the optical axis. Its image mapping is broadly constant with rotation around the centre. If a point is displaced left, right, up, or down from centre, the mapping is governed by the same radial behaviour.

An anamorphic lens breaks that symmetry. It has different power in two orthogonal planes. The horizontal and vertical directions are not treated as interchangeable.

This produces:

* different effective focal lengths in the two axes
* different magnification in the two axes
* different distortion behaviour in the two axes
* different aberration behaviour in the two axes
* different focus and blur behaviour in the two axes

A simple way to think about it:

```text
spherical lens:    x and y are treated similarly
anamorphic lens:  x and y are treated differently
```

This is why Rimell Anamorphic contains axis-specific controls such as edge compression, vertical compensation, horizontal smear, tangential smear, squeeze ratio, and vertical sharpness compensation.

---

### Cylindrical optics

The classic anamorphic lens uses cylindrical optical surfaces. A spherical lens surface has curvature in both axes. A cylindrical surface has power mainly in one axis.

That is the physical reason anamorphic lenses can compress or expand one dimension more than the other.

A single cylindrical element can focus light differently in one plane than another, which introduces astigmatic behaviour unless the system is carefully corrected. Real anamorphic lens design is therefore much harder than simply adding a cylinder to a normal lens. The horizontal and vertical planes need to be made to image onto a compatible focal plane, otherwise one axis can be sharp while the other is not.

This is also why real anamorphic zoom lenses are especially complex. A zoom lens already has moving variator and compensator groups to change focal length while keeping focus stable. An anamorphic zoom adds the requirement that the two orthogonal imaging planes maintain a consistent anamorphic ratio and remain optically usable through the zoom range.

Rimell Anamorphic is not designing real lens groups, but its controls are based on the same broad idea: the image should be handled differently in the horizontal and vertical directions.

---

### Squeeze and desqueeze

A real anamorphic lens records a squeezed image. The squeeze ratio describes how much compression is applied.

Common squeeze ratios include:

| Ratio   | Typical use / effect                                                                                         |
| ------- | ------------------------------------------------------------------------------------------------------------ |
| `1.25x` | mild anamorphic widening, often for modern digital formats                                                   |
| `1.33x` | common adapter ratio for 16:9 sensors aiming for a wider frame                                               |
| `1.5x`  | stronger anamorphic character while staying manageable on digital sensors                                    |
| `1.8x`  | closer to classic 2x feel with slightly less extreme handling                                                |
| `2x`    | classic strong anamorphic squeeze, often associated with pronounced oval bokeh and widescreen cinema formats |

With real anamorphic source footage, desqueeze is a corrective operation. It restores the intended geometry.

With spherical source footage, desqueeze is usually the wrong operation. There is no squeezed image to undo. Stretching the image only makes the plate wider. It does not create genuine extra field of view, and it can make faces and objects look obviously wrong.

That is why the default mode of Rimell Anamorphic is not a literal desqueeze of spherical footage. Instead, it creates a virtual anamorphic presentation by layering geometry, edge, highlight, flare, and chromatic behaviours over the original spherical plate.

---

### Field of view and lost information

A real anamorphic lens can capture a wider horizontal field of view onto a given sensor or film frame because the lens compresses that wider field into the recorded image.

A post-production plugin cannot do that retroactively.

If the camera did not record extra information at the left and right edges of the scene, Rimell Anamorphic cannot invent a real wider field of view. It can only:

* crop to a wider aspect ratio
* simulate horizontal expansion behaviour
* warp or compress parts of the frame
* create anamorphic-style edge and highlight treatment
* add flares, bloom, bokeh approximation, vignetting, and chromatic behaviour

This is an important limitation. The plugin can make spherical footage feel more anamorphic, but it cannot make it identical to having used a real anamorphic lens on set.

---

## Visual Differences Between Spherical and Anamorphic Images

### 1. Widescreen framing

The most obvious anamorphic association is widescreen framing, usually around 2.39:1.

However, widescreen framing alone is not anamorphic. A spherical lens can be cropped to 2.39:1. The difference is that anamorphic capture historically used more of the recording area by squeezing a wide field onto the frame, while spherical widescreen often discards image area through cropping.

Rimell Anamorphic includes aspect guides, safe area guides, letterbox preview, and output aspect choices because framing is part of the anamorphic workflow, even though framing alone is not the whole effect.

---

### 2. Elliptical bokeh

Anamorphic lenses are associated with oval or elliptical out-of-focus highlights. A circular point of light can become vertically oval in the final desqueezed image.

This happens because the optical system treats the two axes differently. The squeeze/desqueeze relationship and cylindrical elements affect how defocused points are shaped.

Spherical lenses more commonly produce circular bokeh, modified by aperture blade shape, optical vignetting, cat-eye effects, and lens design.

Rimell Anamorphic approximates this through:

* oval highlight bloom
* bokeh-style highlight shaping
* stretch and rotation controls
* edge falloff
* threshold scaling
* bloom vignette dimming
* cat-eye style edge dimming

This is an approximation. Without depth data or true focus information, the plugin cannot fully reconstruct real depth-dependent bokeh. It can detect and reshape highlight regions, but it does not know the true distance of every object in the frame.

---

### 3. Differential depth of field

Because an anamorphic lens has different optical behaviour in the horizontal and vertical planes, depth-of-field behaviour can feel different from a spherical lens.

The image may feel wide horizontally while retaining a different vertical field relationship. This is part of the reason anamorphic footage can feel spacious but still intimate. The exact behaviour depends on sensor size, focal length, squeeze ratio, aperture, focus distance, lens design, and how the lens is desqueezed.

Rimell Anamorphic does not currently implement a physically complete depth-of-field model. Instead, it approximates the perceived result with edge falloff, focus distance response, edge blur, directional smear, oval highlight bloom, and vertical sharpness compensation.

---

### 4. Horizontal flares

Horizontal streak flares are one of the most recognisable anamorphic traits. They are caused by the way bright light interacts with the optical system, coatings, and cylindrical elements.

They are also overused. A blue horizontal line is the cheapest shorthand for “anamorphic”, but it is not enough by itself.

Rimell Anamorphic includes directional streak flares, but the plugin is built so flare is only one component of the look. The effect is more convincing when flare works with geometry, edge softness, highlight bloom, CA, and vignette rather than sitting on top as an obvious graphic overlay.

Implemented controls include:

* flare angle
* flare length
* flare intensity
* flare colour
* flare threshold
* flare density
* flare span
* flare falloff
* highlight-driven response shaping

---

### 5. Veiling glare and bloom

Anamorphic images often show a soft wash around bright sources, depending on the lens, coatings, exposure, and lighting. This can lift the local blacks, soften contrast around highlights, and create a creamy or atmospheric image.

This is different from a clean digital glow. Real glare interacts with contrast and colour in a messy optical way.

Rimell Anamorphic models this through:

* oval highlight bloom
* veil shaping
* highlight cream
* centre veil contribution
* black-lift protection
* threshold scaling
* sampling/ring controls

Black-lift protection exists because uncontrolled veil can make an image look muddy very quickly.

---

### 6. Ghosting and internal reflections

Bright lights inside or near the frame can bounce between optical elements, producing secondary reflections or ghost images.

This is not unique to anamorphic lenses, but anamorphic systems can produce distinctive ghosting because their optical path is more complex and axis-dependent.

Rimell Anamorphic includes ghost reflections with:

* count
* spread
* tint
* intensity

The goal is controlled optical suggestion, not a physically perfect multi-element ray-traced ghost model.

---

### 7. Barrel, pincushion, mustache, and axis-specific distortion

Spherical lenses often show radial distortion:

| Distortion   | Description                                                                                          |
| ------------ | ---------------------------------------------------------------------------------------------------- |
| `Barrel`     | straight lines bow outward from the centre                                                           |
| `Pincushion` | straight lines pinch inward toward the centre                                                        |
| `Mustache`   | distortion changes character across the radius, often barrel in one region and pincushion in another |

Anamorphic lenses can include those behaviours, but the distortion is often more complex because the horizontal and vertical axes do not behave the same.

This is where an anamorphic or aximorphic distortion model becomes useful: rather than applying one radial distortion equally, the plugin can shape the image differently by axis.

Rimell Anamorphic currently includes:

* barrel distortion
* mustache distortion
* vertical compensation
* edge compression
* centre protection
* width compensation behaviour
* creative warp mode

This is one of the most important parts of the plugin because it moves the effect away from a simple overlay and into image-space remapping.

---

### 8. Edge softness and field falloff

Many lenses are sharper in the centre than the edges. Anamorphic lenses can have especially distinctive edge behaviour because aberrations differ across the two axes.

The edge may appear stretched, smeared, softer, or less stable than the centre. This can be attractive when subtle because it draws attention toward the subject and makes the frame feel less digitally flat.

Rimell Anamorphic includes:

* edge focus falloff
* edge blur
* tangential smear
* horizontal smear
* radial falloff shaping
* vertical sharpness compensation

Centre protection is important here. If the entire image is softened evenly, it does not feel like lens character. It just looks blurred.

---

### 9. Chromatic aberration

Chromatic aberration occurs when different wavelengths of light do not align perfectly. It can produce coloured fringing around high-contrast edges or near the edge of the frame.

It is not exclusive to anamorphic lenses, but it is part of the broader optical character that many people associate with vintage or imperfect cinema glass.

Rimell Anamorphic includes:

* lateral chromatic aberration
* pixel-scaled CA
* edge-only CA option
* longitudinal CA approximation
* focus/edge-dependent channel softness

Subtle CA can help sell the illusion of an optical process. Heavy CA quickly looks like a bad filter.

---

### 10. Vignetting, light falloff, and cat-eye edge behaviour

Real lenses often darken toward the edges. Wide apertures and optical/mechanical vignetting can also reshape out-of-focus highlights near the frame edge into cat-eye forms.

Anamorphic systems can make this feel more elliptical or axis-dependent.

Rimell Anamorphic includes:

* oval vignette
* asymmetry
* corner bias
* edge highlight vignette
* cat-eye style dimming
* bloom vignette dimming

This is partly aesthetic and partly corrective: the edges of the frame should often feel optically different from the centre.

---

### 11. Anamorphic mumps

“Anamorphic mumps” is a close-focus distortion where faces or central subjects can appear wider or oddly swollen. It is associated with changes in anamorphic behaviour at close focus, particularly in older or less corrected designs.

Modern high-end anamorphic lenses often try to avoid this. For a plugin, mumps should therefore be optional, not a default.

Rimell Anamorphic includes close-focus centre squeeze variation and width compensation behaviour so this can be used as a controlled vintage defect rather than an unavoidable problem.

---

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

It uses an OFX filter architecture so the effect can be applied directly to footage in a host application. The current implementation works as source-to-output image processing rather than as a multi-input compositing system. That means the plugin has to infer highlight behaviour from the plate itself and cannot yet use depth maps, lens calibration profiles, or separate matte inputs.

---

## Current Functionality

Rimell Anamorphic is currently focused on converting normal spherical footage into a stylized, already-desqueezed anamorphic-looking image. It also includes utility controls for genuine anamorphic source footage, but the main creative path assumes the input was shot spherical.

The plugin currently implements the following behaviour:

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

### 2. Virtual anamorphic geometry and axis-specific warping

* Barrel and mustache distortion
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

---

## Conceptual Model and Limits

* Main path intent: spherical input -> virtual anamorphic presentation.
* Main path geometry does not desqueeze normal circular-lens footage; it builds a synthetic anamorphic transfer map and samples the original plate through that map.
* Real anamorphic desqueeze remains available as a utility path for genuinely squeezed source footage.
* The plugin can simulate anamorphic traits, but it cannot reconstruct horizontal scene information that was never captured in-frame.
* Oval highlight behaviour in this workflow is an approximation of anamorphic highlight response, not a physically complete depth-aware bokeh reconstruction.
* Flare, ghosting, glare, CA, vignette, and blur are image-processing approximations rather than calibrated ray-traced simulations of a measured optical assembly.
* The plugin does not currently know the real lens, aperture, focus distance, subject distance, sensor size, or scene depth unless those behaviours are manually approximated through controls.
* The plugin does not currently import measured lens profiles, export STMaps, or blend external view/footage maps.

---

## Implementation Notes: How the Effect Maps to Lens Concepts

| Lens concept                  | Real optical behaviour                                  | Rimell Anamorphic approximation                                                                            |
| ----------------------------- | ------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| Anamorphic squeeze            | Horizontal compression during capture, desqueeze later  | Real utility mode supports squeeze/desqueeze; creative mode avoids stretching spherical footage by default |
| Axis-dependent mapping        | Different power in horizontal and vertical planes       | Shared procedural lens map, Anamorphic Transfer, Axis Warp, edge compression, vertical compensation       |
| Cylindrical optical character | One-axis optical power and axis-dependent aberration    | Horizontal/tangential smear, vertical sharpness compensation, oval highlight shaping                       |
| Elliptical bokeh              | Defocused points become vertically oval after desqueeze | Highlight isolation, oval bloom stretch, rotation, edge falloff, cat-eye dimming                           |
| Linear flare                  | Bright sources produce horizontal streaks               | Directional streak flare system with angle, span, threshold, density, colour, falloff, and identity scale |
| Veiling glare                 | Optical wash and contrast loss around strong light      | Bloom/veil, centre veil, highlight cream, black-lift protection                                            |
| Ghosting                      | Internal element reflections                            | Count/spread/tint/intensity ghost model with anamorphic axis bias                                          |
| Distortion                    | Barrel, pincushion, mustache, axis-dependent warp       | Barrel/mustache distortion, centre protection, edge compression                                            |
| Chromatic aberration          | Wavelength-dependent misregistration                    | Lateral CA, edge-only CA, longitudinal CA approximation                                                    |
| Mumps                         | Close-focus anamorphic widening / astigmatic behaviour  | Close-focus centre squeeze variation and width compensation                                                |
| Breathing                     | Framing/magnification changes during focus pulls        | Focus-distance based breathing response                                                                    |

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
* Identity optimization: if Mix is 0, the effect reports source identity
* ROI expansion is implemented to account for flare, bloom, blur, and CA sampling footprint

---

## What Is Not Implemented Yet

The current version does not include:

* Depth-map or matte input for depth-aware bokeh
* True physically based lens simulation
* Measured lens profile system
* Lens calibration workflow
* STMap import/export workflow
* GPU render path
* Built-in preset browser
* Multi-clip workflows beyond source/output filter processing
* Real measured lens model interchange
* Physically accurate internal reflection path tracing

---

## Possible Future Direction

The current plugin is an OFX filter effect, not a full lens distortion interchange system. A future advanced version could expand towards a more profile-driven workflow.

Possible future features:

* lens profile presets for common anamorphic styles
* STMap export for geometry-only workflows
* STMap import for measured lens distortion
* spherical-to-anamorphic distortion transitions
* calibrated view maps for specific real lenses
* depth/matte input for stronger bokeh behaviour
* GPU render path for heavier sampling
* separate utility plugin for desqueeze/framing only
* real-time preview optimisations

One useful long-term model would be to separate the plugin into two layers:

```text
1. Creative optical character: flare, bloom, ghosting, CA, edge softness, bokeh approximation
2. Distortion/profile mapping: squeeze, desqueeze, STMap-style remapping, measured or synthetic lens profiles
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

## Project Name

Plugin label: Rimell Anamorphic

OFX grouping: Rimell / Lens

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

## License

To be decided.

---

## Disclaimer

Rimell Anamorphic is an independent plugin project and is not affiliated with ARRI, ZEISS, Panavision, Blackmagic Design, DaVinci Resolve, SPIE, FrancoAngeli, or other lens/software manufacturers.

The plugin approximates visual characteristics associated with anamorphic optics. It is not a physically complete replacement for shooting with real anamorphic lenses.
