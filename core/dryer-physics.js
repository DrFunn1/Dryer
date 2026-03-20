/**
 * Dryer Physics Engine - Enhanced Version
 * Custom rigid body physics for tennis ball in rotating drum with vanes
 * 
 * PHYSICS EXPLANATION:
 * ====================
 * We simulate the ball in the ROTATING REFERENCE FRAME of the drum.
 * This means the drum appears stationary, but we must add "fictitious forces":
 * 
 * 1. CENTRIFUGAL FORCE: F = m*ω²*r (pushes outward from rotation axis)
 * 2. CORIOLIS FORCE: F = -2m(ω × v) (deflects moving objects perpendicular to motion)
 * 3. GRAVITY: Must be transformed to rotating frame
 * 
 * The "internal wind" effect you're seeing is likely from MISSING CORIOLIS FORCE.
 * At high RPM, Coriolis becomes significant and affects trajectory realism.
 */

class DryerPhysics {
    constructor() {
        // Parameters (will be updated from UI)
        this.rpm = 20;
        this.drumRadius = 0.80; // meters (80cm)
        this.vaneCount = 5;
        this.vaneHeight = 0.30; // fraction of radius
        
        // Ball properties - NOW CONFIGURABLE for testing!
        this.ball = {
            x: 0,
            y: 0,
            vx: 0,
            vy: 0,
            
            // BALL SIZE: Affects collision detection and visual appearance
            // Tennis ball: 0.035m, Racquetball: 0.028m, Baseball: 0.037m, Larger ball: 0.05m
            radius: 0.035, // meters
            
            // BALL MASS: More mass = less affected by air drag and Coriolis
            // Tennis ball: 0.058kg, Baseball: 0.145kg, Racquetball: 0.040kg, Heavy ball: 0.200kg
            mass: 0.058, // kg
            
            // RESTITUTION: How "bouncy" the ball is (0 = no bounce, 1 = perfect bounce)
            restitution: 0.75, // tennis ball is quite bouncy
            
            // DRAG COEFFICIENT: Sphere in turbulent air flow
            // Smooth sphere: 0.47, Tennis ball (fuzzy): 0.55, Rough sphere: 0.8
            dragCoeff: 0.55, // tennis ball has fuzzy surface
            
            // Cross-sectional area (calculated from radius)
            get area() { return Math.PI * this.radius * this.radius; },

            // Volume (calculated from radius)
            get volume() { return (4/3) * Math.PI * Math.pow(this.radius, 3); }
        };
        
        // Physical constants
        this.gravity = 9.81; // m/s² - Earth's gravitational acceleration
        this.earthGravity = 9.81; // Store original for moon toggle
        this.moonGravity = 1.635; // 1/6th of Earth gravity
        this.airDensity = 1.225; // kg/m³ at sea level, 20°C
        
        // Feature toggles
        this.lintTrapEnabled = false;
        this.lintTrapThreshold = 0.15; // m/s - minimum velocity to trigger sound/MIDI
        this.moonGravityEnabled = false;
        
        // Drum rotation
        this.drumAngle = 0; // current rotation angle (radians)
        this.drumAngularVelocity = 0; // ω (rad/s)
        
        // Visual debug: offset for segment highlighting (adjust if needed)
        this.segmentIndexOffset = 0; // Change this to fix visual mismatch
        
        // Enable/disable physics effects for debugging
        this.enableCoriolis = true; // DEFAULT ON - fixes "wind" effect!
        this.enableCentrifugal = true;
        this.enableAirDrag = true;
        this.coriolisSignFlip = 1; // +1 or -1 to flip Coriolis direction
        
        // Surface tracking for MIDI
        this.surfaces = [];
        this.lastCollisionSurface = null;
        this.collisionCallbacks = [];
        
        // Debug info
        this.debugInfo = {
            centrifugalMagnitude: 0,
            coriolisMagnitude: 0,
            dragMagnitude: 0,
            totalVelocity: 0
        };
        
        // Initialize ball at center
        this.reset();
        this.updateSurfaces();
    }
    
    setParameters(rpm, drumSizeCm, vaneCount, vaneHeightPercent) {
        this.rpm = rpm;
        this.drumRadius = drumSizeCm / 100; // convert cm to meters
        this.vaneCount = Math.floor(vaneCount);
        this.vaneHeight = vaneHeightPercent / 100;
        
        // Update angular velocity (rad/s)
        this.drumAngularVelocity = (rpm * 2 * Math.PI) / 60;
        
        // Regenerate surfaces
        this.updateSurfaces();
    }
    
    // NEW: Allow changing ball properties during runtime for testing
    setBallProperties(radius, mass, restitution, dragCoeff) {
        if (radius !== undefined) this.ball.radius = radius;
        if (mass !== undefined) this.ball.mass = mass;
        if (restitution !== undefined) this.ball.restitution = restitution;
        if (dragCoeff !== undefined) this.ball.dragCoeff = dragCoeff;
    }
    
    // Ball type presets
    setTennisBall() {
        this.setBallProperties(0.035, 0.058, 0.75, 0.55);
        console.log('🎾 Tennis ball selected');
    }
    
    setSandbagBall() {
        this.setBallProperties(0.05, 0.5, 0.15, 0.8); // 10cm diameter, 500g, low bounce, high drag
        console.log('🏋️ Sandbag selected');
    }

    setBalloonBall() {
        this.setBallProperties(0.13, 0.01228, 0.30, 0.47); // 13cm radius, 1g rubber + 11.28g air inside
        console.log('🎈 Balloon selected');
    }

    // Feature toggles
    setLintTrap(enabled) {
        this.lintTrapEnabled = enabled;
        console.log(`🧺 Lint trap: ${enabled ? 'ON (filtering low velocity)' : 'OFF'}`);
    }
    
    setMoonGravity(enabled) {
        this.moonGravityEnabled = enabled;
        this.gravity = enabled ? this.moonGravity : this.earthGravity;
        console.log(`🌙 Moon gravity: ${enabled ? 'ON (1/6th Earth)' : 'OFF (normal)'}`);
    }
    
    updateSurfaces() {
        this.surfaces = [];
        
        // Generate surface IDs for each segment between vanes and each vane face
        for (let i = 0; i < this.vaneCount; i++) {
            // Drum segment between vanes
            this.surfaces.push({
                type: 'drum',
                id: `drum_${i}`,
                index: i,
                color: this.getSurfaceColor(i * 2)
            });
            
            // Vane surfaces (both sides)
            this.surfaces.push({
                type: 'vane_leading',
                id: `vane_${i}_lead`,
                index: i,
                color: this.getSurfaceColor(i * 2 + 1)
            });
            this.surfaces.push({
                type: 'vane_trailing',
                id: `vane_${i}_trail`,
                index: i,
                color: this.getSurfaceColor(i * 2 + 1)
            });
        }
    }
    
    getSurfaceColor(index) {
        const colors = [
            '#ff6b6b', '#4ecdc4', '#ffe66d', '#a8e6cf', 
            '#ff8b94', '#c7ceea', '#ffd3b6', '#ffaaa5',
            '#dcedc1', '#a8d8ea', '#ffccf9', '#b4f8c8'
        ];
        return colors[index % colors.length];
    }
    
    reset() {
        // Place ball slightly off-center
        this.ball.x = this.drumRadius * 0.3;
        this.ball.y = 0;
        this.ball.vx = 0;
        this.ball.vy = 0;
        this.drumAngle = 0;
    }
    
    onCollision(callback) {
        this.collisionCallbacks.push(callback);
    }
    
    step(dt) {
        // Update drum rotation
        this.drumAngle += this.drumAngularVelocity * dt;
        
        // =====================================================================
        // ROTATING REFERENCE FRAME PHYSICS
        // =====================================================================
        
        // 1. GRAVITATIONAL FORCE (transformed to rotating frame)
        // Gravity always points down in world frame, but drum is rotating
        const cos = Math.cos(this.drumAngle);
        const sin = Math.sin(this.drumAngle);
        
        const gravityX = -this.gravity * sin;
        const gravityY = -this.gravity * cos;
        
        // 2. BUOYANCY FORCE (Archimedes' principle)
        // F_buoyancy = ρ_air × V_ball × g (opposes gravity)
        // Expressed as acceleration: a = (ρ_air × V_ball × g) / m_ball = (ρ_air / ρ_ball) × g
        const buoyancyFactor = (this.airDensity * this.ball.volume) / this.ball.mass;
        const buoyancyX = -gravityX * buoyancyFactor;
        const buoyancyY = -gravityY * buoyancyFactor;

        // 3. CENTRIFUGAL FORCE (fictitious force in rotating frame)
        // F_centrifugal = m*ω²*r (always points away from rotation axis)
        let centrifugalX = 0;
        let centrifugalY = 0;

        if (this.enableCentrifugal) {
            const distFromCenter = Math.sqrt(this.ball.x * this.ball.x + this.ball.y * this.ball.y);
            if (distFromCenter > 0.0001) {
                const centrifugalMagnitude = this.drumAngularVelocity * this.drumAngularVelocity * distFromCenter;
                centrifugalX = (this.ball.x / distFromCenter) * centrifugalMagnitude;
                centrifugalY = (this.ball.y / distFromCenter) * centrifugalMagnitude;

                // Buoyancy also opposes centrifugal force in rotating frame
                centrifugalX *= (1 - buoyancyFactor);
                centrifugalY *= (1 - buoyancyFactor);

                this.debugInfo.centrifugalMagnitude = centrifugalMagnitude;
            }
        }
        
        // 4. CORIOLIS FORCE (fictitious force in rotating frame) - THIS WAS MISSING!
        // F_coriolis = -2m(ω × v)
        // In 2D: F_x = 2*m*ω*v_y, F_y = -2*m*ω*v_x (sign depends on rotation direction)
        // This deflects moving objects perpendicular to their motion
        let coriolisX = 0;
        let coriolisY = 0;
        
        if (this.enableCoriolis) {
            // Note: We don't multiply by mass here since we're calculating acceleration (F/m)
            // Sign convention: positive ω is counter-clockwise rotation
            const sign = this.coriolisSignFlip || 1;
            coriolisX = sign * 2 * this.drumAngularVelocity * this.ball.vy;
            coriolisY = sign * -2 * this.drumAngularVelocity * this.ball.vx;
            
            const coriolisMag = Math.sqrt(coriolisX * coriolisX + coriolisY * coriolisY);
            this.debugInfo.coriolisMagnitude = coriolisMag;
        }
        
        // 5. AIR DRAG FORCE (with velocity field from rotating air)
        let dragX = 0;
        let dragY = 0;

        if (this.enableAirDrag) {
            // Calculate ball's radial position
            const r = Math.sqrt(this.ball.x * this.ball.x + this.ball.y * this.ball.y);

            if (r > 0.001) {
                // Coupling coefficient: how well air locks to drum rotation
                // c → 1 with more/taller vanes (solid body rotation)
                // c → 0 with fewer/shorter vanes (quadratic profile)
                const h = this.vaneHeight; // fraction (0.1 to 0.5)
                const n = this.vaneCount;
                const k = 0.5; // empirical coupling constant
                const c = 1 - Math.exp(-k * n * h);

                // Air velocity in rotating frame (tangential component)
                // v_air(r) = ω*r*[(c + (1-c)*r/R) - 1] = ω*r*(1-c)*(r/R - 1)
                const omega = this.drumAngularVelocity;
                const vAirTangential = omega * r * (1 - c) * (r / this.drumRadius - 1);

                // Convert tangential air velocity to Cartesian
                const theta = Math.atan2(this.ball.y, this.ball.x);
                const vAirX = -vAirTangential * Math.sin(theta);
                const vAirY = vAirTangential * Math.cos(theta);

                // Relative velocity (ball velocity relative to local air)
                const vRelX = this.ball.vx - vAirX;
                const vRelY = this.ball.vy - vAirY;
                const vRelSpeed = Math.sqrt(vRelX * vRelX + vRelY * vRelY);

                if (vRelSpeed > 0.001) {
                    // Quadratic drag: F = -0.5 * ρ * |v_rel|² * C_d * A * v̂_rel
                    const dragForceMagnitude = 0.5 * this.airDensity * vRelSpeed * vRelSpeed *
                                              this.ball.dragCoeff * this.ball.area;
                    const dragAccelMagnitude = dragForceMagnitude / this.ball.mass;

                    // Drag opposes relative velocity
                    dragX = -(vRelX / vRelSpeed) * dragAccelMagnitude;
                    dragY = -(vRelY / vRelSpeed) * dragAccelMagnitude;

                    this.debugInfo.dragMagnitude = dragAccelMagnitude;
                    this.debugInfo.airVelocity = Math.sqrt(vAirX * vAirX + vAirY * vAirY);
                }
            }
        }

        // =====================================================================
        // APPLY ALL FORCES (as accelerations)
        // =====================================================================

        let totalAccelX = gravityX + buoyancyX + centrifugalX + coriolisX + dragX;
        let totalAccelY = gravityY + buoyancyY + centrifugalY + coriolisY + dragY;
        
        this.ball.vx += totalAccelX * dt;
        this.ball.vy += totalAccelY * dt;
        
        // Update debug info
        this.debugInfo.totalVelocity = Math.sqrt(this.ball.vx * this.ball.vx + this.ball.vy * this.ball.vy);
        
        // Update position
        this.ball.x += this.ball.vx * dt;
        this.ball.y += this.ball.vy * dt;
        
        // Check collisions
        this.handleCollisions();
    }
    
    handleCollisions() {
        const ballDist = Math.sqrt(this.ball.x * this.ball.x + this.ball.y * this.ball.y);
        
        // Check collision with drum wall
        if (ballDist + this.ball.radius > this.drumRadius) {
            const penetration = ballDist + this.ball.radius - this.drumRadius;
            
            // Normal vector (pointing toward center)
            const nx = -this.ball.x / ballDist;
            const ny = -this.ball.y / ballDist;
            
            // Move ball back to surface
            this.ball.x += nx * penetration;
            this.ball.y += ny * penetration;
            
            // Calculate relative velocity normal to surface
            const vn = this.ball.vx * nx + this.ball.vy * ny;
            
            if (vn < 0) { // Moving into wall
                // IMPORTANT: Calculate which segment BEFORE moving ball back
                // Otherwise ball gets pushed into adjacent segment at boundaries
                const ballAngle = Math.atan2(this.ball.y, this.ball.x);
                const anglePerSegment = (2 * Math.PI) / this.vaneCount;
                
                // Normalize angle to [0, 2π)
                let normalizedAngle = ballAngle;
                if (normalizedAngle < 0) normalizedAngle += 2 * Math.PI;
                
                // Calculate segment index using floor (standard binning)
                let segmentIndex = Math.floor(normalizedAngle / anglePerSegment) % this.vaneCount;
                
                // Now reflect velocity with restitution
                this.ball.vx -= (1 + this.ball.restitution) * vn * nx;
                this.ball.vy -= (1 + this.ball.restitution) * vn * ny;
                
                // Apply offset if visual doesn't match physics
                segmentIndex = (segmentIndex + this.segmentIndexOffset) % this.vaneCount;
                if (segmentIndex < 0) segmentIndex += this.vaneCount;
                
                // Find the surface object
                const surface = this.surfaces.find(s => s.type === 'drum' && s.index === segmentIndex);
                
                if (surface) {
                    // Debug logging (temporary - can remove later)
                    if (window.dryerDebugMode) {
                        console.log(`🎯 Drum collision: angle=${(normalizedAngle*180/Math.PI).toFixed(2)}° → segment ${segmentIndex} (before offset: ${Math.floor(normalizedAngle / anglePerSegment)}) → ID: ${surface.id}`);
                    }
                    this.triggerCollision(surface, Math.abs(vn));
                }
            }
        }
        
        // Check collision with vanes
        this.checkVaneCollisions();
    }
    
    checkVaneCollisions() {
        const vaneInnerRadius = this.drumRadius * (1 - this.vaneHeight);
        
        for (let i = 0; i < this.vaneCount; i++) {
            const vaneAngle = (i / this.vaneCount) * 2 * Math.PI;
            
            // Vane is a line segment from inner radius to drum radius
            const vx1 = vaneInnerRadius * Math.cos(vaneAngle);
            const vy1 = vaneInnerRadius * Math.sin(vaneAngle);
            const vx2 = this.drumRadius * Math.cos(vaneAngle);
            const vy2 = this.drumRadius * Math.sin(vaneAngle);
            
            // Vector from vane start to ball
            const dx = this.ball.x - vx1;
            const dy = this.ball.y - vy1;
            
            // Vane direction vector
            const vdx = vx2 - vx1;
            const vdy = vy2 - vy1;
            const vaneLength = Math.sqrt(vdx * vdx + vdy * vdy);
            
            // Project ball onto vane line
            const t = (dx * vdx + dy * vdy) / (vaneLength * vaneLength);
            
            if (t >= 0 && t <= 1) {
                // Closest point on vane
                const closestX = vx1 + t * vdx;
                const closestY = vy1 + t * vdy;
                
                // Distance from ball to vane
                const distX = this.ball.x - closestX;
                const distY = this.ball.y - closestY;
                const dist = Math.sqrt(distX * distX + distY * distY);
                
                if (dist < this.ball.radius) {
                    // Collision detected
                    const penetration = this.ball.radius - dist;
                    
                    // Normal vector (perpendicular to vane)
                    let nx = distX / dist;
                    let ny = distY / dist;
                    
                    // Move ball out of vane
                    this.ball.x += nx * penetration;
                    this.ball.y += ny * penetration;
                    
                    // Calculate relative velocity
                    const vn = this.ball.vx * nx + this.ball.vy * ny;
                    
                    if (vn < 0) {
                        // Reflect velocity
                        this.ball.vx -= (1 + this.ball.restitution) * vn * nx;
                        this.ball.vy -= (1 + this.ball.restitution) * vn * ny;
                        
                        // Determine which side of vane (leading or trailing)
                        const perpX = -vdy / vaneLength;
                        const perpY = vdx / vaneLength;
                        const side = (dx * perpX + dy * perpY) > 0 ? 'vane_leading' : 'vane_trailing';
                        
                        const surface = this.surfaces.find(s => s.type === side && s.index === i);
                        if (surface) {
                            this.triggerCollision(surface, Math.abs(vn));
                        }
                    }
                }
            }
        }
    }
    
    triggerCollision(surface, velocity) {
        // Apply lint trap filter if enabled
        if (this.lintTrapEnabled && velocity < this.lintTrapThreshold) {
            return; // Ignore low-velocity collisions
        }
        
        // Debounce rapid collisions with same surface
        if (this.lastCollisionSurface === surface.id) {
            return;
        }
        
        this.lastCollisionSurface = surface.id;
        setTimeout(() => {
            if (this.lastCollisionSurface === surface.id) {
                this.lastCollisionSurface = null;
            }
        }, 50);
        
        // Notify all listeners
        this.collisionCallbacks.forEach(cb => cb(surface, velocity));
    }
    
    // Get ball position in screen coordinates (for rendering)
    getBallPosition(canvasSize) {
        const scale = canvasSize / (this.drumRadius * 2.2);
        const centerX = canvasSize / 2;
        const centerY = canvasSize / 2;
        
        // Transform from rotating frame to screen coordinates
        const cos = Math.cos(this.drumAngle);
        const sin = Math.sin(this.drumAngle);
        const screenX = this.ball.x * cos - this.ball.y * sin;
        const screenY = this.ball.x * sin + this.ball.y * cos;
        
        return {
            x: centerX + screenX * scale,
            y: centerY - screenY * scale,
            radius: this.ball.radius * scale
        };
    }
    
    // Get vane positions for rendering
    getVanePositions(canvasSize) {
        const scale = canvasSize / (this.drumRadius * 2.2);
        const centerX = canvasSize / 2;
        const centerY = canvasSize / 2;
        const vaneInnerRadius = this.drumRadius * (1 - this.vaneHeight);
        
        const vanes = [];
        for (let i = 0; i < this.vaneCount; i++) {
            const angle = (i / this.vaneCount) * 2 * Math.PI + this.drumAngle;
            
            const innerX = centerX + vaneInnerRadius * Math.cos(angle) * scale;
            const innerY = centerY - vaneInnerRadius * Math.sin(angle) * scale;
            const outerX = centerX + this.drumRadius * Math.cos(angle) * scale;
            const outerY = centerY - this.drumRadius * Math.sin(angle) * scale;
            
            vanes.push({ innerX, innerY, outerX, outerY, index: i });
        }
        
        return vanes;
    }
    
    // Get debug information
    getDebugInfo() {
        return {
            ...this.debugInfo,
            rpm: this.rpm,
            angularVel: this.drumAngularVelocity.toFixed(3),
            ballMass: this.ball.mass,
            ballRadius: this.ball.radius,
            position: `(${this.ball.x.toFixed(3)}, ${this.ball.y.toFixed(3)})`,
            velocity: `(${this.ball.vx.toFixed(3)}, ${this.ball.vy.toFixed(3)})`
        };
    }
    
    // ===================================================================
    // CONSOLE DEBUG HELPERS - Easy toggling of physics features
    // ===================================================================
    
    toggleCoriolis(enable) {
        this.enableCoriolis = enable !== undefined ? enable : !this.enableCoriolis;
        console.log(`🌀 Coriolis force: ${this.enableCoriolis ? 'ENABLED' : 'DISABLED'}`);
        return this.enableCoriolis;
    }
    
    toggleCentrifugal(enable) {
        this.enableCentrifugal = enable !== undefined ? enable : !this.enableCentrifugal;
        console.log(`💫 Centrifugal force: ${this.enableCentrifugal ? 'ENABLED' : 'DISABLED'}`);
        return this.enableCentrifugal;
    }
    
    toggleDrag(enable) {
        this.enableAirDrag = enable !== undefined ? enable : !this.enableAirDrag;
        console.log(`💨 Air drag: ${this.enableAirDrag ? 'ENABLED' : 'DISABLED'}`);
        return this.enableAirDrag;
    }
    
    flipCoriolisSign() {
        this.coriolisSignFlip = (this.coriolisSignFlip || 1) * -1;
        console.log(`🔄 Coriolis sign: ${this.coriolisSignFlip > 0 ? 'POSITIVE' : 'NEGATIVE'}`);
        return this.coriolisSignFlip;
    }
    
    showPhysicsState() {
        console.log('=== PHYSICS STATE ===');
        console.log(`🌀 Coriolis: ${this.enableCoriolis ? 'ON' : 'OFF'} (sign: ${(this.coriolisSignFlip || 1) > 0 ? '+' : '-'})`);
        console.log(`💫 Centrifugal: ${this.enableCentrifugal ? 'ON' : 'OFF'}`);
        console.log(`💨 Air drag: ${this.enableAirDrag ? 'ON' : 'OFF'} (vane-coupled)`);
        console.log(`⚙️  RPM: ${this.rpm}`);
        console.log(`📏 Ball: ${(this.ball.radius * 100).toFixed(1)}cm, ${(this.ball.mass * 1000).toFixed(1)}g`);
        console.log('====================');
        const debug = this.getDebugInfo();
        console.log(`Centrifugal: ${debug.centrifugalMagnitude.toFixed(2)} m/s²`);
        console.log(`Coriolis: ${debug.coriolisMagnitude.toFixed(2)} m/s²`);
        console.log(`Drag: ${debug.dragMagnitude.toFixed(2)} m/s²`);
        console.log(`Velocity: ${debug.totalVelocity.toFixed(2)} m/s`);
        return debug;
    }
    
    // Quick presets
    originalPhysics() {
        this.enableCoriolis = false;
        this.enableCentrifugal = true;
        this.enableAirDrag = true;
        console.log('✅ Reverted to ORIGINAL physics (no Coriolis)');
        this.showPhysicsState();
    }
    
    enhancedPhysics() {
        this.enableCoriolis = true;
        this.enableCentrifugal = true;
        this.enableAirDrag = true;
        console.log('✅ Enabled ENHANCED physics (all forces)');
        this.showPhysicsState();
    }
    
    onlyGravity() {
        this.enableCoriolis = false;
        this.enableCentrifugal = false;
        this.enableAirDrag = false;
        console.log('✅ ONLY GRAVITY (all other forces disabled)');
        this.showPhysicsState();
    }
}

// ===================================================================
// GLOBAL CONSOLE SHORTCUTS - For easy debugging
// ===================================================================
// These will be available in browser console as window.dryerDebug

window.dryerDebug = {
    help: function() {
        console.log(`
╔══════════════════════════════════════════════════════════════════╗
║                   DRYER PHYSICS DEBUG CONSOLE                    ║
╚══════════════════════════════════════════════════════════════════╝

QUICK COMMANDS (copy/paste into console):
─────────────────────────────────────────────────────────────────────

📊 SHOW CURRENT STATE:
   dryerDebug.show()

🔄 TOGGLE INDIVIDUAL FORCES:
   dryerDebug.coriolis()        - Toggle Coriolis force
   dryerDebug.centrifugal()     - Toggle centrifugal force  
   dryerDebug.drag()            - Toggle air drag (vane-coupled)
   dryerDebug.flipCoriolis()    - Flip Coriolis sign (+/-)

🎯 QUICK PRESETS:
   dryerDebug.original()        - Original physics (baseline)
   dryerDebug.enhanced()        - Enhanced physics (all features)
   dryerDebug.gravityOnly()     - Only gravity (debug mode)

🎈 BALL TYPES:
   dryerDebug.tennis()          - Tennis ball (default)
   dryerDebug.sandbag()         - Sandbag (10cm, 500g, low bounce)
   dryerDebug.balloon()         - Balloon (15cm, 1g, buoyant)
   dryerDebug.baseball()        - Baseball (heavier)
   dryerDebug.pingPong()        - Ping pong ball (very light)

🌙 NEW FEATURES:
   dryerDebug.moon()            - Toggle moon gravity (1/6th Earth)
   dryerDebug.lintTrap()        - Toggle velocity filter

🧪 ISOLATE FORCES (test one at a time):
   dryerDebug.testCoriolis()    - ONLY Coriolis + Gravity
   dryerDebug.testCentrifugal() - ONLY Centrifugal + Gravity

📝 EXAMPLES:
   dryerDebug.show()            // See what's currently enabled
   dryerDebug.sandbag()         // Switch to sandbag
   dryerDebug.moon()            // Enable moon gravity
   dryerDebug.lintTrap()        // Filter low-velocity hits
   
─────────────────────────────────────────────────────────────────────
        `);
    },
    
    // Access physics engine
    get physics() {
        return window.dryerApp?.physics;
    },
    
    // Show state
    show: function() {
        return this.physics?.showPhysicsState();
    },
    
    // Toggle individual forces
    coriolis: function() {
        return this.physics?.toggleCoriolis();
    },
    
    centrifugal: function() {
        return this.physics?.toggleCentrifugal();
    },
    
    drag: function() {
        return this.physics?.toggleDrag();
    },
    
    flipCoriolis: function() {
        return this.physics?.flipCoriolisSign();
    },
    
    // Presets
    original: function() {
        this.physics?.originalPhysics();
    },
    
    enhanced: function() {
        this.physics?.enhancedPhysics();
    },
    
    gravityOnly: function() {
        this.physics?.onlyGravity();
    },
    
    // Test modes - isolate individual forces
    testCoriolis: function() {
        this.physics.enableCoriolis = true;
        this.physics.enableCentrifugal = false;
        this.physics.enableAirDrag = false;
        console.log('🧪 TEST MODE: Only Coriolis + Gravity');
        this.show();
    },
    
    testCentrifugal: function() {
        this.physics.enableCoriolis = false;
        this.physics.enableCentrifugal = true;
        this.physics.enableAirDrag = false;
        console.log('🧪 TEST MODE: Only Centrifugal + Gravity');
        this.show();
    },
    
    // Ball property adjustments
    setBall: function(radius, mass) {
        if (radius) this.physics.ball.radius = radius;
        if (mass) this.physics.ball.mass = mass;
        console.log(`⚽ Ball updated: ${(this.physics.ball.radius*100).toFixed(1)}cm, ${(this.physics.ball.mass*1000).toFixed(1)}g`);
    },
    
    // Presets for different ball types
    tennisBall: function() {
        this.setBall(0.035, 0.058);
        this.physics.ball.dragCoeff = 0.55;
        console.log('🎾 Tennis ball');
    },
    
    baseball: function() {
        this.setBall(0.037, 0.145);
        this.physics.ball.dragCoeff = 0.47;
        console.log('⚾ Baseball');
    },
    
    pingPong: function() {
        this.setBall(0.020, 0.0027);
        this.physics.ball.dragCoeff = 0.47;
        console.log('🏓 Ping pong ball');
    },
    
    sandbag: function() {
        this.physics.setSandbagBall();
    },

    balloon: function() {
        this.physics.setBalloonBall();
    },

    tennis: function() {
        this.physics.setTennisBall();
    },
    
    // Feature toggles
    lintTrap: function(enabled) {
        if (enabled === undefined) {
            this.physics.lintTrapEnabled = !this.physics.lintTrapEnabled;
        } else {
            this.physics.lintTrapEnabled = enabled;
        }
        console.log(`🧺 Lint trap: ${this.physics.lintTrapEnabled ? 'ON' : 'OFF'} (threshold: ${this.physics.lintTrapThreshold} m/s)`);
        return this.physics.lintTrapEnabled;
    },
    
    moon: function(enabled) {
        if (enabled === undefined) {
            this.physics.moonGravityEnabled = !this.physics.moonGravityEnabled;
        } else {
            this.physics.moonGravityEnabled = enabled;
        }
        this.physics.gravity = this.physics.moonGravityEnabled ? this.physics.moonGravity : this.physics.earthGravity;
        console.log(`🌙 Moon gravity: ${this.physics.moonGravityEnabled ? 'ON (1.635 m/s²)' : 'OFF (9.81 m/s²)'}`);
        return this.physics.moonGravityEnabled;
    },
    
    // Debug collision highlighting
    debugCollisions: function() {
        window.dryerDebugMode = !window.dryerDebugMode;
        console.log(`🔍 Collision logging: ${window.dryerDebugMode ? 'ON' : 'OFF'}`);
        if (window.dryerDebugMode) {
            console.log('Watch for: 🎯 messages showing segment calculations');
        }
        return window.dryerDebugMode;
    },
    
    // Show where ball currently is
    whereBall: function() {
        const p = this.physics;
        const ballAngle = Math.atan2(p.ball.y, p.ball.x);
        const angleDeg = (ballAngle * 180 / Math.PI).toFixed(1);
        const normalizedAngle = ballAngle < 0 ? ballAngle + 2*Math.PI : ballAngle;
        const normalizedDeg = (normalizedAngle * 180 / Math.PI).toFixed(1);
        const anglePerSegment = (2 * Math.PI) / p.vaneCount;
        const segmentIndex = Math.floor(normalizedAngle / anglePerSegment) % p.vaneCount;
        const drumAngleDeg = (p.drumAngle * 180 / Math.PI).toFixed(1);
        const worldAngleDeg = ((normalizedAngle + p.drumAngle) * 180 / Math.PI).toFixed(1);
        
        console.log('=== BALL POSITION ===');
        console.log(`Rotating frame: ${normalizedDeg}° (segment ${segmentIndex})`);
        console.log(`World frame: ${worldAngleDeg}° (drum rotated ${drumAngleDeg}°)`);
        console.log(`Ball coords: (${p.ball.x.toFixed(3)}, ${p.ball.y.toFixed(3)})`);
        console.log(`Distance from center: ${Math.sqrt(p.ball.x**2 + p.ball.y**2).toFixed(3)}m`);
        console.log(`Drum radius: ${p.drumRadius}m`);
        console.log('====================');
        
        // Show which segment SHOULD be highlighted
        const surface = p.surfaces.find(s => s.type === 'drum' && s.index === segmentIndex);
        if (surface) {
            console.log(`Should highlight: ${surface.id}`);
        }
    },
    
    // Adjust segment offset to fix visual mismatch
    fixSegmentOffset: function(offset) {
        if (offset === undefined) {
            console.log(`Current segment offset: ${this.physics.segmentIndexOffset}`);
            console.log('Usage: dryerDebug.fixSegmentOffset(+1) or dryerDebug.fixSegmentOffset(-1)');
            console.log('Try adjusting by ±1 until visual matches collision');
            return this.physics.segmentIndexOffset;
        }
        this.physics.segmentIndexOffset = offset;
        console.log(`✅ Segment offset set to: ${offset}`);
        console.log('Watch collisions to see if this fixes the visual mismatch');
        return offset;
    }
};

// Show help on load
console.log('╔════════════════════════════════════════════════════════════╗');
console.log('║  🔬 Dryer Physics Debug Console Loaded!                   ║');
console.log('║  Type: dryerDebug.help()  for available commands         ║');
console.log('╚════════════════════════════════════════════════════════════╝');
