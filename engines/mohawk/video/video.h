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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/mohawk/video/video.h $
 * $Id: video.h 48056 2010-02-13 23:34:18Z mthreepwood $
 *
 */

#ifndef MOHAWK_VIDEO_H
#define MOHAWK_VIDEO_H

#include "graphics/pixelformat.h"

namespace Mohawk {

class MohawkEngine;

struct MLSTRecord {
	uint16 index;
	uint16 movieID;
	uint16 code;
	uint16 left;
	uint16 top;
	uint16 u0[3];
	uint16 loop;
	uint16 volume;
	uint16 u1;
};

class QTPlayer;

struct VideoEntry {
	QTPlayer *video;
	uint16 x;
	uint16 y;
	bool loop;
	Common::String filename;
	uint16 id; // Riven only
	bool enabled;

	QTPlayer *operator->() const { assert(video); return video; }
};

typedef int32 VideoHandle;

enum {
	NULL_VID_HANDLE = -1
};

class VideoManager {
public:
	VideoManager(MohawkEngine *vm);
	~VideoManager();

	// Generic movie functions
	void playMovie(Common::String filename, uint16 x = 0, uint16 y = 0, bool clearScreen = false);
	void playMovieCentered(Common::String filename, bool clearScreen = true);
	void playBackgroundMovie(Common::String filename, int16 x = -1, int16 y = -1, bool loop = false);
	bool updateBackgroundMovies();
	void pauseVideos();
	void resumeVideos();
	void stopVideos();

	// Riven-related functions
	void activateMLST(uint16 mlstId, uint16 card);
	void enableMovie(uint16 id);
	void disableMovie(uint16 id);
	void disableAllMovies();
	void playMovie(uint16 id);
	void stopMovie(uint16 id);
	void playMovieBlocking(uint16 id);

	// Riven-related variables
	Common::Array<MLSTRecord> _mlstRecords;

private:
	MohawkEngine *_vm;

	void waitUntilMovieEnds(VideoHandle videoHandle);

	// Keep tabs on any videos playing
	Common::Array<VideoEntry> _videoStreams;
	uint32 _pauseStart;

	VideoHandle createVideoHandle(uint16 id, uint16 x, uint16 y, bool loop);
	VideoHandle createVideoHandle(Common::String filename, uint16 x, uint16 y, bool loop);
};

} // End of namespace Mohawk

#endif
