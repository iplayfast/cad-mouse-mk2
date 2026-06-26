# CAD Mouse MK2

Watch the build video ↓

[<img src="./images/CAD_Mouse_MK2_Thumbnail.jpg">](https://youtu.be/62xlzGs8LXA)

This is the second iteration of my DIY CAD Mouse, rebuilt to behave like a real 6DoF controller. There are still some motion processing issues, but it's much better than the previous version. It uses a custom PCB with three magnetic sensors, a 3D printed spring, and a redesigned enclosure that is smaller and easier to build.

  Build instructions → [Instructables](https://www.instructables.com/CAD-Mouse-MK2-a-6DoF-Space-Mouse-Using-Magnets)

<sub>⚠️ There have been several comments raising concerns about the longevity of the PETG spring. If it does not last as expected, a revision of the knob design will be needed.</sub>

## Calibration

Because the three magnetic sensors are physically close together, each sensor responds to motion in every direction — not just the one you're moving. Without correction, pushing the puck sideways would also produce spurious tilt and twist outputs. Calibration solves this by fitting a **6×9 decoupling matrix** that maps the raw 9-channel sensor readings (3 axes × 3 sensors) to clean 6-DoF outputs (TX, TY, TZ, RX, RY, RZ).

The fitted matrix is written to `firmware/include/Calibration.h` and compiled into the firmware. You should recalibrate whenever you rebuild the mouse, swap a sensor, or notice persistent drift or cross-axis bleed-through.

### Requirements

- Python 3 with `numpy` and `pyserial` (`pip install numpy pyserial`)
- The mouse flashed and connected via USB (default port `/dev/ttyACM0`, 115200 baud)

### Steps

1. Flash the firmware and connect the mouse.
2. From the project root, run:
   ```bash
   ./run_calibration.sh
   ```
3. The script walks you through **6 movements**, one per axis:

   | Step | Movement |
   |------|----------|
   | TX | Slide the puck **left and right** — translate only, no tilt or twist |
   | TY | Slide the puck **forward and back** — translate only, no tilt or twist |
   | TZ | Press the puck **straight down** then release — no tilt or sideways |
   | RX | **Tilt front-to-back** — front edge dips then rises |
   | RY | **Tilt left-to-right** — left edge dips then rises |
   | RZ | **Twist clockwise then counterclockwise** around the puck's centre |

4. For each step: press **Enter** to open the live monitor, make the movement slowly (stay well under the ±350 output limit), then press **Q** to stop recording.
5. After all 6 logs are collected, `calibrate.py` fits the matrix and prints a cross-talk table. Any axis bleeding more than 50 % into another is flagged — if you see warnings, recollect that axis's log.
6. `firmware/include/Calibration.h` is updated automatically. **Reflash the firmware** to apply the new calibration.

### How it works

`calibrate.py` uses least-squares regression (`numpy.linalg.lstsq`) across all six labeled movement logs simultaneously to find the 6×9 matrix that best predicts the intended single-axis output from the 9 raw sensor deltas. Fitting all axes together means the solver can distinguish shared sensor patterns that belong to one DoF from those that belong to another.

[![CC BY-NC-SA 4.0][cc-by-nc-sa-shield]][cc-by-nc-sa]

[![CC BY-NC-SA 4.0][cc-by-nc-sa-image]][cc-by-nc-sa]

[cc-by-nc-sa]: http://creativecommons.org/licenses/by-nc-sa/4.0/
[cc-by-nc-sa-image]: https://licensebuttons.net/l/by-nc-sa/4.0/88x31.png
[cc-by-nc-sa-shield]: https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg
