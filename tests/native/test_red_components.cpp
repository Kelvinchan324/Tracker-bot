#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "camera_frame.h"

struct GuardedWorkspace {
  uint32_t before = 0xABCD1234;
  RedTargetWorkspace work;
  uint32_t after = 0x5678ABCD;
} storage;
static_assert(sizeof(RedTargetWorkspace) == 14400, "Keep capture workspace size reviewed");
std::vector<uint8_t> frame(160 * 120 * 2, 0);
void clear() { std::fill(frame.begin(), frame.end(), 0); }
void sample(unsigned x, unsigned y) { frame[(y * 2 * 160 + x * 2) * 2] = 0xF8; }
void block(unsigned x, unsigned y, unsigned w, unsigned h) {
  for (unsigned j = y; j < y + h; ++j)
    for (unsigned i = x; i < x + w; ++i) sample(i, j);
}
CameraTarget detect() {
  auto target = processCameraFrame(frame.data(), frame.size(), 160, 120,
                                   true, 1, 0, 1100000, 750, storage.work);
  assert(storage.before == 0xABCD1234 && storage.after == 0x5678ABCD);
  assert(target.timestamp == 1000);
  return target;
}
int main() {
  clear(); block(2, 3, 4, 5);
  auto target = detect();
  assert(target.found && target.candidates == 1 && target.selectedPixels == 20);
  assert(target.pixels == 20 && target.state == CameraFrameState::Marker);
  assert(std::fabs(target.x - (2.0F * 7 / 159 - 1)) < 1e-6F);
  assert(std::fabs(target.y - (2.0F * 10 / 119 - 1)) < 1e-6F);
  const auto single = target;
  for (unsigned i = 0; i < 5; ++i) sample(60 + i * 2, 50);
  target = detect();
  assert(target.found && target.pixels == 25 && target.selectedPixels == 20);
  assert(target.x == single.x && target.y == single.y); // Noise cannot shift centroid.

  clear(); block(0, 0, 3, 4); block(60, 50, 3, 4);
  target = detect(); // 24 total samples, but neither component qualifies.
  assert(!target.found && target.candidates == 0 && target.pixels == 24);
  assert(target.state == CameraFrameState::NoMarker);
  clear(); block(0, 0, 4, 5); block(60, 50, 5, 5);
  target = detect();
  assert(!target.found && target.candidates == 2 && target.pixels == 45);
  assert(target.x == 0 && target.y == 0 && target.selectedPixels == 0);
  assert(target.state == CameraFrameState::Ambiguous);
  assert(std::strcmp(cameraFrameStateName(target.state), "ambiguous") == 0);
  // Even a much larger first region cannot silently win over a second candidate.
  clear(); block(0, 0, 30, 30); block(60, 50, 4, 5);
  assert(detect().state == CameraFrameState::Ambiguous);

  clear(); block(0, 0, 4, 5); block(4, 5, 4, 5);
  assert(detect().candidates == 2); // Diagonal-only contact is not four-connected.
  sample(4, 4);
  target = detect(); // Documented limitation: a one-sample bridge merges regions.
  assert(target.found && target.candidates == 1 && target.selectedPixels == 41);
  clear(); block(0, 0, 80, 60);
  target = detect(); // Worst queue occupancy; no full-red-area rejection claim.
  assert(target.found && target.pixels == 4800 && target.selectedPixels == 4800);
  clear();
  for (unsigned y = 0; y < 60; ++y)
    for (unsigned x = 0; x < 80; ++x) if ((x + y) % 2 == 0) sample(x, y);
  target = detect();
  assert(!target.found && target.pixels == 2400 && target.candidates == 0);
  clear(); assert(detect().pixels == 0); // Workspace from previous frame cannot leak.
  block(75, 55, 5, 5);
  target = detect(); assert(target.found && target.x > .9F && target.y > .9F);

  std::vector<uint8_t> odd(159 * 119 * 2, 0);
  for (unsigned y = 110; y < 119; y += 2)
    for (unsigned x = 150; x < 159; x += 2) odd[(y * 159 + x) * 2] = 0xF8;
  auto result = findRedTarget(odd.data(), odd.size(), 159, 119, storage.work);
  assert(result.found && result.pixels == 25 && result.x <= 1 && result.y <= 1);
  assert(!findRedTarget(odd.data(), odd.size(), 161, 119, storage.work).found);
  uint8_t tiny[8] = {0xF8, 0, 0, 0, 0, 0, 0, 0};
  assert(!findRedTarget(tiny, sizeof(tiny), 2, 2, storage.work).found);
  assert(storage.before == 0xABCD1234 && storage.after == 0x5678ABCD);
  std::puts("PASS: single/ambiguous components, speckles, threshold, adjacency, edge, full-frame capacity and workspace reuse");
}
