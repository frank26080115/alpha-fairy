# Installation Setup on PC

Install [Arduino IDE](https://www.arduino.cc/en/software), version is 1.8.19 or later

Download a copy of this particular GitHub repo, find the directory called "arduino_workspace". From inside Arduino IDE, use the menu bar, File->Preferences, put the path to "arduino_workspace" into "Sketchbook Location".

Follow instructions to install ESP32 for Arduino IDE
 * official instructions https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 * unofficial instructions https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/ , https://microcontrollerslab.com/install-esp32-arduino-ide/

Connect the M5StickC-Plus to the computer, follow instructions to install the FTDI driver (if required): https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino (this may also contain instructions for steps I've already listed)

Install the "Arduino ESP32 filesystem uploader", follow instructions: https://github.com/me-no-dev/arduino-esp32fs-plugin

Close the Arduino IDE and open it again.

Open Arduino IDE, from the menu bar, select the correct board: Tools->Boards->ESP32 Boards->M5StickC

Select the correct serial port: Tools->Port->(select the option that matches the M5StickC)

Using Arduino IDE, open the file at "arduino_workspace/AlphaFairy/AlphaFairy.ino"

Upload the image files: Tools->"ESP32 Sketch Data Upload"

Click "Upload" on the tool-bar

# Connecting From Camera

When the AlphaFairy code is running on the M5StickC, turn on the camera.

Follow instructions similar to https://support.d-imaging.sony.co.jp/app/imagingedge/en/instruction/4_1_connection.php (note: we are using the Wi-Fi Access Point method, without pairing)

From the camera menu, the option "PC control method" should be "Wi-Fi Access Point".

From the camera menu, airplane mode should be disabled, FTP should be disabled, control with smartphone should be disabled.

From the camera menu, the option "connect without pairing" should be enabled.

From the camera menu, the option "Still Image Save Destination" should be set to "Camera Only".

From the camera menu, connect to the SSID that the AlphaFairy is broadcasting ("afairywifi" by default), the password should be "1234567890"

On the M5StickC's screen, the "no signal" icon should disappear. On the camera's screen, the Wi-Fi symbol should be fully white and the PC icon should be fully white.

# General Usage

There are three buttons: "big", "side", and "power"

The side button navigates, chooses the next item.

The big button is used to either enter a menu item, or activate a menu item.

The power button turns the device on, and holding it down for 4 seconds (or more) will shutdown the device. It also acts as an exit button when you press it quickly.

When trying to adjust a configurable option, there will be either a plus (+) or minus (-) sign beside the number. Tilting the device to the right will use plus mode, pressing the big button will add to the number. Tilting the device to the left will use minus mode, pressing the big button will subtract from the number.

When in delayed remote shutter mode, completely rotating the device will change the delay.

When in focus pull mode, the tilt of the device determines the speed and direction of the focus change, press the big button to perform the focus pull.

When in the dual shutter mode, first register the setting for the second photo by setting up your camera, then pressing the big button on the M5StickC. Then change the settings on your camera again for the first photo. Navigate to the "press to shoot" menu item after that. Now, pressing the big button (or activating auto-focus) will take two photos, each with a different shutter speed.
