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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/made/made.h $
 * $Id: made.h 47579 2010-01-26 22:48:45Z fingolfin $
 *
 */

#ifndef MADE_H
#define MADE_H

#include "common/scummsys.h"
#include "common/endian.h"
#include "common/util.h"
#include "common/file.h"
#include "common/savefile.h"
#include "common/system.h"
#include "common/hash-str.h"
#include "common/events.h"
#include "common/keyboard.h"

#include "graphics/surface.h"

#include "sound/audiostream.h"
#include "sound/mixer.h"
#include "sound/decoders/voc.h"
#include "sound/audiocd.h"

#include "engines/engine.h"

#include "made/sound.h"

/**
 * This is the namespace of the Made engine.
 *
 * Status of this engine: ???
 *
 * Supported games:
 * - ???
 */
namespace Made {

enum MadeGameID {
	GID_RTZ		= 0,
	GID_MANHOLE	= 1,
	GID_LGOP2	= 2,
	GID_RODNEY	= 3
};

enum MadeGameFeatures {
	GF_DEMO				= 1 << 0,
	GF_CD				= 1 << 1,
	GF_CD_COMPRESSED	= 1 << 2,
	GF_FLOPPY			= 1 << 3
};

const uint32 kTimerResolution = 40;

struct MadeGameDescription;

class ResourceReader;
class PmvPlayer;
class Screen;
class ScriptInterpreter;
class GameDatabase;
class MusicPlayer;

class MadeEngine : public ::Engine {
	int _gameId;

protected:

	// Engine APIs
	virtual Common::Error run();

public:
	MadeEngine(OSystem *syst, const MadeGameDescription *gameDesc);
	virtual ~MadeEngine();

	virtual bool hasFeature(EngineFeature f) const;
	virtual void syncSoundSettings();

	int getGameId() {
		return _gameId;
	}

	Common::RandomSource *_rnd;
	const MadeGameDescription *_gameDescription;
	uint32 getGameID() const;
	uint32 getFeatures() const;
	uint16 getVersion() const;
	Common::Platform getPlatform() const;

private:
public:
	PmvPlayer *_pmvPlayer;
	ResourceReader *_res;
	Screen *_screen;
	GameDatabase *_dat;
	ScriptInterpreter *_script;
	MusicPlayer *_music;

	uint16 _eventNum;
	int _eventMouseX, _eventMouseY;
	uint16 _eventKey;

	int _soundRate;
	bool _autoStopSound;
	uint _soundEnergyIndex;
	SoundEnergyArray *_soundEnergyArray;

	uint32 _musicBeatStart;
	uint32 _cdTimeStart;

	int32 _timers[50];
	int16 getTicks();
	int16 getTimer(int16 timerNum);
	void setTimer(int16 timerNum, int16 value);
	void resetTimer(int16 timerNum);
	int16 allocTimer();
	void freeTimer(int16 timerNum);
	void resetAllTimers();

	const Common::String getTargetName() { return _targetName; }
	Common::String getSavegameFilename(int16 saveNum);

	void handleEvents();

};

} // End of namespace Made

#endif /* MADE_H */
