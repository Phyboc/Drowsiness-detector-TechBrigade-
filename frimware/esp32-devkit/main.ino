#include <WiFi.h>
#include <BlynkSimpleEsp32.h>


char auth[] = "YOUR_BLYNK_TOKEN";
char ssid[] = "YOUR_WIFI";
char pass[] = "YOUR_PASS";

#define BUZZER_PIN 12

String eyeState = "open";
unsigned long closedStart = 0;

void setup() {
  Serial.begin(115200);  // UART from ESP32-CAM
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Blynk.begin(auth, ssid, pass);
}

void loop() {
  Blynk.run();

  if (Serial.available()) {
    eyeState = Serial.readStringUntil('\n');
    eyeState.trim();

    if (eyeState == "closed") {
      if (closedStart == 0) {
        closedStart = millis();
      } else if (millis() - closedStart > 2000) {
        digitalWrite(BUZZER_PIN, HIGH);
      }
    } else {
      closedStart = 0;
      digitalWrite(BUZZER_PIN, LOW);
    }

    Blynk.virtualWrite(V1, eyeState);
  }
}
