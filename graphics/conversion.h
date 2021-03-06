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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/graphics/conversion.h $
 * $Id: conversion.h 47541 2010-01-25 01:39:44Z lordhoto $
 *
 */

#ifndef GRAPHICS_CONVERSION_H
#define GRAPHICS_CONVERSION_H

#include "common/util.h"
#include "graphics/pixelformat.h"

namespace Graphics {

/** Converting a color from YUV to RGB colorspace. */
inline static void YUV2RGB(byte y, byte u, byte v, byte &r, byte &g, byte &b) {
	r = CLIP<int>(y + ((1357 * (v - 128)) >> 10), 0, 255);
	g = CLIP<int>(y - (( 691 * (v - 128)) >> 10) - ((333 * (u - 128)) >> 10), 0, 255);
	b = CLIP<int>(y + ((1715 * (u - 128)) >> 10), 0, 255);
}

/** Converting a color from RGB to YUV colorspace. */
inline static void RGB2YUV(byte r, byte g, byte b, byte &y, byte &u, byte &v) {
	y = CLIP<int>( ((r * 306) >> 10) + ((g * 601) >> 10) + ((b * 117) >> 10)      , 0, 255);
	u = CLIP<int>(-((r * 172) >> 10) - ((g * 340) >> 10) + ((b * 512) >> 10) + 128, 0, 255);
	v = CLIP<int>( ((r * 512) >> 10) - ((g * 429) >> 10) - ((b *  83) >> 10) + 128, 0, 255);
}

/** Converting a color from YUV to RGB colorspace, Cinepak style. */
inline static void CPYUV2RGB(byte y, byte u, byte v, byte &r, byte &g, byte &b) {
	r = CLIP<int>(y + 2 * (v - 128), 0, 255);
	g = CLIP<int>(y - (u - 128) / 2 - (v - 128), 0, 255);
	b = CLIP<int>(y + 2 * (u - 128), 0, 255);
}

// TODO: generic YUV to RGB blit

/**
 * Blits a rectangle from one graphical format to another.
 *
 * @param dstbuf	the buffer which will recieve the converted graphics data
 * @param srcbuf	the buffer containing the original graphics data
 * @param dstpitch	width in bytes of one full line of the dest buffer
 * @param srcpitch	width in bytes of one full line of the source buffer
 * @param w			the width of the graphics data
 * @param h			the height of the graphics data
 * @param dstFmt	the desired pixel format
 * @param srcFmt	the original pixel format
 * @return			true if conversion completes successfully,
 *					false if there is an error.
 *
 * @note This implementation currently arbitrarily requires that the
 *		 destination's format have at least as high a bytedepth as
 *		 the source's.
 * @note This can convert a rectangle in place, if the source and
 *		 destination format have the same bytedepth.
 *
 */
bool crossBlit(byte *dst, const byte *src, int dstpitch, int srcpitch,
						int w, int h, const Graphics::PixelFormat &dstFmt, const Graphics::PixelFormat &srcFmt);

} // End of namespace Graphics

#endif // GRAPHICS_CONVERSION_H
