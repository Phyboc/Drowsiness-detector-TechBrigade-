#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// Blynk project template
#define BLYNK_TEMPLATE_ID   "TMPL3Hy5PI7Ve"
#define BLYNK_TEMPLATE_NAME "Drowsiness Detection System"

// Blynk authentication token
char auth[] = "E3SiPhzjxRyXR0CKaEiY86-cziIlg-iV";
// Wi-Fi credentials
char ssid[] = "YOUR_WIFI";
char pass[] = "YOUR_PASS";

// Buzzer connected to this pin
#define BUZZER_PIN 12

// Hardware Serial interface to ESP32-CAM
HardwareSerial camSerial(2);  
#define RXD2 16  // DevKit RX2 pin connected to CAM TX
#define TXD2 17  // DevKit TX2 pin connected to CAM RX

// Eye state tracking variables
String eyeState = "open";           // Stores current eye state received from CAM
unsigned long closedStart = 0;      // Timestamp when eyes were first detected closed

void setup() {
  // Start USB serial for debugging
  Serial.begin(115200);  

  // Start hardware serial to communicate with ESP32-CAM
  camSerial.begin(115200, SERIAL_8N1, RXD2, TXD2);

  // Setup buzzer pin as output and turn it off initially
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Connect to Blynk server
  Blynk.begin(auth, ssid, pass);

  Serial.println("ESP32-DevKit Ready");
}

void loop() {
  // Keep Blynk connection alive
  Blynk.run();

  // Check if ESP32-CAM sent a new eye state message
  if (camSerial.available()) {
    // Read until newline character
    eyeState = camSerial.readStringUntil('\n');
    eyeState.trim(); // Remove whitespace and newline characters

    if (eyeState == "closed") {
      if (closedStart == 0) {
        // Eyes just closed, store the timestamp
        closedStart = millis();

        // Update Blynk widget V1 to show "Eyes Closed"
        Blynk.virtualWrite(V1, "Eyes Closed");
        Blynk.setProperty(V1, "color", "#FFA500"); // Orange color
        Blynk.setProperty(V1, "label", "Closed");
      } 
      else if (millis() - closedStart > 2000) {
        // Eyes have been closed for >2 seconds → Drowsiness detected
        digitalWrite(BUZZER_PIN, HIGH);              // Turn buzzer ON
        Blynk.virtualWrite(V1, "DROWSINESS ALERT!"); // Update Blynk label
        Blynk.setProperty(V1, "color", "#FF0000");   // Red color
        Blynk.setProperty(V1, "label", "DROWSY");
      } 
      else {
        // Eyes closed but less than 2 seconds
        Blynk.virtualWrite(V1, "Eyes Closed");
        Blynk.setProperty(V1, "color", "#FFA500"); // Orange color
        Blynk.setProperty(V1, "label", "Closed");
      }
    } 
    else {
      // Eyes open → reset everything
      closedStart = 0;
      digitalWrite(BUZZER_PIN, LOW); // Turn buzzer OFF
      Blynk.virtualWrite(V1, "Eyes Open");  
      Blynk.setProperty(V1, "color", "#00FF00"); // Green color
      Blynk.setProperty(V1, "label", "Open");
    }
  }
}
