#include "controllers/HIDController.h"

#include <math.h>
#include <string.h>

#include "Config.h"

// Singleton pointer used by the static TinyUSB callbacks.
static HIDController* gHidInstance = nullptr;

namespace {

const uint8_t kHidReportDescriptor[] PROGMEM = {
    // ── Multi-axis controller (report IDs 1 and 3) ───────────────────────────
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
    0x09, 0x08,        // USAGE (Multi-axis Controller)
    0xA1, 0x01,        // COLLECTION (Application)
    0xA1, 0x00,        //   COLLECTION (Physical)
    0x85, 0x01,        //   REPORT_ID (1)
    0x16, 0xA2, 0xFE,  //   LOGICAL_MINIMUM  (-350)
    0x26, 0x5E, 0x01,  //   LOGICAL_MAXIMUM  (350)
    0x09, 0x30,        //   USAGE (X)
    0x09, 0x31,        //   USAGE (Y)
    0x09, 0x32,        //   USAGE (Z)
    0x09, 0x33,        //   USAGE (Rx)
    0x09, 0x34,        //   USAGE (Ry)
    0x09, 0x35,        //   USAGE (Rz)
    0x75, 0x10,        //   REPORT_SIZE (16)
    0x95, 0x06,        //   REPORT_COUNT (6)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)
    0xC0,              //   END_COLLECTION
    0xA1, 0x00,        //   COLLECTION (Physical)
    0x85, 0x03,        //   REPORT_ID (3)
    0x05, 0x09,        //   USAGE_PAGE (Button)
    0x19, 0x01,        //   USAGE_MINIMUM (Button 1)
    0x29, 0x02,        //   USAGE_MAXIMUM (Button 2)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,        //   REPORT_SIZE (1)
    0x95, 0x02,        //   REPORT_COUNT (2)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)
    0x95, 0x0E,        //   REPORT_COUNT (14) padding
    0x81, 0x01,        //   INPUT (Const,Array,Abs)
    0xC0,              //   END_COLLECTION
    0xC0,              // END_COLLECTION (Multi-axis)

    // ── Vendor-defined: calibration data exchange (report IDs 4 and 5) ───────
    // Report ID 4: FEATURE  — host reads 9 sensor-delta floats (36 bytes)
    // Report ID 5: OUTPUT   — host writes 54-float calibration matrix (216 bytes)
    0x06, 0x00, 0xFF,  // USAGE_PAGE (Vendor Defined 0xFF00)
    0x09, 0x01,        // USAGE (Vendor 1)
    0xA1, 0x01,        // COLLECTION (Application)
    0x85, 0x04,        //   REPORT_ID (4)
    0x09, 0x02,        //   USAGE (Vendor 2)
    0x75, 0x08,        //   REPORT_SIZE (8)
    0x95, 0x24,        //   REPORT_COUNT (36)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,  //   LOGICAL_MAXIMUM (255)
    0xB1, 0x02,        //   FEATURE (Data,Var,Abs)
    0x85, 0x05,        //   REPORT_ID (5)
    0x09, 0x03,        //   USAGE (Vendor 3)
    0x75, 0x08,        //   REPORT_SIZE (8)
    0x95, 0xD8,        //   REPORT_COUNT (216)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,  //   LOGICAL_MAXIMUM (255)
    0x91, 0x02,        //   OUTPUT (Data,Var,Abs)
    0xC0               // END_COLLECTION (Vendor)
};

}  // namespace

void HIDController::begin() {
  gHidInstance = this;
  if (!TinyUSBDevice.isInitialized()) {
    TinyUSBDevice.begin(0);
  }
  usbHid_.setReportDescriptor(kHidReportDescriptor, sizeof(kHidReportDescriptor));
  usbHid_.setReportCallback(onGetReport, onSetReport);
  usbHid_.enableOutEndpoint(true);
  usbHid_.setPollInterval(1);
  usbHid_.begin();
}

void HIDController::task() { TinyUSBDevice.task(); }

// ── Static TinyUSB callbacks ─────────────────────────────────────────────────

uint16_t HIDController::onGetReport(uint8_t report_id, hid_report_type_t report_type,
                                    uint8_t* buffer, uint16_t reqlen) {
  if (gHidInstance) {
    return gHidInstance->handleGetReport(report_id, buffer, reqlen);
  }
  return 0;
}

void HIDController::onSetReport(uint8_t report_id, hid_report_type_t report_type,
                                uint8_t const* buffer, uint16_t bufsize) {
  if (gHidInstance) {
    gHidInstance->handleSetReport(report_id, buffer, bufsize);
  }
}

uint16_t HIDController::handleGetReport(uint8_t report_id, uint8_t* buffer,
                                        uint16_t reqlen) {
  if (report_id == 4 && reqlen >= 36) {
    memcpy(buffer, sensorDelta_, 36);
    return 36;
  }
  return 0;
}

void HIDController::handleSetReport(uint8_t report_id, uint8_t const* buffer,
                                    uint16_t bufsize) {
  if (report_id == 5 && bufsize >= 216) {
    memcpy(pendingMatrix_, buffer, 216);
    calibrationMatrixReady_ = true;
  }
}

// ── Public API ───────────────────────────────────────────────────────────────

void HIDController::setSensorDelta(const float delta[9]) {
  memcpy(sensorDelta_, delta, sizeof(sensorDelta_));
}

bool HIDController::takeCalibrationMatrix(float matrix[6][9]) {
  if (!calibrationMatrixReady_) return false;
  memcpy(matrix, pendingMatrix_, sizeof(pendingMatrix_));
  calibrationMatrixReady_ = false;
  return true;
}

HIDController::ReportAxes HIDController::makeAxesReport(const float motion[6]) {
  ReportAxes axes{};
  axes.x  = static_cast<int16_t>(motion[0]);
  axes.y  = static_cast<int16_t>(motion[1]);
  axes.z  = static_cast<int16_t>(motion[2]);
  axes.rx = static_cast<int16_t>(motion[3]);
  axes.ry = static_cast<int16_t>(motion[4]);
  axes.rz = static_cast<int16_t>(motion[5]);
  return axes;
}

bool HIDController::axesReportChanged(const ReportAxes& axes) const {
  return axes.x  != lastSentAxes_.x  ||
         axes.y  != lastSentAxes_.y  ||
         axes.z  != lastSentAxes_.z  ||
         axes.rx != lastSentAxes_.rx ||
         axes.ry != lastSentAxes_.ry ||
         axes.rz != lastSentAxes_.rz;
}

bool HIDController::sendReports(const float motion[6], uint16_t buttonBits) {
  const ReportAxes axes = makeAxesReport(motion);
  const bool sendAxes    = axesReportChanged(axes);
  const bool sendButtons = (buttonBits != buttonBitsSent_);

  if (!usbHid_.ready() || (!sendAxes && !sendButtons)) {
    return false;
  }

  if (sendAxes) {
    usbHid_.sendReport(0x01, &axes, sizeof(axes));
    lastSentAxes_ = axes;
  }

  if (sendButtons) {
    ReportButtons btn{};
    btn.bits = buttonBits & 0x0003;
    usbHid_.sendReport(0x03, &btn, sizeof(btn));
    buttonBitsSent_ = buttonBits;
  }

  return true;
}
