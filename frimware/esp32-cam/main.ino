#include <Arduino.h>
#include <esp_camera.h>
#include <edge-impulse-sdk/classifier/ei_run_classifier.h>

#define DEBUG_SERIAL Serial
#define DEVKIT_SERIAL Serial1  

#define CAM_TX_PIN 14  // ESP32-CAM TX to DevKit RX
#define CAM_RX_PIN 15  // ESP32-CAM RX to DevKit TX

#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

void setup() {
  DEBUG_SERIAL.begin(115200);
  DEVKIT_SERIAL.begin(115200, SERIAL_8N1, CAM_RX_PIN, CAM_TX_PIN);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    DEBUG_SERIAL.println("Camera init failed");
    return;
  }

  DEBUG_SERIAL.println("ESP32-CAM Ready");
}

void loop() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    DEBUG_SERIAL.println("Frame buffer failed");
    return;
  }

  signal_t signal;
  numpy::signal_from_buffer(fb->buf, fb->len, &signal);

  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

  esp_camera_fb_return(fb);

  if (res != EI_IMPULSE_OK) {
    DEBUG_SERIAL.println("Classifier error");
    return;
  }

  float open_score   = result.classification[0].value;
  float closed_score = result.classification[1].value;

  if (closed_score > open_score) {
    DEBUG_SERIAL.println("closed");
    DEVKIT_SERIAL.println("closed");
  } else {
    DEBUG_SERIAL.println("open");
    DEVKIT_SERIAL.println("open");
  }

  delay(200); 
}
