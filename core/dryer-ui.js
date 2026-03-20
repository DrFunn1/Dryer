/**
 * Dryer UI Controller
 * Handles knob interactions and visual feedback
 */

class DryerUI {
    constructor() {
        this.knobs = {};
        this.canvas = document.getElementById('dryerCanvas');
        this.ctx = this.canvas.getContext('2d');
        this.activeCollisions = new Map();
        
        this.initKnobs();
    }
    
    initKnobs() {
        // Define knob configurations
        const knobConfigs = [
            { id: 'rpm', min: 0, max: 35, initial: 18, step: 1 },
            { id: 'drumSize', min: 40, max: 80, initial: 60, step: 1 },
            { id: 'vanes', min: 1, max: 7, initial: 4, step: 1 },
            { id: 'vaneHeight', min: 10, max: 50, initial: 30, step: 1 }
        ];
        
        knobConfigs.forEach(config => {
            this.initKnob(config);
        });
    }
    
    initKnob(config) {
        const knobElement = document.getElementById(`${config.id}Knob`);
        const valueElement = document.getElementById(`${config.id}Value`);
        const indicator = knobElement.querySelector('.knob-indicator');
        
        this.knobs[config.id] = {
            element: knobElement,
            valueElement: valueElement,
            indicator: indicator,
            value: config.initial,
            min: config.min,
            max: config.max,
            step: config.step,
            isDragging: false,
            startY: 0,
            startValue: config.initial
        };
        
        // Set initial rotation
        this.updateKnobRotation(config.id);
        
        // Mouse events
        knobElement.addEventListener('mousedown', (e) => this.startKnobDrag(config.id, e));
        document.addEventListener('mousemove', (e) => this.updateKnobDrag(config.id, e));
        document.addEventListener('mouseup', () => this.endKnobDrag(config.id));
        
        // Touch events for mobile
        knobElement.addEventListener('touchstart', (e) => {
            e.preventDefault();
            this.startKnobDrag(config.id, e.touches[0]);
        });
        document.addEventListener('touchmove', (e) => {
            if (this.knobs[config.id].isDragging) {
                e.preventDefault();
                this.updateKnobDrag(config.id, e.touches[0]);
            }
        });
        document.addEventListener('touchend', () => this.endKnobDrag(config.id));
    }
    
    startKnobDrag(knobId, event) {
        const knob = this.knobs[knobId];
        knob.isDragging = true;
        knob.startY = event.clientY;
        knob.startValue = knob.value;
    }
    
    updateKnobDrag(knobId, event) {
        const knob = this.knobs[knobId];
        if (!knob.isDragging) return;
        
        const deltaY = knob.startY - event.clientY; // Inverted: drag up increases
        const range = knob.max - knob.min;
        const sensitivity = 2; // pixels per step
        const steps = Math.round(deltaY / sensitivity);
        
        let newValue = knob.startValue + (steps * knob.step);
        newValue = Math.max(knob.min, Math.min(knob.max, newValue));
        
        if (newValue !== knob.value) {
            knob.value = newValue;
            this.updateKnobRotation(knobId);
            
            // Trigger callback if exists
            if (this.onParameterChange) {
                this.onParameterChange();
            }
        }
    }
    
    endKnobDrag(knobId) {
        const knob = this.knobs[knobId];
        knob.isDragging = false;
    }
    
    updateKnobRotation(knobId) {
        const knob = this.knobs[knobId];
        const normalized = (knob.value - knob.min) / (knob.max - knob.min);
        const degrees = -135 + (normalized * 270); // -135° to +135° range
        
        knob.indicator.style.transform = `translateX(-50%) rotate(${degrees}deg)`;
        knob.valueElement.textContent = Math.round(knob.value);
    }
    
    getParameters() {
        return {
            rpm: this.knobs.rpm.value,
            drumSize: this.knobs.drumSize.value,
            vanes: this.knobs.vanes.value,
            vaneHeight: this.knobs.vaneHeight.value
        };
    }
    
    render(physics) {
        const ctx = this.ctx;
        const width = this.canvas.width;
        const height = this.canvas.height;
        
        // Clear canvas
        ctx.fillStyle = '#000';
        ctx.fillRect(0, 0, width, height);
        
        // Draw drum segments with collision highlighting
        this.drawDrumSegments(physics);
        
        // Draw vanes
        this.drawVanes(physics);
        
        // Draw ball
        this.drawBall(physics);
        
        // Decay collision highlights
        this.updateCollisionHighlights();
    }
    
    drawDrumSegments(physics) {
        const ctx = this.ctx;
        const centerX = this.canvas.width / 2;
        const centerY = this.canvas.height / 2;
        const scale = this.canvas.width / (physics.drumRadius * 2.2);
        const radius = physics.drumRadius * scale;
        
        // Draw each segment between vanes
        const anglePerSegment = (2 * Math.PI) / physics.vaneCount;
        
        for (let i = 0; i < physics.vaneCount; i++) {
            const startAngle = (i * anglePerSegment) + physics.drumAngle;
            const endAngle = startAngle + anglePerSegment;
            
            const surface = physics.surfaces.find(s => s.type === 'drum' && s.index === i);
            const highlight = this.activeCollisions.get(surface?.id) || 0;
            
            // Base color with collision highlight
            const baseColor = surface ? surface.color : '#333';
            const alpha = 0.3 + (highlight * 0.5);
            
            ctx.beginPath();
            ctx.arc(centerX, centerY, radius, -startAngle, -endAngle, true);
            ctx.strokeStyle = baseColor;
            ctx.globalAlpha = alpha;
            ctx.lineWidth = 8;
            ctx.stroke();
            ctx.globalAlpha = 1.0;
        }
    }
    
    drawVanes(physics) {
        const ctx = this.ctx;
        const vanes = physics.getVanePositions(this.canvas.width);
        
        vanes.forEach(vane => {
            // Check for collision highlights on vane surfaces
            const leadSurface = physics.surfaces.find(s => s.type === 'vane_leading' && s.index === vane.index);
            const trailSurface = physics.surfaces.find(s => s.type === 'vane_trailing' && s.index === vane.index);
            
            const leadHighlight = this.activeCollisions.get(leadSurface?.id) || 0;
            const trailHighlight = this.activeCollisions.get(trailSurface?.id) || 0;
            const maxHighlight = Math.max(leadHighlight, trailHighlight);
            
            const color = leadSurface ? leadSurface.color : '#555';
            
            ctx.beginPath();
            ctx.moveTo(vane.innerX, vane.innerY);
            ctx.lineTo(vane.outerX, vane.outerY);
            ctx.strokeStyle = color;
            ctx.globalAlpha = 0.8 + (maxHighlight * 0.2);
            ctx.lineWidth = 4 + (maxHighlight * 4);
            ctx.stroke();
            ctx.globalAlpha = 1.0;
        });
    }
    
    drawBall(physics) {
        const ctx = this.ctx;
        const ball = physics.getBallPosition(this.canvas.width);
        
        // Tennis ball appearance
        const gradient = ctx.createRadialGradient(
            ball.x - ball.radius * 0.3, 
            ball.y - ball.radius * 0.3, 
            0,
            ball.x, 
            ball.y, 
            ball.radius
        );
        gradient.addColorStop(0, '#e8f436');
        gradient.addColorStop(1, '#b8c406');
        
        ctx.beginPath();
        ctx.arc(ball.x, ball.y, ball.radius, 0, Math.PI * 2);
        ctx.fillStyle = gradient;
        ctx.fill();
        
        // Tennis ball seam lines
        ctx.strokeStyle = '#ffffff';
        ctx.lineWidth = 1.5;
        ctx.globalAlpha = 0.6;
        ctx.beginPath();
        ctx.arc(ball.x, ball.y, ball.radius * 0.7, 0.2, Math.PI - 0.2);
        ctx.stroke();
        ctx.beginPath();
        ctx.arc(ball.x, ball.y, ball.radius * 0.7, Math.PI + 0.2, Math.PI * 2 - 0.2);
        ctx.stroke();
        ctx.globalAlpha = 1.0;
    }
    
    highlightCollision(surfaceId) {
        this.activeCollisions.set(surfaceId, 1.0);
    }
    
    updateCollisionHighlights() {
        // Decay highlights over time
        for (const [id, value] of this.activeCollisions.entries()) {
            const newValue = value - 0.05;
            if (newValue <= 0) {
                this.activeCollisions.delete(id);
            } else {
                this.activeCollisions.set(id, newValue);
            }
        }
    }
}
