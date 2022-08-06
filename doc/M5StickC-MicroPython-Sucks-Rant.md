The M5Stack and M5Stick MicroPython port is terrible. Do not use it.

The first major problem is that it's simply too memory limited. This project is simply a remote control for a camera, it's not that complicated. But I wrote all of the Python code out, tested it on my PC first, then moved it over to the M5StickC, and it failed to import due to memory limits! I'm using built-in libraries as much as possible and import statements don't account for allocation of buffers so you can't blame me for this.

The second major problem is that it's running all of your code within a container called "m5cloud". This container allows you to use their website-based IDE to remotely update your code (so making a soft-access-point device is also impossible with the web IDE, yay...). It sounds nice but it means you never have access to the MicroPython REPL, print() statements cannot be shown, and there's a global try-catch that puts whatever error you encounter onto the LCD screen.

Do you want to read your exceptions on a 80 pixel wide LCD screen?! There's no stacktrace!

The container also means each import statement is permanent for the session, you can't edit a file and run import again, the change won't take effect until you fully reboot.

I bet these monkeys wrote "m5cloud" in plaintext Python instead of a proper C backend so it's hogging all the dynamic memory. I'll never find out since they hide the source code.

They've even discountinued their desktop IDE so you can't use the latest IDE with the USB port. Even with the VSCode extension, you can't view output from print() statements, and worse, the extension holds the COM port open so you can't have a second serial terminal application running.

(my solution to all of this is to use com0com along with some of my own code to make a serial-port-spy program, I can send it the KeyboardInterrupt exception to exit the m5cloud container and use REPL)

(officially, M5Stack don't care, and other community members have said their own solutions involve side channels like MQTT or using Bluetooth LE with a terminal on a smartphone)

Their MicroPython API calls are also horribly documented, you can try going into github and reading their .py files but you'll soon realize that nothing matches up and everything is outdated.

Their IR LED only has one modulation encoder implemented in MicroPython and they made it in a way that stops anybody from implementing another modulation encoder.

(I do love MicroPython, I have done a project that involved a Wi-Fi web server which did real-time image processing from a camera, pattern recognition on the image, etc. Totally taking advantage of memory management and dynamic typing)

Working with C++ is much better. M5Stack basically gathered a crap-ton of Arduino libraries that other people wrote and tossed it into their own library and released it. M5Stack's own code is pretty poorly documented. (Espressif's code is pretty well documented!)

M5Stack seems to have removed the JPG and PNG decoders from the M5StickC library, it seems like they thought "it doesn't have a microSD slot so it won't have files", which is completely wrong since all ESP32 variants have a flash file-system. I had to re-implement these decoders.
