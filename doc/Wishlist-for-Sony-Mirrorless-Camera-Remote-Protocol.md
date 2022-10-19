## Wishlist for Sony Mirrorless Camera Remote Protocol

 * Make the option to save to "Camera Only" an option on all models of cameras and for all modes
 * Give an option to unpair a computer without doing a full factory reset
 * Add focal length data to the device property packets
   * for all lenses, not just power zoom lenses
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
