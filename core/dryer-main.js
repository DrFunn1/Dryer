/**
 * Dryer Main Controller
 * Integrates physics, audio, and UI components
 */

class DryerApp {
    constructor() {
        this.physics = new DryerPhysics();
        this.audio = new DryerAudio();
        this.ui = new DryerUI();
        
        this.isRunning = false;
        this.animationFrameId = null;
        this.lastTime = 0;
        
        this.init();
    }
    
    async init() {
        // Connect UI parameter changes to physics
        this.ui.onParameterChange = () => {
            const params = this.ui.getParameters();
            this.physics.setParameters(
                params.rpm,
                params.drumSize,
                params.vanes,
                params.vaneHeight
            );
            
            // Update MIDI note assignments when surfaces change
            this.audio.assignNotesToSurfaces(this.physics.surfaces);
        };
        
        // Connect physics collisions to audio and visual feedback
        this.physics.onCollision((surface, velocity) => {
            this.audio.onCollision(surface, velocity);
            this.ui.highlightCollision(surface.id);
        });
        
        // Set up transport buttons
        document.getElementById('startBtn').addEventListener('click', () => this.start());
        document.getElementById('stopBtn').addEventListener('click', () => this.stop());
        
        // Set up new feature controls
        const scaleSelect = document.getElementById('scaleSelect');
        const ballTypeSelect = document.getElementById('ballTypeSelect');
        const lintTrapToggle = document.getElementById('lintTrapToggle');
        const moonGravityToggle = document.getElementById('moonGravityToggle');
        const scatterToggle = document.getElementById('scatterToggle');

        // Populate scale dropdown from DRYER_SCALES (defined in dryer-audio.js)
        if (scaleSelect) {
            DRYER_SCALES.forEach((scale, i) => {
                const option = document.createElement('option');
                option.value = i;
                option.textContent = scale.label;
                if (i === 0) option.selected = true;
                scaleSelect.appendChild(option);
            });

            scaleSelect.addEventListener('change', (e) => {
                const scale = DRYER_SCALES[parseInt(e.target.value)];
                this.audio.setScale(scale.vector);
                this.audio.assignNotesToSurfaces(this.physics.surfaces);
            });
        }
        
        if (ballTypeSelect) {
            ballTypeSelect.addEventListener('change', (e) => {
                if (e.target.value === 'tennis') {
                    this.physics.setTennisBall();
                } else if (e.target.value === 'sandbag') {
                    this.physics.setSandbagBall();
                } else if (e.target.value === 'balloon') {
                    this.physics.setBalloonBall();
                }
                // Reset ball position when changing type
                if (!this.isRunning) {
                    this.physics.reset();
                    this.ui.render(this.physics);
                }
            });
        }
        
        if (lintTrapToggle) {
            lintTrapToggle.addEventListener('change', (e) => {
                this.physics.setLintTrap(e.target.checked);
            });
        }
        
        if (moonGravityToggle) {
            moonGravityToggle.addEventListener('change', (e) => {
                this.physics.setMoonGravity(e.target.checked);
            });
        }

        if (scatterToggle) {
            scatterToggle.addEventListener('change', (e) => {
                this.audio.setScatter(e.target.checked);
                this.audio.assignNotesToSurfaces(this.physics.surfaces);
            });
        }

        // MIDI status click to enable MIDI output
        document.getElementById('midiStatus').addEventListener('click', async () => {
            if (!this.audio.isInitialized) {
                await this.audio.initialize();
            }
            const success = await this.audio.toggleMIDIOutput();
            this.updateMIDIStatus();
        });
        
        // Initialize audio on first user interaction
        document.addEventListener('click', async () => {
            if (!this.audio.isInitialized) {
                await this.audio.initialize();
                this.audio.assignNotesToSurfaces(this.physics.surfaces);
                this.updateMIDIStatus();
            }
        }, { once: true });
        
        // Initial render
        this.ui.render(this.physics);
        
        // Set initial parameters
        this.ui.onParameterChange();
    }
    
    async start() {
        if (this.isRunning) return;
        
        // Initialize audio context if needed
        if (!this.audio.isInitialized) {
            await this.audio.initialize();
            this.audio.assignNotesToSurfaces(this.physics.surfaces);
            this.updateMIDIStatus();
        }
        
        this.isRunning = true;
        this.lastTime = performance.now();
        
        // Update button states
        document.getElementById('startBtn').classList.add('active');
        document.getElementById('stopBtn').classList.remove('active');
        
        // Start animation loop
        this.animate();
    }
    
    stop() {
        if (!this.isRunning) return;
        
        this.isRunning = false;
        
        if (this.animationFrameId) {
            cancelAnimationFrame(this.animationFrameId);
            this.animationFrameId = null;
        }
        
        // Reset ball position
        this.physics.reset();
        
        // Update button states
        document.getElementById('startBtn').classList.remove('active');
        document.getElementById('stopBtn').classList.add('active');
        
        // Final render
        this.ui.render(this.physics);
    }
    
    animate() {
        if (!this.isRunning) return;
        
        const currentTime = performance.now();
        const deltaTime = (currentTime - this.lastTime) / 1000; // Convert to seconds
        this.lastTime = currentTime;
        
        // Limit delta time to prevent large jumps
        const clampedDelta = Math.min(deltaTime, 0.033); // Max 33ms (30 fps minimum)
        
        // Update physics (multiple substeps for stability)
        const substeps = 4;
        const substepDelta = clampedDelta / substeps;
        for (let i = 0; i < substeps; i++) {
            this.physics.step(substepDelta);
        }
        
        // Render
        this.ui.render(this.physics);
        
        // Continue loop
        this.animationFrameId = requestAnimationFrame(() => this.animate());
    }
    
    updateMIDIStatus() {
        const statusElement = document.getElementById('midiStatus');
        const statusText = statusElement.querySelector('.midi-status-text');
        statusText.textContent = this.audio.getMIDIStatus();
        
        if (this.audio.midiEnabled) {
            statusElement.classList.add('connected');
        } else {
            statusElement.classList.remove('connected');
        }
    }
}

// Initialize app when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        window.dryerApp = new DryerApp();
    });
} else {
    window.dryerApp = new DryerApp();
}
