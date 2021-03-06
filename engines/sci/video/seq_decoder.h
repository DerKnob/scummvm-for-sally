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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/video/seq_decoder.h $
 * $Id: seq_decoder.h 46737 2009-12-30 10:09:48Z thebluegr $
 *
 */

#ifndef SEQ_DECODER_H
#define SEQ_DECODER_H

#include "graphics/video/video_player.h"

namespace Sci {

/**
 * Implementation of the Sierra SEQ decoder, used in KQ6 DOS floppy/CD and GK1 DOS
 */
class SeqDecoder : public Graphics::VideoDecoder {
public:
	SeqDecoder() {}
	virtual ~SeqDecoder();

	/**
	 * Load a SEQ encoded video file
	 * @param filename	the filename to load
	 */
	bool loadFile(const char *fileName) { return loadFile(fileName, 10); }

	/**
	 * Load a SEQ encoded video file
	 * @param filename	the filename to load
	 * @param frameDelay the delay between frames, in ticks
	 */
	bool loadFile(const char *fileName, int frameDelay);

	/**
	 * Close a SEQ encoded video file
	 */
	void closeFile();

	bool decodeNextFrame();

private:
	bool decodeFrame(byte *rleData, int rleSize, byte *litData, int litSize, byte *dest, int left, int width, int height, int colorKey);
};

} // End of namespace Sci

#endif
