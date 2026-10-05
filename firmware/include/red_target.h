#pragma once
#include <stddef.h>
#include <stdint.h>

struct RedTarget {
  bool found;
  float x, y;
  uint32_t pixels; // All red samples, including subthreshold components.
  uint16_t candidates;
  uint32_t selectedPixels;
};

// One workspace per concurrent caller. Capture owns static storage, not its
// 4096-byte task stack. No heap allocation/recursion; capacity is QQVGA only.
struct RedTargetWorkspace {
  static constexpr size_t kSamples = 80 * 60;
  uint8_t red[kSamples];
  uint16_t queue[kSamples];
};

// MSB-first RGB565, sampled every second row/column. A marker candidate is a
// four-connected component of >=20 samples. Exactly one candidate is required.
// This is spatial ambiguity rejection, NOT identity tracking or person detection.
inline RedTarget findRedTarget(const uint8_t* data, size_t length,
    uint16_t width, uint16_t height, RedTargetWorkspace& work) {
  RedTarget result{false, 0, 0, 0, 0, 0};
  if (!data || width < 2 || height < 2 || width > 160 || height > 120 ||
      length != size_t(width) * height * 2) return result;
  const uint16_t columns = (width + 1) / 2, rows = (height + 1) / 2;
  const uint16_t samples = columns * rows;
  for (uint16_t cell = 0; cell < samples; ++cell) {
    const size_t x = (cell % columns) * 2, y = (cell / columns) * 2;
    const size_t i = (y * width + x) * 2;
    const uint16_t p = (uint16_t(data[i]) << 8) | data[i + 1];
    const int r = ((p >> 11) & 31) * 255 / 31;
    const int g = ((p >> 5) & 63) * 255 / 63;
    const int b = (p & 31) * 255 / 31;
    work.red[cell] = r > 100 && r > g * 3 / 2 && r > b * 3 / 2;
    result.pixels += work.red[cell];
  }
  for (uint16_t seed = 0; seed < samples; ++seed) {
    if (!work.red[seed]) continue;
    uint16_t head = 0, tail = 0;
    uint32_t sx = 0, sy = 0;
    auto enqueue = [&](uint16_t cell) {
      if (work.red[cell]) {
        work.red[cell] = 0; // Mark when queued: every cell enters at most once.
        work.queue[tail++] = cell;
      }
    };
    enqueue(seed);
    while (head < tail) {
      const uint16_t cell = work.queue[head++];
      const uint16_t x = cell % columns, y = cell / columns;
      sx += x * 2; sy += y * 2;
      if (x) enqueue(cell - 1);
      if (x + 1 < columns) enqueue(cell + 1);
      if (y) enqueue(cell - columns);
      if (y + 1 < rows) enqueue(cell + columns);
    }
    if (tail >= 20) {
      ++result.candidates;
      if (result.candidates == 1) {
        result.x = 2.0F * sx / tail / (width - 1) - 1.0F;
        result.y = 2.0F * sy / tail / (height - 1) - 1.0F;
        result.selectedPixels = tail;
      }
    }
  }
  result.found = result.candidates == 1;
  if (!result.found) { result.x = 0; result.y = 0; result.selectedPixels = 0; }
  return result;
}
