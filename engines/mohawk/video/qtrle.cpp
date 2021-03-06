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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/mohawk/video/qtrle.cpp $
 * $Id: qtrle.cpp 47541 2010-01-25 01:39:44Z lordhoto $
 *
 */

// QuickTime RLE Decoder
// Based off ffmpeg's QuickTime RLE decoder (written by Mike Melanson)

#include "mohawk/video/qtrle.h"

#include "common/scummsys.h"
#include "common/stream.h"
#include "common/system.h"
#include "graphics/colormasks.h"
#include "graphics/surface.h"

namespace Mohawk {

QTRLEDecoder::QTRLEDecoder(uint16 width, uint16 height, byte bitsPerPixel) : Graphics::Codec() {
	_bitsPerPixel = bitsPerPixel;
	_pixelFormat = g_system->getScreenFormat();

	// We need to increase the surface size to a multiple of 4
	uint16 wMod = width % 4;
	if(wMod != 0)
		width += 4 - wMod;

	debug(2, "QTRLE corrected width: %d", width);

	_surface = new Graphics::Surface();
	_surface->create(width, height, _bitsPerPixel <= 8 ? 1 : _pixelFormat.bytesPerPixel);
}

#define CHECK_STREAM_PTR(n) \
  if ((stream->pos() + n) > stream->size()) { \
    warning ("Problem: stream out of bounds (%d >= %d)", stream->pos() + n, stream->size()); \
    return; \
  }

#define CHECK_PIXEL_PTR(n) \
  if ((int32)pixelPtr + n > _surface->w * _surface->h) { \
    warning ("Problem: pixel ptr = %d, pixel limit = %d", pixelPtr + n, _surface->w * _surface->h); \
    return; \
  } \

void QTRLEDecoder::decode1(Common::SeekableReadStream *stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	byte *rgb = (byte *)_surface->pixels;

	while (linesToChange) {
		CHECK_STREAM_PTR(2);
		byte skip = stream->readByte();
		int8 rleCode = stream->readSByte();

		if (rleCode == 0)
			break;

		if (skip & 0x80) {
			linesToChange--;
			rowPtr += _surface->w;
			pixelPtr = rowPtr + 2 * (skip & 0x7f);
		} else
			pixelPtr += 2 * skip;

		if (rleCode < 0) {
			// decode the run length code
			rleCode = -rleCode;
			// get the next 2 bytes from the stream, treat them as groups of 8 pixels, and output them rleCode times */
			CHECK_STREAM_PTR(2);
			byte pi0 = stream->readByte();
			byte pi1 = stream->readByte();
			CHECK_PIXEL_PTR(rleCode * 2);

			while (rleCode--) {
				rgb[pixelPtr++] = pi0;
				rgb[pixelPtr++] = pi1;
			}
		} else {
			// copy the same pixel directly to output 2 times
			rleCode *= 2;
			CHECK_STREAM_PTR(rleCode);
			CHECK_PIXEL_PTR(rleCode);

			while (rleCode--)
				rgb[pixelPtr++] = stream->readByte();
		}
	}
}

void QTRLEDecoder::decode2_4(Common::SeekableReadStream *stream, uint32 rowPtr, uint32 linesToChange, byte bpp) {
	uint32 pixelPtr = 0;
	byte *rgb = (byte *)_surface->pixels;
	byte numPixels = (bpp == 4) ? 8 : 16;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + (numPixels * (stream->readByte() - 1));

		for (int8 rleCode = stream->readSByte(); rleCode != -1; rleCode = stream->readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += (numPixels * (stream->readByte() - 1));
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;

				// get the next 4 bytes from the stream, treat them as palette indices, and output them rleCode times */
				CHECK_STREAM_PTR(4);

				byte pi[16]; // 16 palette indices

				for (int8 i = numPixels - 1; i >= 0; i--) {
					pi[numPixels - 1 - i] = (stream->readByte() >> ((i * bpp) & 0x07)) & ((1 << bpp) - 1);

					// FIXME: Is this right?
					//stream_ptr += ((i & ((num_pixels>>2)-1)) == 0);
					if ((i & ((numPixels >> 2) - 1)) == 0)
						stream->readByte();
				}

				CHECK_PIXEL_PTR(rleCode * numPixels);

				while (rleCode--)
					for (byte i = 0; i < numPixels; i++)
						rgb[pixelPtr++] = pi[i];
			} else {
				// copy the same pixel directly to output 4 times
				rleCode *= 4;
				CHECK_STREAM_PTR(rleCode);
				CHECK_PIXEL_PTR(rleCode * (numPixels >> 2));

				while (rleCode--) {
					byte temp = stream->readByte();
					if (bpp == 4) {
						rgb[pixelPtr++] = (temp >> 4) & 0x0f;
						rgb[pixelPtr++] = temp & 0x0f;
					} else {
						rgb[pixelPtr++] = (temp >> 6) & 0x03;
						rgb[pixelPtr++] = (temp >> 4) & 0x03;
						rgb[pixelPtr++] = (temp >> 2) & 0x03;
						rgb[pixelPtr++] = temp & 0x03;
					}
				}
			}
		}

		rowPtr += _surface->w;
	}
}

void QTRLEDecoder::decode8(Common::SeekableReadStream *stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	byte *rgb = (byte *)_surface->pixels;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + 4 * (stream->readByte() - 1);

		for (int8 rleCode = stream->readSByte(); rleCode != -1; rleCode = stream->readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += 4 * (stream->readByte() - 1);
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;

				// get the next 4 bytes from the stream, treat them as palette indices, and output them rleCode times
				CHECK_STREAM_PTR(4);

				byte pi[4];  // 4 palette indexes

				for (byte i = 0; i < 4; i++)
					pi[i] = stream->readByte();

				CHECK_PIXEL_PTR(rleCode * 4);

				while (rleCode--)
					for (byte i = 0; i < 4; i++)
						rgb[pixelPtr++] = pi[i];
			} else {
				// copy the same pixel directly to output 4 times
				rleCode *= 4;
				CHECK_STREAM_PTR(rleCode);
				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--)
					rgb[pixelPtr++] = stream->readByte();
			}
		}

		rowPtr += _surface->w;
	}
}

void QTRLEDecoder::decode16(Common::SeekableReadStream *stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	OverlayColor *rgb = (OverlayColor *)_surface->pixels;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + stream->readByte() - 1;

		for (int8 rleCode = stream->readSByte(); rleCode != -1; rleCode = stream->readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream->readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;
				CHECK_STREAM_PTR(2);

				uint16 rgb16 = stream->readUint16BE();

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--) {
					// Convert from RGB555 to the format specified by the Overlay
					byte r = 0, g = 0, b = 0;
					Graphics::colorToRGB<Graphics::ColorMasks<555> >(rgb16, r, g, b);
					rgb[pixelPtr++] = _pixelFormat.RGBToColor(r, g, b);
				}
			} else {
				CHECK_STREAM_PTR(rleCode * 2);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				while (rleCode--) {
					uint16 rgb16 = stream->readUint16BE();

					// Convert from RGB555 to the format specified by the Overlay
					byte r = 0, g = 0, b = 0;
					Graphics::colorToRGB<Graphics::ColorMasks<555> >(rgb16, r, g, b);
					rgb[pixelPtr++] = _pixelFormat.RGBToColor(r, g, b);
				}
			}
		}

		rowPtr += _surface->w;
	}
}

void QTRLEDecoder::decode24(Common::SeekableReadStream *stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	OverlayColor *rgb = (OverlayColor *)_surface->pixels;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + stream->readByte() - 1;

		for (int8 rleCode = stream->readSByte(); rleCode != -1; rleCode = stream->readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream->readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;

				CHECK_STREAM_PTR(3);

				byte r = stream->readByte();
				byte g = stream->readByte();
				byte b = stream->readByte();

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--)
					rgb[pixelPtr++] = _pixelFormat.RGBToColor(r, g, b);
			} else {
				CHECK_STREAM_PTR(rleCode * 3);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				while (rleCode--) {
					byte r = stream->readByte();
					byte g = stream->readByte();
					byte b = stream->readByte();
					rgb[pixelPtr++] = _pixelFormat.RGBToColor(r, g, b);
				}
			}
		}

		rowPtr += _surface->w;
	}
}

void QTRLEDecoder::decode32(Common::SeekableReadStream *stream, uint32 rowPtr, uint32 linesToChange) {
	uint32 pixelPtr = 0;
	OverlayColor *rgb = (OverlayColor *)_surface->pixels;

	while (linesToChange--) {
		CHECK_STREAM_PTR(2);
		pixelPtr = rowPtr + stream->readByte() - 1;

		for (int8 rleCode = stream->readSByte(); rleCode != -1; rleCode = stream->readSByte()) {
			if (rleCode == 0) {
				// there's another skip code in the stream
				CHECK_STREAM_PTR(1);
				pixelPtr += stream->readByte() - 1;
			} else if (rleCode < 0) {
				// decode the run length code
				rleCode = -rleCode;

				CHECK_STREAM_PTR(4);

				byte a = stream->readByte();
				byte r = stream->readByte();
				byte g = stream->readByte();
				byte b = stream->readByte();

				CHECK_PIXEL_PTR(rleCode);

				while (rleCode--)
					rgb[pixelPtr++] = _pixelFormat.ARGBToColor(a, r, g, b);
			} else {
				CHECK_STREAM_PTR(rleCode * 4);
				CHECK_PIXEL_PTR(rleCode);

				// copy pixels directly to output
				while (rleCode--) {
					byte a = stream->readByte();
					byte r = stream->readByte();
					byte g = stream->readByte();
					byte b = stream->readByte();
					rgb[pixelPtr++] = _pixelFormat.ARGBToColor(a, r, g, b);
				}
			}
		}

		rowPtr += _surface->w;
	}
}

Graphics::Surface *QTRLEDecoder::decodeImage(Common::SeekableReadStream *stream) {
	uint16 start_line = 0;
	uint16 height = _surface->h;

	// check if this frame is even supposed to change
	if (stream->size() < 8)
		return _surface;

	// start after the chunk size
	stream->readUint32BE();

	// fetch the header
	uint16 header = stream->readUint16BE();

	// if a header is present, fetch additional decoding parameters
	if (header & 8) {
		if(stream->size() < 14)
			return _surface;
		start_line = stream->readUint16BE();
		stream->readUint16BE(); // Unknown
		height = stream->readUint16BE();
		stream->readUint16BE(); // Unknown
	}

	uint32 row_ptr = _surface->w * start_line;

	switch (_bitsPerPixel) {
		case 1:
		case 33:
			decode1(stream, row_ptr, height);
			break;
		case 2:
		case 34:
			decode2_4(stream, row_ptr, height, 2);
			break;
		case 4:
		case 36:
			decode2_4(stream, row_ptr, height, 4);
			break;
		case 8:
		case 40:
			decode8(stream, row_ptr, height);
			break;
		case 16:
			decode16(stream, row_ptr, height);
			break;
		case 24:
			decode24(stream, row_ptr, height);
			break;
		case 32:
			decode32(stream, row_ptr, height);
			break;
		default:
			error ("Unsupported bits per pixel %d", _bitsPerPixel);
	}

	return _surface;
}

QTRLEDecoder::~QTRLEDecoder() {
	_surface->free();
}

} // End of namespace Mohawk
