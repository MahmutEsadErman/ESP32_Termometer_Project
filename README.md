# ESP32 Thermometer Project

A comprehensive temperature monitoring system built with ESP32 that displays temperature readings on an LCD display and provides visual feedback through an RGB LED. The system features a button-controlled dual-screen interface and supports both Wokwi simulation and physical hardware deployment. The reason for me to make this project is to practise FreeRTOS concepts such as semaphores, queues as well as communication protocols like i2c and understand basics of multitasking in embedded systems.

## Features

- **Real-time temperature monitoring** using NTC thermistor sensor
- **Dual-screen LCD interface** switchable via button press:
  - Temperature display screen (default)
  - System uptime counter screen
- **Visual temperature indication** through RGB LED color mapping
- **Wokwi simulator support** for easy testing and development
- **ESP-IDF framework** based implementation
- **FreeRTOS multitasking** architecture

## 🔧 Hardware Components

- **ESP32 DevKit V1** - Main microcontroller
- **LCD 1602 with I2C backpack** - Temperature and status display
- **NTC Temperature Sensor** - Temperature measurement
- **RGB LED (Common Cathode)** - Visual temperature indication
- **Push Button** - Screen switching functionality
- **Resistors** - Pull-up and current limiting

## 📊 Pin Configuration

### LCD (I2C)
- **SDA**: GPIO 21
- **SCL**: GPIO 22
- **I2C Address**: 0x27

### RGB LED
- **Red**: GPIO 19
- **Green**: GPIO 18
- **Blue**: GPIO 5

### NTC Thermistor
- **Analog Input**: GPIO 34 (ADC1_CH6)
- **Configuration**: 12-bit resolution with 12dB attenuation

### Button
- **Input**: GPIO 13
- **Configuration**: Pull-up enabled, rising edge interrupt

## Getting Started

### Prerequisites

1. **ESP-IDF**: Install the ESP-IDF development framework. Follow the official ESP-IDF installation guide https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
   

2. **Wokwi Extension** (Optional): For simulation
   - Install the Wokwi extension in VS Code for simulation testing

### Building and Flashing

1. **Clone or download** this repository
2. **Navigate** to the project directory
3. **Set the target** (ESP32):
   ```bash
   idf.py set-target esp32
   ```
4. **Configure** the project (optional):
   ```bash
   idf.py menuconfig
   ```
5. **Build** the project:
   ```bash
   idf.py build
   ```
6. **Flash** to ESP32:
   ```bash
   idf.py -p COM_PORT flash monitor
   ```
   Replace `COM_PORT` with your ESP32's serial port.

### Wokwi Simulation

This project includes a complete Wokwi simulation setup:

1. Open the project in VS Code with Wokwi extension
2. Press `F1` and run "Wokwi: Start Simulator"
3. The simulation will automatically load the circuit diagram and start running

## Multitasking Architecture
- **Temperature Task**: Periodic sensor reading via ESP timer
- **LCD Task**: Display management and button handling
- **LED Task**: RGB color control based on temperature
- **Semaphore synchronization**: Ensures thread-safe data sharing

## 📁 Project Structure

```
├── main/
│   ├── main.c              # Main application logic
│   ├── lcd.c               # LCD driver and I2C 
│   ├── rgb_led.c           # RGB LED PWM control
│   ├── CMakeLists.txt      # Component build configuration
│   └── idf_component.yml   # Component dependencies
├── build/                  # Compiled binaries and build artifacts
├── managed_components/     # External component dependencies
├── CMakeLists.txt         # Project build configuration
├── sdkconfig              # ESP-IDF configuration
├── wokwi.toml            # Wokwi simulator configuration
├── diagram.json          # Wokwi circuit diagram
└── README.md             # This file
```

## Future Enhancements

- **Wi-Fi connectivity** for remote monitoring
- **Data logging** to SD card or cloud storage
- **Multiple sensor support** for environmental monitoring
- **Mobile app integration** for remote control
- **Alarm system** for temperature thresholds
- **Historical data visualization**

## License

This project is open source and available under the MIT License.

---
