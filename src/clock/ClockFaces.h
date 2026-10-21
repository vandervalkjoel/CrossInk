#pragma once

#include "clock/ClockModel.h"

class GfxRenderer;

namespace deskclock {

struct FaceContext {
  ClockTime time;
  bool use12h;
  bool lowBattery;
  int batteryPercent;
};

// Draws one face into the (already cleared) frame buffer, in the renderer's
// current orientation. Black on white; CrossInk's Night Mode inverts it.
void drawFace(GfxRenderer& renderer, ClockFace face, ClockOrientation orientation, const FaceContext& ctx);

}  // namespace deskclock
