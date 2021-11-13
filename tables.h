/** ***************************************************************************
 * Description       : Teletext tables for VBIT 
 * Compiler          : GCC
 *
 * Copyright (C) 2010, Peter Kwan
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 *****************************************************************************/

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