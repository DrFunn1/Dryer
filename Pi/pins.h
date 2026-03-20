#include <cstdint>
#ifndef PINS_H
#define PINS_H

// ============================================================================
// DRYER EURORACK MODULE - PIN DEFINITIONS
// Hardware: Raspberry Pi Zero 2W
// ============================================================================

// I2C Bus for ADS1115 ADC
#define I2C_BUS             1           // /dev/i2c-1
#define ADS1115_ADDRESS     0x48        // Default ADS1115 I2C address

// ADS1115 ADC Channels (0-3.3V analog inputs)
#define ADC_CHAN_RPM        0           // A0: RPM potentiometer
#define ADC_CHAN_DRUM_SIZE  1           // A1: Drum Size potentiometer  
#define ADC_CHAN_VANES      2           // A2: Vanes potentiometer
#define ADC_CHAN_VANE_HEIGHT 3          // A3: Vane Height potentiometer

// GPIO Digital Inputs (3.3V toggle switches)
#define GPIO_BALL_TYPE      17          // Ball type selector (tennis/balloon)
#define GPIO_LINT_TRAP      27          // Lint trap filter enable
#define GPIO_MOON_GRAVITY   22          // Moon gravity mode enable

// GPIO Digital Outputs (0-3.3V triggers)
#define GPIO_TRIGGER_OUT_1  23          // Trigger output 1 (drum collision)
#define GPIO_TRIGGER_OUT_2  24          // Trigger output 2 (vane collision)
#define GPIO_TRIG_PULSE_MS  10          // Trigger pulse duration (ms)

// UART for MIDI Output
#define UART_DEVICE         "/dev/serial0"  // Hardware UART
#define MIDI_BAUD_RATE      31250       // MIDI standard baud rate

// Display
#define DISPLAY_WIDTH       480
#define DISPLAY_HEIGHT      480
#define DISPLAY_FPS         60

// ADC Conversion Parameters
#define ADC_MAX_VALUE       26400       // ADS1115 16-bit max (accounting for PGA)
#define ADC_REF_VOLTAGE     3.3         // Reference voltage

// Parameter Ranges (matching original JavaScript)
struct ParamRanges {
    // RPM: 1-40
    static constexpr float RPM_MIN = 1.0f;
    static constexpr float RPM_MAX = 40.0f;
    
    // Drum Size: 60-100 cm
    static constexpr float DRUM_SIZE_MIN = 60.0f;
    static constexpr float DRUM_SIZE_MAX = 100.0f;
    
    // Vanes: 1-9
    static constexpr int VANES_MIN = 1;
    static constexpr int VANES_MAX = 9;
    
    // Vane Height: 10-50%
    static constexpr float VANE_HEIGHT_MIN = 10.0f;
    static constexpr float VANE_HEIGHT_MAX = 50.0f;
};

// Helper function to map ADC value to parameter range
inline float mapADCToRange(uint16_t adcValue, float minVal, float maxVal) {
    float normalized = static_cast<float>(adcValue) / ADC_MAX_VALUE;
    return minVal + (normalized * (maxVal - minVal));
}

#endif // PINS_H
