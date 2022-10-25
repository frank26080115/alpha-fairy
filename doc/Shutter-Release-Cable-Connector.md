# Shutter Release Cable Attachment

    The use of the software, firmware, instructions, and scripts on this site is done at your own discretion and risk and with agreement that you will be solely responsible for any damage to your electronic devices or loss of data that results from such activities.

It may be wise to use the intervalometer mode with a shutter release cable, due to the chance of the Wi-Fi connection being unreliable or having too much latency.

The M5StickC-Plus that the Alpha-Fairy is built upon has a pin connector at the end. You can construct something that connects a shutter release cable to this connector.

![](img/shutter_release_cable_plugged_in.jpg)

![](img/shutter_release_cable_connector.jpg)

You need:

 * a shutter release cable for Sony cameras, it should be USB-micro-B on one end and 3.5mm TRS male plug on the other
 * a chunk of perfboard, a kind of circuit board that has holes arranged in a grid. you only need 5 holes by 8 holes so you will probably buy a big one and cut it down
 * a PJ-320A jack, which is a 3.5mm TRS female jack, commonly found on Amazon/Ebay/(some other cheap online store) sold in large packs
 * some wire, soldering equipment, something that can cut perfboard, a small file, etc

![](img/shutter_release_cable_parts.png)

This diagram below shows how it should be put together, make it look like the photo above

![](img/shutter_release_cable_design.png)

Do not plug in your camera unless you are 100% sure you've wired this correctly. Also avoid charging or updating the firmware with the camera plugged in.

If you have the skills, you can try adding an optocoupler to the circuit to further protect your camera. This is not required but it adds an additional layer of protection.
