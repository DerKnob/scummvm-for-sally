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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/sound/decoders/flac.h $
 * $Id: flac.h 47846 2010-02-03 09:42:11Z lordhoto $
 *
 */

/**
 * @file
 * Sound decoder used in engines:
 *  - agos
 *  - kyra
 *  - m4
 *  - queen
 *  - saga
 *  - scumm
 *  - sword1
 *  - sword2
 *  - touche
 *  - tucker
 */

#ifndef SOUND_FLAC_H
#define SOUND_FLAC_H

#include "common/types.h"
#include "common/scummsys.h"

#ifdef USE_FLAC

namespace Common {
	class SeekableReadStream;
}

namespace Audio {

class AudioStream;
class SeekableAudioStream;

/**
 * Create a new SeekableAudioStream from the FLAC data in the given stream.
 * Allows for seeking (which is why we require a SeekableReadStream).
 *
 * @param stream            the SeekableReadStream from which to read the FLAC data
 * @param disposeAfterUse   whether to delete the stream after use
 * @return   a new SeekableAudioStream, or NULL, if an error occured
 */
SeekableAudioStream *makeFLACStream(
	Common::SeekableReadStream *stream,
	DisposeAfterUse::Flag disposeAfterUse);

} // End of namespace Audio

#endif // #ifdef USE_FLAC
#endif // #ifndef SOUND_FLAC_H
