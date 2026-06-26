#pragma once

class MotionController {
 public:
  void reset();
  void compute(const float raw[9], const float* baseline, float dt, float out[6]);
  bool hasMotionActivity() const;
  void setDecouplingMatrix(const float matrix[6][9]);

 private:
  static float clampf(float v, float lo, float hi);
  static float lowpass(float prev, float x, float dt, float tau);
  static float axisBaseDead(int i);
  float filt_[6] = {};
  bool motionActive_ = false;
  float customMatrix_[6][9] = {};
  bool hasCustomMatrix_ = false;
};
