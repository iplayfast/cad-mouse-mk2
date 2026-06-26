#include "controllers/MotionController.h"

#include <Arduino.h>
#include <math.h>

#include "Config.h"

namespace {
enum RawIndex {
  RAW_MAG1_X = 0,
  RAW_MAG1_Y,
  RAW_MAG1_Z,
  RAW_MAG2_X,
  RAW_MAG2_Y,
  RAW_MAG2_Z,
  RAW_MAG3_X,
  RAW_MAG3_Y,
  RAW_MAG3_Z
};

enum AxisIndex {
  AXIS_TX = 0,
  AXIS_TY,
  AXIS_TZ,
  AXIS_RX,
  AXIS_RY,
  AXIS_RZ
};
}  // namespace

void MotionController::reset() {
  for (int i = 0; i < 6; i++) {
    filt_[i] = 0.0;
  }
  motionActive_ = false;
}

float MotionController::clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

float MotionController::hardZero(float v, float thr) {
  return (fabs(v) < thr) ? 0.0 : v;
}

float MotionController::lowpass(float prev, float x, float dt, float tau) {
  if (tau <= 0.0) return x;
  const float a = dt / (tau + dt);
  return prev + a * (x - prev);
}

float MotionController::axisBaseDead(int i) {
  return (i < 3) ? Config::DEAD_T : Config::DEAD_R;
}

void MotionController::compute(const float raw[9], const float* baseline, float dt,
                               float out[6]) {
  // Baseline subtraction converts magnetic deltas around the calibrated rest pose.
  const float mag1x = raw[RAW_MAG1_X] - baseline[RAW_MAG1_X];
  const float mag1y = raw[RAW_MAG1_Y] - baseline[RAW_MAG1_Y];
  const float mag1z = raw[RAW_MAG1_Z] - baseline[RAW_MAG1_Z];
  const float mag2x = raw[RAW_MAG2_X] - baseline[RAW_MAG2_X];
  const float mag2y = raw[RAW_MAG2_Y] - baseline[RAW_MAG2_Y];
  const float mag2z = raw[RAW_MAG2_Z] - baseline[RAW_MAG2_Z];
  const float mag3x = raw[RAW_MAG3_X] - baseline[RAW_MAG3_X];
  const float mag3y = raw[RAW_MAG3_Y] - baseline[RAW_MAG3_Y];
  const float mag3z = raw[RAW_MAG3_Z] - baseline[RAW_MAG3_Z];

  const float delta[9] = {
    mag1x, mag1y, mag1z,
    mag2x, mag2y, mag2z,
    mag3x, mag3y, mag3z,
  };
  float y[6] = {};
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 9; j++) {
      y[i] += Config::DECOUPLING_M[i][j] * delta[j];
    }
  }

  // Filter, clamp to range and dead zones.
  motionActive_ = false;
  for (int i = 0; i < 6; i++) {
    const float dead = axisBaseDead(i);

    if (fabs(y[i]) < dead) {
      filt_[i] = 0.0;
    } else {
      filt_[i] = lowpass(filt_[i], y[i], dt, Config::SMOOTH_TAU_S);
    }

    const float limited =
        clampf(filt_[i], -Config::AXIS_LIMIT, Config::AXIS_LIMIT);
    out[i] = hardZero(limited, dead);
    if (out[i] != 0.0) {
      motionActive_ = true;
    }
  }
}

bool MotionController::hasMotionActivity() const { return motionActive_; }
