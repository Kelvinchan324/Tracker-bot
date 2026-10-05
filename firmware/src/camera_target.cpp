#include <Arduino.h>
#include "camera_target.h"
#include "camera_frame.h"
#include "app_config.h"
#ifdef TRACKER_XIAO_SENSE
#include "esp_camera.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
namespace {
QueueHandle_t queue = nullptr;
void captureTask(void*) {
  for (;;) {
    camera_fb_t* frame = esp_camera_fb_get();
    CameraTarget target{false, 0, 0, 0, 0, CameraFrameState::NoFrame};
    if (frame) {
      target = processCameraFrame(frame->buf, frame->len, frame->width, frame->height,
          frame->format == PIXFORMAT_RGB565, frame->timestamp.tv_sec,
          frame->timestamp.tv_usec, esp_timer_get_time(), config::kTargetTimeoutMs);
      esp_camera_fb_return(frame);
    }
    xQueueOverwrite(queue, &target);
    vTaskDelay(pdMS_TO_TICKS(80));
  }
}
}
bool startCameraTarget() {
  camera_config_t c{};
  c.ledc_channel = LEDC_CHANNEL_7; c.ledc_timer = LEDC_TIMER_3;
  c.pin_d0 = 15; c.pin_d1 = 17; c.pin_d2 = 18; c.pin_d3 = 16;
  c.pin_d4 = 14; c.pin_d5 = 12; c.pin_d6 = 11; c.pin_d7 = 48;
  c.pin_xclk = 10; c.pin_pclk = 13; c.pin_vsync = 38; c.pin_href = 47;
  c.pin_sccb_sda = 40; c.pin_sccb_scl = 39;
  c.pin_pwdn = -1; c.pin_reset = -1; c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_RGB565; c.frame_size = FRAMESIZE_QQVGA;
  c.jpeg_quality = 12; c.fb_count = 1; c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  if (!psramFound() || esp_camera_init(&c) != ESP_OK) return false;
  queue = xQueueCreate(1, sizeof(CameraTarget));
  if (!queue) { esp_camera_deinit(); return false; }
  if (xTaskCreate(captureTask, "target-camera", 4096, nullptr, 1, nullptr) != pdPASS) {
    vQueueDelete(queue); queue = nullptr; esp_camera_deinit(); return false;
  }
  return true;
}
bool readCameraTarget(CameraTarget& target) {
  return queue && xQueueReceive(queue, &target, 0) == pdTRUE;
}
#else
bool startCameraTarget() { return false; }
bool readCameraTarget(CameraTarget&) { return false; }
#endif
