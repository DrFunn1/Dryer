#include "dryer-renderer.h"
#include <iostream>
#include <cmath>
#include <algorithm>

DryerRenderer::DryerRenderer(int width, int height)
    : window(nullptr)
    , renderer(nullptr)
    , width(width)
    , height(height)
    , initialized(false)
{
}

DryerRenderer::~DryerRenderer() {
    shutdown();
}

bool DryerRenderer::initialize(bool fullscreen) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create window
    Uint32 windowFlags = SDL_WINDOW_SHOWN;
    if (fullscreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }
    
    window = SDL_CreateWindow("Dryer",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              width, height,
                              windowFlags);
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    // Create renderer with VSync
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    
    // Set blend mode for alpha
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    initialized = true;
    std::cout << "SDL renderer initialized: " << width << "x" << height << std::endl;
    
    return true;
}

void DryerRenderer::shutdown() {
    if (!initialized) return;
    
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    
    SDL_Quit();
    initialized = false;
}

void DryerRenderer::setDrawColor(uint32_t color, float alpha) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    uint8_t a = static_cast<uint8_t>(alpha * 255);
    
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void DryerRenderer::clear() {
    // Black background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void DryerRenderer::present() {
    SDL_RenderPresent(renderer);
}

void DryerRenderer::render(const DryerPhysics& physics) {
    clear();
    
    // Draw components
    drawDrumSegments(physics);
    drawVanes(physics);
    drawBall(physics);
    
    // Apply circular mask for round display
    applyCircleMask();
    
    // Update collision highlights
    updateCollisionHighlights();
    
    present();
}

void DryerRenderer::drawDrumSegments(const DryerPhysics& physics) {
    int centerX = width / 2;
    int centerY = height / 2;
    float scale = width / (physics.getDrumRadius() * 2.2f);
    float radius = physics.getDrumRadius() * scale;
    
    int vaneCount = physics.getVaneCount();
    float anglePerSegment = (2.0f * M_PI) / vaneCount;
    
    // Draw each drum segment
    for (int i = 0; i < vaneCount; i++) {
        float startAngle = (i * anglePerSegment) + physics.getDrumAngle();
        float endAngle = startAngle + anglePerSegment;
        
        // Find corresponding surface
        const auto& surfaces = physics.getSurfaces();
        auto it = std::find_if(surfaces.begin(), surfaces.end(),
            [i](const Surface& s) {
                return s.type == "drum" && s.index == i;
            });
        
        if (it == surfaces.end()) continue;
        
        // Get highlight intensity
        float highlight = 0.0f;
        auto collIt = activeCollisions.find(it->id);
        if (collIt != activeCollisions.end()) {
            highlight = collIt->second;
        }
        
        float alpha = 0.3f + (highlight * 0.5f);
        setDrawColor(it->color, alpha);
        
        // Draw arc as series of line segments
        int segments = 20;
        for (int j = 0; j < segments; j++) {
            float t1 = static_cast<float>(j) / segments;
            float t2 = static_cast<float>(j + 1) / segments;
            
            float a1 = startAngle + t1 * anglePerSegment;
            float a2 = startAngle + t2 * anglePerSegment;
            
            int x1 = centerX + radius * std::cos(-a1);
            int y1 = centerY + radius * std::sin(-a1);
            int x2 = centerX + radius * std::cos(-a2);
            int y2 = centerY + radius * std::sin(-a2);
            
            // Draw thick line (simulate lineWidth)
            for (int offset = -4; offset <= 4; offset++) {
                float perpAngle = -a1 + M_PI / 2;
                int ox = offset * std::cos(perpAngle);
                int oy = offset * std::sin(perpAngle);
                
                SDL_RenderDrawLine(renderer, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
            }
        }
    }
}

void DryerRenderer::drawVanes(const DryerPhysics& physics) {
    const auto& surfaces = physics.getSurfaces();
    auto vanes = physics.getVanePositions(width);
    
    for (const auto& vane : vanes) {
        // Get highlight for this vane
        float highlight = 0.0f;
        
        auto leadIt = std::find_if(surfaces.begin(), surfaces.end(),
            [&vane](const Surface& s) {
                return s.type == "vane_leading" && s.index == vane.index;
            });
        
        auto trailIt = std::find_if(surfaces.begin(), surfaces.end(),
            [&vane](const Surface& s) {
                return s.type == "vane_trailing" && s.index == vane.index;
            });
        
        if (leadIt != surfaces.end()) {
            auto collIt = activeCollisions.find(leadIt->id);
            if (collIt != activeCollisions.end()) {
                highlight = std::max(highlight, collIt->second);
            }
        }
        
        if (trailIt != surfaces.end()) {
            auto collIt = activeCollisions.find(trailIt->id);
            if (collIt != activeCollisions.end()) {
                highlight = std::max(highlight, collIt->second);
            }
        }
        
        float alpha = 0.8f + (highlight * 0.2f);
        uint32_t color = (leadIt != surfaces.end()) ? leadIt->color : 0x555555;
        setDrawColor(color, alpha);
        
        // Draw thick line for vane
        int lineWidth = 4 + static_cast<int>(highlight * 4);
        for (int offset = -lineWidth/2; offset <= lineWidth/2; offset++) {
            float dx = vane.outerX - vane.innerX;
            float dy = vane.outerY - vane.innerY;
            float len = std::sqrt(dx*dx + dy*dy);
            float perpX = -dy / len * offset;
            float perpY = dx / len * offset;
            
            SDL_RenderDrawLine(renderer,
                             vane.innerX + perpX, vane.innerY + perpY,
                             vane.outerX + perpX, vane.outerY + perpY);
        }
    }
}

void DryerRenderer::drawBall(const DryerPhysics& physics) {
    auto ball = physics.getBallPosition(width);
    
    // Draw ball as filled circle (tennis ball yellow-green)
    // SDL2 doesn't have native circle drawing, so we approximate
    
    int radius = static_cast<int>(ball.radius);
    int x0 = static_cast<int>(ball.x);
    int y0 = static_cast<int>(ball.y);
    
    // Tennis ball gradient approximation (draw as filled circle with gradient)
    // Use multiple passes with decreasing alpha for gradient effect
    
    for (int r = radius; r >= 0; r--) {
        float t = static_cast<float>(r) / radius;
        
        // Color gradient from bright yellow-green to darker
        uint8_t red = static_cast<uint8_t>(232 - t * 30);
        uint8_t green = static_cast<uint8_t>(244 - t * 50);
        uint8_t blue = static_cast<uint8_t>(54 - t * 30);
        
        SDL_SetRenderDrawColor(renderer, red, green, blue, 255);
        
        // Draw filled circle using midpoint circle algorithm
        int x = r;
        int y = 0;
        int err = 0;
        
        while (x >= y) {
            SDL_RenderDrawLine(renderer, x0 - x, y0 + y, x0 + x, y0 + y);
            SDL_RenderDrawLine(renderer, x0 - x, y0 - y, x0 + x, y0 - y);
            SDL_RenderDrawLine(renderer, x0 - y, y0 + x, x0 + y, y0 + x);
            SDL_RenderDrawLine(renderer, x0 - y, y0 - x, x0 + y, y0 - x);
            
            if (err <= 0) {
                y += 1;
                err += 2*y + 1;
            }
            if (err > 0) {
                x -= 1;
                err -= 2*x + 1;
            }
        }
    }
    
    // Draw tennis ball seam (white curved lines)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
    int seamRadius = static_cast<int>(ball.radius * 0.7f);
    
    // Simplified seam representation
    for (int angle = 10; angle < 170; angle += 2) {
        float rad = angle * M_PI / 180.0f;
        int x1 = x0 + seamRadius * std::cos(rad);
        int y1 = y0 + seamRadius * std::sin(rad);
        int x2 = x0 + seamRadius * std::cos(rad + 0.05f);
        int y2 = y0 + seamRadius * std::sin(rad + 0.05f);
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
    
    for (int angle = 190; angle < 350; angle += 2) {
        float rad = angle * M_PI / 180.0f;
        int x1 = x0 + seamRadius * std::cos(rad);
        int y1 = y0 + seamRadius * std::sin(rad);
        int x2 = x0 + seamRadius * std::cos(rad + 0.05f);
        int y2 = y0 + seamRadius * std::sin(rad + 0.05f);
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    }
}

void DryerRenderer::applyCircleMask() {
    // Draw black corners to create circular display effect
    int centerX = width / 2;
    int centerY = height / 2;
    int displayRadius = width / 2;
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    
    // Draw black outside the circle
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dx = x - centerX;
            int dy = y - centerY;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            if (dist > displayRadius) {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}

void DryerRenderer::highlightCollision(const std::string& surfaceId) {
    activeCollisions[surfaceId] = 1.0f;
}

void DryerRenderer::updateCollisionHighlights() {
    // Decay highlights over time
    auto it = activeCollisions.begin();
    while (it != activeCollisions.end()) {
        it->second -= 0.05f;
        
        if (it->second <= 0.0f) {
            it = activeCollisions.erase(it);
        } else {
            ++it;
        }
    }
}
