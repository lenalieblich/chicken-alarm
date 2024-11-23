#include <Grove_I2C_Motor_Driver.h>

#define I2C_ADDRESS 0x0f  // Default I2C address of the motor driver
#define SPEAKER_PIN 8     // Pin for the speaker (used for tone function)

const int SWITCH_PIN = 13;
const int dtPin = 12;
const int clkPin = 11;

// Schritte pro Umdrehung des Drehgebers
const int stepsPerRevolution = 96;  

// Variablen zum Speichern des Zustands
int currentCLK;
int lastCLK;
int position = 0; // Aktuelle Position in Schritten
bool direction;   // Drehrichtung: true = Uhrzeigersinn, false = Gegen-Uhrzeigersinn

// Schritte und Zeitkonfiguration
const int motorStepsPerRevolution = 512; // Schritte pro Umdrehung des Steppers
// const long totalMillisIn24Hours = 24L * 60L * 60L * 1000L; // 24 Stunden in Millisekunden
const long totalMillisIn24Hours = 0.1L * 60L * 60L * 1000L; // 24 Stunden in Millisekunden
const long stepDelay = totalMillisIn24Hours / stepsPerRevolution; // Verzögerung pro Schritt

// Variablen zur Position
int currentStep = 0; // Aktuelle Position in Steps

// Variablen für die Zeiten
int stepperHour = 0;
int stepperMinute = 0;
int sensorHour = 0;
int sensorMinute = 0;

// Non-blocking timing variables
unsigned long previousMillis = 0; // stores last time stepper was moved
unsigned long currentMillis = 0;

void setup() {
  Serial.begin(9600);
  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT); // Set up the speaker pin

  Motor.begin(I2C_ADDRESS); // Initialize motor driver
  lastCLK = digitalRead(clkPin); // Initialize last state of CLK
}

void loop() {
  // Get the current time in milliseconds
  currentMillis = millis();
  
  // Check the rotation angle sensor continuously
  checkRotationAngleSensor();  

  // Only move the stepper motor if enough time has passed (non-blocking)
  if (currentMillis - previousMillis >= stepDelay) {
    previousMillis = currentMillis;  // Save the last time stepper moved
    checkStepperMotor();  // Handle stepper motor movement and time calculation
  }
  
  // Compare times between the stepper motor and the rotation angle sensor
  compareTimes();  
}

void checkStepperMotor() {
  // Move the stepper motor one step forward
  moveStepper(1);

  // Calculate the hours and minutes from the current step of the motor
  float hours = (currentStep * 24.0) / motorStepsPerRevolution;
  stepperHour = int(hours) % 24;
  stepperMinute = int((hours - stepperHour) * 60);

  // Calculate the angle of the motor
  float angle = (360.0 / motorStepsPerRevolution) * currentStep;

  // Trigger beep if the motor reaches -3 degrees
  if (angle <= -3.0) {
    tone(SPEAKER_PIN, 1000, 500);  // Emit a tone at 1000 Hz for 500 ms
    delay(500);  // Delay to allow the beep to sound (this won't block the rest of the loop)
  }
}

void moveStepper(int steps) {
  // Move the motor forward or backward depending on the step sign
  if (steps > 0) {
    Motor.StepperRun(1, 0, steps);  // Forward movement
  } else if (steps < 0) {
    Motor.StepperRun(1, 1, -steps); // Backward movement
  }

  // Update the stepper motor's current position
  currentStep += steps;

  // Ensure the stepper motor's position stays within bounds (0 to stepsPerRevolution)
  if (currentStep < 0) {
    currentStep += motorStepsPerRevolution;
  }
  if (currentStep >= motorStepsPerRevolution) {
    currentStep -= motorStepsPerRevolution;
  }
}

void checkRotationAngleSensor() {
    // Aktuellen Zustand von CLK lesen
  int currentCLK = digitalRead(clkPin);

  // Wenn sich der Zustand von CLK ändert, wird ein Schritt erkannt
  if (currentCLK != lastCLK) {
    // DT lesen, um die Richtung zu bestimmen
    int stateDT = digitalRead(dtPin);

    // Richtung bestimmen
    if (stateDT != currentCLK) {
      position++; // Uhrzeigersinn
    } else {
      position--; // Gegen-Uhrzeigersinn
    }

    // Position in einem Bereich von 0–stepsPerRevolution halten
    if (position < 0) {
      position += stepsPerRevolution;
    }
    if (position >= stepsPerRevolution) {
      position -= stepsPerRevolution;
    }

    // Winkel und Uhrzeit berechnen
    float angle = (360.0 / stepsPerRevolution) * position; // Winkel in Grad
    sensorHour = (position / 4) % 24;                       // Stunde (24-Stunden-Format)
    sensorMinute = (position % 4) * 15;                     // Minute (in Viertelstunden)

    // Uhrzeit und Winkel im seriellen Monitor ausgeben
    Serial.print("Sensor Winkel: ");
    Serial.print(angle);
    Serial.print(" Grad, Sensor Uhrzeit: ");
    if (sensorHour < 10) Serial.print("0"); // Führende Null für Stunden
    Serial.print(sensorHour);
    Serial.print(":");
    if (sensorMinute < 10) Serial.print("0"); // Führende Null für Minuten
    Serial.println(sensorMinute);
  }

  // Letzten Zustand von CLK aktualisieren
  lastCLK = currentCLK;
}

void compareTimes() {
  // Check if the time difference between the stepper and sensor is within ±3 minutes
  int timeDifference = abs((stepperHour * 60 + stepperMinute) - (sensorHour * 60 + sensorMinute));

  if (timeDifference <= 3) {
    // Beep if the difference is within ±3 minutes
    tone(SPEAKER_PIN, 1000, 500);  // Emit a tone at 1000 Hz for 500 ms
    Serial.println("Time difference within ±3 minutes. Beep triggered!");
  }

  // Debug: Print the current times for verification
  Serial.print("Stepper Time: ");
  Serial.print(stepperHour);
  Serial.print(":");
  Serial.print(stepperMinute);
  Serial.print(" - Sensor Time: ");
  Serial.print(sensorHour);
  Serial.print(":");
  Serial.println(sensorMinute);
}
