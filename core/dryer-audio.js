/**
 * Dryer Audio Engine
 * Handles Web Audio synthesis and Web MIDI output
 */

// Scale vectors: intervals in semitones, cycled indefinitely across all surfaces.
// For diatonic scales the vector sums to 12, so each full cycle lands on the next octave.
// For shorter vectors (e.g. [3,4] sums to 7) the octave accumulates across multiple cycles.
const DRYER_SCALES = [
    { label: 'Minor 3rds+4ths',  vector: [3, 4] },
    { label: 'Chromatic',         vector: [1] },
    { label: 'Whole Tone',        vector: [2] },
    { label: 'Whole+Half',        vector: [2, 1] },
    { label: 'Major',             vector: [2, 2, 1, 2, 2, 2, 1] },
    { label: 'Lydian',            vector: [2, 2, 2, 1, 2, 2, 1] },
    { label: 'Mixolydian',        vector: [2, 2, 1, 2, 2, 1, 2] },
    { label: 'Natural Minor',     vector: [2, 1, 2, 2, 1, 2, 2] },
    { label: 'Dorian',            vector: [2, 1, 2, 2, 2, 1, 2] },
    { label: 'Pentatonic Major',  vector: [2, 2, 3, 2, 3] },
    { label: 'Pentatonic Minor',  vector: [3, 2, 2, 3, 2] },
    { label: 'Blues',             vector: [3, 2, 1, 1, 3, 2] },
    { label: 'Diminished',        vector: [2, 1, 2, 1, 2, 1, 2, 1] },
];

class DryerAudio {
    constructor() {
        this.audioContext = null;
        this.midiOutput = null;
        this.midiEnabled = false;
        this.surfaceToNote = new Map();
        this.baseNote = 24; // C1 - low base for wider note spread
        this.scaleVector = [3, 4]; // default: Minor 3rds+4ths
        this.scatterEnabled = false;
        this.isInitialized = false;
    }
    
    async initialize() {
        if (this.isInitialized) return;
        
        // Initialize Web Audio
        this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        
        // Try to initialize Web MIDI
        try {
            if (navigator.requestMIDIAccess) {
                const midiAccess = await navigator.requestMIDIAccess();
                this.setupMIDI(midiAccess);
            }
        } catch (e) {
            console.log('MIDI not available:', e);
        }
        
        this.isInitialized = true;
    }
    
    setupMIDI(midiAccess) {
        const outputs = Array.from(midiAccess.outputs.values());
        if (outputs.length > 0) {
            this.midiOutput = outputs[0]; // Use first MIDI output
            this.midiEnabled = true;
            console.log('MIDI output connected:', this.midiOutput.name);
        }
        
        // Listen for new MIDI devices
        midiAccess.onstatechange = (e) => {
            if (e.port.type === 'output' && e.port.state === 'connected') {
                this.midiOutput = e.port;
                this.midiEnabled = true;
                console.log('MIDI device connected:', e.port.name);
            }
        };
    }
    
    getMIDIStatus() {
        if (this.midiEnabled && this.midiOutput) {
            return `MIDI: ${this.midiOutput.name}`;
        }
        return 'MIDI: Web Audio Only';
    }
    
    setScale(vector) {
        this.scaleVector = vector;
    }

    setScatter(enabled) {
        this.scatterEnabled = enabled;
    }

    assignNotesToSurfaces(surfaces) {
        this.surfaceToNote.clear();

        // Build the sequential note list from the scale vector
        const notes = [];
        let noteNumber = this.baseNote;
        surfaces.forEach((surface, index) => {
            notes.push(noteNumber);
            noteNumber += this.scaleVector[index % this.scaleVector.length];
        });

        // Scatter: Fisher-Yates shuffle redistributes notes randomly across surfaces.
        // The same set of pitches is used — only which surface plays which note changes.
        if (this.scatterEnabled) {
            for (let i = notes.length - 1; i > 0; i--) {
                const j = Math.floor(Math.random() * (i + 1));
                [notes[i], notes[j]] = [notes[j], notes[i]];
            }
        }

        surfaces.forEach((surface, index) => {
            this.surfaceToNote.set(surface.id, notes[index]);
        });
    }
    
    onCollision(surface, velocity) {
        if (!this.isInitialized) return;
        
        const noteNumber = this.surfaceToNote.get(surface.id) || this.baseNote;
        const velocityMIDI = Math.min(127, Math.floor(velocity * 300)); // Scale collision velocity to MIDI velocity
        
        // Send MIDI if available
        if (this.midiEnabled && this.midiOutput) {
            this.sendMIDINote(noteNumber, velocityMIDI);
        }
        
        // Always play through Web Audio for preview
        this.playWebAudioNote(noteNumber, velocityMIDI, surface.color);
    }
    
    sendMIDINote(noteNumber, velocity) {
        if (!this.midiOutput) return;
        
        const channel = 0; // MIDI channel 1 (0-indexed)
        const noteOn = [0x90 + channel, noteNumber, velocity];
        const noteOff = [0x80 + channel, noteNumber, 0];
        
        this.midiOutput.send(noteOn);
        
        // Send note off after 100ms
        this.midiOutput.send(noteOff, window.performance.now() + 100);
    }
    
    playWebAudioNote(noteNumber, velocity, color) {
        if (!this.audioContext) return;
        
        const now = this.audioContext.currentTime;
        const frequency = this.midiNoteToFrequency(noteNumber);
        const amplitude = velocity / 127 * 0.3; // Scale to reasonable volume
        
        // Create a percussive FM synthesis sound (like a drum hit)
        const duration = 0.15 + (velocity / 127) * 0.2;
        
        // Carrier oscillator
        const carrier = this.audioContext.createOscillator();
        carrier.frequency.setValueAtTime(frequency, now);
        carrier.type = 'sine';
        
        // Modulator for FM synthesis (gives it more body)
        const modulator = this.audioContext.createOscillator();
        modulator.frequency.setValueAtTime(frequency * 2, now);
        
        const modulationGain = this.audioContext.createGain();
        modulationGain.gain.setValueAtTime(frequency * 0.5, now);
        modulationGain.gain.exponentialRampToValueAtTime(0.01, now + duration * 0.3);
        
        modulator.connect(modulationGain);
        modulationGain.connect(carrier.frequency);
        
        // Amplitude envelope
        const gainNode = this.audioContext.createGain();
        gainNode.gain.setValueAtTime(amplitude, now);
        gainNode.gain.exponentialRampToValueAtTime(0.01, now + duration);
        
        // Add some noise for texture
        const noise = this.audioContext.createBufferSource();
        const noiseBuffer = this.createNoiseBuffer(duration);
        noise.buffer = noiseBuffer;
        
        const noiseGain = this.audioContext.createGain();
        noiseGain.gain.setValueAtTime(amplitude * 0.2, now);
        noiseGain.gain.exponentialRampToValueAtTime(0.001, now + duration * 0.3);
        
        const noiseFilter = this.audioContext.createBiquadFilter();
        noiseFilter.type = 'highpass';
        noiseFilter.frequency.setValueAtTime(frequency * 2, now);
        
        noise.connect(noiseFilter);
        noiseFilter.connect(noiseGain);
        noiseGain.connect(this.audioContext.destination);
        
        // Connect carrier to output
        carrier.connect(gainNode);
        gainNode.connect(this.audioContext.destination);
        
        // Start and stop
        carrier.start(now);
        modulator.start(now);
        noise.start(now);
        
        carrier.stop(now + duration);
        modulator.stop(now + duration);
        noise.stop(now + duration);
    }
    
    createNoiseBuffer(duration) {
        const bufferSize = this.audioContext.sampleRate * duration;
        const buffer = this.audioContext.createBuffer(1, bufferSize, this.audioContext.sampleRate);
        const output = buffer.getChannelData(0);
        
        for (let i = 0; i < bufferSize; i++) {
            output[i] = Math.random() * 2 - 1;
        }
        
        return buffer;
    }
    
    midiNoteToFrequency(noteNumber) {
        // MIDI note to frequency: f = 440 * 2^((n-69)/12)
        return 440 * Math.pow(2, (noteNumber - 69) / 12);
    }
    
    async toggleMIDIOutput() {
        if (!navigator.requestMIDIAccess) {
            alert('Web MIDI API not supported in this browser');
            return false;
        }
        
        try {
            const midiAccess = await navigator.requestMIDIAccess();
            const outputs = Array.from(midiAccess.outputs.values());
            
            if (outputs.length === 0) {
                alert('No MIDI outputs detected. Please connect a MIDI device or virtual MIDI port.');
                return false;
            }
            
            // Show selection dialog if multiple outputs
            if (outputs.length > 1) {
                const selection = prompt(
                    'Select MIDI output:\n' + 
                    outputs.map((o, i) => `${i}: ${o.name}`).join('\n')
                );
                const index = parseInt(selection);
                if (index >= 0 && index < outputs.length) {
                    this.midiOutput = outputs[index];
                }
            } else {
                this.midiOutput = outputs[0];
            }
            
            this.midiEnabled = true;
            return true;
        } catch (e) {
            console.error('MIDI access error:', e);
            return false;
        }
    }
}
