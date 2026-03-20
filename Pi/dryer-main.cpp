#include "dryer-physics.h"
#include "dryer-hardware.h"
#include "dryer-renderer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <signal.h>

// ============================================================================
// DRYER MAIN APPLICATION
// Integrates physics, hardware I/O, and rendering
// ============================================================================

// Global flag for clean shutdown
volatile bool g_running = true;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

class DryerApp {
public:
    DryerApp()
        : running(false)
        , baseNote(36)  // C2 - good bass range for percussion
    {
    }
    
    bool initialize() {
        std::cout << "=====================================" << std::endl;
        std::cout << "   DRYER - Chaotic Percussion Gen   " << std::endl;
        std::cout << "=====================================" << std::endl;
        
        // Initialize hardware
        if (!hardware.initialize()) {
            std::cerr << "Failed to initialize hardware" << std::endl;
            return false;
        }
        
        // Initialize renderer
        if (!renderer.initialize(true)) {  // true = fullscreen
            std::cerr << "Failed to initialize renderer" << std::endl;
            return false;
        }
        
        // Set up collision callback
        physics.onCollision([this](const Surface& surface, float velocity) {
            this->onCollision(surface, velocity);
        });
        
        // Initial parameter read
        updateParameters();
        
        // Assign MIDI notes to surfaces
        assignMIDINotes();
        
        std::cout << "Initialization complete!" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        
        return true;
    }
    
    void shutdown() {
        std::cout << "Shutting down..." << std::endl;
        renderer.shutdown();
        hardware.shutdown();
    }
    
    void run() {
        running = true;
        
        auto lastTime = std::chrono::high_resolution_clock::now();
        auto lastParamUpdate = lastTime;
        
        const float targetDt = 1.0f / 60.0f;  // 60 FPS target
        const int substeps = 4;               // Physics substeps for stability
        const float substepDt = targetDt / substeps;
        
        // Parameter update rate (don't read ADC every frame)
        const auto paramUpdateInterval = std::chrono::milliseconds(50);  // 20Hz
        
        while (running && g_running) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            
            // Limit delta time to prevent large jumps
            float clampedDt = std::min(elapsed, 0.033f);  // Max 33ms
            
            // Update parameters periodically
            if (currentTime - lastParamUpdate >= paramUpdateInterval) {
                updateParameters();
                lastParamUpdate = currentTime;
            }
            
            // Update physics (with substeps)
            for (int i = 0; i < substeps; i++) {
                physics.step(substepDt);
            }
            
            // Update trigger outputs
            hardware.updateTriggers();
            
            // Render
            renderer.render(physics);
            
            // Handle events (for clean shutdown)
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
            }
            
            // Sleep to maintain frame rate (VSync should handle this, but just in case)
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
private:
    DryerPhysics physics;
    DryerHardware hardware;
    DryerRenderer renderer;
    
    bool running;
    int baseNote;
    std::map<std::string, int> surfaceToNote;
    
    void updateParameters() {
        auto params = hardware.readParameters();
        
        // Update physics parameters
        physics.setParameters(params.rpm, params.drumSize, params.vanes, params.vaneHeight);
        
        // Update ball type
        static bool lastBallType = false;
        if (params.ballTypeBalloon != lastBallType) {
            if (params.ballTypeBalloon) {
                physics.setBalloonBall();
            } else {
                physics.setTennisBall();
            }
            lastBallType = params.ballTypeBalloon;
        }
        
        // Update features
        static bool lastLintTrap = false;
        if (params.lintTrapEnabled != lastLintTrap) {
            physics.setLintTrap(params.lintTrapEnabled);
            lastLintTrap = params.lintTrapEnabled;
        }
        
        static bool lastMoonGravity = false;
        if (params.moonGravityEnabled != lastMoonGravity) {
            physics.setMoonGravity(params.moonGravityEnabled);
            lastMoonGravity = params.moonGravityEnabled;
        }
        
        // Reassign MIDI notes if surface count changed
        assignMIDINotes();
    }
    
    void assignMIDINotes() {
        surfaceToNote.clear();
        
        const auto& surfaces = physics.getSurfaces();
        for (size_t i = 0; i < surfaces.size(); i++) {
            int noteNumber = baseNote + static_cast<int>(i);
            surfaceToNote[surfaces[i].id] = noteNumber;
        }
    }
    
    void onCollision(const Surface& surface, float velocity) {
        // Get MIDI note for this surface
        auto it = surfaceToNote.find(surface.id);
        if (it == surfaceToNote.end()) return;
        
        int noteNumber = it->second;
        
        // Scale velocity to MIDI range (0-127)
        int velocityMIDI = std::min(127, static_cast<int>(velocity * 300));
        
        // Send MIDI note
        hardware.sendMIDINoteOn(noteNumber, velocityMIDI);
        
        // Schedule note off after 100ms
        // (In a real implementation, you'd use a timer thread)
        std::thread([this, noteNumber]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            hardware.sendMIDINoteOff(noteNumber);
        }).detach();
        
        // Trigger CV output
        if (surface.type == "drum") {
            hardware.triggerPulse(GPIO_TRIGGER_OUT_1);
        } else if (surface.type == "vane_leading" || surface.type == "vane_trailing") {
            hardware.triggerPulse(GPIO_TRIGGER_OUT_2);
        }
        
        // Update visual feedback
        renderer.highlightCollision(surface.id);
        
        // Debug output
        // std::cout << "Collision: " << surface.id << " vel=" << velocity 
        //          << " note=" << noteNumber << std::endl;
    }
};

int main(int argc, char* argv[]) {
    // Set up signal handler for clean shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    DryerApp app;
    
    if (!app.initialize()) {
        std::cerr << "Initialization failed!" << std::endl;
        return 1;
    }
    
    app.run();
    app.shutdown();
    
    std::cout << "Goodbye!" << std::endl;
    return 0;
}
