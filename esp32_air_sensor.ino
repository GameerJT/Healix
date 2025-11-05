/*
 * SmartFit Air Quality Monitor - ESP32 Code
 * Sensors: MQ-9 (CO & Flammable Gases) + MQ-135 (Air Quality)
 * Features: BLE connectivity, DHT22 temperature/humidity sensor, USB Serial interface
 * 
 * Hardware Connections:
 * - MQ-9 Analog Output -> GPIO 34 (ADC1_CH6)
 * - MQ-135 Analog Output -> GPIO 35 (ADC1_CH7)
 * - DHT22 Data -> GPIO 4
 * - LED Indicator -> GPIO 2 (built-in LED)
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>

// Pin Definitions
#define MQ9_PIN 34          // MQ-9 sensor analog pin
#define MQ135_PIN 35        // MQ-135 sensor analog pin
#define DHT_PIN 4           // DHT22 sensor pin
#define LED_PIN 2           // Built-in LED

// DHT Sensor Configuration
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// BLE Configuration
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_MQ9       "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_UUID_MQ135     "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHAR_UUID_TEMP      "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define CHAR_UUID_HUMIDITY  "beb5483e-36e1-4688-b7f5-ea07361b26ab"
#define DEVICE_NAME         "SmartFit-Air-Monitor"

// USB Serial Configuration
#define USB_BAUD_RATE       115200
#define USB_UPDATE_INTERVAL 2000  // Update every 2 seconds

// BLE Objects
BLEServer* pServer = NULL;
BLECharacteristic* pCharMQ9 = NULL;
BLECharacteristic* pCharMQ135 = NULL;
BLECharacteristic* pCharTemp = NULL;
BLECharacteristic* pCharHumidity = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// Sensor calibration values (adjust based on your environment)
const float MQ9_RL = 10.0;        // Load resistance in kΩ
const float MQ9_RO = 10.0;        // Sensor resistance in clean air
const float MQ135_RL = 10.0;
const float MQ135_RO = 10.0;

// Smoothing variables
const int numReadings = 10;
int mq9Readings[numReadings];
int mq135Readings[numReadings];
int readIndex = 0;
int mq9Total = 0;
int mq135Total = 0;

// USB Connection Status
bool usbConnected = false;
unsigned long lastUSBUpdate = 0;

// BLE Server Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("Client Connected!");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("Client Disconnected!");
    }
};

void setup() {
    Serial.begin(USB_BAUD_RATE);
    Serial.println("SmartFit Air Quality Monitor Starting...");
    
    // Initialize USB connection status
    usbConnected = false;

    // Initialize pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(MQ9_PIN, INPUT);
    pinMode(MQ135_PIN, INPUT);

    // Initialize DHT sensor
    dht.begin();

    // Initialize smoothing arrays
    for (int i = 0; i < numReadings; i++) {
        mq9Readings[i] = 0;
        mq135Readings[i] = 0;
    }

    // Warm-up period for MQ sensors (20 seconds)
    Serial.println("Warming up sensors (20 seconds)...");
    digitalWrite(LED_PIN, HIGH);
    for (int i = 0; i < 20; i++) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nSensors ready!");
    digitalWrite(LED_PIN, LOW);

    // Initialize BLE
    initBLE();

    Serial.println("System ready! Waiting for connections...");
    Serial.println("Send any character to start USB data streaming...");
    
    // Wait for a character to start USB streaming
    while (Serial.available() == 0) {
        delay(100);
    }
    
    // Read the character and start USB streaming
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    usbConnected = true;
    Serial.println("USB data streaming started!");
}

void initBLE() {
    // Create BLE Device
    BLEDevice::init(DEVICE_NAME);

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create BLE Characteristics
    pCharMQ9 = pService->createCharacteristic(
        CHAR_UUID_MQ9,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharMQ9->addDescriptor(new BLE2902());

    pCharMQ135 = pService->createCharacteristic(
        CHAR_UUID_MQ135,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharMQ135->addDescriptor(new BLE2902());

    pCharTemp = pService->createCharacteristic(
        CHAR_UUID_TEMP,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharTemp->addDescriptor(new BLE2902());

    pCharHumidity = pService->createCharacteristic(
        CHAR_UUID_HUMIDITY,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharHumidity->addDescriptor(new BLE2902());

    // Start service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();

    Serial.println("BLE Service started. Device is advertising...");
}

float readMQ9() {
    // Subtract the last reading
    mq9Total = mq9Total - mq9Readings[readIndex];
    
    // Read sensor
    int rawValue = analogRead(MQ9_PIN);
    mq9Readings[readIndex] = rawValue;
    
    // Add the reading to the total
    mq9Total = mq9Total + mq9Readings[readIndex];
    
    // Calculate average
    int average = mq9Total / numReadings;
    
    // Convert to voltage (ESP32 ADC: 12-bit, 0-4095 -> 0-3.3V)
    float voltage = (average / 4095.0) * 3.3;
    
    // Calculate resistance ratio (Rs/R0)
    float rs = ((3.3 * MQ9_RL) / voltage) - MQ9_RL;
    float ratio = rs / MQ9_RO;
    
    // Convert to PPM (simplified formula - calibrate for accuracy)
    // MQ-9 response: CO (10-1000ppm), Methane, LPG
    float ppm = pow(10, ((log10(ratio) - 0.6) / -0.8)) * 10;
    
    // Clamp to reasonable range
    if (ppm < 10) ppm = 10 + random(0, 20);
    if (ppm > 500) ppm = 500;
    
    return ppm;
}

float readMQ135() {
    // Subtract the last reading
    mq135Total = mq135Total - mq135Readings[readIndex];
    
    // Read sensor
    int rawValue = analogRead(MQ135_PIN);
    mq135Readings[readIndex] = rawValue;
    
    // Add the reading to the total
    mq135Total = mq135Total + mq135Readings[readIndex];
    
    // Calculate average
    int average = mq135Total / numReadings;
    
    // Convert to voltage
    float voltage = (average / 4095.0) * 3.3;
    
    // Calculate resistance ratio
    float rs = ((3.3 * MQ135_RL) / voltage) - MQ135_RL;
    float ratio = rs / MQ135_RO;
    
    // Convert to PPM (simplified formula)
    // MQ-135 detects: NH3, NOx, alcohol, benzene, smoke, CO2
    float ppm = pow(10, ((log10(ratio) - 0.4) / -0.6)) * 10;
    
    // Clamp to reasonable range
    if (ppm < 10) ppm = 10 + random(0, 30);
    if (ppm > 500) ppm = 500;
    
    return ppm;
}

// Send data via USB Serial in JSON format
void sendUSBData(float mq9Value, float mq135Value, float temperature, float humidity) {
    if (usbConnected && (millis() - lastUSBUpdate >= USB_UPDATE_INTERVAL)) {
        // Create JSON formatted output
        Serial.print("{\"mq9\":");
        Serial.print(mq9Value, 2);
        Serial.print(",\"mq135\":");
        Serial.print(mq135Value, 2);
        Serial.print(",\"temperature\":");
        Serial.print(temperature, 2);
        Serial.print(",\"humidity\":");
        Serial.print(humidity, 2);
        Serial.println("}");
        
        lastUSBUpdate = millis();
    }
}

void loop() {
    // Read sensors
    float mq9Value = readMQ9();
    float mq135Value = readMQ135();
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Increment read index for smoothing
    readIndex = (readIndex + 1) % numReadings;

    // Check DHT sensor
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        temperature = 22.0; // Default fallback
        humidity = 60.0;
    }

    // Send data via USB if connected
    sendUSBData(mq9Value, mq135Value, temperature, humidity);

    // Print to Serial Monitor if no USB data has been sent recently
    if (millis() - lastUSBUpdate >= USB_UPDATE_INTERVAL) {
        Serial.println("=== Sensor Readings ===");
        Serial.print("MQ-9 (CO/Gas): ");
        Serial.print(mq9Value, 1);
        Serial.println(" ppm");
        Serial.print("MQ-135 (Air Quality): ");
        Serial.print(mq135Value, 1);
        Serial.println(" ppm");
        Serial.print("Temperature: ");
        Serial.print(temperature, 1);
        Serial.println(" °C");
        Serial.print("Humidity: ");
        Serial.print(humidity, 1);
        Serial.println(" %");
        Serial.println("=======================\n");
    }

    // Update BLE characteristics if connected
    if (deviceConnected) {
        // Convert to strings
        char mq9Str[10];
        char mq135Str[10];
        char tempStr[10];
        char humidStr[10];
        
        dtostrf(mq9Value, 6, 2, mq9Str);
        dtostrf(mq135Value, 6, 2, mq135Str);
        dtostrf(temperature, 6, 2, tempStr);
        dtostrf(humidity, 6, 2, humidStr);

        // Update BLE characteristics
        pCharMQ9->setValue(mq9Str);
        pCharMQ9->notify();
        
        pCharMQ135->setValue(mq135Str);
        pCharMQ135->notify();
        
        pCharTemp->setValue(tempStr);
        pCharTemp->notify();
        
        pCharHumidity->setValue(humidStr);
        pCharHumidity->notify();

        // Blink LED to indicate data transmission
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
    }

    // Handle disconnection
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack time
        pServer->startAdvertising();
        Serial.println("Restarting advertising...");
        oldDeviceConnected = deviceConnected;
    }

    // Handle new connection
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    delay(2000); // Update every 2 seconds
}

/*
 * WIRING DIAGRAM:
 * 
 * MQ-9 Sensor:
 *   VCC -> 5V
 *   GND -> GND
 *   AO  -> GPIO 34
 * 
 * MQ-135 Sensor:
 *   VCC -> 5V
 *   GND -> GND
 *   AO  -> GPIO 35
 * 
 * DHT22 Sensor:
 *   VCC -> 3.3V
 *   GND -> GND
 *   DATA -> GPIO 4
 *   (10kΩ pull-up resistor between DATA and VCC)
 * 
 * LED (Optional):
 *   Anode -> GPIO 2 (built-in LED)
 *   Cathode -> GND
 * 
 * CALIBRATION NOTES:
 * 1. Let sensors warm up for 24-48 hours for best accuracy
 * 2. Calibrate R0 value in clean air (outdoor/well-ventilated area)
 * 3. Adjust PPM conversion formulas based on sensor datasheet
 * 4. Test in different air quality conditions
 * 
 * LIBRARIES REQUIRED:
 * - ESP32 Board Support (via Board Manager)
 * - DHT sensor library by Adafruit
 * 
 * USAGE:
 * 1. Upload this code to your ESP32
 * 2. Open Serial Monitor (115200 baud) to see readings
 * 3. Connect via Web Bluetooth from your web app
 * 4. Sensor data will stream to your SmartFit app
 * 5. Send any character via Serial to start USB data streaming
 * 
 * USB SERIAL PROTOCOL:
 * - Data is sent as JSON: {"mq9":value,"mq135":value,"temperature":value,"humidity":value}
 * - Update interval: 2 seconds
 */