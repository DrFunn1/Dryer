# Dryer

A physics simulation of an object bouncing around inside a clothes dryer — used as a **deterministic randomness engine** for music and sound design.

The simulation models real physics (gravity, friction, angular momentum, collisions) to produce organic, non-repeating but reproducible rhythmic and timbral patterns. Seed the physics, get a unique but stable sequence. Change the seed, get a different but equally stable one.

## Background

The reference implementation (`core/`) is plain vanilla JavaScript, developed and tested at [drfunn.com/dryer](https://drfunn.com/dryer). The testing period with musicians and sound designers is now complete. This repo migrates the algorithm into multiple deployment targets.

## Repository Structure

```
Dryer/
  core/    JavaScript reference implementation (ported from drfunn-website/dryer)
  Pi/      Raspberry Pi + round HDMI display (shelved — display/cabling too costly for target price)
  VST3/    VST3 plugin (JUCE-based C++) — for DAW integration
  VCV/     VCV Rack module (C++) — for modular synthesis environments
  ESP/     ESP32-S3 + round display — physical Eurorack module build target
```

## Design Goals

- **$120 or under** production cost for the physical Eurorack module (ESP target)
- **Deterministic** — same seed always produces the same sequence
- **Organic** — physics-based, not table-based; feels alive rather than patterned
- **Portable algorithm** — `core/` serves as the canonical spec; all targets are ports of the same physics engine

## Targets

### `core/` — JavaScript Reference
The live web implementation. Used for algorithm development and user testing. Runnable directly in a browser with no build step.

### `VST3/` — DAW Plugin
JUCE-based C++ port. Intended for use inside any VST3-compatible DAW (Ableton, Reaper, Bitwig, etc.).

### `VCV/` — VCV Rack Module
C++ module for the VCV Rack modular synthesis platform. Exposes physics parameters as panel controls and outputs triggers/CV.

### `ESP/` — Eurorack Hardware Module
ESP32-S3 target with a round display (GC9A01 or similar). Physical panel with knobs, CV inputs, and gate outputs. Target build cost under $120.

### `Pi/` — Raspberry Pi (Shelved)
Early prototype using a Raspberry Pi with a round HDMI display. Shelved due to prohibitive display + cabling costs for the production price target.

## Related Repos

- [`drfunn-website`](https://github.com/DrFunn1/drfunn-website) — live web deployment of `core/`
- [`Dryer-pi`](https://github.com/DrFunn1/Dryer-pi) — archived Pi prototype
