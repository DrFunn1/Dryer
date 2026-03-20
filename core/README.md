# Dryer - Chaotic Percussion Generator

A physics-based eurorack-style percussion generator that simulates a tennis ball bouncing in a rotating dryer drum. Each collision with drum surfaces and vanes generates MIDI notes, creating chaotic but deterministic rhythmic patterns.

## Features

- **Real-time Physics Simulation**: Custom collision detection for rotating drum with configurable vanes
- **Interactive Eurorack UI**: Vintage 1950s dryer aesthetic with rotary knobs
- **Adjustable Parameters**:
  - RPM (1-40): Drum rotation speed
  - Drum Size (60-100cm): Physical drum diameter
  - Vanes (1-9): Number of interior baffles
  - Vane Height (10-50%): How far vanes extend into drum
- **Dual Audio Output**:
  - Built-in Web Audio synthesis for immediate feedback
  - Web MIDI output for external hardware/software synths
- **Visual Feedback**: Surface highlighting on collision events
- **Zero Dependencies**: Pure vanilla JavaScript, no external libraries

## Quick Start

### Local Testing

1. Open `dryer.html` in a modern web browser (Chrome, Firefox, Edge)
2. Click anywhere to initialize audio
3. Click "START" to begin simulation
4. Adjust knobs while running for real-time parameter changes

### Cloudflare Pages Deployment

1. Create a new Cloudflare Pages project
2. Upload these files:
   - `dryer.html` (rename to `index.html`)
   - `dryer-physics.js`
   - `dryer-audio.js`
   - `dryer-ui.js`
   - `dryer-main.js`

3. Configure custom domain: `dryer.drfunn.com`

### MIDI Setup

**For Software Synths:**
1. Install a virtual MIDI port (e.g., loopMIDI on Windows, IAC Driver on Mac)
2. Configure your DAW to listen on that port
3. Click the MIDI status area in Dryer to select the port

**For Hardware Synths:**
1. Connect MIDI interface to your computer
2. Click the MIDI status area to select your MIDI output device
3. Connect interface to your eurorack module or synth

## Technical Details

### Physics Engine

The simulation runs in the rotating reference frame of the drum, applying:
- Gravitational force (transformed to rotating frame)
- Centrifugal force
- Air drag
- Collision detection with:
  - Cylindrical drum wall
  - Radial vanes (line segments)
- Coefficient of restitution: 0.75 (tennis ball)

### MIDI Note Mapping

Each collision surface is assigned a unique MIDI note starting from C2 (MIDI note 36). Notes are assigned chromatically as surfaces are created:
- Drum segments between vanes
- Leading edge of each vane
- Trailing edge of each vane

Velocity is scaled from collision impact velocity (0-127 MIDI range).

### Audio Synthesis

Built-in Web Audio uses FM synthesis to create percussive sounds:
- Carrier frequency based on MIDI note
- Modulator at 2x carrier frequency
- White noise burst for attack
- Exponential decay envelope
- Duration and amplitude scaled by collision velocity

## Browser Compatibility

- **Chrome/Edge**: Full support (recommended)
- **Firefox**: Full support
- **Safari**: Limited MIDI support (Web Audio works)
- **Mobile**: Touch controls supported, MIDI unavailable

## Future Enhancements

Planned features for hardware eurorack module:
- Multiple object types (beanbag, sock, shoe) with different physics properties
- CV inputs for parameter control
- Trigger outputs for individual surfaces
- MIDI clock sync
- Pattern memory/recall

## Development Notes

### Porting to Microcontroller

Key considerations for embedded version:
1. **Physics Loop**: Runs at ~240Hz (4 substeps × 60fps)
2. **Collision Detection**: Simple geometry, no complex libraries needed
3. **Note Assignment**: Direct lookup table
4. **Display**: Could use small circular OLED (128x128)
5. **Knobs**: Standard potentiometers with ADC
6. **MIDI Output**: Hardware UART at 31.25 kbaud

Recommended MCU: ESP32 or Teensy 4.x (sufficient processing power, hardware MIDI)

### Code Structure

```
dryer.html          - Main HTML and CSS
dryer-physics.js    - Physics simulation engine
dryer-audio.js      - Web Audio + Web MIDI
dryer-ui.js         - Canvas rendering and knob controls
dryer-main.js       - Application controller
```

All JavaScript is vanilla ES6, no transpilation needed. Each module is self-contained and could be ported to C/C++ independently.

## License

MIT License - Feel free to use for commercial eurorack products!

## Credits

Created by Edward for experimental electronic music production.
Original concept from 2017 Python/Pygame prototype.

---

**Beta Testing Feedback Welcome!**
Contact: [your contact info]
