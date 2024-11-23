#include <Grove_I2C_Motor_Driver.h>

// default I2C address is 0x0f
#define I2C_ADDRESS 0x0f
const int SWITCH_PIN = 13;
const int dtPin = 12;
const int clkPin = 11;

// Schritte pro Umdrehung des Drehgebers
// Anpassen je nach Drehgeber (z. B. 96 Schritte/Umdrehung -> 24 x 4)
const int stepsPerRevolution = 96; 

// Variablen zum Speichern des Zustands
int currentStateCLK;
int lastStateCLK;
int position = 0; // Aktuelle Position in Schritten
bool direction;   // Drehrichtung: true = Uhrzeigersinn, false = Gegen-Uhrzeigersinn

// Schritte und Zeitkonfiguration
const int motorStepsPerRevolution = 512; // Schritte pro Umdrehung
const long totalMillisIn24Hours = 24L * 60L * 60L * 1000L; // 24 Stunden in Millisekunden -> 168750000
const long stepDelay = totalMillisIn24Hours / stepsPerRevolution; // Verzögerung pro Schritt

// Variablen zur Position
int currentStep = 0; // Aktuelle Position in Steps

void setup() {
    // Serielle Kommunikation starten
  Serial.begin(9600);
  // Pins als Eingänge definieren
  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);
  Motor.begin(I2C_ADDRESS);
  // Initialen Zustand von CLK speichern
  lastStateCLK = digitalRead(clkPin);
}

void loop() {
  checkRotationAngleSensor();
  checkStepperMotor();
}

void checkStepperMotor(){
  moveStepper(1); // Bewege den Motor 1 Schritt vorwärts
    // Aktuelle Uhrzeit basierend auf der Position berechnen
  float hours = (currentStep * 24.0 / motorStepsPerRevolution); // Stunden
  int hour = int(hours) % 24;                 // Ganze Stunden
  int minute = int((hours - hour) * 60);      // Minuten

  // Winkel berechnen
  float angle = (360.0 / motorStepsPerRevolution) * currentStep; // Winkel in Grad

  // Daten ausgeben
  Serial.print("Winkel: ");
  Serial.print(angle, 1);
  Serial.print(" Grad, Uhrzeit: ");
  if (hour < 10) Serial.print("0");
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10) Serial.print("0");
  Serial.println(minute);
  
  delay(stepDelay);
}

// Funktion zum Bewegen des Steppers
void moveStepper(int steps) {
  // Richtung festlegen
  if (steps > 0) {
    Motor.StepperRun(1, 0, steps); // Vorwärts drehen
  } else if (steps < 0) {
    Motor.StepperRun(1, 1, -steps); // Rückwärts drehen
  }
  // Position aktualisieren
  currentStep += steps;

  // Position in einem Bereich von 0 bis stepsPerRevolution halten
  if (currentStep < 0) {
    currentStep += motorStepsPerRevolution;
  }
  if (currentStep >= motorStepsPerRevolution) {
    currentStep -= motorStepsPerRevolution;
  }
}

void checkRotationAngleSensor(){
    // Aktuellen Zustand von CLK lesen
  currentStateCLK = digitalRead(clkPin);

  // Wenn sich der Zustand von CLK ändert, wird ein Schritt erkannt
  if (currentStateCLK != lastStateCLK) {
    // DT lesen, um die Richtung zu bestimmen
    int stateDT = digitalRead(dtPin);

    // Richtung bestimmen
    if (currentStateCLK == HIGH) {
      if (stateDT == LOW) {
        // Drehen im Uhrzeigersinn
        position++;
        direction = true;
      } else {
        // Drehen gegen den Uhrzeigersinn
        position--;
        direction = false;
      }
    }

    // Position in einem Bereich von 0–stepsPerRevolution halten
    if (position < 0) {
      position += stepsPerRevolution;
    }
    if (position >= stepsPerRevolution) {
      position -= stepsPerRevolution;
    }

    // Winkel berechnen
    float angle = (360.0 / stepsPerRevolution) * position;
    int hour = (position / 4) % 24;                       // Stunde (24-Stunden-Format)
    int minute = (position % 4) * 15;                     // Minute (in Viertelstunden)

    // Uhrzeit und Winkel im seriellen Monitor ausgeben
    Serial.print("Winkel: ");
    Serial.print(angle);
    Serial.print(" Grad, Uhrzeit: ");
    if (hour < 10) Serial.print("0"); // Führende Null für Stunden
    Serial.print(hour);
    Serial.print(":");
    if (minute < 10) Serial.print("0"); // Führende Null für Minuten
    Serial.println(minute);
  }

  // Letzten Zustand von CLK aktualisieren
  lastStateCLK = currentStateCLK;
}
