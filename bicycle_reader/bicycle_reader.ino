#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Volatile variables - only store raw data in ISR
volatile unsigned long lastPulseMicros = 0;
volatile unsigned long currentPulseDelta = 0;
volatile bool newPulseAvailable = false;

// Constants
const float CIRCUM_M = 7;             
const unsigned long DEBOUNCE_US = 2000; 
const float MAX_SPEED = 60.0;           
const unsigned long TIMEOUT_MS = 2000;  

// Non-volatile variables for main loop
float currentSpeed = 0.0;
float smoothSpeed = 0.0;
unsigned long lastPulseMillis = 0;

// Safe ISR - minimal processing, no floats!
void IRAM_ATTR pulseISR() {
  unsigned long now = micros();
  unsigned long dt = now - lastPulseMicros;

  // Simple debouncing - only store raw timing data
  if (dt > DEBOUNCE_US) {
    currentPulseDelta = dt;
    lastPulseMicros = now;
    newPulseAvailable = true;
  }
}

void setup() {
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  pinMode(25, INPUT_PULLUP);
  
  if (!SerialBT.begin("ESP32_Speedometer")) {
    Serial.println("Bluetooth gagal start!");
    while (true) delay(1000);
  }

  // Small delay before attaching interrupt
  delay(100);
  attachInterrupt(digitalPinToInterrupt(25), pulseISR, FALLING);
  Serial.println(ESP.getChipModel());
  Serial.println("🚴 Safe ESP32 Speedometer Ready!");
  SerialBT.println("🚴 Safe ESP32 Speedometer Ready!");
  // Serial.println("Crash-resistant version");
  // SerialBT.println("Crash-resistant version");
}

void loop() {
  unsigned long now = millis();
  bool hasNewPulse = false;
  unsigned long pulseDelta = 0;
  
  // Safely read ISR data
  noInterrupts();
  if (newPulseAvailable) {
    hasNewPulse = true;
    pulseDelta = currentPulseDelta;
    newPulseAvailable = false;
    lastPulseMillis = now;
  }
  interrupts();
  
  // Process new pulse data (floating point math in main loop only)
  if (hasNewPulse) {
    float mps = CIRCUM_M / (pulseDelta / 1000000.0);
    float newSpeed = mps * 3.6;
    
    // Apply speed limit and validation
    if (newSpeed <= MAX_SPEED && newSpeed > 0.1) {
      currentSpeed = newSpeed;
    }
  }
  
  // Handle timeout (stopped)
  if (now - lastPulseMillis > TIMEOUT_MS) {
    currentSpeed = 0.0;
  }
  
  // Smooth the speed
  smoothSpeed = 0.7 * smoothSpeed + 0.3 * currentSpeed;
  
  // Output (rate limited)
  static unsigned long lastPrint = 0;
  if (now - lastPrint >= 500) {
    lastPrint = now;
    // Serial.printf("Speed: %.2f km/h\n", smoothSpeed);
    String data = String("{\"speed\":") + smoothSpeed + "}";
    SerialBT.println(data);
    Serial.println(data);
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

// Alternative: Even simpler ISR-safe version
// void IRAM_ATTR simplePulseISR() {
//   static unsigned long lastTime = 0;
//   unsigned long now = micros();
  
//   if (now - lastTime > DEBOUNCE_US) {
//     currentPulseDelta = now - lastTime;
//     lastTime = now;
//     newPulseAvailable = true;
//   }
// }