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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/sound/decoders/iff_sound.h $
 * $Id: iff_sound.h 47579 2010-01-26 22:48:45Z fingolfin $
 *
 */

/**
 * @file
 * Sound decoder used in engines:
 *  - parallaction
 */

#ifndef SOUND_IFF_H
#define SOUND_IFF_H

namespace Common {
	class ReadStream;
}

namespace Audio {

class AudioStream;

AudioStream *make8SVXStream(Common::ReadStream &stream, bool loop);

}

#endif
