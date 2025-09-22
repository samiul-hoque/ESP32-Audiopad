#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "config.h"

class ButtonManager {
private:
    bool lastButtonState[NUM_BUTTONS];
    bool currentButtonState[NUM_BUTTONS];
    unsigned long lastDebounceTime[NUM_BUTTONS];
    
public:
    ButtonManager();
    void init();
    void checkButtons();
    
    // Callback function pointer for button press events
    void (*onButtonPressed)(int buttonNum) = nullptr;
};

// Implementation
ButtonManager::ButtonManager() {
    // Initialize arrays
    for(int i = 0; i < NUM_BUTTONS; i++) {
        lastButtonState[i] = HIGH;
        currentButtonState[i] = HIGH;
        lastDebounceTime[i] = 0;
    }
}

void ButtonManager::init() {
    // Initialize buttons (using external pull-up resistors)
    for(int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(BUTTON_PINS[i], INPUT);
    }
}

void ButtonManager::checkButtons() {
    // Check each button
    for (int i = 0; i < NUM_BUTTONS; i++) {
        // Read current button state
        bool reading = digitalRead(BUTTON_PINS[i]);
        
        // Check if button state has changed (for debouncing)
        if (reading != lastButtonState[i]) {
            lastDebounceTime[i] = millis(); // Reset debounce timer
        }
        
        // Check if enough time has passed since last state change
        if ((millis() - lastDebounceTime[i]) > DEBOUNCE_DELAY) {
            // If the reading is different from current state
            if (reading != currentButtonState[i]) {
                currentButtonState[i] = reading;
                
                // Button pressed (LOW because button connects to ground)
                if (currentButtonState[i] == LOW) {
                    Serial.print("✓ BUTTON ");
                    Serial.print(i + 1);
                    Serial.print(" PRESSED  -> GPIO ");
                    Serial.print(BUTTON_PINS[i]);
                    Serial.println(" = LOW");
                    
                    // Call callback if set
                    if (onButtonPressed != nullptr) {
                        onButtonPressed(i + 1);
                    }
                }
                // Button released (HIGH because of pull-up resistor)
                else {
                    Serial.print("✗ BUTTON ");
                    Serial.print(i + 1);
                    Serial.print(" RELEASED -> GPIO ");
                    Serial.print(BUTTON_PINS[i]);
                    Serial.println(" = HIGH");
                }
            }
        }
        
        // Save the current reading for next loop iteration
        lastButtonState[i] = reading;
    }
}

#endif