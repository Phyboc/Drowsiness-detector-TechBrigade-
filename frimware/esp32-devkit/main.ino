#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#define BLYNK_TEMPLATE_ID "TMPL3Hy5PI7Ve"
#define BLYNK_TEMPLATE_NAME "Drowsiness Detection System"

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
        Blynk.virtualWrite(V1, "Eyes Closed");
        Blynk.setProperty(V1, "color", "#FFA500");
        Blynk.setProperty(V1, "label", "Closed");
      } else if (millis() - closedStart > 2000) {
        digitalWrite(BUZZER_PIN, HIGH);
        Blynk.virtualWrite(V1, "DROWSINESS ALERT!");
        Blynk.setProperty(V1, "color", "#FF0000");
        Blynk.setProperty(V1, "label", "DROWSY");
      } else {
        Blynk.virtualWrite(V1, "Eyes Closed");
        Blynk.setProperty(V1, "color", "#FFA500");
        Blynk.setProperty(V1, "label", "Closed");
      }
    } else {
      closedStart = 0;
      digitalWrite(BUZZER_PIN, LOW);
      Blynk.virtualWrite(V1, "Eyes Open");
      Blynk.setProperty(V1, "color", "#00FF00");
      Blynk.setProperty(V1, "label", "Open");
    }
  }
}
