#pragma once
#include <cmath>
#include <cstring>
#include <functional>

// Maximum number of vanes supported (surfaces = 3 * maxVanes)
static const int DRYER_MAX_VANES = 9;
static const int DRYER_MAX_SURFACES = 3 * DRYER_MAX_VANES;

struct DryerSurface {
    enum Type { DRUM, VANE_LEADING, VANE_TRAILING } type;
    int index;        // vane index (0-based)
    bool active;      // true if valid for current vane count
};

struct DryerCollision {
    DryerSurface* surface;
    float velocity;   // normal impact speed (m/s)
};

class DryerPhysics {
public:
    // --- Parameters ---
    float rpm           = 18.0f;
    float drumRadius    = 0.60f;  // meters (60cm default, matches JS knob default)
    int   vaneCount     = 4;
    float vaneHeight    = 0.30f;  // fraction of radius

    // --- Feature flags ---
    bool  enableCoriolis    = true;
    bool  enableCentrifugal = true;
    bool  enableAirDrag     = true;
    bool  moonGravity       = false;
    bool  lintTrap          = false;
    float lintTrapThreshold = 0.15f;  // m/s

    // --- Ball state ---
    float bx = 0.f, by = 0.f;   // position (m)
    float vx = 0.f, vy = 0.f;   // velocity (m/s)

    // Ball physical properties (tennis ball defaults)
    float ballRadius      = 0.035f;
    float ballMass        = 0.058f;
    float ballRestitution = 0.75f;
    float ballDragCoeff   = 0.55f;

    // --- Drum state ---
    float drumAngle           = 0.f;  // rad
    float drumAngularVelocity = 0.f;  // rad/s

    // --- Pending collisions (cleared each step) ---
    DryerCollision pendingCollisions[DRYER_MAX_SURFACES];
    int            pendingCount = 0;

    // --- Debounce ---
    // Per-surface debounce timer (counts down in seconds)
    float debounce[DRYER_MAX_SURFACES] = {};
    static const float DEBOUNCE_TIME;  // 50ms

    // --- Surface table (rebuilt on vane count change) ---
    DryerSurface surfaces[DRYER_MAX_SURFACES];
    int          surfaceCount = 0;

    DryerPhysics() {
        reset();
        setParameters(rpm, drumRadius * 100.f, vaneCount, vaneHeight * 100.f);
    }

    // rpm, drumSizeCm, vaneCount, vaneHeightPercent — matches JS API
    void setParameters(float rpm_, float drumSizeCm, int vanes, float vaneHeightPct) {
        rpm       = rpm_;
        drumRadius = drumSizeCm / 100.f;
        vaneCount  = vanes;
        if (vaneCount < 1) vaneCount = 1;
        if (vaneCount > DRYER_MAX_VANES) vaneCount = DRYER_MAX_VANES;
        vaneHeight = vaneHeightPct / 100.f;
        drumAngularVelocity = (rpm * 2.f * M_PI_F) / 60.f;
        buildSurfaces();
    }

    void setBallTennis()  { ballRadius=0.035f; ballMass=0.058f;   ballRestitution=0.75f; ballDragCoeff=0.55f; }
    void setBallSandbag() { ballRadius=0.050f; ballMass=0.500f;   ballRestitution=0.15f; ballDragCoeff=0.80f; }
    void setBallBalloon() { ballRadius=0.130f; ballMass=0.01228f; ballRestitution=0.30f; ballDragCoeff=0.47f; }

    void reset() {
        bx = drumRadius * 0.3f;
        by = 0.f;
        vx = 0.f;
        vy = 0.f;
        drumAngle = 0.f;
        memset(debounce, 0, sizeof(debounce));
    }

    // Call this at the physics rate (e.g. 250Hz). dt in seconds.
    // Collision results are in pendingCollisions[0..pendingCount-1].
    void step(float dt) {
        pendingCount = 0;

        // Tick debounce timers
        for (int i = 0; i < surfaceCount; i++) {
            if (debounce[i] > 0.f) debounce[i] -= dt;
        }

        // Advance drum angle
        drumAngle += drumAngularVelocity * dt;

        // --- Forces (all as accelerations) ---
        const float g       = moonGravity ? 1.635f : 9.81f;
        const float cosA    = cosf(drumAngle);
        const float sinA    = sinf(drumAngle);

        // 1. Gravity in rotating frame
        float ax = -g * sinA;
        float ay = -g * cosA;

        // 2. Buoyancy
        const float airDensity    = 1.225f;
        const float ballArea      = M_PI_F * ballRadius * ballRadius;
        const float ballVolume    = (4.f/3.f) * M_PI_F * ballRadius * ballRadius * ballRadius;
        const float buoyancyFactor = (airDensity * ballVolume) / ballMass;
        ax += -(-g * sinA) * buoyancyFactor;  // opposes gravity components
        ay += -(-g * cosA) * buoyancyFactor;

        // 3. Centrifugal
        if (enableCentrifugal) {
            const float r = sqrtf(bx*bx + by*by);
            if (r > 0.0001f) {
                const float cmag = drumAngularVelocity * drumAngularVelocity * r * (1.f - buoyancyFactor);
                ax += (bx / r) * cmag;
                ay += (by / r) * cmag;
            }
        }

        // 4. Coriolis
        if (enableCoriolis) {
            ax += 2.f * drumAngularVelocity * vy;
            ay += -2.f * drumAngularVelocity * vx;
        }

        // 5. Air drag with vane-coupled velocity field
        if (enableAirDrag) {
            const float r = sqrtf(bx*bx + by*by);
            if (r > 0.001f) {
                const float h = vaneHeight;
                const float n = (float)vaneCount;
                const float c = 1.f - expf(-0.5f * n * h);
                const float omega = drumAngularVelocity;
                const float vAirTangential = omega * r * (1.f - c) * (r / drumRadius - 1.f);
                const float theta = atan2f(by, bx);
                const float vAirX = -vAirTangential * sinf(theta);
                const float vAirY =  vAirTangential * cosf(theta);
                const float vRelX = vx - vAirX;
                const float vRelY = vy - vAirY;
                const float vRelSpd = sqrtf(vRelX*vRelX + vRelY*vRelY);
                if (vRelSpd > 0.001f) {
                    const float dragMag = 0.5f * airDensity * vRelSpd * vRelSpd
                                        * ballDragCoeff * ballArea / ballMass;
                    ax += -(vRelX / vRelSpd) * dragMag;
                    ay += -(vRelY / vRelSpd) * dragMag;
                }
            }
        }

        // Integrate
        vx += ax * dt;
        vy += ay * dt;
        bx += vx * dt;
        by += vy * dt;

        handleCollisions();
    }

private:
    static constexpr float M_PI_F = 3.14159265358979323846f;

    void buildSurfaces() {
        surfaceCount = 0;
        for (int i = 0; i < vaneCount; i++) {
            surfaces[surfaceCount++] = { DryerSurface::DRUM,           i, true };
            surfaces[surfaceCount++] = { DryerSurface::VANE_LEADING,   i, true };
            surfaces[surfaceCount++] = { DryerSurface::VANE_TRAILING,  i, true };
        }
    }

    int surfaceIndex(DryerSurface::Type type, int idx) {
        for (int i = 0; i < surfaceCount; i++) {
            if (surfaces[i].type == type && surfaces[i].index == idx) return i;
        }
        return -1;
    }

    void triggerCollision(int surfIdx, float speed) {
        if (surfIdx < 0) return;
        if (lintTrap && speed < lintTrapThreshold) return;
        if (debounce[surfIdx] > 0.f) return;
        debounce[surfIdx] = DEBOUNCE_TIME;
        if (pendingCount < DRYER_MAX_SURFACES) {
            pendingCollisions[pendingCount++] = { &surfaces[surfIdx], speed };
        }
    }

    void handleCollisions() {
        const float ballDist = sqrtf(bx*bx + by*by);

        // --- Drum wall ---
        if (ballDist + ballRadius > drumRadius) {
            const float penetration = ballDist + ballRadius - drumRadius;
            const float nx = -bx / ballDist;
            const float ny = -by / ballDist;
            bx += nx * penetration;
            by += ny * penetration;

            const float vn = vx*nx + vy*ny;
            if (vn < 0.f) {
                // Determine segment before reflecting
                float angle = atan2f(by, bx);
                if (angle < 0.f) angle += 2.f * M_PI_F;
                const float anglePerSeg = (2.f * M_PI_F) / (float)vaneCount;
                int seg = (int)(angle / anglePerSeg) % vaneCount;

                vx -= (1.f + ballRestitution) * vn * nx;
                vy -= (1.f + ballRestitution) * vn * ny;

                triggerCollision(surfaceIndex(DryerSurface::DRUM, seg), fabsf(vn));
            }
        }

        // --- Vanes ---
        const float vaneInner = drumRadius * (1.f - vaneHeight);
        for (int i = 0; i < vaneCount; i++) {
            const float vaneAngle = (float)i / (float)vaneCount * 2.f * M_PI_F;
            const float vx1 = vaneInner  * cosf(vaneAngle);
            const float vy1 = vaneInner  * sinf(vaneAngle);
            const float vx2 = drumRadius * cosf(vaneAngle);
            const float vy2 = drumRadius * sinf(vaneAngle);

            const float dvx = vx2 - vx1;
            const float dvy = vy2 - vy1;
            const float vaneLen = sqrtf(dvx*dvx + dvy*dvy);

            const float dx = bx - vx1;
            const float dy = by - vy1;
            const float t = (dx*dvx + dy*dvy) / (vaneLen * vaneLen);

            if (t < 0.f || t > 1.f) continue;

            const float closestX = vx1 + t * dvx;
            const float closestY = vy1 + t * dvy;
            const float distX = bx - closestX;
            const float distY = by - closestY;
            const float dist  = sqrtf(distX*distX + distY*distY);

            if (dist < ballRadius) {
                const float penetration = ballRadius - dist;
                const float nx = distX / dist;
                const float ny = distY / dist;

                bx += nx * penetration;
                by += ny * penetration;

                const float vn = vx*nx + vy*ny;
                if (vn < 0.f) {
                    vx -= (1.f + ballRestitution) * vn * nx;
                    vy -= (1.f + ballRestitution) * vn * ny;

                    // Determine leading vs trailing face
                    const float perpX = -dvy / vaneLen;
                    const float perpY =  dvx / vaneLen;
                    const bool  leading = (dx*perpX + dy*perpY) > 0.f;
                    const auto  stype = leading ? DryerSurface::VANE_LEADING
                                                : DryerSurface::VANE_TRAILING;
                    triggerCollision(surfaceIndex(stype, i), fabsf(vn));
                }
            }
        }
    }
};

const float DryerPhysics::DEBOUNCE_TIME = 0.050f;  // 50ms
