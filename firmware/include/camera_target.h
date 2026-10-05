#pragma once
#include <stdint.h>
struct CameraTarget { bool found; float x; float y; uint32_t timestamp; };
bool startCameraTarget();
bool readCameraTarget(CameraTarget& target);
