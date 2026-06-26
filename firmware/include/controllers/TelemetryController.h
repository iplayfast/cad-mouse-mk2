#pragma once

class TelemetryController {
 public:
  void begin();
  void publish(const float delta[9], const float motion[6], int buttonBits, bool hidReportSent);
  bool enabled() const;

 private:
  int tick_ = 0;
};
