#include "Arduino.h"
#include "Lepton.h"
#include "Wire.h"
#include "SPI.h"

#define LEPTON_WAIT_TIMEOUT 1000

#define rotationHorizont 0
#define rotationVert 0

static byte leptonFrame[164];
unsigned short  smallBuffer[160*120];
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

static volatile bool vsync_triggered = 0;
static volatile uint64_t vsync_time = 0;

static void IRAM_ATTR onVsyncISR()
{
  vsync_triggered = 1;
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

void Lepton::doSetCommand(uint16_t commandIdBase, uint16_t *data, uint16_t dataLen)
{
  writeData(data, dataLen);
  writeRegister(REG_COMMAND_ID, commandIdBase | TYPE_SET);
  waitIdle();
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
  lepton_spi->transferBytes(NULL,leptonFrame,164);

  if((leptonFrame[0] & 0x0F) == 0x0F)
    return 1;

  //Check if the line number matches the expected line
  if (leptonFrame[1] != line)
   return 2;

  //For the Lepton3.x, check if the segment number matches
  if (line == 20)
  {
    byte segment = (leptonFrame[0] >> 4);
    if (segment == 0)
      return 3;
    if (segment != seg)
      return 4;
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
    uint16_t result = (uint16_t)(leptonFrame[(column << 1) + 4] << 8 | leptonFrame[(column << 1) + 5]);

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
      return 0;
    }
    else
    {
      smallBuffer[addr_base + column] = result;
    }

  }

  //Everything worked
  return 1;
}

bool WaitForVsync()
{
  uint32_t t = millis();
  vsync_triggered = 0;
  while (vsync_triggered == 0 && (millis() - t) < LEPTON_WAIT_TIMEOUT) {
    yield();
  }
  return vsync_triggered != 0;
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

  doGetCommand(CMD_SYS_FPA_TEMPERATURE_KELVIN, &fpa_temp);
  doGetCommand(CMD_SYS_AUX_TEMPERATURE_KELVIN, &aux_temp);

  //End lepton_spi Transmission
  end();
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
  uint8_t error = Wire1.endTransmission();
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
    Serial.printf("%4x ", data[i]);
  }
  Serial.println();
}
