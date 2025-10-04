#include <Arduino.h>
#include <esp_camera.h>
#include <new_inferencing.h>

#define DEBUG_SERIAL Serial
#define DEVKIT_SERIAL Serial1  

#define CAM_TX_PIN 17  // ESP32-CAM TX -> DevKit RX2 (GPIO16)
#define CAM_RX_PIN 16  // ESP32-CAM RX -> DevKit TX2 (GPIO17)


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
    delay(200);
    return;
  }

  // Determine model input shape (use macros if available, else fallback)
  size_t model_w = 0, model_h = 0, model_ch = 0;

  #ifdef EI_CLASSIFIER_INPUT_WIDTH
    model_w = EI_CLASSIFIER_INPUT_WIDTH;
  #endif
  #ifdef EI_CLASSIFIER_INPUT_HEIGHT
    model_h = EI_CLASSIFIER_INPUT_HEIGHT;
  #endif

  // try common channel macros (several SDKs use different names)
  #if defined(EI_CLASSIFIER_INPUT_CHANNELS)
    model_ch = EI_CLASSIFIER_INPUT_CHANNELS;
  #elif defined(EI_CLASSIFIER_INPUT_FRAME_SIZE)
    model_ch = EI_CLASSIFIER_INPUT_FRAME_SIZE;
  #elif defined(EI_CLASSIFIER_INPUT_FRAMES)
    model_ch = EI_CLASSIFIER_INPUT_FRAMES;
  #endif

  // sensible fallbacks if macros not provided
  if (model_w == 0) model_w = fb->width;   // fallback to framebuffer width
  if (model_h == 0) model_h = fb->height;  // fallback to framebuffer height
  if (model_ch == 0) model_ch = 1;         // assume 1 channel if unknown

  // total float samples required by the model
  size_t samples = model_w * model_h * model_ch;

  // sanity check to avoid insane allocations
  if (samples == 0 || samples > 800000) {
    DEBUG_SERIAL.print("Bad sample count: ");
    DEBUG_SERIAL.println(samples);
    esp_camera_fb_return(fb);
    delay(200);
    return;
  }

  // allocate float buffer dynamically
  float input_buf = (float)malloc(sizeof(float) * samples);
  if (!input_buf) {
    DEBUG_SERIAL.println("Failed to allocate input_buf");
    esp_camera_fb_return(fb);
    delay(200);
    return;
  }

  // Fill input_buf from fb->buf (uint8_t*). Behavior:
  // - If fb->len >= samples, copy first 'samples' bytes -> floats.
  // - If fb->len < samples, copy all bytes then zero-pad the rest.
  // NOTE: This is a generic fallback. If your model expects pixels or
  // a specific layout, replace this with the correct conversion/resizing.
  size_t bytes_available = fb->len;
  size_t to_copy = (bytes_available > samples) ? samples : bytes_available;

  for (size_t i = 0; i < to_copy; ++i) {
    input_buf[i] = (float)fb->buf[i];
  }
  // zero-pad remainder if necessary
  for (size_t i = to_copy; i < samples; ++i) {
    input_buf[i] = 0.0f;
  }

  // Build signal and run classifier
  signal_t signal;
  int sig_res = numpy::signal_from_buffer(input_buf, samples, &signal);
  if (sig_res != 0) {
    DEBUG_SERIAL.print("signal_from_buffer failed: ");
    DEBUG_SERIAL.println(sig_res);
    free(input_buf);
    esp_camera_fb_return(fb);
    delay(200);
    return;
  }

  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

  // cleanup
  free(input_buf);
  esp_camera_fb_return(fb);

  if (res != EI_IMPULSE_OK) {
    DEBUG_SERIAL.println("Classifier error");
    delay(200);
    return;
  }

  // Count labels in a portable way
  int labels = 0;
  #ifdef EI_CLASSIFIER_LABEL_COUNT
    labels = EI_CLASSIFIER_LABEL_COUNT;
  #else
    labels = sizeof(result.classification) / sizeof(result.classification[0]);
  #endif

  if (labels >= 2) {
    float open_score   = result.classification[0].value;
    float closed_score = result.classification[1].value;

    if (closed_score > open_score) {
      DEBUG_SERIAL.println("closed");
      DEVKIT_SERIAL.println("closed");
    } else {
      DEBUG_SERIAL.println("open");
      DEVKIT_SERIAL.println("open");
    }
  } else {
    DEBUG_SERIAL.println("Result values:");
    for (int i = 0; i < labels; ++i) {
      DEBUG_SERIAL.print(i);
      DEBUG_SERIAL.print(": ");
      DEBUG_SERIAL.println(result.classification[i].value);
      // if labels have string names available (some SDKs include them)
      #ifdef EI_CLASSIFIER_LABELS
        // Some SDKs provide EI_CLASSIFIER_LABELS macro or similar, but it's not standard.
      #endif
    }
  }

  delay(200);
}
