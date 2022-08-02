# alpha-fairy

This is a tiny remote control for Sony Alpha cameras. The hardware platform is a [M5StickC-Plus](https://shop.m5stack.com/products/m5stickc-plus-esp32-pico-mini-iot-development-kit), which is a DIY device with a [ESP32-PICO](https://www.espressif.com/en/products/socs/esp32) inside, along with a colour LCD screen, rechargable battery, some buttons, and a few other features.

This remote combines the features of other simple camera remotes, plus many complex functions that automates some tasks that photographers would like to do. It communicates wirelessly with the camera via [Picture Transfer Protocol](https://en.wikipedia.org/wiki/Picture_Transfer_Protocol), mostly reverse engineered by spying on Sony's own [Imaging Edge Remote](https://imagingedge.sony.net/l/ie-desktop.html#remote) application with [Wireshark](https://www.wireshark.org/).

This GitHub repo contains the C++ source code (and graphics files) that can be compiled and loaded into the M5StickC-Plus, so anybody can [create this remote](INSTRUCTIONS.md).

Supported camera models: A1 (tested), untested but theoretically can support: A7SM3, A9M2, A7M4A, A7RM4, A7C, A7M4, ZV-E10, ZV-1

The fun part about this project for me is to create a usable user interface on a device with only two buttons plus an IMU.

## Main Features

 * Remote Shutter (with optional timer)
 * Remote Movie Record
 * Focus Stack
   * takes consecutive photos as the focus is slowly shifted, this is a technique used in macro photography and some other camera brands offer this in-camera (but not Sony)
 * Focus 9-Point
   * takes consecutive photos as the auto-focus point moves around the scene, to obtain multiple photos focused on multiple objects, convenient for landscape photography
 * Focus Pull
 * Dual Shutter
   * takes two consecutive photos, with different shutter speeds, for compositing photos with both sharp subjects and blurred features
 * Intervalometer
 * Astrophotography Intervalometer
   * same as intervalometer but more focused on bulb mode, and uses pause time instead of fixed interval time

### Minor Features

 * can fall-back to using infrared communication if Wi-Fi is disconnected
 * can use shutter release cable connected to GPIO
 * status bar with battery indicator and connection indicator
 * auto power save
 * configurable options
 * serial port debugging and command line interface

## Known Problems

The camera does not re-establish a broken connection. If the remote is turned off (or disconnected for other reasons), you need to turn off the camera and turn it back on (after the remote is turned back on).

The features that can change shutter speed are not reliable. The camera takes a long (and variable) time to register a remote command to change the shutter speed.

Battery life is a bit short. Do not use it for intervalometer purposes without an external power source. Also, using Wi-Fi for intervalometer is ill-advised, use a real shutter release cable if possible.
