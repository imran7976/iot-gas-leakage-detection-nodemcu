/**
 * @file Gas_Leakage_Detection_ESP8266.ino
 * @brief IoT-Based Gas Leakage Detection and Web-Based Alert System using NodeMCU & Blynk
 * @author Md. Al-Imran
 * @details Paper presented at IEEE CS BDC Summer Symposium 2025 (Paper ID: 525)
 * 
 * Hardware Connections:
 *   - NodeMCU (ESP8266-12E)
 *   - MQ-6 LPG Gas Sensor: Analog Out -> Pin A0
 *   - Active Buzzer:      Positive -> Pin D1 (GPIO 5)
 *   - Red Warning LED:    Anode -> Pin D2 (GPIO 4) with 220 ohm / 10k resistor
 *   - Safe Status LED:    Anode -> Pin D3 (GPIO 0) with 220 ohm / 10k resistor
 */

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "config.h"

// Blynk hardware timer
BlynkTimer timer;

// System state tracking
bool isSystemArmed = true;            // Controlled via Blynk virtual switch
unsigned long lastNotificationTime = 0; // Cooldown timer tracking

// Function prototypes
void sampleAndEvaluateGas();
int readGasConcentrationPPM();
void triggerLocalAlarms(bool isHazardous);
void sendRemoteAlerts(int gasPpm);

// ==========================================
// BLYNK VIRTUAL PIN HANDLERS
// ==========================================
// Remote System Arm/Disarm Switch (Virtual Pin V2)
BLYNK_WRITE(VPIN_SYSTEM_SWITCH) {
    int switchState = param.asInt();
    isSystemArmed = (switchState == 1);
    Serial.print(F("[BLYNK] Remote System State Updated: "));
    Serial.println(isSystemArmed ? F("ARMED (Active Monitoring)") : F("DISARMED (Standby)"));

    if (!isSystemArmed) {
        triggerLocalAlarms(false);
        Blynk.virtualWrite(VPIN_STATUS_TEXT, "SYSTEM DISARMED");
    }
}

// Synchronize virtual pins on reconnection
BLYNK_CONNECTED() {
    Blynk.syncVirtual(VPIN_SYSTEM_SWITCH);
}

// ==========================================
// SETUP ROUTINE
// ==========================================
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000);

    Serial.println(F("=================================================="));
    Serial.println(F(" Gas Leakage Detection and Web Alert System"));
    Serial.println(F(" Platform: NodeMCU ESP8266 + MQ-6 Gas Sensor    "));
    Serial.println(F(" Author: Md. Al-Imran | IEEE CS BDC 2025        "));
    Serial.println(F("=================================================="));

    // Configure GPIO modes
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_LED_HAZARD, OUTPUT);
    pinMode(PIN_LED_SAFE, OUTPUT);
    pinMode(PIN_MQ6_ANALOG, INPUT);

    // Initial safe state indicators
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_HAZARD, LOW);
    digitalWrite(PIN_LED_SAFE, HIGH); // Safe indicator active on startup

    // Connect to Wi-Fi and Blynk Cloud
    Serial.println(F("[WIFI] Connecting to Wi-Fi and Blynk Server..."));
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

    // Set recurring 1.0-second gas sampling timer
    timer.setInterval(SENSOR_POLL_INTERVAL_MS, sampleAndEvaluateGas);

    Serial.println(F("[SYSTEM READY] Sensor heater stabilized. Monitoring started..."));
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
    Blynk.run();
    timer.run();
}

// ==========================================
// GAS SAMPLING & HIERARCHICAL EVALUATION
// ==========================================
void sampleAndEvaluateGas() {
    if (!isSystemArmed) {
        return;
    }

    int gasPpm = readGasConcentrationPPM();

    Serial.print(F("[SENSOR] Gas Concentration: "));
    Serial.print(gasPpm);
    Serial.print(F(" PPM | Threshold: "));
    Serial.print(THRESHOLD_PPM);
    Serial.println(F(" PPM"));

    // Push real-time numeric reading to Blynk Dashboard
    Blynk.virtualWrite(VPIN_GAS_LEVEL, gasPpm);

    // Evaluate against empirical safety threshold (600 PPM)
    if (gasPpm >= THRESHOLD_PPM) {
        // Hazardous Gas Leakage Detected!
        Serial.println(F("[ALERT] Gas Leakage Detected! Activating Dual-Mode Alarms..."));
        
        triggerLocalAlarms(true);
        Blynk.virtualWrite(VPIN_STATUS_TEXT, "HAZARDOUS - LEAK DETECTED!");
        
        // Dispatch Remote Push & Email Notification (respecting 30s cooldown)
        sendRemoteAlerts(gasPpm);
    } else {
        // Normal Safe Condition
        triggerLocalAlarms(false);
        Blynk.virtualWrite(VPIN_STATUS_TEXT, "SAFE - NO LEAK DETECTED");
    }
}

// ==========================================
// MQ-6 ADC SAMPLING & PPM CONVERSION
// ==========================================
int readGasConcentrationPPM() {
    // 5-sample temporal smoothing filter to eliminate spurious noise
    long adcSum = 0;
    for (int i = 0; i < 5; i++) {
        adcSum += analogRead(PIN_MQ6_ANALOG);
        delay(10);
    }
    int avgAdc = adcSum / 5;

    // Linear mapping calibrated between clean air baseline (~439 PPM) and LPG saturation
    // Raw NodeMCU ADC ranges 0-1023 (0 - 1.0V internal / 0 - 3.3V board divider)
    int estimatedPpm = map(avgAdc, 100, 900, 200, 1000);
    if (estimatedPpm < 200) estimatedPpm = 200;

    return estimatedPpm;
}

// ==========================================
// LOCAL AUDIOVISUAL ACTUATION
// ==========================================
void triggerLocalAlarms(bool isHazardous) {
    if (isHazardous) {
        digitalWrite(PIN_BUZZER, HIGH);     // Continuous alert tone
        digitalWrite(PIN_LED_HAZARD, HIGH); // Red hazard LED ON
        digitalWrite(PIN_LED_SAFE, LOW);    // White/Safe LED OFF
    } else {
        digitalWrite(PIN_BUZZER, LOW);      // Silence alarm
        digitalWrite(PIN_LED_HAZARD, LOW);  // Red LED OFF
        digitalWrite(PIN_LED_SAFE, HIGH);   // White/Safe LED ON (Indicates power & safety)
    }
}

// ==========================================
// CLOUD NOTIFICATION & EMAIL DISPATCH
// ==========================================
void sendRemoteAlerts(int gasPpm) {
    unsigned long currentMillis = millis();

    // 30-Second Cooldown Logic: Prevents notification spamming
    if (currentMillis - lastNotificationTime >= ALERT_COOLDOWN_MS || lastNotificationTime == 0) {
        lastNotificationTime = currentMillis;

        Serial.println(F("[CLOUD] Triggering Blynk Push Notification & Email Event..."));

        // Trigger predefined Blynk event ("gas_leak") with dynamic PPM reading
        String alertMsg = String("gas_leak\nGas level: ") + String(gasPpm);
        Blynk.logEvent("gas_leak", alertMsg);

        Serial.println(F("[CLOUD] Push notification and email dispatched successfully."));
    } else {
        Serial.print(F("[COOLDOWN] Suppressing repeated notification (Remaining: "));
        Serial.print((ALERT_COOLDOWN_MS - (currentMillis - lastNotificationTime)) / 1000);
        Serial.println(F("s)"));
    }
}
