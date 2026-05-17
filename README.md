# ESP32 Energy Monitor

## Hardware
- ESP32 DevKit
- PZEM-004T v3.0 (UART2: RX=GPIO16, TX=GPIO17)
- I2C LCD 16x2 (SDA=GPIO21, SCL=GPIO22, addr=0x27)

## Dependencies
- PZEM004Tv30
- LiquidCrystal_I2C
- ArduinoJson

## Features
- Real-time voltage/current/power monitoring
- Prepaid balance system with EEPROM persistence
- Web dashboard on port 80
- Theft detection, OV/OC alerts

## Usage
1. Set WiFi credentials in the sketch
2. Flash via Arduino IDE or PlatformIO
3. Access dashboard at the IP shown on LCD
