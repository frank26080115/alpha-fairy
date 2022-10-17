# Reverse Engineering

My first attempt at a project that can control a Sony camera used USB. The Sony Imaging Edge Remote application supports both USB and network communication. This was when I discovered that the camera used PTP (Picture Transfer Protocol) by packet-sniffing over USB.

Digging around the internet, it looks like the protocol is from 2000 and one link available to download: https://people.ece.cornell.edu/land/courses/ece4760/FinalProjects/f2012/jmv87/site/files/pima15740-2000.pdf . Later, TCP/IP was added as a transport mechanism for PTP, I managed to find a draft version of the document: https://www.cipa.jp/std/documents/e/DC-X005.pdf but a lot of the values in there are undefined (don't worry, they are obviously defined today and Wireshark will decode them for you)

If the link for these documents are down (ISO charges $200 for the latest copy of the ISO 15740 document, and CIPA requires a membership for the latest DC-500 document), I have [uploaded copies here](../ext_docs/).

The protocol involves operation codes and property codes. Sony cameras seem to use a mix of standard codes in the document, plus additional proprietary ones not described in the document. These required more packet-sniffing to figure out. For example, setting the focus point coordinate packs the X and Y coordinates as 16 bit integers, adjusting the focus was done in steps of 1, 3, or 7, no other numbers worked.

Conveniently other people have worked on similar projects and I was able to get some of these codes from: https://github.com/Parrot-Developers/sequoia-ptpy/blob/master/ptpy/extensions/sony.py , thanks Parrot! Another source, a bit harder to decipher, is the libgphoto project: https://github.com/gphoto/libgphoto2/tree/master/camlibs/ptp2/cameras

## Initial Handshake

There are two parts to the handshake. The first part is pretty well described by the protocol documentation. The second part involves a series of operation requests using Sony's own operation codes. This appears to be slightly different between camera models, so it adds to the difficulty of supporting camera models that I do not own. However, I did write my source code in a way that this sequence is described in a table, so new camera models can be supported very easily once I get some sniffed data from the new camera model.

#### August 2022

It seems like the handshake is the same across at least two camera models, I'm fairly confident it will be the same across all camera models. Also, the act of "pairing" is simply a delay after the host sends the "initialization command request" which contains the host's friendly name, the name is shown on the camera's screen and the user has to press OK. When OK is pressed, "initialization command acknowledgment" is returned to the host. The host's GUID is memorized by the camera so it won't be prompted for any future handshakes.

#### Warning About Pairing!!!

It looks like when a camera is "paired" to an application, you cannot unpair it!!! The only way to unpair is through a factory reset, which would cause you to lose all of your custom key mappings and memory recall slots.

Please use the "Connect Without Pairing" option!!!

## Device Properties

One key feature for me to implement is to read the camera's status (current settings, battery level, etc). This is done by requesting all device properties, and then getting a gigantic chunk of data as a reply. There are a few annoying things about the way this works:

 * different cameras have different properties, and also depends on what mode the camera is in
   * this makes it a bit hard to just pre-program a list of properties
 * each property in the chunk has a variable data length, you can't just find a property before reading all of the properties before it
 * the data chunk is huge, easily more than 3kb, very problematic for small microcontrollers, and takes a long time to receive and iterate through
   * conveniently, a event packet is sent when a property changes, so periodic polling is not necessary, saving a lot of traffic
 * Wireshark and other sniffers/decoders do not automatically extract individual properties for you

The first property packet decoder I wrote worked on an older A6600 camera, it conformed to the PIMA 15740 document. When I used the same code on my newer A1 camera, it failed to decode, it seems like either the protocol was updated since the year 2000 or Sony started to deviate from what was documented. To resolve this, I looked at the entire packet for 16 bit sequences that looked like a property code (which all look like 0xD2??), which let me compare what my code thought was a complete single property vs what was actually a single complete property.

There isn't any indicator if a camera model used the older property packet format or the newer one, so supporting additional camera models is difficult and requires additional packet sniffing.

The newer packet format seems to include multiple enum lists for each property, instead of the older way of including just one enum list. I don't know why there are two lists, it could be one list each for photo mode and movie mode, or it could be a list with items that are enabled and another list with all possible items so that a GUI can grey-out some items. I did not put the effort into investigating this further, and my code simply keeps track of the size of the list, and copies the list when a new list is longer than the remembered list.

These lists (available options) can change during run-time, according to:

 * mechanical vs electronic shutter
 * whether or not BULB mode is available
 * ISO range specified by the user
 * movie vs photo mode
 * what apertures are available on the attached lens

To help myself understand more of the unknown individual property items, I experimented with code that monitored each individual property for changes. I monitored these changes as I played around with my camera. This allowed me to fill in the gaps that my other resources lacked.

## Other Gotchas

Some camera models support "save image to camera only", while other cameras only support "save image to camera and PC" without an option to not save to PC, which is bad for any embedded application. In the latter case, the image will be stuck in the buffer of the camera until a PTP transaction retrieves it. For devices with limited memory, the only workaround is to retrieve the data but toss it out, but this is still very very slow (we are talking about 25 megabytes of data for one file from a low megapixel camera, 80 MB or 120 MB from a high megapixel camera). Exacerbating the problem, when the camera saves both raw format and JPEG format, two files need to be retrieved but only one notification is given, and it only counts as 1 image when you try to read the buffer size property.

There are some commands that will override the physical controls on the camera. For example, if the camera is in auto-focus mode, and a manual focus toggle command is sent to go into manual focus mode, then the camera is stuck in manual focus mode, the user cannot use the on-camera controls to return it to auto-focus mode until another PTP command is issued to disable manual focus mode. This is especially confusing when some camera models have a physical dial for focus modes, some lenses have a focus mode toggle, and there's also a menu item on the camera for focus modes. For example, if the Wi-Fi connection is interrupted after a manual focus PTP command is sent, the dial on the camera stops working until you reboot the camera.

## Supporting Other Cameras

I've mentioned above some of the difficulties with adding support for other camera models that I do not personally own. The main hurtle is that I need to packet-sniff each camera model in order to obtain the handshake sequence and the device property format.

Sony does list the cameras that support wireless communication with the Imaging Edge Remote PC application. These are the cameras that can be more easily supported, because anybody can simply run Wireshark to obtain a packet-sniff log.

Also, [Sony does actually provide a SDK](https://support.d-imaging.sony.co.jp/app/sdk/en/index.html) for some cameras! But the SDK is meant for Windows, MacOS, and Linux (with a heavy focus on Raspberry Pi in the documentation, thanks Sony! Did you know Sony UK manufactures Raspberry Pis?). The SDK does not expose any of the protocol and its codes, and cannot be used for a microcontroller platform.

## Adding the HTTP JSON-RPC Protocol, mid August 2022

I figured out that older cameras use a deprecated API and SDK that Sony used to offer: https://developer.sony.com/develop/cameras/api-information/supported-features-and-compatible-cameras

It's a totally proprietary protocol (no standard organization), and is missing manual focusing (so focus stacking is impossible, as is any other feature that requires manual focus). It uses plain JSON and is fairly well documented.

## Adding Zoom, Obtaining Focal Length, mid Oct 2022

I have a friend who does some cool virtual production stuff (think of how The Mandalorian was filmed on a virtual set), and he said that it would be super useful to obtain the lens current focal length for calibration, because that information is used for the camera 3D positional tracking. While testing I didn't notice any property codes changing while I zoomed with my lenses.

It turns out that turning on APS-C mode on my full frame camera disables a whole bunch of the PTP commands from working. So testing with the cheap kit 16-50mm power zoom lens isn't valid. I bit the bullet and got the 16-35mm full frame power zoom lens SELP1635G. Bam! Now the camera is spitting out a property code, `0xD25D`, and it contains the zoom as a percentage (number 0 to 100). So this feature only works with official Sony full frame lenses with power zoom.
