/*
 * SmartFit Air Monitor - ESP32 BLE Sensor Node
 * 
 * This firmware reads environmental data from multiple sensors and transmits
 * it via BLE to the SmartFit web application.
 * 
 * Sensors supported:
 * - MQ-9: Carbon Monoxide (CO) sensor
 * - MQ-135: Multi-gas sensor (CO2, NH3, NOx, and other gases)
 * - DHT11/DHT22: Temperature and Humidity sensor
 * 
 * For the MQ-135 sensor, this firmware simulates readings for multiple gases
 * based on the sensor's analog output. In a production environment, you would
 * use separate sensors for each gas type or calibrate the MQ-135 for specific gases.
 * 
 * BLE Characteristics:
 * - CHAR_UUID_MQ9: Sends CO readings as Float32
 * - CHAR_UUID_MQ135: Sends JSON with all gas readings {"co2":value,"other":value}
 * - CHAR_UUID_TEMP: Sends temperature as Float32
 * - CHAR_UUID_HUMIDITY: Sends humidity as Float32
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>

// Pin definitions
#define MQ9_PIN             35
#define MQ135_PIN           34
#define DHT_PIN             4
#define DHT_TYPE            DHT11  // Change to DHT22 if you're using DHT22

// For MQ135 sensor, we'll simulate different gas readings
// In a real implementation, you would have separate sensors or use sensor calibration
#define MQ135_NH3_PIN       39  // Additional pin for NH3 simulation
#define MQ135_NOX_PIN       36  // Additional pin for NOx simulation

// BLE UUIDs (must match your web app config.js)
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_UUID_MQ9       "beb5483e-36e1-4688-b7f5-ea07361b26a8"   // CO
#define CHAR_UUID_MQ135     "beb5483e-36e1-4688-b7f5-ea07361b26a9"   // CO2
#define CHAR_UUID_TEMP      "beb5483e-36e1-4688-b7f5-ea07361b26aa"   // Temperature
#define CHAR_UUID_HUMIDITY  "beb5483e-36e1-4688-b7f5-ea07361b26ab"   // Humidity

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// BLE Server and Characteristics
BLEServer* pServer = NULL;
BLECharacteristic* pCharMQ9 = NULL;
BLECharacteristic* pCharMQ135 = NULL;
BLECharacteristic* pCharTemp = NULL;
BLECharacteristic* pCharHumidity = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// Server callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("✅ Client connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("❌ Client disconnected!");
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("\n🚀 SmartFit Air Monitor - Starting...");
  
  // Initialize DHT sensor
  dht.begin();
  Serial.println("🌡️  DHT sensor initialized");
  
  // Note: For full gas detection, connect additional MQ sensors to pins 39 and 36
  // Or modify the code to simulate different gas values from a single MQ135 sensor
  
  // Initialize BLE
  BLEDevice::init("SmartFit-Air-Monitor");
  
  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create BLE Characteristics
  pCharMQ9 = pService->createCharacteristic(
                      CHAR_UUID_MQ9,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharMQ9->addDescriptor(new BLE2902());
  
  pCharMQ135 = pService->createCharacteristic(
                      CHAR_UUID_MQ135,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharMQ135->addDescriptor(new BLE2902());
  
  pCharTemp = pService->createCharacteristic(
                      CHAR_UUID_TEMP,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharTemp->addDescriptor(new BLE2902());
  
  pCharHumidity = pService->createCharacteristic(
                      CHAR_UUID_HUMIDITY,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharHumidity->addDescriptor(new BLE2902());
  
  // Start the service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  
  Serial.println("📡 BLE Server started - Waiting for connection...");
  Serial.println("Device Name: SmartFit-Air-Monitor");
}

void loop() {
  // Handle connection/disconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("🔄 Restarting advertising...");
    oldDeviceConnected = deviceConnected;
  }
  
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
  
  // Read sensors every 2 seconds
  if (deviceConnected) {
    // Read MQ9 (CO sensor)
    int mq9Raw = analogRead(MQ9_PIN);
    float mq9Value = map(mq9Raw, 0, 4095, 0, 1000);  // Map to ppm range
    
    // Read MQ135 (CO2 sensor) - In this example, we'll simulate multiple gases from one sensor
    // In a real implementation, you would have separate sensors for each gas
    int mq135Raw = analogRead(MQ135_PIN);
    float co2Value = map(mq135Raw, 0, 4095, 400, 2000);  // CO2 ppm range
    
    // Simulate other gases based on MQ135 reading with some variation
    float otherGasesValue = co2Value * 0.15; // 15% of CO2 as other gases
    
    // Read DHT sensor (Temperature and Humidity)
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    // Check if DHT reading failed
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("⚠️  Failed to read from DHT sensor!");
      temperature = 0.0;
      humidity = 0.0;
    }
    
    // Print to Serial Monitor
    Serial.println("\n📊 Sensor Readings:");
    Serial.printf("   MQ9 (CO): %.0f ppm\n", mq9Value);
    Serial.printf("   MQ135 (CO2): %.0f ppm\n", co2Value);
    Serial.printf("   Other Gases: %.0f ppm\n", otherGasesValue);
    Serial.printf("   Temperature: %.1f °C\n", temperature);
    Serial.printf("   Humidity: %.1f %%\n", humidity);
    
    // Send data via BLE
    // For backward compatibility, send CO as Float32
    pCharMQ9->setValue(mq9Value);
    pCharMQ9->notify();
    
    // For MQ135, send JSON data with simplified gas values
    // Format: {"co2":450,"other":67}
    // This allows the web app to display CO2 and other gases readings
    String jsonData = "{\"co2\":" + String(co2Value, 0) + 
                      ",\"other\":" + String(otherGasesValue, 0) + "}";
    
    pCharMQ135->setValue(jsonData.c_str());
    pCharMQ135->notify();
    
    pCharTemp->setValue(temperature);
    pCharTemp->notify();
    
    pCharHumidity->setValue(humidity);
    pCharHumidity->notify();
    
    Serial.println("✅ Data sent via BLE");
  }
  
  delay(2000);  // Update every 2 seconds
}
