#include <Arduino.h>
#include <esp_camera.h>
#include <new_inferencing.h>  // Edge Impulse inference library

#define DEBUG_SERIAL Serial
#define DEVKIT_SERIAL Serial1  

// ESP32-CAM <-> DevKit UART pins
#define CAM_TX_PIN 17  // CAM TX -> DevKit RX2 (GPIO16)
#define CAM_RX_PIN 16  // CAM RX -> DevKit TX2 (GPIO17)

// Camera pin definitions for ESP32-CAM OV2640
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
    // Initialize debug serial for USB monitor
    DEBUG_SERIAL.begin(115200);
    
    // Initialize hardware serial to communicate with DevKit
    DEVKIT_SERIAL.begin(115200, SERIAL_8N1, CAM_RX_PIN, CAM_TX_PIN);

    // Configure camera
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
    config.pixel_format = PIXFORMAT_RGB565; // RGB565 raw format
    config.frame_size = FRAMESIZE_QVGA;    // 320x240 for RAM efficiency
    config.jpeg_quality = 12;
    config.fb_count = 1; // single framebuffer

    // Initialize camera
    if (esp_camera_init(&config) != ESP_OK) {
        DEBUG_SERIAL.println("Camera init failed");
        return;
    }

    DEBUG_SERIAL.println("ESP32-CAM Ready");
}

void loop() {
    // Capture a frame
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        DEBUG_SERIAL.println("Frame buffer failed");
        delay(200);
        return;
    }

    // Determine model input dimensions
    size_t model_w = (defined(EI_CLASSIFIER_INPUT_WIDTH) ? EI_CLASSIFIER_INPUT_WIDTH : fb->width);
    size_t model_h = (defined(EI_CLASSIFIER_INPUT_HEIGHT) ? EI_CLASSIFIER_INPUT_HEIGHT : fb->height);
    size_t model_ch = (defined(EI_CLASSIFIER_INPUT_CHANNELS) ? EI_CLASSIFIER_INPUT_CHANNELS : 1);

    // Total number of samples needed by model
    size_t samples = model_w * model_h * model_ch;

    // Sanity check: avoid huge allocations
    if (samples == 0 || samples > 800000) {
        DEBUG_SERIAL.println("Invalid sample count");
        esp_camera_fb_return(fb);
        delay(200);
        return;
    }

    // Allocate buffer for int8 quantized input
    int8_t *input_buf = (int8_t*)malloc(samples);
    if (!input_buf) {
        DEBUG_SERIAL.println("Failed to allocate input_buf");
        esp_camera_fb_return(fb);
        delay(200);
        return;
    }

    // Convert framebuffer pixels to int8
    // For RGB565, we take the raw byte values and map 0..255 -> -128..127
    size_t to_copy = (fb->len > samples) ? samples : fb->len;
    for (size_t i = 0; i < to_copy; ++i) {
        input_buf[i] = (int8_t)(fb->buf[i] - 128); // simple quantization
    }
    // Zero-pad remaining samples if model input > framebuffer bytes
    for (size_t i = to_copy; i < samples; ++i) {
        input_buf[i] = 0;
    }

    // Wrap input buffer into Edge Impulse signal
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

    // Run the quantized classifier
    ei_impulse_result_t result = {0};
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

    // Cleanup buffer and frame
    free(input_buf);
    esp_camera_fb_return(fb);

    if (res != EI_IMPULSE_OK) {
        DEBUG_SERIAL.println("Classifier error");
        delay(200);
        return;
    }

    // Determine number of labels
    int labels = (defined(EI_CLASSIFIER_LABEL_COUNT) ? EI_CLASSIFIER_LABEL_COUNT : 2);

    // Check open/closed results
    if (labels >= 2) {
        float open_score = result.classification[0].value;
        float closed_score = result.classification[1].value;

        if (closed_score > open_score) {
            DEBUG_SERIAL.println("closed");       // debug
            DEVKIT_SERIAL.println("closed");      // send to DevKit
        } else {
            DEBUG_SERIAL.println("open");
            DEVKIT_SERIAL.println("open");
        }
    } else {
        // For models with different label counts
        for (int i = 0; i < labels; ++i) {
            DEBUG_SERIAL.print(i);
            DEBUG_SERIAL.print(": ");
            DEBUG_SERIAL.println(result.classification[i].value);
        }
    }

    // Small delay to avoid flooding
    delay(200);
}
