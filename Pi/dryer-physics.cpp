#include "dryer-physics.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// Color palette
static const uint32_t SURFACE_COLORS[] = {
    0xff6b6b, 0x4ecdc4, 0xffe66d, 0xa8e6cf,
    0xff8b94, 0xc7ceea, 0xffd3b6, 0xffaaa5,
    0xdcedc1, 0xa8d8ea, 0xffccf9, 0xb4f8c8
};

DryerPhysics::DryerPhysics() {
    // Initialize parameters
    rpm = 20.0f;
    drumRadius = 0.80f;     // 80cm default
    vaneCount = 5;
    vaneHeight = 0.30f;     // 30% of radius
    
    // Initialize ball properties
    ball.x = 0.0f;
    ball.y = 0.0f;
    ball.vx = 0.0f;
    ball.vy = 0.0f;
    ball.radius = 0.035f;   // Tennis ball: 3.5cm
    ball.mass = 0.058f;     // Tennis ball: 58g
    ball.restitution = 0.75f;
    ball.dragCoeff = 0.55f;
    
    // Physical constants
    gravity = 9.81f;
    earthGravity = 9.81f;
    moonGravity = 1.635f;
    airDensity = 1.225f;
    
    // Feature toggles
    lintTrapEnabled = false;
    lintTrapThreshold = 0.15f;
    moonGravityEnabled = false;
    useQuadraticDrag = false;
    
    // Drum rotation
    drumAngle = 0.0f;
    drumAngularVelocity = 0.0f;
    
    // Physics toggles (Coriolis ON by default - fixes "wind" effect)
    enableCoriolis = true;
    enableCentrifugal = true;
    enableAirDrag = true;
    coriolisSignFlip = 1;
    
    // Initialize
    reset();
    updateSurfaces();
}

void DryerPhysics::setParameters(float rpm, float drumSizeCm, int vaneCount, float vaneHeightPercent) {
    this->rpm = rpm;
    this->drumRadius = drumSizeCm / 100.0f;  // cm to meters
    this->vaneCount = vaneCount;
    this->vaneHeight = vaneHeightPercent / 100.0f;
    
    // Update angular velocity (rad/s)
    this->drumAngularVelocity = (rpm * 2.0f * M_PI) / 60.0f;
    
    // Regenerate surfaces
    updateSurfaces();
}

void DryerPhysics::setBallProperties(float radius, float mass, float restitution, float dragCoeff) {
    ball.radius = radius;
    ball.mass = mass;
    ball.restitution = restitution;
    ball.dragCoeff = dragCoeff;
}

void DryerPhysics::setTennisBall() {
    setBallProperties(0.035f, 0.058f, 0.75f, 0.55f);
    std::cout << "🎾 Tennis ball selected" << std::endl;
}

void DryerPhysics::setBalloonBall() {
    setBallProperties(0.075f, 0.001f, 0.10f, 0.47f);
    std::cout << "🎈 Balloon ball selected" << std::endl;
}

void DryerPhysics::setLintTrap(bool enabled) {
    lintTrapEnabled = enabled;
    std::cout << "🧺 Lint trap: " << (enabled ? "ON" : "OFF") << std::endl;
}

void DryerPhysics::setMoonGravity(bool enabled) {
    moonGravityEnabled = enabled;
    gravity = enabled ? moonGravity : earthGravity;
    std::cout << "🌙 Moon gravity: " << (enabled ? "ON" : "OFF") << std::endl;
}

void DryerPhysics::updateSurfaces() {
    surfaces.clear();
    
    for (int i = 0; i < vaneCount; i++) {
        // Drum segment
        Surface drumSurf;
        drumSurf.type = "drum";
        drumSurf.id = "drum_" + std::to_string(i);
        drumSurf.index = i;
        drumSurf.color = getSurfaceColor(i * 2);
        surfaces.push_back(drumSurf);
        
        // Vane leading edge
        Surface vaneLeading;
        vaneLeading.type = "vane_leading";
        vaneLeading.id = "vane_" + std::to_string(i) + "_lead";
        vaneLeading.index = i;
        vaneLeading.color = getSurfaceColor(i * 2 + 1);
        surfaces.push_back(vaneLeading);
        
        // Vane trailing edge
        Surface vaneTrailing;
        vaneTrailing.type = "vane_trailing";
        vaneTrailing.id = "vane_" + std::to_string(i) + "_trail";
        vaneTrailing.index = i;
        vaneTrailing.color = getSurfaceColor(i * 2 + 1);
        surfaces.push_back(vaneTrailing);
    }
}

uint32_t DryerPhysics::getSurfaceColor(int index) const {
    return SURFACE_COLORS[index % (sizeof(SURFACE_COLORS) / sizeof(SURFACE_COLORS[0]))];
}

void DryerPhysics::reset() {
    ball.x = drumRadius * 0.3f;
    ball.y = 0.0f;
    ball.vx = 0.0f;
    ball.vy = 0.0f;
    drumAngle = 0.0f;
}

void DryerPhysics::onCollision(CollisionCallback callback) {
    collisionCallbacks.push_back(callback);
}

void DryerPhysics::step(float dt) {
    // Update drum rotation
    drumAngle += drumAngularVelocity * dt;
    
    // Gravitational force (transformed to rotating frame)
    float cosAngle = std::cos(drumAngle);
    float sinAngle = std::sin(drumAngle);
    
    float gravityX = -gravity * sinAngle;
    float gravityY = -gravity * cosAngle;
    
    // Centrifugal force
    float centrifugalX = 0.0f;
    float centrifugalY = 0.0f;
    
    if (enableCentrifugal) {
        float distFromCenter = std::sqrt(ball.x * ball.x + ball.y * ball.y);
        if (distFromCenter > 0.0001f) {
            float centrifugalMagnitude = drumAngularVelocity * drumAngularVelocity * distFromCenter;
            centrifugalX = (ball.x / distFromCenter) * centrifugalMagnitude;
            centrifugalY = (ball.y / distFromCenter) * centrifugalMagnitude;
            debugInfo.centrifugalMagnitude = centrifugalMagnitude;
        }
    }
    
    // Coriolis force
    float coriolisX = 0.0f;
    float coriolisY = 0.0f;
    
    if (enableCoriolis) {
        float sign = static_cast<float>(coriolisSignFlip);
        coriolisX = sign * 2.0f * drumAngularVelocity * ball.vy;
        coriolisY = sign * -2.0f * drumAngularVelocity * ball.vx;
        
        float coriolisMag = std::sqrt(coriolisX * coriolisX + coriolisY * coriolisY);
        debugInfo.coriolisMagnitude = coriolisMag;
    }
    
    // Air drag
    float dragX = 0.0f;
    float dragY = 0.0f;
    
    if (enableAirDrag) {
        float speed = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
        
        if (speed > 0.001f) {
            if (useQuadraticDrag) {
                float dragForceMagnitude = 0.5f * airDensity * speed * speed * ball.dragCoeff * ball.area();
                float dragAccelMagnitude = dragForceMagnitude / ball.mass;
                
                dragX = -(ball.vx / speed) * dragAccelMagnitude;
                dragY = -(ball.vy / speed) * dragAccelMagnitude;
                
                debugInfo.dragMagnitude = dragAccelMagnitude;
            } else {
                // Linear drag (original)
                float dragCoeff = 0.1f;
                float dampingFactor = std::exp(-dragCoeff * dt);
                ball.vx *= dampingFactor;
                ball.vy *= dampingFactor;
                
                debugInfo.dragMagnitude = dragCoeff * speed;
            }
        }
    }
    
    // Apply all forces
    float totalAccelX = gravityX + centrifugalX + coriolisX;
    float totalAccelY = gravityY + centrifugalY + coriolisY;
    
    if (useQuadraticDrag) {
        totalAccelX += dragX;
        totalAccelY += dragY;
    }
    
    ball.vx += totalAccelX * dt;
    ball.vy += totalAccelY * dt;
    
    // Update debug info
    debugInfo.totalVelocity = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
    
    // Update position
    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;
    
    // Check collisions
    handleCollisions();
}

void DryerPhysics::handleCollisions() {
    float ballDist = std::sqrt(ball.x * ball.x + ball.y * ball.y);
    
    // Drum wall collision
    if (ballDist + ball.radius > drumRadius) {
        float penetration = ballDist + ball.radius - drumRadius;
        
        // Normal vector (pointing toward center)
        float nx = -ball.x / ballDist;
        float ny = -ball.y / ballDist;
        
        // Move ball back to surface
        ball.x += nx * penetration;
        ball.y += ny * penetration;
        
        // Relative velocity normal to surface
        float vn = ball.vx * nx + ball.vy * ny;
        
        if (vn < 0.0f) {  // Moving into wall
            // Calculate segment before reflecting velocity
            float ballAngle = std::atan2(ball.y, ball.x);
            float anglePerSegment = (2.0f * M_PI) / vaneCount;
            
            // Normalize to [0, 2π)
            float normalizedAngle = ballAngle;
            if (normalizedAngle < 0.0f) normalizedAngle += 2.0f * M_PI;
            
            int segmentIndex = static_cast<int>(std::floor(normalizedAngle / anglePerSegment)) % vaneCount;
            
            // Reflect velocity
            ball.vx -= (1.0f + ball.restitution) * vn * nx;
            ball.vy -= (1.0f + ball.restitution) * vn * ny;
            
            // Find surface
            auto it = std::find_if(surfaces.begin(), surfaces.end(),
                [segmentIndex](const Surface& s) {
                    return s.type == "drum" && s.index == segmentIndex;
                });
            
            if (it != surfaces.end()) {
                triggerCollision(*it, std::abs(vn));
            }
        }
    }
    
    // Vane collisions
    checkVaneCollisions();
}

void DryerPhysics::checkVaneCollisions() {
    float vaneInnerRadius = drumRadius * (1.0f - vaneHeight);
    
    for (int i = 0; i < vaneCount; i++) {
        float vaneAngle = (static_cast<float>(i) / vaneCount) * 2.0f * M_PI;
        
        // Vane endpoints
        float vx1 = vaneInnerRadius * std::cos(vaneAngle);
        float vy1 = vaneInnerRadius * std::sin(vaneAngle);
        float vx2 = drumRadius * std::cos(vaneAngle);
        float vy2 = drumRadius * std::sin(vaneAngle);
        
        // Vector from vane start to ball
        float dx = ball.x - vx1;
        float dy = ball.y - vy1;
        
        // Vane direction
        float vdx = vx2 - vx1;
        float vdy = vy2 - vy1;
        float vaneLength = std::sqrt(vdx * vdx + vdy * vdy);
        
        // Project ball onto vane
        float t = (dx * vdx + dy * vdy) / (vaneLength * vaneLength);
        
        if (t >= 0.0f && t <= 1.0f) {
            // Closest point on vane
            float closestX = vx1 + t * vdx;
            float closestY = vy1 + t * vdy;
            
            // Distance from ball to vane
            float distX = ball.x - closestX;
            float distY = ball.y - closestY;
            float dist = std::sqrt(distX * distX + distY * distY);
            
            if (dist < ball.radius) {
                float penetration = ball.radius - dist;
                
                // Normal vector
                float nx = distX / dist;
                float ny = distY / dist;
                
                // Move ball out
                ball.x += nx * penetration;
                ball.y += ny * penetration;
                
                // Relative velocity
                float vn = ball.vx * nx + ball.vy * ny;
                
                if (vn < 0.0f) {
                    // Reflect velocity
                    ball.vx -= (1.0f + ball.restitution) * vn * nx;
                    ball.vy -= (1.0f + ball.restitution) * vn * ny;
                    
                    // Determine side
                    float perpX = -vdy / vaneLength;
                    float perpY = vdx / vaneLength;
                    std::string side = (dx * perpX + dy * perpY) > 0.0f ? "vane_leading" : "vane_trailing";
                    
                    // Find surface
                    auto it = std::find_if(surfaces.begin(), surfaces.end(),
                        [&side, i](const Surface& s) {
                            return s.type == side && s.index == i;
                        });
                    
                    if (it != surfaces.end()) {
                        triggerCollision(*it, std::abs(vn));
                    }
                }
            }
        }
    }
}

void DryerPhysics::triggerCollision(const Surface& surface, float velocity) {
    // Lint trap filter
    if (lintTrapEnabled && velocity < lintTrapThreshold) {
        return;
    }
    
    // Debounce
    if (lastCollisionSurface == surface.id) {
        return;
    }
    
    lastCollisionSurface = surface.id;
    
    // Notify all listeners
    for (auto& callback : collisionCallbacks) {
        callback(surface, velocity);
    }
}

DryerPhysics::BallPosition DryerPhysics::getBallPosition(int canvasSize) const {
    float scale = canvasSize / (drumRadius * 2.2f);
    float centerX = canvasSize / 2.0f;
    float centerY = canvasSize / 2.0f;
    
    // Transform from rotating frame to screen coordinates
    float cosAngle = std::cos(drumAngle);
    float sinAngle = std::sin(drumAngle);
    float screenX = ball.x * cosAngle - ball.y * sinAngle;
    float screenY = ball.x * sinAngle + ball.y * cosAngle;
    
    BallPosition pos;
    pos.x = centerX + screenX * scale;
    pos.y = centerY - screenY * scale;
    pos.radius = ball.radius * scale;
    return pos;
}

std::vector<Vane> DryerPhysics::getVanePositions(int canvasSize) const {
    float scale = canvasSize / (drumRadius * 2.2f);
    float centerX = canvasSize / 2.0f;
    float centerY = canvasSize / 2.0f;
    float vaneInnerRadius = drumRadius * (1.0f - vaneHeight);
    
    std::vector<Vane> vanes;
    for (int i = 0; i < vaneCount; i++) {
        float angle = (static_cast<float>(i) / vaneCount) * 2.0f * M_PI + drumAngle;
        
        Vane vane;
        vane.innerX = centerX + vaneInnerRadius * std::cos(angle) * scale;
        vane.innerY = centerY - vaneInnerRadius * std::sin(angle) * scale;
        vane.outerX = centerX + drumRadius * std::cos(angle) * scale;
        vane.outerY = centerY - drumRadius * std::sin(angle) * scale;
        vane.index = i;
        
        vanes.push_back(vane);
    }
    
    return vanes;
}

void DryerPhysics::toggleCoriolis(bool enable) {
    enableCoriolis = enable;
    std::cout << "🌀 Coriolis: " << (enable ? "ON" : "OFF") << std::endl;
}

void DryerPhysics::toggleCentrifugal(bool enable) {
    enableCentrifugal = enable;
    std::cout << "💫 Centrifugal: " << (enable ? "ON" : "OFF") << std::endl;
}

void DryerPhysics::toggleDrag(bool enable) {
    enableAirDrag = enable;
    std::cout << "💨 Air drag: " << (enable ? "ON" : "OFF") << std::endl;
}
