#ifndef _PTPIP_UTILS_H_
#define _PTPIP_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

uint32_t copy_bytes_to_utf16   (void* dest, void* src);
uint32_t copy_utf16_to_bytes   (void* dest, void* src);
void     copyn_utf16_to_bytes  (void* dest, void* src, uint32_t n);
void     buffer_consume        (uint8_t buff[], uint32_t* buff_idx, uint32_t read_cnt, uint32_t buff_max);
void     print_buffer_hex      (uint8_t* data, uint32_t len);
int32_t  decode_chunk_to_int   (uint16_t data_type, uint8_t* data_chunk, uint8_t data_length);
uint32_t decode_chunk_to_uint  (uint16_t data_type, uint8_t* data_chunk, uint8_t data_length);
void     propdecoder_print_hex (uint16_t datatype, uint8_t* dptr, int cnt);
uint8_t  property_data_get_size(uint16_t data_type);

#endif