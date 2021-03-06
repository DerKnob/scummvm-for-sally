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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/gob/sound/cdrom.h $
 * $Id: cdrom.h 41632 2009-06-18 13:27:14Z drmccoy $
 *
 */

#ifndef GOB_SOUND_CDROM_H
#define GOB_SOUND_CDROM_H

namespace Gob {

class DataStream;

class CDROM {
public:
	CDROM();
	~CDROM();

	void readLIC(DataStream &stream);
	void freeLICBuffer();

	void startTrack(const char *trackName);
	void stopPlaying();

	bool isPlaying() const;

	int32 getTrackPos(const char *keyTrack = 0) const;
	const char *getCurTrack() const;

	void testCD(int trySubst, const char *label);

protected:
	byte *_LICbuffer;
	byte *_curTrackBuffer;
	char _curTrack[16];
	uint16 _numTracks;
	uint32 _trackStop;
	uint32 _startTime;
	bool _cdPlaying;

	void play(uint32 from, uint32 to);
	void stop();

	byte *getTrackBuffer(const char *trackName) const;
};

} // End of namespace Gob

#endif // GOB_SOUND_CDROM_H
