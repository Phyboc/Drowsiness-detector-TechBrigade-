# 📄 **README for `AI/`**

# AI / Machine Learning

This folder contains the AI components of the **Drowsiness Detector** project.  
We used **Edge Impulse** to train a lightweight model that detects **eye blinking (open vs closed eyes)** using images from ESP32-CAM.

## Contents

- `exported-model/` – The Edge Impulse exported model (TFLite format)
- `dataset/` – Dataset containing eye images for training

## Classes

- **Open Eyes**  
- **Closed Eyes**

## Model Details

- **Framework**: Edge Impulse
- **Input**: 96x96 image (grayscale or RGB) from ESP32-CAM
- **Output**: Binary classification → `Open` or `Closed`
- **Format**: TensorFlow Lite (float32) for ESP32 deployment

## Deployment

1. Train model in Edge Impulse (already done)
2. Export TFLite model → placed in `exported-model/`
3. Deploy onto ESP32-CAM via Arduino IDE + Edge Impulse SDK
4. ESP32 runs inference and signals ESP32-DevKit to trigger buzzer if eyes remain closed >2s

## Notes

- **Head tilting and yawning detection was planned but not implemented in this version**
- Current AI only detects **eye blinking / closed eyes**
- Future improvement: Add head pose and yawn estimation model
