/**
 * @file config.h
 * @brief System Pinouts, Thresholds, and IoT Configuration for Gas Leakage Detection System
 * @author Md. Al-Imran
 * @conference IEEE CS BDC Summer Symposium 2025
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. BLYNK IOT CONFIGURATION
// ==========================================
// Place your Blynk Cloud Template credentials here (from Blynk Console)
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxx"
#define BLYNK_TEMPLATE_NAME "Gas Leakage Detection System"
#define BLYNK_AUTH_TOKEN    "Your_Blynk_Auth_Token"

// Wi-Fi Access Point Credentials
#define WIFI_SSID           "Your_WiFi_SSID"
#define WIFI_PASS           "Your_WiFi_Password"

// ==========================================
// 2. HARDWARE PIN DEFINITIONS (NodeMCU ESP8266)
// ==========================================
#define PIN_MQ6_ANALOG      A0    // MQ-6 Gas Sensor Analog Output (0 - 3.3V via voltage divider)
#define PIN_BUZZER          D1    // GPIO 5: Active Piezo Buzzer
#define PIN_LED_HAZARD      D2    // GPIO 4: Red Warning LED (Leakage Alert)
#define PIN_LED_SAFE        D3    // GPIO 0: White / Green Status LED (Normal State)

// Virtual Pin Mapping for Blynk Dashboard
#define VPIN_GAS_LEVEL      V0    // Gauge / Value Display: Gas Concentration (PPM)
#define VPIN_STATUS_TEXT    V1    // Labeled Value: Status ("SAFE" or "GAS LEAK DETECTED")
#define VPIN_SYSTEM_SWITCH  V2    // Button Widget: Remote System Arm / Disarm

// ==========================================
// 3. THRESHOLD & TIMING CONSTANTS
// ==========================================
// Predefined Safety Threshold from Paper Evaluation (600 PPM)
#define THRESHOLD_PPM       600

// Sensor Sampling Interval (1 second timer routine)
#define SENSOR_POLL_INTERVAL_MS 1000

// Alert Cooldown: Prevents notification flooding (30 seconds as specified in paper)
#define ALERT_COOLDOWN_MS       30000

// ADC Conversion & Calibration Parameters
#define ADC_MAX_VALUE       1023.0f
#define V_REF               3.3f    // NodeMCU ADC internal reference voltage

#endif // CONFIG_H
