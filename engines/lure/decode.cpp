/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/lure/decode.cpp $
 * $Id: decode.cpp 44634 2009-10-04 21:26:33Z fingolfin $
 *
 */

#include "lure/decode.h"
#include "lure/lure.h"
#include "lure/memory.h"
#include "lure/luredefs.h"
#include "common/endian.h"

namespace Lure {

/*--------------------------------------------------------------------------*/
/* PictureDecoder class                                                     */
/*                                                                          */
/* Provides the functionality for decoding screens                          */
/*--------------------------------------------------------------------------*/

void PictureDecoder::writeByte(MemoryBlock *dest, byte v) {
	if (outputOffset == dest->size())
		error("Decoded data exceeded allocated output buffer size");
	dest->data()[outputOffset++] = v;
}

void PictureDecoder::writeBytes(MemoryBlock *dest, byte v, uint16 numBytes) {
	if (outputOffset + numBytes > dest->size())
		error("Decoded data exceeded allocated output buffer size");
	dest->setBytes(v, outputOffset, numBytes);
	outputOffset += numBytes;
}

byte PictureDecoder::DSSI(bool incr) {
	if (dataPos > dataIn->size())
		error("PictureDecoder went beyond end of source data");

	byte result = (dataPos == dataIn->size()) ? 0 :
		dataIn->data()[dataPos];
	if (incr) ++dataPos;
	return result;
}

byte PictureDecoder::ESBX(bool incr) {
	if (dataPos2 >= dataIn->size())
		error("PictureDecoder went beyond end of source data");

	byte result = dataIn->data()[dataPos2];
	if (incr) ++dataPos2;
	return result;
}

void PictureDecoder::decrCtr() {
	--CL;
	if (CL == 0) {
		CH = ESBX();
		CL = 8;
	}
}

bool PictureDecoder::shlCarry() {
	bool result = (CH & 0x80) != 0;
	CH <<= 1;
	return result;
}

// decode
// Decodes a compressed Lure of the Temptress screen

MemoryBlock *PictureDecoder::decode(MemoryBlock *src, uint32 maxOutputSize) {
	bool isEGA = LureEngine::getReference().isEGA();
	return isEGA ? egaDecode(src, maxOutputSize) : vgaDecode(src, maxOutputSize);
}

// egaDecode
// Takes care of decoding a compressed EGA screen

#define READ_BIT_DX { bitFlag = (dx & 0x8000) != 0; dx <<= 1; if (--bitCtr == 0) { dx = (dx & 0xff00) | DSSI(); bitCtr = 8; } }
#define READ_BITS(loops) for (int ctr = 0; ctr < loops; ++ctr) READ_BIT_DX

MemoryBlock *PictureDecoder::egaDecode(MemoryBlock *src, uint32 maxOutputSize) {
	MemoryBlock *dest = Memory::allocate(maxOutputSize);
	byte popTable[32 + 128];
	uint8 al;
	bool bitFlag;

	// Set up initial states
	dataIn = src;
	dataPos = 6;

	uint16 dx = READ_BE_UINT16(src->data() + dataPos);
	dataPos += sizeof(uint16);
	int bitCtr = 8;

	// Decode the colour popularity table

	for (int nibbleCtr = 0; nibbleCtr < 32; ++nibbleCtr) {
		for (int byteCtr = 0; byteCtr < 128; byteCtr += 32) {
			popTable[nibbleCtr + byteCtr] = dx >> 11;
			READ_BITS(5);
		}
	}

	// ok, on to the real thing

	outputOffset = 0;
	al = dx >> 11;
	writeByte(dest, al);
	READ_BITS(5);

	uint16 tableOffset = al;
	uint8 v = 0;

	for (;;) {
		READ_BIT_DX

		if (!bitFlag) {
			// Get the favourite colour
			v = popTable[tableOffset];

		} else {

			READ_BIT_DX

			if (bitFlag) {
				// Get another bit
				READ_BIT_DX

				if (bitFlag) {
					// We have no favourite. Could this be a repeat?
					al = dx >> 11;
					READ_BITS(5);

					if (al == popTable[tableOffset]) {
						// Repeat 16 bits
						uint16 numLoops = dx & 0xff00;
						READ_BITS(8);
						numLoops |= (dx >> 8);
						READ_BITS(8);

						if (numLoops == 0)
							// Finished decoding
							break;
						writeBytes(dest, al, numLoops);
						continue;

					} else if (al == popTable[tableOffset + 32]) {
						// Repeat 8 bits
						writeBytes(dest, tableOffset, dx >> 8);
						READ_BITS(8);
						continue;

					} else if (al == popTable[tableOffset + 64]) {
						// Repeat 6 bits
						writeBytes(dest, tableOffset, dx >> 10);
						READ_BITS(6);
						continue;

					} else if (al == popTable[tableOffset + 96]) {
						// Repeat 5 bits
						writeBytes(dest, tableOffset, dx >> 11);
						READ_BITS(5);
						continue;

					} else {
						// It's a new colour
						v = al;
					}

				} else {
					// Fourth favourite
					v = popTable[tableOffset + 96];
				}

			} else {
				// Get another bit
				READ_BIT_DX

				if (bitFlag) {
					// Third favourite
					v = popTable[tableOffset + 64];
				} else {
					// Second favourite
					v = popTable[tableOffset + 32];
				}
			}
		}

		tableOffset = v;
		writeByte(dest, v);
	}

	// Resize the output to be the number of outputed bytes and return it
	if (outputOffset < dest->size()) dest->reallocate(outputOffset);

	return dest;
}

// vgaDecode
// Takes care of decoding a compressed vga screen

MemoryBlock *PictureDecoder::vgaDecode(MemoryBlock *src, uint32 maxOutputSize) {
	MemoryBlock *dest = Memory::allocate(maxOutputSize);

	// Set up initial states
	dataIn = src;
	outputOffset = 0;
	dataPos = READ_LE_UINT32(dataIn->data() + 0x400);
	dataPos2 = 0x404;

	CH = ESBX();
	CL = 9;

Loc754:
	AL = DSSI();
	writeByte(dest, AL);
	BP = ((uint16) AL) << 2;

Loc755:
	decrCtr();
	if (shlCarry()) goto Loc761;
	decrCtr();
	if (shlCarry()) goto Loc759;
	AL = dataIn->data()[BP];

Loc758:
	writeByte(dest, AL);
	BP = ((uint16) AL) << 2;
	goto Loc755;

Loc759:
	AL = (byte) (BP >> 2);
	AH = DSSI();
	if (AH == 0) goto Loc768;

	writeBytes(dest, AL, AH);
	goto Loc755;

Loc761:
	decrCtr();
	if (shlCarry()) goto Loc765;
	decrCtr();

	if (shlCarry()) goto Loc764;
	AL = dataIn->data()[BP+1];
	goto Loc758;

Loc764:
	AL = dataIn->data()[BP+2];
	goto Loc758;

Loc765:
	decrCtr();
	if (shlCarry()) goto Loc767;
	AL = dataIn->data()[BP+3];
	goto Loc758;

Loc767:
	goto Loc754;

Loc768:
	AL = DSSI();
	if (AL != 0) goto Loc755;

	// Resize the output to be the number of outputed bytes and return it
	if (outputOffset < dest->size()) dest->reallocate(outputOffset);
	return dest;
}

/*--------------------------------------------------------------------------*/
/* AnimationDecoder class                                                   */
/*                                                                          */
/* Provides the functionality for decoding animations                       */
/*--------------------------------------------------------------------------*/

// The code below is responsible for decompressing the pixel data
// for an animation. I'm not currently sure of the of the exact details
// of the compression format - for now I've simply copied the code
// from the executable

void AnimationDecoder::rcl(uint16 &value, bool &carry) {
	bool result = (value & 0x8000) != 0;
	value = (value << 1) + (carry ? 1 : 0);
	carry = result;
}

#define GET_BYTE currData = (currData & 0xff00) | *pSrc++
#define BX_VAL(x) *((byte *) (dest->data() + tableOffset + x))
#define SET_HI_BYTE(x,v) x = (x & 0xff) | ((v) << 8);
#define SET_LO_BYTE(x,v) x = (x & 0xff00) | (v);

void AnimationDecoder::decode_data_2(MemoryBlock *src, byte *&pSrc, uint16 &currData,
									 uint16 &bitCtr, uint16 &dx, bool &carry) {
	SET_HI_BYTE(dx, currData >> 8);

	for (int v = 0; v < 8; ++v) {
		rcl(currData, carry);
		if (--bitCtr == 0) {
			uint32 offset = (uint32) (pSrc - src->data());
			if (offset >= src->size())
				// Beyond end of source, so read in a 0 value
				currData &= 0xff00;
			else
				GET_BYTE;
			bitCtr = 8;
		}
	}
}

uint32 AnimationDecoder::decode_data(MemoryBlock *src, MemoryBlock *dest, uint32 srcPos) {
	byte *pSrc = src->data() + srcPos;
	byte *pDest = dest->data();
	uint16 v;
	bool carry = false;
	uint16 currData, bitCtr, dx;
	byte tableOffset;
	uint16 tempReg1, tempReg2;

	// Handle splitting up 16 bytes into individual nibbles
	for (int numBytes = 0; numBytes < 16; ++numBytes, ++pDest) {
		// Split up next byte to pDest and pDest+0x10
		currData = *pSrc++;
		*(pDest + 0x10) = currData & 0xf;
		*pDest = (currData >> 4) & 0xf;

		// Split up next byte to pDest+0x20 and pDest+0x30
		currData = *pSrc++;
		*(pDest + 0x30) = currData & 0xf;
		*(pDest + 0x20) = (currData >> 4) & 0xf;
	}

	pDest = (byte *) (dest->data() + 0x40);
	currData = READ_BE_UINT16(pSrc);
	pSrc += sizeof(uint16);

	bitCtr = 4;
	*pDest = (currData >> 8) & 0xf0;
	tableOffset = currData >> 12;
	currData <<= 4;
	dx = 1;

	for (;;) {
		carry = false;
		rcl(currData, carry);
		if (--bitCtr == 0) {
			GET_BYTE;
			bitCtr = 8;
		}
		if (carry) goto loc_1441;
		tableOffset = BX_VAL(0);

loc_1439:
		dx ^= 1;
		if ((dx & 1) != 0) {
			SET_HI_BYTE(dx, tableOffset << 4);
			*pDest = dx >> 8;
		} else {
			*pDest++ |= tableOffset;
		}
		continue;

loc_1441:
		rcl(currData, carry);
		if (--bitCtr == 0) {
			GET_BYTE;
			bitCtr = 8;
		}
		if (!carry) {
			rcl(currData, carry);
			if (--bitCtr == 0) {
				GET_BYTE;
				bitCtr = 8;
			}

			if (!carry) {
				tableOffset = BX_VAL(0x10);
			} else {
				tableOffset = BX_VAL(0x20);
			}
			goto loc_1439;
		}

		rcl(currData, carry);
		if (--bitCtr == 0) {
			GET_BYTE;
			bitCtr = 8;
		}
		if (!carry) {
			tableOffset = BX_VAL(0x30);
			goto loc_1439;
		}

		SET_HI_BYTE(dx, currData >> 12);
		carry = false;
		for (int ctr = 0; ctr < 4; ++ctr) {
			rcl(currData, carry);
			if (--bitCtr == 0) {
				GET_BYTE;
				bitCtr = 8;
			}
		}

		byte dxHigh = dx >> 8;
		if (dxHigh == BX_VAL(0)) {
			tempReg1 = bitCtr;
			tempReg2 = dx;
			decode_data_2(src, pSrc, currData, bitCtr, dx, carry);

			SET_LO_BYTE(dx, dx >> 8);
			decode_data_2(src, pSrc, currData, bitCtr, dx, carry);
			SET_HI_BYTE(bitCtr, dx & 0xff);
			SET_LO_BYTE(bitCtr, dx >> 8);
			dx = tempReg2;

			if (bitCtr == 0)
				// Exit out of infinite loop
				break;

		} else if (dxHigh == BX_VAL(0x10)) {
			tempReg1 = bitCtr;
			decode_data_2(src, pSrc, currData, bitCtr, dx, carry);
			bitCtr = dx >> 8;

		} else if (dxHigh == BX_VAL(0x20)) {
			SET_HI_BYTE(dx, currData >> 10);

			for (v = 0; v < 6; ++v) {
				rcl(currData, carry);
				if (--bitCtr == 0) {
					GET_BYTE;
					bitCtr = 8;
				}
			}

			tempReg1 = bitCtr;
			bitCtr = dx >> 8;

		} else if (dxHigh == BX_VAL(0x30)) {
			SET_HI_BYTE(dx, currData >> 11);

			for (v = 0; v < 5; ++v) {
				rcl(currData, carry);
				if (--bitCtr == 0) {
					GET_BYTE;
					bitCtr = 8;
				}
			}

			tempReg1 = bitCtr;
			bitCtr = dx >> 8;

		} else {
			tableOffset = dx >> 8;
			goto loc_1439;
		}

		if ((dx & 1) == 1) {
			*pDest++ |= tableOffset;
			--bitCtr;
			dx &= 0xfffe;
		}

		SET_HI_BYTE(dx, tableOffset << 4);
		tableOffset |= dx >> 8;

		v = bitCtr >> 1;
		while (v-- > 0) *pDest++ = tableOffset;

		bitCtr &= 1;
		if (bitCtr != 0) {
			*pDest = tableOffset & 0xf0;
			dx |= 1; //dx.l
		}

		bitCtr = tempReg1;
		tableOffset &= 0x0f;
	}

	// Return number of bytes written
	return pDest - dest->data();
}

} // End of namespace Lure
