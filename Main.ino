#define BUTTON_PIN 2
#define ENA 9  // Speed control
#define IN1 4  // Direction control
#define IN2 5  // Direction control

const unsigned long CONTRACT_TIME = 3500;  // Full contraction time
const unsigned long EXTEND_TIME = 3500;    // Full extension time
const unsigned long BUTTON_DEBOUNCE_TIME = 700;  // Increased debounce time to 700ms

bool moving = false;
bool contracting = true;
bool stopped = true;
bool contractComplete = false;
bool stoppedWhileExtending = false; // Tracks if actuator was stopped while extending

unsigned long moveStartTime = 0;
unsigned long lastButtonPressTime = 0;

unsigned long contractTime = 0;
unsigned long extendTime = EXTEND_TIME;

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT_PULLUP);  
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 255);

    Serial.println("System Ready. Waiting for button press...");
}

void loop() {
    static bool lastButtonState = HIGH;
    bool buttonState = digitalRead(BUTTON_PIN);
    unsigned long currentTime = millis();

    // Software debounce with additional stability check
    if (buttonState == LOW && lastButtonState == HIGH) {
        delay(50);  // Short delay to confirm it's a real press
        if (digitalRead(BUTTON_PIN) == LOW && (currentTime - lastButtonPressTime >= BUTTON_DEBOUNCE_TIME)) {  
            lastButtonPressTime = currentTime;
            Serial.println("Button Pressed!");

            if (moving) {  
                stopActuator();
                Serial.println("Actuator Stopped.");
                stopped = true;
                moving = false;
                if (contracting) {
                    contractTime += millis() - moveStartTime;  // Accumulate contract time when stopped mid-contraction
                }
                if (!contracting) {
                    stoppedWhileExtending = true;  // Mark that it was stopped while extending
                }
            } else if (stopped) {
                if (stoppedWhileExtending) {
                    Serial.println("Actuator Contracting (Forced due to safety)...");
                    moveActuator(true);
                    moveStartTime = millis();
                    moving = true;
                    stopped = false;
                    contracting = true;
                    stoppedWhileExtending = false;  // Reset safety flag
                } else if (contractTime >= extendTime) {  
                    Serial.println("Extending Actuator...");
                    moveActuator(false);
                    moveStartTime = millis();
                    moving = true;
                    stopped = false;
                    contracting = false;
                    contractComplete = false;
                } else {  
                    Serial.println("Actuator Contracting...");
                    moveActuator(true);
                    moveStartTime = millis();
                    moving = true;
                    stopped = false;
                    contracting = true;
                }
            }
        }
    }
    lastButtonState = buttonState;

    if (moving && contracting) {
        unsigned long elapsedTime = millis() - moveStartTime;
        if (elapsedTime >= CONTRACT_TIME) {
            contractTime += elapsedTime;
            Serial.println("Fully Contracted.");
            stopActuator();
            moving = false;
            stopped = true;
            contractComplete = true;
            extendTime = contractTime;
        }
    }

    if (moving && !contracting) {
        unsigned long elapsedTime = millis() - moveStartTime;
        if (elapsedTime >= EXTEND_TIME) {
            Serial.println("Fully Extended. Stopping Actuator.");
            stopActuator();
            moving = true;
            stopped = true;
            contracting = true;
            contractTime = 0;
        }
    }
}

void moveActuator(bool contract) {
    if (contract) {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
    } else {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
    }
}

void stopActuator() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
}
