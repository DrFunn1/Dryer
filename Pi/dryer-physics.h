#include <cstdint>
#ifndef DRYER_PHYSICS_H
#define DRYER_PHYSICS_H

#include <vector>
#include <string>
#include <functional>
#include <cmath>
#include <map>

// ============================================================================
// DRYER PHYSICS ENGINE - C++ PORT
// Custom rigid body physics for ball in rotating drum with vanes
// ============================================================================

struct Surface {
    std::string type;       // "drum", "vane_leading", "vane_trailing"
    std::string id;         // Unique identifier
    int index;              // Surface index for lookup
    uint32_t color;         // RGB color (0xRRGGBB)
};

struct Ball {
    // Position and velocity (meters, m/s)
    float x, y;             // Position in rotating frame
    float vx, vy;           // Velocity in rotating frame
    
    // Physical properties
    float radius;           // meters
    float mass;             // kg
    float restitution;      // Coefficient of restitution (0-1)
    float dragCoeff;        // Drag coefficient
    
    // Calculated property
    float area() const { return M_PI * radius * radius; }
};

struct Vane {
    float innerX, innerY;   // Inner endpoint (screen coords)
    float outerX, outerY;   // Outer endpoint (screen coords)
    int index;              // Vane index
};

struct DebugInfo {
    float centrifugalMagnitude;
    float coriolisMagnitude;
    float dragMagnitude;
    float totalVelocity;
};

class DryerPhysics {
public:
    DryerPhysics();
    ~DryerPhysics() = default;
    
    // Configuration
    void setParameters(float rpm, float drumSizeCm, int vaneCount, float vaneHeightPercent);
    void setBallProperties(float radius, float mass, float restitution, float dragCoeff);
    
    // Ball type presets
    void setTennisBall();
    void setBalloonBall();
    
    // Feature toggles
    void setLintTrap(bool enabled);
    void setMoonGravity(bool enabled);
    
    // Physics simulation
    void step(float dt);
    void reset();
    
    // Collision callback
    using CollisionCallback = std::function<void(const Surface&, float velocity)>;
    void onCollision(CollisionCallback callback);
    
    // Rendering helpers
    struct BallPosition {
        float x, y;
        float radius;
    };
    BallPosition getBallPosition(int canvasSize) const;
    std::vector<Vane> getVanePositions(int canvasSize) const;
    
    // Accessors
    const Ball& getBall() const { return ball; }
    const std::vector<Surface>& getSurfaces() const { return surfaces; }
    float getDrumAngle() const { return drumAngle; }
    float getDrumRadius() const { return drumRadius; }
    int getVaneCount() const { return vaneCount; }
    float getVaneHeight() const { return vaneHeight; }
    DebugInfo getDebugInfo() const { return debugInfo; }
    
    // Debug toggles
    void toggleCoriolis(bool enable);
    void toggleCentrifugal(bool enable);
    void toggleDrag(bool enable);
    
private:
    // Parameters
    float rpm;
    float drumRadius;       // meters
    int vaneCount;
    float vaneHeight;       // fraction of radius
    
    // Ball
    Ball ball;
    
    // Physical constants
    float gravity;
    float earthGravity;
    float moonGravity;
    float airDensity;       // kg/m³
    
    // Feature toggles
    bool lintTrapEnabled;
    float lintTrapThreshold;
    bool moonGravityEnabled;
    bool useQuadraticDrag;
    
    // Drum rotation
    float drumAngle;        // radians
    float drumAngularVelocity;  // rad/s
    
    // Physics effect toggles
    bool enableCoriolis;
    bool enableCentrifugal;
    bool enableAirDrag;
    int coriolisSignFlip;
    
    // Surface tracking
    std::vector<Surface> surfaces;
    std::string lastCollisionSurface;
    std::vector<CollisionCallback> collisionCallbacks;
    
    // Debug
    DebugInfo debugInfo;
    
    // Private methods
    void updateSurfaces();
    uint32_t getSurfaceColor(int index) const;
    void handleCollisions();
    void checkVaneCollisions();
    void triggerCollision(const Surface& surface, float velocity);
};

#endif // DRYER_PHYSICS_H
