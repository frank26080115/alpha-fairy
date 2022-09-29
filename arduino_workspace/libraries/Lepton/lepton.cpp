#include "Arduino.h"
#include "Lepton.h"
#include "Wire.h"
#include "SPI.h"

#define LEPTON_WAIT_TIMEOUT 1000

#define rotationHorizont 0
#define rotationVert 0

static uint8_t leptonFrame[164];
uint16_t smallBuffer[160*120];
uint16_t raw_max = 0, raw_min = 0xFFFF;
//uint16_t frame_buffer[4][60][164];
uint16_t aux_temp = 0, fpa_temp = 0, max_x, max_y, min_x, min_y;

static SPIClass* lepton_spi = new SPIClass(HSPI);

static void ESP_DelayUS(uint64_t us)
{
  uint64_t m = ESP_GetUS();
  uint64_t e = (m + us);
  if (us)
  {
    //todo: this sucks if ref tick is 100Hz
    if (us > (3000 * portTICK_PERIOD_MS))
    {
      vTaskDelay((us - (2000 * portTICK_PERIOD_MS)) / (portTICK_PERIOD_MS * 1000));
      m = ESP_GetUS();
    }

    if (m > e)
    { //overflow
      while (ESP_GetUS() > e) {
      }
    }

    while (ESP_GetUS() < e) {
    }
  }
}

static volatile bool vsync_triggered = false;
static volatile uint64_t vsync_time = 0;

static void IRAM_ATTR onVsyncISR()
{
  vsync_triggered = true;
  //vsync_time = (uint64_t)esp_timer_get_time();
}

Lepton::Lepton(int sdaPin, int sclPin, int ssPin, int syncPin) : _sdaPin(sdaPin), _sclPin(sclPin), _ssPin(ssPin), _syncPin(syncPin) {
}

bool Lepton::begin()
{
  // reset pin toggling now done in main app

  //pinMode(LEPTON_RESET_PIN, OUTPUT);
  //digitalWrite(LEPTON_RESET_PIN, HIGH);
  //delay(100);
  //digitalWrite(LEPTON_RESET_PIN, LOW);
  //delay(300);
  //digitalWrite(LEPTON_RESET_PIN, HIGH);
  //delay(50);

  pinMode(_syncPin, INPUT); //vsync
  attachInterrupt(_syncPin, onVsyncISR, RISING);

  pinMode(_ssPin, OUTPUT);
  digitalWrite(_ssPin, HIGH);

  lepton_spi->begin(2, 25, 34, 35);

  // do a quick I2C bus check, the error will show up with _i2cAvail
  waitIdle();
  return i2cAvail();
  //return true;
}

uint16_t Lepton::readRegister(uint16_t reg) {
  setRegister(reg);
  Wire1.requestFrom(DEVICE_ID, (uint8_t)2);
  return readWord();
}

void Lepton::writeRegister(uint16_t reg, uint16_t value) {
  startTransmission(reg);
  transmitWord(value);
  endTransmission();
}

uint16_t Lepton::doGetCommand(uint16_t commandIdBase, uint16_t *data)
{
  writeRegister(REG_COMMAND_ID, commandIdBase | TYPE_GET);
  waitIdle();
  return readData(data);
}

void Lepton::doGetCommandStart(uint16_t commandIdBase)
{
  writeRegister(REG_COMMAND_ID, commandIdBase | TYPE_GET);
}

void Lepton::doSetCommand(uint16_t commandIdBase, uint16_t *data, uint16_t dataLen, bool wait)
{
  writeData(data, dataLen);
  writeRegister(REG_COMMAND_ID, commandIdBase | TYPE_SET);
  if (wait) {
    waitIdle();
  }
}

uint16_t Lepton::doRunCommand(uint16_t commandIdBase, uint16_t *data, uint16_t dataLen)
{
  writeData(data, dataLen);
  writeRegister(REG_COMMAND_ID, commandIdBase | TYPE_RUN);
  waitIdle();
  return readData(data);
}

/* Get one line package from the Lepton */
int Lepton::getPackage(byte line, byte seg)
{
  lepton_spi->transferBytes(NULL, leptonFrame, 164);

  if((leptonFrame[0] & 0x0F) == 0x0F) {
    // discard packet
    return 1;
  }

  //Check if the line number matches the expected line
  if (leptonFrame[1] != line) {
    return 2;
  }

  //For the Lepton3.x, check if the segment number matches
  if (line == 20)
  {
    byte segment = (leptonFrame[0] >> 4);
    if (segment == 0) {
      return 3;
    }
    if (segment != seg) {
      return 4;
    }
  }

  return 0;
}

/* Store one package of 80 columns into RAM */
bool Lepton::savePackage(byte line, byte segment)
{
  uint16_t x_base = (line & 0x01) ? 80 : 0;
  uint16_t y_base = ((segment - 1) * 30) + (line >> 1);
  uint16_t addr_base = y_base * 160 + x_base;
  //Go through the video pixels for one video line
  for (int column = 0; column < 80; column++)
  {
    //Make a 16-bit rawvalue from the lepton frame
    int cidx = column << 1;
    uint16_t h8 = leptonFrame[(cidx) + 4];
    uint16_t l8 = leptonFrame[(cidx) + 5];
    uint16_t result = (uint16_t)((h8 << 8) | l8);

    if (result > raw_max)
    {
      raw_max = result;
      max_x = x_base + column;
      max_y = y_base;
    }
    if (result < raw_min)
    {
      raw_min = result;
      min_x = x_base + column;
      min_y = y_base;
    }

    //Invalid value, return
    if (result == 0)
    {
      return false;
    }
    else
    {
      smallBuffer[addr_base + column] = result;
    }

  }

  //Everything worked
  return true;
}

bool WaitForVsync()
{
  uint32_t t = millis();
  vsync_triggered = false;
  while (vsync_triggered == false && (millis() - t) < LEPTON_WAIT_TIMEOUT) {
    yield();
  }
  return vsync_triggered != false;
}

/* Get one frame of raw values from the lepton */
void Lepton::getRawValues()
{
  byte line, error, segment;
  raw_max = 0;
  raw_min = 0xFFFF;
  max_x = 0;
  max_y = 0;
  min_x = 0;
  min_y = 0;
  syncFrame();

  //Go through the segments
  for (segment = 1; segment <= 4; segment++)
  {
    if (WaitForVsync())
    {
      error = 0; //Reset error counter for each segment
      do //Go through one segment, equals 60 lines of 80 values
      {
        for (line = 0; line < 60; line++)
        {
          if (error >= 255) //Maximum error count
          {
            segment = 1; //Reset segment
            error = 0; //Reset error
            reset(); //Reset Lepton lepton_spi
            break; //Restart at line 0
          }

          int retVal = getPackage(line, segment); //Get a package from the lepton

          //If everythin worked, continue
          if (retVal == 0)
          {
            if (savePackage(line, segment)) continue;
          }

          //Raise lepton error
          error++;

          ESP_DelayUS(900);
          /*
          unsigned long T = micros();
          //Stabilize framerate
          uint32_t time = micros();
          while ((micros() - time) < 800)
            __asm__ volatile ("nop");
          printf("T = %ld\n", micros() - T);
          */
          break;
        }
      } while (line != 60);
    }
    else
    {
      Serial.println("Lepton Vsync Failed");
      //Raise lepton error
      error++;
      if (error >= 3) //Maximum error count
      {
        segment = 1; //Reset segment
        error = 0; //Reset error
        reset(); //Reset Lepton lepton_spi
        break;
      }
    }
  }

  doGetCommand(CMD_SYS_FPA_TEMPERATURE_KELVIN, (uint16_t*)&fpa_temp);
  doGetCommand(CMD_SYS_AUX_TEMPERATURE_KELVIN, (uint16_t*)&aux_temp);

  //End lepton_spi Transmission
  end();
}

uint8_t Lepton::state_machine_inner()
{
    uint8_t ret = LEPSMRET_NONE;
    uint8_t sts_reg;
    switch (sm_state)
    {
        case LEPSTATE_RST0:
            if (sm_init_err > 3) {
                ret = LEPSMRET_ERROR;
                return ret;
            }
            Serial.print("R");
            sm_sync_err = 0;
            pinMode(LEPTON_RESET_PIN, OUTPUT);
            digitalWrite(LEPTON_RESET_PIN, HIGH);
            pinMode(_syncPin, INPUT); //vsync
            attachInterrupt(_syncPin, onVsyncISR, RISING);
            pinMode(_ssPin, OUTPUT);
            digitalWrite(_ssPin, HIGH);
            sm_time = millis();
            sm_state = LEPSTATE_RST1;
            break;
        case LEPSTATE_RST1:
            if ((millis() - sm_time) >= 100)
            {
                digitalWrite(LEPTON_RESET_PIN, LOW);
                sm_state = LEPSTATE_RST2;
                sm_time = millis();
            }
            break;
        case LEPSTATE_RST2:
            if ((millis() - sm_time) >= 300)
            {
                lepton_spi->begin(2, 25, 34, 35);
                digitalWrite(LEPTON_RESET_PIN, HIGH);
                sm_state = LEPSTATE_RST3;
                sm_time = millis();
            }
            break;
        case LEPSTATE_RST3:
            if ((millis() - sm_time) >= 1000)
            {
                _i2cAvail = true;
                sm_state = LEPSTATE_INIT_BOOT;
                sm_time = millis();
            }
            break;
        case LEPSTATE_INIT_BOOT:
            if (((sts_reg = readRegister(REG_STATUS)) & STATUS_BIT_BOOT_STATUS) != 0 && _i2cAvail)
            {
                sm_state = LEPSTATE_INIT_SYNC;
                sm_time = millis();
            }
            else if ((millis() - sm_time) > 5000 || _i2cAvail == false)
            {
                ret = LEPSMRET_ERROR;
                sm_state = LEPSTATE_RST0;
                sm_init_err++;
            }
            break;
        case LEPSTATE_INIT_SYNC:
            if (((sts_reg = readRegister(REG_STATUS)) & STATUS_BIT_BUSY) == 0 && _i2cAvail)
            {
                //lepton_spi->beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE3));
                //digitalWrite(_ssPin, LOW);
                sm_state = LEPSTATE_INIT_SYNC_WAIT;
                sm_time = millis();
            }
            else if ((millis() - sm_time) > 5000 || _i2cAvail == false)
            {
                ret = LEPSMRET_ERROR;
                sm_state = LEPSTATE_RST0;
                sm_init_err++;
            }
            break;
        case LEPSTATE_INIT_SYNC_WAIT:
            if ((millis() - sm_time) >= 1000)
            {
                sm_state = LEPSTATE_INIT_CMD1;
                sm_time = millis();
            }
            break;
        case LEPSTATE_INIT_CMD1:
        case LEPSTATE_INIT_CMD2:
            if (((sts_reg = readRegister(REG_STATUS)) & STATUS_BIT_BUSY) == 0 && _i2cAvail)
            {
                if (sm_state == LEPSTATE_INIT_CMD1) {
                    uint16_t SYNC = 5 * 1;
                    doSetCommand(CMD_OEM_SYNC_SET, &SYNC, 1, false);
                }
                else if (sm_state == LEPSTATE_INIT_CMD2) {
                    uint16_t DELAY = 3 * 1;
                    doSetCommand(CMD_OEM_DELAY_SET, &DELAY, 1, false);
                }
                sm_state++;
                sm_time = millis();
            }
            else if ((millis() - sm_time) > 1000 || _i2cAvail == false)
            {
                Serial.printf("X 0x%04X ", sts_reg);
                //end();
                sm_state = LEPSTATE_RST0;
                ret = LEPSMRET_ERROR;
                sm_init_err++;
            }
            break;
        case LEPSTATE_INIT_CMD_END:
            if (((sts_reg = readRegister(REG_STATUS)) & STATUS_BIT_BUSY) == 0 && _i2cAvail)
            {
                //end();
                Serial.printf("lepton init done\r\n");
                sm_frm_cnt = 0;
                sm_state = LEPSTATE_INIT_DONE;
                sm_time = millis();
            }
            else if ((millis() - sm_time) > 5000 || _i2cAvail == false)
            {
                //end();
                sm_state = LEPSTATE_RST0;
                ret = LEPSMRET_ERROR;
            }
            break;
        case LEPSTATE_INIT_DONE:
            if ((millis() - sm_time) >= 1000)
            {
                sm_state = LEPSTATE_SYNCFRAME;
            }
            break;
        case LEPSTATE_SYNCFRAME:
            // reset variables for frame
            // do not try to read frame during transfer
            raw_max = 0;
            raw_min = 0xFFFF;
            max_x = 0;
            max_y = 0;
            min_x = 0;
            min_y = 0;
            sm_time_frm = millis(); // use this time for the frame rate
        case LEPSTATE_RESYNCFRAME:
            vsync_triggered = false;
            sm_line = -1;
            sm_line_err = 0;
            lepton_spi->beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE3));
            digitalWrite(_ssPin, LOW);
            sm_state = LEPSTATE_SYNCFRAME_WAIT;
            ret = LEPSMRET_LOOP;
            sm_time_us = ESP_GetUS();
            break;
            ESP_DelayUS(20);
        case LEPSTATE_SYNCFRAME_WAIT:
            if ((ESP_GetUS() - sm_time_us) >= 20)
            {
                sm_state = LEPSTATE_VSYNC1;
                sm_time = millis();
            }
            ret = LEPSMRET_LOOP;
            break;
        case LEPSTATE_VSYNC1:
        case LEPSTATE_VSYNC2:
        case LEPSTATE_VSYNC3:
        case LEPSTATE_VSYNC4:
            if ((millis() - sm_time_frm) >= 200)
            {
                sm_state = LEPSTATE_VSYNC_CMD1;
                sm_time = millis();
            }
            else if (sm_line >= 0) // iteration has started
            {
                if (sm_line_err >= 255)
                {
                    sm_line_err = 0;
                    end(); // will restart SPI later
                    sm_state_next = LEPSTATE_RESYNCFRAME; // cache state
                    sm_state = LEPSTATE_RESYNC_DLY; // 186 ms delay
                    sm_time = millis();
                    break;
                }

                ret = LEPSMRET_LOOP;
                int seg = (sm_state - LEPSTATE_VSYNC1) + 1; // determine seg number (1 to 4) from state variable
                int retVal = getPackage((byte)sm_line, (byte)seg);
                if (retVal == 0)
                {
                    bool saveRet = savePackage((byte)sm_line, (byte)seg);
                    if (saveRet == false) {
                        retVal = 9;
                    }
                    if (saveRet)
                    {
                        //Serial.print(".");
                    }
                    else
                    {
                        //Serial.print("-");
                    }
                }
                else
                {
                    //Serial.printf("%u", retVal);
                }
                if (retVal != 0) // error
                {
                    sm_line_err++;
                    sm_state_next = sm_state; // cache state
                    sm_state = LEPSTATE_BAD_LINE; // perform short wait
                    sm_time_us = ESP_GetUS();     // perform short wait
                    sm_line = 0; // restart iteration
                    break;
                }
                else
                {
                    sm_line++; // next line
                    if (sm_line >= 60) // all lines done
                    {
                        sm_line = -1; // stop iteration
                        sm_state++;   // next segment (or go do the commands)
                        sm_time = millis();
                        vsync_triggered = false; // wait for next edge
                    }
                }
            }
            else if (vsync_triggered != false) // wait for next edge
            {
                //Serial.print("!");
                sm_line = 0; // starts the iteration through all 60 lines
                //vsync_triggered = false;
                sm_line_err = 0;
                ret = LEPSMRET_LOOP;
            }
            else if ((millis() - sm_time) >= 1000) // timeout waiting for vsync
            {
                #if 0
                end();
                ret = LEPSMRET_ERROR;
                sm_sync_err++;
                if (sm_sync_err < 10)
                {
                    sm_state = LEPSTATE_RESYNC_DLY; // 186 ms delay
                    sm_state_next = LEPSTATE_SYNCFRAME;
                    sm_time = millis();
                    break;
                }
                else
                {
                    Serial.printf("lepton sync err give up\r\n");
                    sm_state = LEPSTATE_RST0;
                }
                #else
                Serial.print("?");
                sm_line = 0;
                sm_line_err = 0;
                ret = LEPSMRET_LOOP;
                #endif
            }
            break;
        case LEPSTATE_VSYNC_CMD1:
            end();
        case LEPSTATE_VSYNC_CMD2:
            ret = LEPSMRET_LOOP;
            doGetCommandStart(sm_state == LEPSTATE_VSYNC_CMD1 ? CMD_SYS_FPA_TEMPERATURE_KELVIN : CMD_SYS_AUX_TEMPERATURE_KELVIN);
            sm_state++;
            sm_time = millis();
            break;
        case LEPSTATE_VSYNC_CMD1_WAIT:
        case LEPSTATE_VSYNC_CMD2_WAIT:
            ret = LEPSMRET_LOOP;
            if (((sts_reg = readRegister(REG_STATUS)) & STATUS_BIT_BUSY) == 0 && _i2cAvail)
            {
                if (readData((uint16_t*)((sm_state == LEPSTATE_VSYNC_CMD1_WAIT) ? (&fpa_temp) : (&aux_temp))) > 0) {
                    sm_state++;
                    break;
                }
                else {
                    sm_state = LEPSTATE_RST0;
                    ret = LEPSMRET_ERROR;
                }
            }
            else if ((millis() - sm_time) > 5000 || _i2cAvail == false)
            {
                sm_state = LEPSTATE_RST0;
                ret = LEPSMRET_ERROR;
            }
            break;
        case LEPSTATE_VSYNC_END:
            _new_data = true;
            sm_frm_cnt++;
            ret = LEPSMRET_NEWDATA;
            sm_state = LEPSTATE_VSYNC_END_DELAY;
            sm_time = millis();
            break;
        case LEPSTATE_VSYNC_END_DELAY:
            if ((millis() - sm_time_frm) >= (1060/27)) // should be 1/27 of a second
            {
                sm_sync_err = 0;
                sm_line_err = 0;
                sm_state = LEPSTATE_SYNCFRAME;
                sm_time = millis();
            }
            break;
        case LEPSTATE_BAD_LINE:
            if ((ESP_GetUS() - sm_time_us) >= 900)
            {
                sm_state = sm_state_next; // this should go back to one of the vsync wait states
            }
            break;
        case LEPSTATE_RESYNC_DLY:
            if ((millis() - sm_time) >= 186) // datasheet "8.2.2.3.1 Establishing/Re-Establishing Sync" says 185 msec
            {
                sm_state = sm_state_next;
                sm_time = millis();
            }
            break;
        default:
            sm_state = LEPSTATE_RST0;
            ret = LEPSMRET_ERROR;
            break;
    }

    if (ret == LEPSMRET_NONE && sm_state <= LEPSTATE_INIT_DONE) {
        ret = LEPSMRET_INIT;
    }
    return ret;
}

uint8_t Lepton::state_machine()
{
    uint8_t r;
    do
    {
        r = state_machine_inner();
    }
    while (r == LEPSMRET_LOOP);
    return r;
}

void Lepton::reset()
{
  end();
  delay(186);
  syncFrame();
}

void Lepton::syncFrame()
{
  lepton_spi->beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE3));
  digitalWrite(_ssPin, LOW);
  ESP_DelayUS(20);
}

void Lepton::end()
{
  digitalWrite(_ssPin, HIGH);
  lepton_spi->endTransaction();
}

int Lepton::readFrame(uint16_t* data)
{
  for (byte segment = 1; segment <= 4; segment++){
    uint16_t row = 0;
    uint16_t id = waitNextFrame();
    while ((id & 0xfff) == row) {
      uint16_t crc = readFrameWord();
      for (int col = 0; col < 80; col++) {
        data[(segment - 1) * 4800 + row * 80 + col] = readFrameWord();
      }

      if ((row == 20)){
      //byte seg = (id >> 12);
      //if (seg == 0)
       // return 1;
      ///if (segment != seg)
       // return 2;
      }
      //Serial.printf("row = %d, segment = %d,  id = %d,id_row = %d\n", row, segment, id >> 12, id & 0xfff);
      row++;
      if (row < 60) {
        id = readFrameWord();
      } else {
        //return 1;
        break;
      }
      //Serial.printf("................readFrame ended with row %4x != id %4x\n", row, id);
    }

  }
  return 0;
}

int Lepton::read_160x120_Frame(uint16_t* data) {
  uint16_t row = 0;
  uint16_t id = waitNextFrame();
  while ((id & 0xfff) == row) {
    uint16_t crc = readFrameWord();
    for (int col = 0; col < 80; col++) {
      data[row * 80 + col] = readFrameWord();
    }

   // Serial.printf(" %d %d %d\n",  (id & 0x7000)>>12 ,(id & 0xfff) , row);
    row++;
    if (row < 60) {
      id = readFrameWord();
    } else {
      return 1;
    }
    //Serial.printf("................readFrame ended with row %4x != id %4x\n", row, id);
  }
  //Serial.printf("readFrame ended with row %4x != id %4x\n", row, id);
  return 0;
}



void Lepton::readFrameRaw(uint16_t* data) {
  data[0] = waitNextFrame();
  for (int i = 1; i < 82 * 60; i++) {
    data[i] = readFrameWord();
  }
}

void Lepton::startTransmission(uint16_t reg) {
  Wire1.beginTransmission(DEVICE_ID);
  transmitWord(reg);
}

void Lepton::transmitWord(uint16_t value) {
  Wire1.write((value >> 8) & 0xff);
  Wire1.write(value & 0xff);
}

void Lepton::endTransmission() {
  int8_t error = Wire1.endTransmission();
  if (error != 0) {
    _i2cAvail = false;
  }
}

uint16_t Lepton::readWord() {
  uint16_t value = Wire1.read() << 8;
  value |= Wire1.read();
  return value;
}

void Lepton::setRegister(uint16_t reg) {
  startTransmission(reg);
  endTransmission();
}

bool Lepton::waitIdle() {
  uint32_t t = millis();
  uint16_t r;
  while (((r = readRegister(REG_STATUS)) & STATUS_BIT_BUSY) != 0 && (millis() - t) < LEPTON_WAIT_TIMEOUT) {
    yield();
  }
  return (r & STATUS_BIT_BUSY) == 0;
}

uint16_t Lepton::readData(uint16_t* data) {
  uint16_t dataLen = readRegister(REG_DATA_LEN) / 2; // The data sheet says the data length register is in 16-bit words, but it actually seems to be in bytes
  setRegister(REG_DATA_BASE);
  Wire1.requestFrom(DEVICE_ID, (uint8_t)(dataLen * 2));
  for (int i = 0; i < dataLen; i++) {
    data[i] = readWord();
  }
  // TODO Check CRC
  return dataLen;
}

void Lepton::writeData(uint16_t* data, uint16_t dataLen) {
  startTransmission(REG_DATA_LEN);
  transmitWord(dataLen);
  for (int i = 0; i < dataLen; i++) {
    transmitWord(data[i]);
  }
  endTransmission();
}

uint16_t Lepton::readFrameWord() {
  uint16_t data = lepton_spi->transfer(0x00) << 8;
  data |= lepton_spi->transfer(0x00);
  return data;
}

uint16_t Lepton::waitNextFrame() {
  uint16_t id = readFrameWord();
  while ((id & 0x0f00) == 0x0f00) {
    for (int i = 0; i < 81; i++) {
      readFrameWord();
    }
    id = readFrameWord();
  }
  return id;
}

uint16_t Lepton::wait_160X120_NextFrame() {
  uint16_t seg = 0;
  for(int i = 0; i < 82 * 60; i++)
  {
       seg = readFrameWord();
       //Serial.printf("seg =  %d ", (seg & 0x7000)>>12);Serial.printf("id =  %d\n", seg & 0x0fff);
  }
  return 0;
}

void Lepton::dumpHex(uint16_t *data, int dataLen) {
  for (int i = 0; i < dataLen; i++) {
    Serial.printf("%4X ", data[i]);
  }
  Serial.println();
}
