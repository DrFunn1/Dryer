# Dryer — VCV Rack Module

Physics-based chaotic percussion and CV generator for VCV Rack 2. A ball bouncing inside a rotating drum with internal vanes. Every collision fires a trigger and outputs a pitch CV — deterministic chaos from real physics.

## Panel (16HP)

```
┌─────────────────────────────┐
│  CLK   TRG    CV    MIDI    │  ← outputs
│ [MOON]              [LINT]  │  ← latching buttons
│                             │
│       ┌─────────┐           │
│       │ display │           │
│       └─────────┘           │
│                             │
│ SPEED  SIZE  NUMBER HEIGHT  │  ← knobs (DRUM | VANES)
│  cv    cv     cv     cv     │  ← CV inputs
│        DRUM        VANES    │  ← group labels
│            DRYER            │
└─────────────────────────────┘
```

## Outputs

| Jack | Signal | Notes |
|------|--------|-------|
| CLK  | Square wave | Speed × 4 / 60 Hz — 20 RPM → 80 BPM. Independent of collisions. |
| TRG  | 10ms gate | Fires on every ball collision |
| CV   | V/oct | Pitch of last collision surface (scale-based) |
| MIDI | Polyphonic V/oct | One channel per surface (up to 21 channels at 7 vanes) |

## Controls

| Control | Range | Notes |
|---------|-------|-------|
| SPEED | 0–35 RPM | Drum rotation speed. Also sets CLK frequency. |
| SIZE | 40–80 cm | Drum diameter |
| NUMBER | 1–7 (integer) | Number of vanes |
| HEIGHT | 10–50% (integer) | Vane height as fraction of drum radius |
| MOON | toggle | Switches gravity to 1/6 Earth (1.635 m/s²) |
| LINT | toggle | Velocity noise gate — ignores collisions below 0.15 m/s |

All four knobs accept ±5V CV for external modulation (additive to knob position).

## MIDI (right-click menu)

- **MIDI Output** — sends Note On/Off with pitch and velocity on every collision. Velocity = impact speed × 300 (clamped to 127).
- **MIDI Input** — CC control: CC1=Speed, CC2=Size, CC3=Number, CC4=Height. CC overrides the knob.

## Pitch / Scale

Surfaces are assigned notes by cycling through a scale vector starting at C2 (MIDI 36). Default scale: minor 3rds + 4ths `[3, 4]`. With 4 vanes (12 surfaces), the pattern spans just over one octave. Output is standard V/oct (0V = C4).

## Physics

The simulation runs in the rotating reference frame of the drum at 250Hz (decimated from audio rate). Forces applied each step:

- **Gravity** — transformed to rotating frame via drum angle
- **Buoyancy** — Archimedes' principle (significant for balloon preset)
- **Centrifugal** — pushes ball outward, proportional to ω² × r
- **Coriolis** — deflects moving ball perpendicular to velocity
- **Air drag** — quadratic, relative to vane-coupled air velocity field

Ball defaults to tennis ball (35mm radius, 58g, restitution 0.75).

## Build Environment

Built with `rack-plugin-toolchain` (v2 branch) running in WSL2 on Windows 11.

```bash
# In WSL2
cd ~/rack-plugin-toolchain
make plugin-build-win-x64 PLUGIN_DIR=/mnt/m/TEMP/Dryer/VCV
```

Requires:
- WSL2 Ubuntu 22.04 (ext4 filesystem — NTFS breaks crosstool-ng)
- Toolchain built at `~/rack-plugin-toolchain` (Windows cross-compiler)
- Rack SDK 2.6.6 at `~/rack-plugin-toolchain/Rack-SDK-win-x64`

## Roadmap

- [ ] Display widget — render spinning drum, vanes, and ball in the panel circle
- [ ] Ball type selector (tennis / sandbag / balloon) via right-click
- [ ] Scale selector via right-click (matches web version scales)
- [ ] Polyphonic trigger output (separate gate per surface)
- [ ] Linux build (add `toolchain-lin` to WSL toolchain)
- [ ] macOS build (requires Mac to generate SDK)
