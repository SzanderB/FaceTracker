# Face-Tracking Security Camera

[🎥 Video Demo](https://youtu.be/4eWTQd3u4Io)

## Overview
A real-time security camera system that detects and tracks faces using a USB webcam, controlled via a Beaglebone Black and TIVA C LaunchPad.  
Features both **automatic tracking** via a PID-style controller and **manual joystick control** for pan/tilt operation.

## Features
- Real-time face detection with OpenCV
- Pan/tilt camera mount controlled by servos
- Dual control modes: automatic face tracking or manual joystick control
- On-device LCD feedback with bounding box overlay
- Modular system: image processing on Beaglebone, motor control on TIVA C

## Hardware & Software
**Hardware:**
- Beaglebone Black
- TIVA C LaunchPad
- USB webcam
- Pan/tilt servo mount
- LCD display
- Joystick, buttons, servo module

**Software & Libraries:**
- Python (socket, serial, OpenCV)
- Haarcascades & LBF face detection algorithms
- RTOS on TIVA C for multi-threaded control
- UART communication between Beaglebone & TIVA

## System Design
- **Beaglebone Black**:
  - Captures frames from USB webcam
  - Runs face detection (LBF algorithm for speed/accuracy trade-off)
  - Sends bounding box coordinates over UART to TIVA C
- **TIVA C LaunchPad**:
  - RTOS-based task scheduling
  - Threads for joystick reading, motor control, LCD display, and button input
  - Implements a K controller to center the camera on the detected face

## Development Challenges
- **Processing speed**: Early Haarcascades detection achieved only ~1–2 FPS; switched to LBF model to reach 5–10 FPS.
- **Latency**: ~1-second camera frame delay limited tracking speed; tuned controller to avoid overshoot.
- **Package installation**: Required manual OpenCV installation on Beaglebone due to networking and package compatibility issues.
- **Mechanical tuning**: Adjusted motor speeds to prevent moving the subject out of frame.

## Key Learnings
- Optimizing algorithm choice can drastically improve performance without hardware changes.
- Real-time systems require balancing mechanical and software responsiveness.
- Multi-device communication (socket, UART) needs careful protocol design to avoid bottlenecks.

