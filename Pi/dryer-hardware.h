#ifndef DRYER_HARDWARE_H
#define DRYER_HARDWARE_H

#include "pins.h"
#include <cstdint>
#include <functional>
#include <vector>

// Forward declare libgpiod C++ types
namespace gpiod { 
    class chip; 
    class line_request; 
}

// ============================================================================
// DRYER HARDWARE INTERFACE
// Handles: ADS1115 ADC, GPIO, MIDI UART, Trigger outputs
// ============================================================================

// Parameter structure read from hardware
struct HardwareParameters {
    float rpm;
    float drumSize;
    int vanes;
    float vaneHeight;
    
    bool ballTypeBalloon;    // false = tennis, true = balloon
    bool lintTrapEnabled;
    bool moonGravityEnabled;
};

class DryerHardware {
public:
    DryerHardware();
    ~DryerHardware();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Read parameters from pots and switches
    HardwareParameters readParameters();
    
    // MIDI output
    void sendMIDINoteOn(uint8_t noteNumber, uint8_t velocity, uint8_t channel = 0);
    void sendMIDINoteOff(uint8_t noteNumber, uint8_t channel = 0);
    
    // Trigger outputs (for eurorack CV/Gate)
    void triggerPulse(int triggerPin, int durationMs = GPIO_TRIG_PULSE_MS);
    
    // Status
    bool isInitialized() const { return initialized; }
    
private:
    // I2C/ADC
    int i2cHandle;
    uint16_t readADC(uint8_t channel);
    bool initADS1115();
    
    // GPIO - libgpiod C++ API (v2)
    gpiod::chip *gpioChip;
    std::vector<gpiod::line_request> gpioInputLines;   // 3 switches
    std::vector<gpiod::line_request> gpioOutputLines;  // 2 triggers
    bool initGPIO();
    bool readGPIO(int pin);
    void writeGPIO(int pin, bool value);
    
    // MIDI UART
    int uartHandle;
    bool initMIDI();
    void sendMIDIByte(uint8_t byte);
    
    // State
    bool initialized;
    bool ads1115Available;
    bool midiAvailable;
    
    // Trigger timing (for pulse generation)
    struct TriggerState {
        bool active;
        uint64_t endTime;
    };
    TriggerState trigger1State;
    TriggerState trigger2State;
    
    // Update trigger outputs (call this regularly)
    void updateTriggers();
    friend class DryerApp;
};

#endif // DRYER_HARDWARE_H
