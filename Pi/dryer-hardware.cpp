#include "dryer-hardware.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <termios.h>
#include <chrono>
#include <thread>
#include <gpiod.hpp>
#include <cstring>

// ADS1115 Register addresses
#define ADS1115_REG_CONVERSION  0x00
#define ADS1115_REG_CONFIG      0x01

// ADS1115 Config register bits
#define ADS1115_OS_SINGLE       0x8000
#define ADS1115_MUX_AIN0        0x4000
#define ADS1115_MUX_AIN1        0x5000
#define ADS1115_MUX_AIN2        0x6000
#define ADS1115_MUX_AIN3        0x7000
#define ADS1115_PGA_4_096V      0x0200
#define ADS1115_MODE_SINGLE     0x0100
#define ADS1115_DR_128SPS       0x0080

DryerHardware::DryerHardware() 
    : i2cHandle(-1)
    , uartHandle(-1)
    , initialized(false)
    , ads1115Available(false)
    , midiAvailable(false)
    , gpioChip(nullptr)
{
    trigger1State.active = false;
    trigger2State.active = false;
}

DryerHardware::~DryerHardware() {
    shutdown();
}

bool DryerHardware::initialize() {
    std::cout << "Initializing Dryer hardware..." << std::endl;
    
    // Initialize components
    bool gpioOk = initGPIO();
    bool adsOk = initADS1115();
    bool midiOk = initMIDI();
    
    if (!gpioOk) {
        std::cerr << "WARNING: GPIO initialization failed" << std::endl;
    }
    
    if (!adsOk) {
        std::cerr << "WARNING: ADS1115 not found, using default parameters" << std::endl;
    }
    
    if (!midiOk) {
        std::cerr << "WARNING: MIDI UART not available" << std::endl;
    }
    
    initialized = true;
    std::cout << "Hardware initialization complete" << std::endl;
    std::cout << "  GPIO: " << (gpioOk ? "OK" : "FAILED") << std::endl;
    std::cout << "  ADS1115: " << (adsOk ? "OK" : "NOT FOUND") << std::endl;
    std::cout << "  MIDI: " << (midiOk ? "OK" : "NOT AVAILABLE") << std::endl;
    
    return gpioOk;
}

void DryerHardware::shutdown() {
    if (!initialized) return;
    
    // Close I2C
    if (i2cHandle >= 0) {
        close(i2cHandle);
        i2cHandle = -1;
    }
    
    // Close UART
    if (uartHandle >= 0) {
        close(uartHandle);
        uartHandle = -1;
    }
    
    // Clear GPIO vectors (line_request destructor handles cleanup)
    gpioInputLines.clear();
    gpioOutputLines.clear();
    
    // Close GPIO chip
    if (gpioChip) {
        delete gpioChip;
        gpioChip = nullptr;
    }
    
    initialized = false;
    std::cout << "Hardware shutdown complete" << std::endl;
}

bool DryerHardware::initGPIO() {
    try {
        // Open GPIO chip (gpiochip0 for Raspberry Pi)
        gpioChip = new gpiod::chip("gpiochip0");
        
        // Resize vectors to hold line requests
        gpioInputLines.resize(3);
        gpioOutputLines.resize(2);
        
        // Configure input pins (switches) with pull-down
        const int inputPins[] = {GPIO_BALL_TYPE, GPIO_LINT_TRAP, GPIO_MOON_GRAVITY};
        
        for (int i = 0; i < 3; i++) {
            gpioInputLines[i] = gpioChip->prepare_request()
                .set_consumer("dryer")
                .add_line_settings(inputPins[i], 
                    gpiod::line_settings()
                        .set_direction(gpiod::line::direction::INPUT)
                        .set_bias(gpiod::line::bias::PULL_DOWN))
                .do_request();
        }
        
        // Configure output pins (triggers) initially low
        const int outputPins[] = {GPIO_TRIGGER_OUT_1, GPIO_TRIGGER_OUT_2};
        
        for (int i = 0; i < 2; i++) {
            gpioOutputLines[i] = gpioChip->prepare_request()
                .set_consumer("dryer")
                .add_line_settings(outputPins[i],
                    gpiod::line_settings()
                        .set_direction(gpiod::line::direction::OUTPUT)
                        .set_output_value(gpiod::line::value::INACTIVE))
                .do_request();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "GPIO initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool DryerHardware::initADS1115() {
    // Open I2C bus
    i2cHandle = open("/dev/i2c-1", O_RDWR);
    if (i2cHandle < 0) {
        return false;
    }
    
    // Set I2C slave address
    if (ioctl(i2cHandle, I2C_SLAVE, ADS1115_ADDRESS) < 0) {
        close(i2cHandle);
        i2cHandle = -1;
        return false;
    }
    
    ads1115Available = true;
    return true;
}

bool DryerHardware::initMIDI() {
    // Open UART device
    uartHandle = open(UART_DEVICE, O_WRONLY | O_NOCTTY);
    if (uartHandle < 0) {
        return false;
    }
    
    // Configure UART for MIDI (31.25 kbaud, 8N1)
    struct termios tty;
    if (tcgetattr(uartHandle, &tty) != 0) {
        close(uartHandle);
        uartHandle = -1;
        return false;
    }
    
    // Set baud rate to 38400 (closest to 31250)
    cfsetospeed(&tty, B38400);
    
    // 8N1 mode
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CLOCAL | CREAD;
    
    // Raw mode
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    
    if (tcsetattr(uartHandle, TCSANOW, &tty) != 0) {
        close(uartHandle);
        uartHandle = -1;
        return false;
    }
    
    midiAvailable = true;
    return true;
}

uint16_t DryerHardware::readADC(uint8_t channel) {
    if (!ads1115Available) {
        return ADC_MAX_VALUE / 2;
    }
    
    // Select channel
    uint16_t config = ADS1115_OS_SINGLE |
                     ADS1115_PGA_4_096V |
                     ADS1115_MODE_SINGLE |
                     ADS1115_DR_128SPS;
    
    switch(channel) {
        case 0: config |= ADS1115_MUX_AIN0; break;
        case 1: config |= ADS1115_MUX_AIN1; break;
        case 2: config |= ADS1115_MUX_AIN2; break;
        case 3: config |= ADS1115_MUX_AIN3; break;
        default: return 0;
    }
    
    // Write config register
    uint8_t writeBuffer[3] = {
        ADS1115_REG_CONFIG,
        static_cast<uint8_t>(config >> 8),
        static_cast<uint8_t>(config & 0xFF)
    };
    
    if (write(i2cHandle, writeBuffer, 3) != 3) {
        return 0;
    }
    
    // Wait for conversion
    usleep(10000);
    
    // Read conversion register
    uint8_t reg = ADS1115_REG_CONVERSION;
    if (write(i2cHandle, &reg, 1) != 1) {
        return 0;
    }
    
    uint8_t readBuffer[2];
    if (read(i2cHandle, readBuffer, 2) != 2) {
        return 0;
    }
    
    uint16_t value = (readBuffer[0] << 8) | readBuffer[1];
    
    // Clip negative values
    if (value > 32768) value = 0;
    
    return value;
}

bool DryerHardware::readGPIO(int pin) {
    try {
        const int inputPins[] = {GPIO_BALL_TYPE, GPIO_LINT_TRAP, GPIO_MOON_GRAVITY};
        
        for (int i = 0; i < 3; i++) {
            if (inputPins[i] == pin && i < (int)gpioInputLines.size()) {
                auto value = gpioInputLines[i].get_value(inputPins[i]);
                return value == gpiod::line::value::ACTIVE;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "GPIO read error: " << e.what() << std::endl;
    }
    return false;
}

void DryerHardware::writeGPIO(int pin, bool value) {
    try {
        const int outputPins[] = {GPIO_TRIGGER_OUT_1, GPIO_TRIGGER_OUT_2};
        
        for (int i = 0; i < 2; i++) {
            if (outputPins[i] == pin && i < (int)gpioOutputLines.size()) {
                gpioOutputLines[i].set_value(outputPins[i], 
                    value ? gpiod::line::value::ACTIVE : gpiod::line::value::INACTIVE);
                return;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "GPIO write error: " << e.what() << std::endl;
    }
}

HardwareParameters DryerHardware::readParameters() {
    HardwareParameters params;
    
    // Read ADC values and map to parameter ranges
    uint16_t rpmADC = readADC(ADC_CHAN_RPM);
    uint16_t drumADC = readADC(ADC_CHAN_DRUM_SIZE);
    uint16_t vanesADC = readADC(ADC_CHAN_VANES);
    uint16_t heightADC = readADC(ADC_CHAN_VANE_HEIGHT);
    
    params.rpm = mapADCToRange(rpmADC, ParamRanges::RPM_MIN, ParamRanges::RPM_MAX);
    params.drumSize = mapADCToRange(drumADC, ParamRanges::DRUM_SIZE_MIN, ParamRanges::DRUM_SIZE_MAX);
    
    // Vanes need to be integer
    float vanesFloat = mapADCToRange(vanesADC, ParamRanges::VANES_MIN, ParamRanges::VANES_MAX);
    params.vanes = static_cast<int>(vanesFloat + 0.5f);
    
    params.vaneHeight = mapADCToRange(heightADC, ParamRanges::VANE_HEIGHT_MIN, ParamRanges::VANE_HEIGHT_MAX);
    
    // Read switches
    params.ballTypeBalloon = readGPIO(GPIO_BALL_TYPE);
    params.lintTrapEnabled = readGPIO(GPIO_LINT_TRAP);
    params.moonGravityEnabled = readGPIO(GPIO_MOON_GRAVITY);
    
    return params;
}

void DryerHardware::sendMIDIByte(uint8_t byte) {
    if (midiAvailable && uartHandle >= 0) {
        write(uartHandle, &byte, 1);
    }
}

void DryerHardware::sendMIDINoteOn(uint8_t noteNumber, uint8_t velocity, uint8_t channel) {
    if (!midiAvailable) return;
    
    uint8_t statusByte = 0x90 | (channel & 0x0F);
    sendMIDIByte(statusByte);
    sendMIDIByte(noteNumber & 0x7F);
    sendMIDIByte(velocity & 0x7F);
}

void DryerHardware::sendMIDINoteOff(uint8_t noteNumber, uint8_t channel) {
    if (!midiAvailable) return;
    
    uint8_t statusByte = 0x80 | (channel & 0x0F);
    sendMIDIByte(statusByte);
    sendMIDIByte(noteNumber & 0x7F);
    sendMIDIByte(0);
}

void DryerHardware::triggerPulse(int triggerPin, int durationMs) {
    // Start trigger pulse
    writeGPIO(triggerPin, true);
    
    // Calculate end time
    auto now = std::chrono::steady_clock::now();
    auto endTime = now + std::chrono::milliseconds(durationMs);
    
    // Store state
    if (triggerPin == GPIO_TRIGGER_OUT_1) {
        trigger1State.active = true;
        trigger1State.endTime = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime.time_since_epoch()).count();
    } else if (triggerPin == GPIO_TRIGGER_OUT_2) {
        trigger2State.active = true;
        trigger2State.endTime = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime.time_since_epoch()).count();
    }
}

void DryerHardware::updateTriggers() {
    auto now = std::chrono::steady_clock::now();
    uint64_t nowMicros = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
    
    // Check trigger 1
    if (trigger1State.active && nowMicros >= trigger1State.endTime) {
        writeGPIO(GPIO_TRIGGER_OUT_1, false);
        trigger1State.active = false;
    }
    
    // Check trigger 2
    if (trigger2State.active && nowMicros >= trigger2State.endTime) {
        writeGPIO(GPIO_TRIGGER_OUT_2, false);
        trigger2State.active = false;
    }
}
