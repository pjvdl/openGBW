OpenGBW
=======

- [OpenGBW](#opengbw)
- [Differences to the original](#differences-to-the-original)
- [Dev environment setup](#dev-environment-setup)
- [Getting started](#getting-started)
- [Flashing firmware onto an ESP32](#flashing-firmware-onto-an-esp32)
- [Wiring](#wiring)
  - [Load Cell](#load-cell)
  - [Display](#display)
  - [Relay](#relay)
  - [Rotary Encoder](#rotary-encoder)
- [BOM](#bom)
- [3D Files](#3d-files)
- [Todo](#todo)
- [Troubleshooting](#troubleshooting)
  - [Can't upload](#cant-upload)

This Project extends and adapts the original by Guillaume Besson. It has been adapted to operate with a [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html) board.

**Note that the 3D base is designed to be used with a modified portafilter mount that must be fabricated from sheet stainless steel (1mm thick or similar). One possible design for this is included in the blender model.**

More info: https://besson.co/projects/coffee-grinder-smart-scale

This mod will add GBW functionality to basically any coffe grinder that can be started and stopped manually.

The included 3D Models are adapted to the Eureka Mignon XL, and Eureka Mignon Classico, but the electronics can be used for any Scale.

-----------

# Differences to the original

- changed IO pins to match [ESP32-S3-DevKitC-1](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3-devkitc-1.html) (44 pin) board
- updated 3D base to suit older Eureka Mignon (now known as the Mignon Classico) and to include mount points for electronic components
- The older Mignon doesn't include a 5V power supply, so we use a VREG7805 linear power regulator to take a 5V power supply from the 12V DC power, connected via a soldered 12V takeoff. We could use a buck converter here instead, but a linear converter is simpler and heat dissipation shouldn't be an issue with the small currents involved. 
- added a rotary encoder to select weight and navigate menus
- made everything user configurable without having to compile your custom firmware
- dynamically adjust the weight offset after each grind
- added relay for greater compatibility
- added different ways to activate the grinder
- added scale only mode

-----------


# Dev environment setup

Setup PlatformIO in VSCode as per OS specific instructions.

Note that this project is designed to be built/uploaded/monitored with the PlatformIO toolchain, rather than the ESP-IDF tools (via the PlatformIO extension navigation page).

USB cable can be connected to the USB or UART ports for upload, but should be plugged into the UART port for the serial monitor (ie. serial.println) to output to the serial terminal.

# Getting started

1) 3D print the included models for a Eureka Mignon XL or design your own
2) flash the firmware onto an ESP32 (see below)
3) connect the display, relay, load cell and rotary encoder to the ESP32 according to the wiring instructions
4) go into the menu by pressing the button of the rotary encoder and set your initial offset. -2g is a good enough starting value for a Mignon XL
5) if you're using the Mignon's push button to activate the grinder set grinding mode to impulse. If you're connected directly to the motor relay use continuous.
6) if you only want to use the scale to check your weight when single dosing, set scale mode to scale only. This will not trigger any relay switching and start a timer when the weight begins to increase. If you'd like to build your own brew scale with timer, this is also the mode to use.
7) calibrate your load cell by placing a 100g weight on it and following the instructions in the menu
8) set your dosing cup weight
5) exit the menu, set your desired weight and place your empty dosing cup on the scale. The first grind might be off by a bit - the accuracy will increase with each grind as the scale auto adjusts the grinding offset

-----------

# Flashing firmware onto an ESP32

This project is designed to be built on VSCode using the PlatformIO plugin. Install VSCode, the plugin and any prerequisites.

The [platformio.ini](./platformio.ini) file defines the build configurations. Build the default esp32_usb configuration for a standard runtime. The debug configuration can be used to debug the firmware.

To build and deploy:

1) Navigate to the PlatformIO IDE from the plugin navigation sidebar
2) Expand the build configuration that you want to use and select Build
3) Once built, upload to the esp32 via the USB port on the esp32 (UART port also works?)

If you want to debug, preform the same steps using the debug configuration. The install and configuration of the PlatformIO plugin and project should have created a number of standard VSCode debug configurations.

1) Build debug
2) Upload debug
3) F5 to run the debug configuration of your choice

-----------

# Wiring

## Load Cell

| Load Cell  | HX711 | ESP32  |
|---|---|---|
| black  | E-  | |
| red  | E+  | |
| green  | A+  | |
| white  | A-  | |
|   | VCC  | VCC/3.3 |
|   | GND  | GND |
|   | SCK  | GPIO 7 |
|   | DT  | GPIO 6 |

## Display

| Display | ESP32 |
|---|---|
| VCC | VCC/3.3 |
| GND | GND |
| SCL | GPIO 17 |
| SDA | GPIO 18 |

## Relay

| Relay | ESP32 | Grinder |
|---|---|---|
| + | VCC/3.3 | |
| - | GND | |
| S | GPIO 14 | |
| Middle Screw Terminal | | push button |
| NO Screw Terminal | | push button |

## Rotary Encoder

| Encoder | ESP32 |
|---|---|
| VCC/+ | VCC/3.3 |
| GND | GND |
| SW | GPIO 42 |
| DT | GPIO 41 |
| CLK | GPIO 40 |

-----------

# BOM

1x ESP32-S3-DevKitC-1  
1x HX711 load cell amplifier  
1x 1.3" OLED Display  
1x KY-040 rotary encoder  
1x 500g load cell 47 x 12 x 6mm
1x VREG7805 

various jumper cables  
a few WAGO or similar connectors

-----------

# 3D Files

You can find the 3D STL models on thangs.com

Eureka XL: https://thangs.com/designer/jbear-xyz/3d-model/Eureka%20Mignon%20XL%20OpenGBW%20scale%20addon-834667?manualModelView=true

These _should_ fit any grinder in the Mignon line up as far as I can tell.

There's also STLs for a universal scale in the repo, though it is mostly meant as a starting off point to create your own. You can use the provided files, but you'll need to print an external enclosure for the ESP32, relay and any other components your setup might need.

# Todo

- ~add option to change grind start/stop behaviour. Right now it pulses for 50ms, this works if its hooked up to the push button of a Eureka grinder. Other models might need constant input while grinding~ done
- add mounting options and cable routing channels to base
- more detailed instructions (with pictures!)
- other grinders?
- ???

# Troubleshooting

## Can't upload

 I now found in one of the espressif docs that I need to press the BOOT button and if that doesn't help, keep the BOOT button pressed and press the RST button as well. 