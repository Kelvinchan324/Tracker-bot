#pragma once
#include <stddef.h>
#include <stdint.h>

struct RedTarget { bool found; float x; float y; uint32_t pixels; };

// Camera RGB565 bytes are MSB first. This is a coloured-marker experiment,
// not person recognition; two red objects produce their combined centroid.
inline RedTarget findRedTarget(const uint8_t* data, size_t length,
                              uint16_t width, uint16_t height) {
  if (!data || width < 2 || height < 2 || width > 640 || height > 480 ||
      length != size_t(width) * height * 2)
    return {false, 0, 0, 0};
  uint32_t count = 0, sx = 0, sy = 0;
  for (uint16_t y = 0; y < height; y += 2) {
    for (uint16_t x = 0; x < width; x += 2) {
      size_t i = (size_t(y) * width + x) * 2;
      uint16_t p = (uint16_t(data[i]) << 8) | data[i + 1];
      int r = ((p >> 11) & 31) * 255 / 31;
      int g = ((p >> 5) & 63) * 255 / 63;
      int b = (p & 31) * 255 / 31;
      if (r > 100 && r > g * 3 / 2 && r > b * 3 / 2) {
        ++count; sx += x; sy += y;
      }
    }
  }
  if (count < 20) return {false, 0, 0, count};
  return {true, 2.0F * sx / count / (width - 1) - 1.0F,
          2.0F * sy / count / (height - 1) - 1.0F, count};
}
