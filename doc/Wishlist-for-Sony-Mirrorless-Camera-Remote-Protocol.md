## Wishlist for Sony Mirrorless Camera Remote Protocol

 * Add focal length data to the device property packets
   * will allow for accurate inside-out camera 3D positional tracking without calibrating for lens focal length
 * Better implementation of manual focusing commands
   * need more step size options
   * need consistent relationships between step sizes
   * need more granularity in the focus plane distance readout, currently 0-100
   * need a command to go-to a particular focus plane distance, at a particular speed
 * Camera needs to reattempt a connection when a connection fails
 * Commands to change exposure settings should not be ignored when photos are buffered
 * Shutter command needs an option that ignores the focus state, forcing the photo to be taken
 * Give access to custom buttons
