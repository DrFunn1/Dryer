# Dryer — CLAUDE.md

Developer reference for the Dryer chaotic percussion generator. This is a physics experiment platform that will eventually ship as a **VST plugin**, **VCV Rack module**, and a **physical Eurorack module** (ESP32-S3 + round display).

---

## What It Is

A ball bouncing inside a rotating drum with internal vanes. Every surface collision triggers a MIDI note (and a Web Audio preview sound). Chaos emerges from deterministic physics — small parameter changes cause wildly different rhythmic patterns.

The web version is the R&D sandbox. All physics and audio logic must eventually be portable to C/C++.

---

## File Map

```
dryer/
  index.html        — Layout, inline CSS, footer/modal JS
  dryer-styles.css  — Shared stylesheet
  dryer-physics.js  — Physics engine (class DryerPhysics)
  dryer-audio.js    — Web Audio + Web MIDI (class DryerAudio)
  dryer-ui.js       — Canvas rendering + knob controls (class DryerUI)
  dryer-main.js     — App controller, wires everything together (class DryerApp)
  footer-dryer.html — Footer with feedback modal
  README.md         — User-facing docs
```

---

## Architecture

### Simulation Loop

```
requestAnimationFrame (≈60fps)
  └─ physics.step(substepDt) × 4   ← 4 substeps = ≈240Hz effective rate
       ├─ apply forces (gravity, buoyancy, centrifugal, Coriolis, air drag)
       ├─ integrate position (Euler)
       └─ handleCollisions()
            ├─ drum wall check
            └─ vane line-segment check × N
  └─ ui.render(physics)
```

### Data Flow

```
UI knobs → physics.setParameters() → surface list regenerated → audio.assignNotesToSurfaces()
physics collision → audio.onCollision(surface, velocity) → Web Audio + MIDI
physics collision → ui.highlightCollision(surface.id) → canvas flash
```

---

## Physics Engine — Key Details

**Reference frame:** Simulation runs in the **rotating frame** of the drum. The drum is stationary; the ball has fictitious forces applied to it.

**Forces applied each substep:**

| Force | Formula | Notes |
|---|---|---|
| Gravity | `a = g × [−sin θ, −cos θ]` | Transformed to rotating frame via drumAngle |
| Buoyancy | `a = (ρ_air × V_ball / m_ball) × −gravity` | Significant for balloon |
| Centrifugal | `a = ω² × r × r̂ × (1 − buoyancyFactor)` | Pushes ball outward |
| Coriolis | `a = 2ω × [vy, −vx] × sign` | Deflects moving ball; was missing, caused "wind" artifact |
| Air drag | Quadratic; relative to vane-coupled air velocity field | `c = 1 − exp(−k × n × h)` coupling coefficient |

**Vane-coupled air model:**
```
c = 1 - exp(-0.5 × vaneCount × vaneHeight)  // coupling 0→1
v_air_tangential(r) = ω × r × (1-c) × (r/R - 1)
```
More vanes or taller vanes → air spins closer to solid-body rotation.

**Surfaces (3 × vaneCount total):**
- `drum_N` — cylindrical wall arc between vane N and N+1
- `vane_N_lead` — leading face of vane N
- `vane_N_trail` — trailing face of vane N

**Collision debounce:** 50ms same-surface lockout (prevents note flooding).

**Feature flags (runtime toggleable):**
```js
physics.enableCoriolis   // default true
physics.enableCentrifugal // default true
physics.enableAirDrag    // default true
physics.coriolisSignFlip  // +1 or -1
```

**Ball presets:**

| Type | Radius | Mass | Restitution | Cd |
|---|---|---|---|---|
| Tennis | 35mm | 58g | 0.75 | 0.55 |
| Sandbag | 50mm | 500g | 0.15 | 0.80 |
| Balloon | 130mm | 12.28g | 0.30 | 0.47 |

Balloon mass = 1g rubber shell + 11.28g air inside (important for correct buoyancy).

**Gravity options:**
- Earth: 9.81 m/s²
- Moon: 1.635 m/s² (1/6th Earth)

**Lint trap:** When enabled, collisions below `lintTrapThreshold` (0.15 m/s) are suppressed — acts as a noise gate.

---

## Audio Engine — Key Details

**MIDI note mapping:**
- Base note: MIDI 24 (C1)
- Notes assigned by cycling through `scaleVector` (interval in semitones per surface, wraps with modulo)
- For diatonic vectors that sum to 12, each full cycle lands on the next octave automatically
- Default scale: `[3, 4]` (Minor 3rds+4ths)
- Scale is changed at runtime via the Scale dropdown; `assignNotesToSurfaces()` must be called after
- Velocity: `min(127, floor(collisionVelocity_m_s × 300))`
- Note duration: `0.15 + (velocity/127) × 0.2` seconds
- Always on MIDI channel 1 (zero-indexed)

**`DRYER_SCALES` constant** (top of `dryer-audio.js`):

| Label | Vector | Sum |
|---|---|---|
| Minor 3rds+4ths | [3, 4] | 7 (cycles across octaves) |
| Chromatic | [1] | 1 |
| Whole Tone | [2] | 2 |
| Whole+Half | [2, 1] | 3 |
| Major | [2, 2, 1, 2, 2, 2, 1] | 12 |
| Natural Minor | [2, 1, 2, 2, 1, 2, 2] | 12 |
| Dorian | [2, 1, 2, 2, 2, 1, 2] | 12 |
| Pentatonic Major | [2, 2, 3, 2, 3] | 12 |
| Pentatonic Minor | [3, 2, 2, 3, 2] | 12 |
| Blues | [3, 2, 1, 1, 3, 2] | 12 |
| Diminished | [2, 1, 2, 1, 2, 1, 2, 1] | 12 |

The dropdown is populated dynamically in `dryer-main.js` from `DRYER_SCALES` — add new scales there, the dropdown updates automatically.

**Web Audio synthesis per hit:**
- Carrier oscillator (sine) at note frequency
- Modulator at `2 × frequency`, modulation depth decays quickly
- White noise burst through highpass filter (attack texture)
- All with exponential decay envelopes

**MIDI output:** First available output port, or user-selectable via `prompt()` if multiple ports.

---

## UI — Key Details

**Knob ranges (as coded in dryer-ui.js):**

| Knob | Min | Max | Default |
|---|---|---|---|
| RPM | 0 | 35 | 18 |
| Drum Size | 40cm | 80cm | 60cm |
| Vanes | 1 | 7 | 4 |
| Vane Height | 10% | 50% | 30% |

> Note: README.md lists different ranges (RPM 1-40, Drum 60-100cm, Vanes 1-9). The README is aspirational — the actual limits are in `dryer-ui.js initKnobs()`.

**Canvas:** 300×300px, drawn to fill a 350px CSS circle (the "dryer window").

**Ball visual:** Always rendered as a yellow tennis ball (green-yellow gradient + white seam arcs). Does not change with ball type — a known gap.

**Collision highlights:** Flash value = 1.0, decays by 0.05 per frame (≈20 frame fade at 60fps).

**Knob drag:** 2px of vertical drag = 1 step. Drag up = increase value.

---

## Console Debug API

A global `dryerDebug` object is available in the browser console:

```js
dryerDebug.help()           // full command list
dryerDebug.show()           // current physics state
dryerDebug.coriolis()       // toggle Coriolis
dryerDebug.centrifugal()    // toggle centrifugal
dryerDebug.drag()           // toggle air drag
dryerDebug.moon()           // toggle moon gravity
dryerDebug.lintTrap()       // toggle velocity filter
dryerDebug.tennis()         // switch ball type
dryerDebug.sandbag()
dryerDebug.balloon()
dryerDebug.gravityOnly()    // debugging preset
dryerDebug.whereBall()      // print ball position + segment info
dryerDebug.debugCollisions() // log every collision to console
dryerDebug.fixSegmentOffset(n) // adjust visual/physics segment alignment
```

---

## Known Issues / Technical Debt

1. **Ball visual is always tennis ball** — `drawBall()` ignores ball type. Sandbag and balloon look identical to tennis ball.
2. **Single-ball only** — physics engine has no multi-object support.
3. **MIDI selection via `prompt()`** — janky UX when multiple ports present.
4. **No parameter persistence** — all knob values reset on page reload.
5. **Segment visual offset** — `segmentIndexOffset` exists as a workaround for a visual/physics mismatch; root cause not fully resolved.
6. **Knob value display in HTML doesn't match initial knob value** — e.g., drumSizeValue shows "80" in HTML but knob initializes to 60.
7. **`stopBtn` starts `.active`** logic is backwards — Stop button should not start active.
8. **No MIDI clock sync** — patterns can't be synced to a DAW tempo.
9. **No polyphonic MIDI** — all notes go to channel 1, no per-surface channel routing.

---

## Target Platforms

### VST Plugin
- Physics engine → C++ DSP thread (no floating point issues, same Euler integration)
- Parameters → VST parameters (automatable)
- MIDI output → plugin MIDI out port
- GUI → plugin editor window (could use JUCE or iPlug2)
- Audio synthesis → plugin audio output (optional; MIDI is primary)
- Key consideration: physics must be sample-accurate or at minimum block-accurate

### VCV Rack Module
- Physics → C++ module `process()` called at audio sample rate or decimated
- CV inputs: RPM, Drum Size, Vanes, Vane Height (±5V or 0-10V)
- Gate/trigger outputs: one per surface (up to 3 × maxVanes = 27 outputs)
- Velocity CV output: collision velocity as 0-10V
- Polyphonic cable option: all surface gates on one poly cable
- No audio synthesis needed — VCV handles that downstream

### Eurorack Hardware (ESP32-S3 + Round Display)
- **MCU:** ESP32-S3 (dual-core Xtensa LX7, 240MHz)
- **Display:** Round TFT, likely 240×240 (GC9A01 driver) via SPI — matches dryer window aesthetic perfectly
- **Physics loop:** Core 0 at ~500Hz (no substeps needed at that rate), Core 1 for display/IO
- **Knobs:** 4× rotary encoders or potentiometers via ADC (12-bit)
- **MIDI out:** Hardware UART at 31.25kbaud (standard 5-pin DIN or TRS)
- **Gate/trigger outputs:** Per-surface gate via shift register or GPIO expander
- **CV outputs:** MCP4728 quad DAC or similar for velocity CV
- **Audio out:** I2S DAC for optional audio synthesis (PCM5102 or similar)
- **Display rendering:** Polar coordinate drawing maps directly to drum circle — draw arc segments and radial vanes from center. Ball position = rotate `(ballX, ballY)` by `drumAngle`, scale to pixels.
- **Physics portability:** `DryerPhysics.step()` is pure math — no DOM, no JS-specific APIs. Port directly to C struct + function.

**ESP32 physics port sketch:**
```c
typedef struct {
    float x, y, vx, vy;
    float radius, mass, restitution, dragCoeff;
} Ball;

typedef struct {
    float drumRadius, drumAngle, drumAngularVelocity;
    int vaneCount;
    float vaneHeight;
    float gravity, airDensity;
    Ball ball;
} DryerState;

void dryer_step(DryerState* s, float dt);
void dryer_handle_collisions(DryerState* s);
```

---

## Extending the Web Version

When adding features:
- **New ball type:** Add preset method to `DryerPhysics`, add `<option>` to `#ballTypeSelect`, add case to `dryer-main.js` change handler. Also update `drawBall()` in `dryer-ui.js` if you want a distinct visual.
- **New force/effect:** Add to `DryerPhysics.step()` with a feature flag. Add a `dryerDebug` shortcut. Keep it togglable for A/B comparison.
- **New surface type:** Must be added to `updateSurfaces()`, collision detection, and `assignNotesToSurfaces()`.
- **New knob:** Add to `initKnobs()` in `dryer-ui.js`, add corresponding HTML in `index.html`, wire in `dryer-main.js` `onParameterChange`.
- **MIDI channels:** `sendMIDINote()` in `dryer-audio.js` — channel is currently hardcoded to 0.

---

## Physics Tuning Notes

- **Too chaotic / never settles:** Increase air drag, reduce restitution, add lint trap
- **Too regular / boring:** Reduce drag, increase restitution, try balloon or ping pong
- **Ball stuck at wall:** Centrifugal too high relative to gravity — reduce RPM or increase gravity
- **Silent stretches:** Too few vanes, ball riding the wall without hitting vanes — increase vane count or height
- **Double-triggering:** Debounce is 50ms (`triggerCollision`). Increase if still getting doubles.
- **Coriolis "wind" artifact:** Was a bug from missing Coriolis force — now included by default. If you see unexpected spiral behavior, check `coriolisSignFlip`.

---

## Audio Tuning Notes

- **Velocity scaling:** `min(127, floor(velocity_m_s × 300))` — at 0.42 m/s you hit max MIDI velocity. Tune the multiplier (300) for desired dynamic range.
- **Note spread:** Alternating +3/+4 semitone pattern from C1. Changing `baseNote` shifts the whole register. Changing the increment pattern changes the harmony/scale.
- **FM ratio:** Modulator is hardcoded at 2× carrier. Changing this changes the timbre character significantly.
- **Noise texture:** Highpass cutoff is at `frequency × 2`. Lowering this gives more body to the attack noise.
