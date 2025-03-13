#define BUTTON_PIN 2
#define ENA 9  // Speed control
#define IN1 4  // Direction control
#define IN2 5  // Direction control

const unsigned long CONTRACT_TIME = 5000;  // Full contraction time
const unsigned long EXTEND_TIME = 5000;    // Full extension time
const unsigned long BUTTON_DEBOUNCE_TIME = 500;  // Mandatory delay (500ms)

bool moving = false;       // Is the actuator currently moving?
bool contracting = true;   // Start in contract mode
bool stopped = true;       // Start with the actuator stopped
bool contractComplete = false;  // Tracks if contraction has been completed

unsigned long moveStartTime = 0;
unsigned long lastButtonPressTime = 0;

unsigned long contractTime = 0;  // Track time spent contracting
unsigned long extendTime = 0;    // Track time spent extending

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT_PULLUP);  
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 255);  // Set max speed for the motor

    Serial.println("System Ready. Waiting for button press...");
}

void loop() {
    static bool lastButtonState = HIGH;
    bool buttonState = digitalRead(BUTTON_PIN);
    unsigned long currentTime = millis();

    // Check if button is pressed with debounce
    if (buttonState == LOW && lastButtonState == HIGH && (currentTime - lastButtonPressTime >= BUTTON_DEBOUNCE_TIME)) {  
        lastButtonPressTime = currentTime;  // Update last press time
        Serial.println("Button Pressed!");

        if (moving) {  
            // If actuator is moving, stop it
            stopActuator();
            Serial.println("Actuator Stopped.");
            Serial.print("Contracted for: ");
            Serial.print(contractTime);
            Serial.println(" ms");
            Serial.print("Extended for: ");
            Serial.print(extendTime);
            Serial.println(" ms");
            stopped = true;
            moving = false;
        } else if (stopped) {
            // If actuator is stopped, decide next action:
            if (contractComplete || contractTime >= EXTEND_TIME) {  
                // If it was previously contracted, or contract time exceeds extend time, now extend
                Serial.println("Extending Actuator...");
                moveActuator(false);
                moveStartTime = millis();
                moving = true;
                stopped = false;
                contracting = false;
                contractComplete = false;  // Reset contraction flag
            } else {  
                // Otherwise, start contracting
                Serial.println("Actuator Contracting...");
                // Calculate contraction time based on how much time it has already extended
                unsigned long remainingContractionTime = EXTEND_TIME - contractTime; // Contract for remaining time
                moveActuator(true);
                moveStartTime = millis();
                moving = true;
                stopped = false;
                contracting = true;  // Set contracting flag
                contractTime = 0;    // Reset contractTime for the next cycle
            }
        }
    }

    lastButtonState = buttonState;

    // Check if contraction is complete
    if (moving && contracting) {
        unsigned long elapsedTime = millis() - moveStartTime;
        contractTime += elapsedTime;  // Accumulate contraction time
        if (contractTime >= CONTRACT_TIME || contractTime >= EXTEND_TIME) {
            Serial.println("Fully Contracted.");
            stopActuator();
            Serial.print("Contracted for: ");
            Serial.print(contractTime);
            Serial.println(" ms");
            moving = false;
            stopped = true;
            contractComplete = true;  // Mark contraction as complete
            contractTime = 0;  // Reset contraction time after full contraction
        }
    }

    // Check if extension is complete
    if (moving && !contracting) {
        unsigned long elapsedTime = millis() - moveStartTime;
        extendTime = elapsedTime;  // Update extension time
        if (elapsedTime >= EXTEND_TIME) {
            Serial.println("Fully Extended. Stopping Actuator.");
            stopActuator();
            Serial.print("Contracted for: ");
            Serial.print(contractTime);  // This should be 0 if contracted is complete
            Serial.println(" ms");
            Serial.print("Extended for: ");
            Serial.print(extendTime);
            Serial.println(" ms");
            moving = false;
            stopped = true;
            contracting = true;  // Reset for next cycle
        }
    }
}

void moveActuator(bool contract) {
    if (contract) {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);  // Contracting
    } else {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);  // Extending
    }
}

void stopActuator() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
}
