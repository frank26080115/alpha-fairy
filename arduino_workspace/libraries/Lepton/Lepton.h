#ifndef Lepton_h
#define Lepton_h

#include "Arduino.h"

#include "img_table.h"
#include "img/ColorT.h"

#define FLIR_X 160
#define FLIR_Y 120

#define LEPTON_RESET_PIN 26

#define ESP_GetUS() ((uint64_t)(esp_timer_get_time()))

enum
{
    LEPSTATE_RST0 = 0,
    LEPSTATE_RST1,
    LEPSTATE_RST2,
    LEPSTATE_RST3,
    LEPSTATE_INIT_BOOT,
    LEPSTATE_INIT_SYNC,
    LEPSTATE_INIT_SYNC_WAIT,
    LEPSTATE_INIT_CMD1,
    LEPSTATE_INIT_CMD2,
    LEPSTATE_INIT_CMD_END,
    LEPSTATE_INIT_DONE,
    LEPSTATE_SYNCFRAME,
    LEPSTATE_RESYNCFRAME,
    LEPSTATE_SYNCFRAME_WAIT,
    LEPSTATE_VSYNC1,
    LEPSTATE_VSYNC2,
    LEPSTATE_VSYNC3,
    LEPSTATE_VSYNC4,
    LEPSTATE_VSYNC_CMD1,
    LEPSTATE_VSYNC_CMD1_WAIT,
    LEPSTATE_VSYNC_CMD2,
    LEPSTATE_VSYNC_CMD2_WAIT,
    LEPSTATE_VSYNC_END,
    LEPSTATE_VSYNC_END_DELAY,
    LEPSTATE_RESYNC_DLY,
    LEPSTATE_BAD_LINE,
};

enum
{
    LEPSMRET_NONE,
    LEPSMRET_INIT,
    LEPSMRET_LOOP,
    LEPSMRET_NEWDATA,
    LEPSMRET_ERROR,
};

class Lepton {
public:
  // Registers for use with readRegister and writeRegister.
  static const uint16_t REG_POWER = 0;
  static const uint16_t REG_STATUS = 2;
  static const uint16_t REG_COMMAND_ID = 4;
  static const uint16_t REG_DATA_LEN = 6;
  static const uint16_t REG_DATA_BASE = 8;

  // Automatic Gain Control commands
  static const uint16_t CMD_AGC_ENABLE = 0x0100;
  static const uint16_t CMD_AGC_ROI = 0x0108;
  static const uint16_t CMD_AGC_HISTOGRAM_STATISTICS = 0x010c;
  static const uint16_t CMD_AGC_HEQ_DAMPING_FACTOR = 0x0124;
  static const uint16_t CMD_AGC_HEQ_CLIP_LIMIT_HIGH = 0x012c;
  static const uint16_t CMD_AGC_HEQ_CLIP_LIMIT_LOW = 0x0130;
  static const uint16_t CMD_AGC_HEQ_EMPTY_COUNT = 0x013c;
  static const uint16_t CMD_AGC_HEQ_SCALE_FACTOR = 0x0144;
  static const uint16_t CMD_AGC_HEQ_CALC_ENABLE_STATE = 0x0148;

  // System commands
  static const uint16_t CMD_SYS_PING = 0x0200;
  static const uint16_t CMD_SYS_STATUS = 0x0204;
  static const uint16_t CMD_SYS_FLIR_SERIAL_NUMBER = 0x0208;
  static const uint16_t CMD_SYS_CAMERA_UPTIME = 0x020C;
  static const uint16_t CMD_SYS_AUX_TEMPERATURE_KELVIN = 0x0210;
  static const uint16_t CMD_SYS_FPA_TEMPERATURE_KELVIN = 0x0214;
  static const uint16_t CMD_SYS_TELEMETRY_ENABLE = 0x0218;
  static const uint16_t CMD_SYS_TELEMETRY_LOCATION = 0x021c;
  static const uint16_t CMD_SYS_FRAMES_TO_AVERAGE = 0x0224;
  static const uint16_t CMD_SYS_CUST_SERIAL_NUMBER = 0x0228;
  static const uint16_t CMD_SYS_SCENE_STATISTICS = 0x022c;
  static const uint16_t CMD_SYS_SCENE_ROI = 0x0230;
  static const uint16_t CMD_SYS_THERMAL_SHUTDOWN_COUNT = 0x0234;
  static const uint16_t CMD_SYS_SHUTTER_POSITION = 0x0238;
  static const uint16_t CMD_SYS_FFC_SHUTTER_MODE = 0x023c;
  static const uint16_t CMD_SYS_FFC_NORMALIZATION = 0x0240;
  static const uint16_t CMD_SYS_FFC_STATUS = 0x0244;

  // Video commands
  static const uint16_t CMD_VID_USER_LUT = 0x0308;
  static const uint16_t CMD_VID_FOCUS_CALC_ENABLE = 0x030c;
  static const uint16_t CMD_VID_FOCUS_ROI = 0x0310;
  static const uint16_t CMD_VID_FOCUS_METRIC_THRESHOLD = 0x0314;
  static const uint16_t CMD_VID_FOCUS_METRIC = 0x0318;
  static const uint16_t CMD_VID_FREEZE_ENABLE = 0x0324;

  //
  static const uint16_t CMD_OEM_SYNC_SET = 0x4854;
  //static const uint16_t CMD_OEM_SYNC_GET = 0x4864;
  static const uint16_t CMD_OEM_DELAY_SET = 0x4858;

  static const uint16_t STATUS_BIT_BUSY = 1;
  static const uint16_t STATUS_BIT_BOOT_MODE = 2;
  static const uint16_t STATUS_BIT_BOOT_STATUS = 4;

  // Create a new Lepton instance
  Lepton(int sdaPin, int sclPin, int ssPin, int syncPin);

  // Start Lepton access
  bool begin();

  // Read a (16-bit) register
  uint16_t readRegister(uint16_t reg);

  // Write a (16-bit) register
  void writeRegister(uint16_t reg, uint16_t value);

  // Do a get command, and get the resulting data
  uint16_t doGetCommand(uint16_t commandIdBase, uint16_t* data);
  void doGetCommandStart(uint16_t commandIdBase);

  // Do a set command, using the provided data
  void doSetCommand(uint16_t commandIdBase, uint16_t* data, uint16_t dataLen, bool wait = true);

  // Do a run command, using the provided data and returning the result in the same buffer
  uint16_t doRunCommand(uint16_t commandIdBase, uint16_t* data, uint16_t dataLen);
  void end();
 
 // (Re-)synchronize the frame stream
  void syncFrame();
  void getRawValues();

  // Read a frame into a 80 * 60 uint16_t buffer
  int readFrame(uint16_t* data);
  //int read160X120Frame(uint16_t* data);
  int read_160x120_Frame(uint16_t* data);

  void reset();

  // Read completely raw frame data - for debugging purposes
  void readFrameRaw(uint16_t* data);
  uint16_t wait_160X120_NextFrame();

  // state machine task
  uint8_t state_machine_inner();
  uint8_t state_machine();
  inline bool hasNewData(bool clr) { bool x = _new_data; if (clr) { _new_data = false;} return x; };

  // checks if the lepton device is actually on the I2C bus
  inline bool i2cAvail(void) { return _i2cAvail; };

private:
  // Device ID on the I2C interface
  static const uint8_t DEVICE_ID = 0x2A;

  // Command types
  static const uint16_t TYPE_GET = 0;
  static const uint16_t TYPE_SET = 1;
  static const uint16_t TYPE_RUN = 2;

  bool _i2cAvail = true;
  bool _new_data = false;
  int16_t sm_state = 0; // state machine state
  int16_t sm_state_next = 0;
  int16_t sm_line = 0;
  uint32_t sm_time = 0;
  uint64_t sm_time_us = 0;
  uint32_t sm_time_frm = 0;
  uint16_t sm_line_err = 0, sm_init_err = 0, sm_sync_err = 0;
  uint32_t sm_frm_cnt = 0;

  // Start I2C transmission relating to a specific register
  void startTransmission(uint16_t reg);

  // Transmit a word over I2C
  void transmitWord(uint16_t value);

  // End I2C transmission
  void endTransmission();

  // Read a word over I2C
  uint16_t readWord();

  // Set the current register over I2C
  void setRegister(uint16_t reg);

  // Wait for the camera to become idle, e.g. after performing a command
  bool waitIdle(); // return true for OK

  // Read data from a buffer
  uint16_t readData(uint16_t* data);
  
  // Write data to a buffer
  void writeData(uint16_t* data, uint16_t dataLen);

  // Read a word of frame data over SPI
  uint16_t readFrameWord();

  // Wait for the next frame over SPI
  uint16_t waitNextFrame();
  //uint16_t wait_160X120_NextFrame();

 // Dump a buffer as hex to serial - for debugging purposes
  void dumpHex(uint16_t *data, int dataLen);
  bool savePackage(byte line, byte segment);
  int getPackage(byte line, byte seg);
  // SDA, SCL and SPI CS pins
  int _sdaPin;
  int _sclPin;
  int _ssPin;
  int _syncPin;
};

#endif
