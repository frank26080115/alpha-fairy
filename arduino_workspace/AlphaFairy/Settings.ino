#include "AlphaFairy.h"
#include <EEPROM.h>

extern uint32_t crc32_le(uint32_t crc, uint8_t const *buf, uint32_t len);
extern uint32_t fletcher32(const uint16_t *data, size_t len);

//#define CHECKSUM(_buf, _len) crc32_le(0, (uint8_t*)(_buf), (size_t)(_len))
#define CHECKSUM(_buf, _len) fletcher32((const uint16_t*)(_buf), (size_t)(_len))

configsettings_t config_settings;

void settings_default() {
  config_settings.focus_pause_time_ms   = 100;
  config_settings.shutter_press_time_ms = 200;
  config_settings.manual_focus_return   = 1;
  config_settings.infrared_enabled      = 1;
  config_settings.nine_point_dist       = 200;
}

bool settings_load() {
  EEPROM.begin(sizeof(configsettings_t));
  EEPROM.readBytes(0, (void*)(&config_settings), sizeof(configsettings_t));
  uint32_t crc_calced = CHECKSUM(&config_settings, sizeof(configsettings_t) - sizeof(uint32_t));
  if (config_settings.magic != CONFIGSETTINGS_MAGIC || crc_calced != config_settings.crc32) {
    Serial.printf("settings not valid, magic 0x%08X, CRC 0x%08X ?= 0x%08X\r\n", config_settings.magic, crc_calced, config_settings.crc32);
    return false;
  }
  return true;
}

void settings_init() {
  if (settings_load() == false) {
    settings_default();
  }
}

void settings_save() {
  config_settings.magic = CONFIGSETTINGS_MAGIC;
  uint32_t crc_calced = CHECKSUM(&config_settings, sizeof(configsettings_t) - sizeof(uint32_t));
  EEPROM.writeBytes(0, (const void*)(&config_settings), sizeof(configsettings_t));
  EEPROM.commit();
}

uint32_t fletcher32(const uint16_t *data, size_t len)
{
	uint32_t c0, c1;
	len = (len + 1) & ~1;      /* Round up len to words */

	/* We similarly solve for n > 0 and n * (n+1) / 2 * (2^16-1) < (2^32-1) here. */
	/* On modern computers, using a 64-bit c0/c1 could allow a group size of 23726746. */
	for (c0 = c1 = 0; len > 0; ) {
		size_t blocklen = len;
		if (blocklen > 360*2) {
			blocklen = 360*2;
		}
		len -= blocklen;
		do {
			c0 = c0 + *data++;
			c1 = c1 + c0;
		} while ((blocklen -= 2));
		c0 = c0 % 65535;
		c1 = c1 % 65535;
	}
	return (c1 << 16 | c0);
}
