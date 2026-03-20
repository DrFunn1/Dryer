#ifndef DRYER_RENDERER_H
#define DRYER_RENDERER_H

#include "dryer-physics.h"
#include <SDL2/SDL.h>
#include <map>
#include <string>

// ============================================================================
// DRYER RENDERER - SDL2 Graphics
// Renders physics simulation to 480x480 round display
// ============================================================================

class DryerRenderer {
public:
    DryerRenderer(int width = 480, int height = 480);
    ~DryerRenderer();
    
    // Initialization
    bool initialize(bool fullscreen = true);
    void shutdown();
    
    // Rendering
    void render(const DryerPhysics& physics);
    
    // Status
    bool isInitialized() const { return initialized; }
    
private:
    // SDL objects
    SDL_Window* window;
    SDL_Renderer* renderer;
    
    // Display properties
    int width;
    int height;
    bool initialized;
    
    // Collision highlighting
    std::map<std::string, float> activeCollisions;  // surfaceId -> intensity
    
    // Drawing methods
    void clear();
    void present();
    void drawDrumSegments(const DryerPhysics& physics);
    void drawVanes(const DryerPhysics& physics);
    void drawBall(const DryerPhysics& physics);
    void updateCollisionHighlights();
    
    // Helper to convert color
    void setDrawColor(uint32_t color, float alpha = 1.0f);
    
    // Circle mask for round display
    void applyCircleMask();
    
public:
    // Called by collision events
    void highlightCollision(const std::string& surfaceId);
};

#endif // DRYER_RENDERER_H
