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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/backends/platform/wince/CEScaler.cpp $
 * $Id: CEScaler.cpp 45975 2009-11-18 15:22:15Z fingolfin $
 *
 */
#include "graphics/scaler/intern.h"
#include "CEScaler.h"

template<int bitFormat>
void PocketPCPortraitTemplate(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height) {
	uint16 *work;

	// Various casts below go via (void *) to avoid warning. This is
	// safe as these are all even addresses.
	while (height--) {
		work = (uint16 *)(void *)dstPtr;

		for (int i=0; i<width; i+=4) {
			// Work with 4 pixels
			uint16 color1 = *(((const uint16 *)(const void *)srcPtr) + i);
			uint16 color2 = *(((const uint16 *)(const void *)srcPtr) + (i + 1));
			uint16 color3 = *(((const uint16 *)(const void *)srcPtr) + (i + 2));
			uint16 color4 = *(((const uint16 *)(const void *)srcPtr) + (i + 3));

			work[0] = interpolate32_3_1<bitFormat>(color1, color2);
			work[1] = interpolate32_1_1<bitFormat>(color2, color3);
			work[2] = interpolate32_3_1<bitFormat>(color4, color3);

			work += 3;
		}
		srcPtr += srcPitch;
		dstPtr += dstPitch;
	}
}
MAKE_WRAPPER(PocketPCPortrait)

void PocketPCLandscapeAspect(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height) {

	const int redblueMasks[] = { 0x7C1F, 0xF81F };
	const int greenMasks[] = { 0x03E0, 0x07E0 };
	const int RBM = redblueMasks[gBitFormat == 565];
	const int GM = greenMasks[gBitFormat == 565];

	int i,j;
	unsigned int p1, p2;
	const uint8 *inbuf, *instart;
	uint8 *outbuf, *outstart;

#define RB(x) ((x & RBM)<<8)
#define G(x)  ((x & GM)<<3)

#define P20(x) (((x)>>2)-((x)>>4))
#define P40(x) (((x)>>1)-((x)>>3))
#define P60(x) (((x)>>1)+((x)>>3))
#define P80(x) (((x)>>1)+((x)>>2)+((x)>>4))

#define MAKEPIXEL(rb,g) ((((rb)>>8) & RBM | ((g)>>3) & GM))

	inbuf = (const uint8 *)srcPtr;
	outbuf = (uint8 *)dstPtr;
	height /= 5;

	// Various casts below go via (void *) to avoid warning. This is
	// safe as these are all even addresses.
	for (i = 0; i < height; i++) {
		instart = inbuf;
		outstart = outbuf;
		for (j=0; j < width; j++) {

			p1 = *(const uint16*)(const void *)inbuf; inbuf += srcPitch;
			*(uint16*)(void *)outbuf = p1; outbuf += dstPitch;

			p2 = *(const uint16*)(const void *)inbuf; inbuf += srcPitch;
			*(uint16*)(void *)outbuf = MAKEPIXEL(P20(RB(p1))+P80(RB(p2)),P20(G(p1))+P80(G(p2)));  outbuf += dstPitch;

			p1 = p2;
			p2 = *(const uint16*)(const void *)inbuf; inbuf += srcPitch;
			*(uint16*)(void *)outbuf = MAKEPIXEL(P40(RB(p1))+P60(RB(p2)),P40(G(p1))+P60(G(p2)));  outbuf += dstPitch;

			p1 = p2;
			p2 = *(const uint16*)(const void *)inbuf; inbuf += srcPitch;
			*(uint16*)(void *)outbuf = MAKEPIXEL(P60(RB(p1))+P40(RB(p2)),P60(G(p1))+P40(G(p2)));  outbuf += dstPitch;

			p1 = p2;
			p2 = *(const uint16*)(const void *)inbuf;
			*(uint16*)(void *)outbuf = MAKEPIXEL(P80(RB(p1))+P20(RB(p2)),P80(G(p1))+P20(G(p2)));  outbuf += dstPitch;

			*(uint16*)(void *)outbuf = p2;

			inbuf = inbuf - srcPitch*4 + sizeof(uint16);
			outbuf = outbuf - dstPitch*5 + sizeof(uint16);
		}
		inbuf = instart + srcPitch*5;
		outbuf = outstart + dstPitch*6;
	}
}

#ifdef ARM
extern "C" {
	void SmartphoneLandscapeARM(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height, int mask);
}

void SmartphoneLandscape(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height) {
	// Rounding constants and masks used for different pixel formats
	static const int redbluegreenMasks[] = { 0x03E07C1F, 0x07E0F81F };
	const int maskUsed = (gBitFormat == 565);
	SmartphoneLandscapeARM(srcPtr, srcPitch, dstPtr, dstPitch, width, height, redbluegreenMasks[maskUsed]);
}

#else

template<int bitFormat>
void SmartphoneLandscapeTemplate(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height) {
	uint8 *work;
	int line = 0;

	while (height--) {
		work = dstPtr;

		for (int i = 0; i < width; i += 3) {
			// Filter 2/3
			uint16 color1 = *(((const uint16 *)srcPtr) + i);
			uint16 color2 = *(((const uint16 *)srcPtr) + (i + 1));
			uint16 color3 = *(((const uint16 *)srcPtr) + (i + 2));

			*(((uint16 *)work) + 0) = interpolate32_3_1<bitFormat>(color1, color2);
			*(((uint16 *)work) + 1) = interpolate32_3_1<bitFormat>(color3, color2);

			work += 2 * sizeof(uint16);
		}
		srcPtr += srcPitch;
		dstPtr += dstPitch;
		line++;
		if (line == 7) {
			line = 0;
			srcPtr += srcPitch;
			height--;
		}
	}
}
MAKE_WRAPPER(SmartphoneLandscape)

#endif

