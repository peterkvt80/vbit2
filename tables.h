#ifndef _TABLES_H_
#define _TABLES_H_
#include <cstdint>

extern const uint8_t ReverseByteTab[256];

extern const uint8_t OddParityTable[128];

extern const uint8_t Hamming8EncodeTable[16];

extern const uint8_t Hamming8DecodeTable[256];

extern const uint8_t Hamming24EncodeTable0[256];
extern const uint8_t Hamming24EncodeTable1[256];
extern const uint8_t Hamming24EncodeTable2[4];
extern const uint8_t Hamming24ParityTable[3][256];

#endif
