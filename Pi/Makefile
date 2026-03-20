# Makefile for Dryer Eurorack Module
# Alternative to CMake for simpler builds

# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Wextra -O3

# Include paths
INCLUDES = -I/usr/include/SDL2

# Libraries
LIBS = -lSDL2 -lgpiod -lpthread -lm

# Source files
SOURCES = dryer-main.cpp \
          dryer-physics.cpp \
          dryer-hardware.cpp \
          dryer-renderer.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Output executable
TARGET = dryer

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)
	@echo "Build complete: $(TARGET)"
	@echo ""
	@echo "To run: sudo ./$(TARGET)"
	@echo ""

# Compile source files to object files
%.o: %.cpp
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

# Install to system
install: $(TARGET)
	@echo "Installing to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Install complete"
	@echo ""
	@echo "Run with: sudo dryer"
	@echo ""

# Uninstall from system
uninstall:
	@echo "Uninstalling from /usr/local/bin..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete"

# Run (with sudo for GPIO access)
run: $(TARGET)
	@echo "Starting Dryer (requires sudo for GPIO)..."
	sudo ./$(TARGET)

# Show dependencies
depends:
	@echo "Required packages:"
	@echo "  - build-essential"
	@echo "  - libsdl2-dev"
	@echo "  - libpigpio-dev"
	@echo "  - i2c-tools"
	@echo ""
	@echo "Install with:"
	@echo "  sudo apt install build-essential libsdl2-dev libpigpio-dev i2c-tools"
	@echo ""

# Help
help:
	@echo "Dryer Eurorack Module - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the project"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make install  - Install to /usr/local/bin"
	@echo "  make uninstall- Remove from /usr/local/bin"
	@echo "  make run      - Build and run (requires sudo)"
	@echo "  make depends  - Show required dependencies"
	@echo "  make help     - Show this help message"
	@echo ""

.PHONY: all clean install uninstall run depends help
