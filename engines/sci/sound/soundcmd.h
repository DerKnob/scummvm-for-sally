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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/sound/soundcmd.h $
 * $Id: soundcmd.h 47475 2010-01-23 14:39:03Z thebluegr $
 *
 */

#ifndef SCI_SOUNDCMD_H
#define SCI_SOUNDCMD_H

#include "sci/sci.h"	// for USE_OLD_MUSIC_FUNCTIONS

#include "common/list.h"
#include "sci/engine/state.h"

namespace Sci {

class Console;
class SciMusic;
class SoundCommandParser;
typedef void (SoundCommandParser::*SoundCommand)(reg_t obj, int16 value);

struct MusicEntryCommand {
	MusicEntryCommand(const char *d, SoundCommand c) : sndCmd(c), desc(d) {}
	SoundCommand sndCmd;
	const char *desc;
};

class SoundCommandParser {
public:
	SoundCommandParser(ResourceManager *resMan, SegManager *segMan, Kernel *kernel, AudioPlayer *audio, SciVersion soundVersion);
	~SoundCommandParser();

	enum {
		kMaxSciVolume = 15
	};

#ifdef USE_OLD_MUSIC_FUNCTIONS
	void updateSfxState(SfxState *newState) { _state = newState; }
#endif

	reg_t parseCommand(int argc, reg_t *argv, reg_t acc);

	// Functions used for game state loading
	void clearPlayList();
	void syncPlayList(Common::Serializer &s);
	void reconstructPlayList(int savegame_version);

	// Functions used for the ScummVM menus
	void setMasterVolume(int vol);
	void pauseAll(bool pause);

	// Debug console functions
	void playSound(reg_t obj) { cmdPlaySound(obj, 0); }
	void stopSound(reg_t obj) { cmdStopSound(obj, 0); }
	void startNewSound(int number);
	void stopAllSounds();
	void printPlayList(Console *con);
	void printSongInfo(reg_t obj, Console *con);

#ifndef USE_OLD_MUSIC_FUNCTIONS
	/**
	 * Synchronizes the current state of the music list to the rest of the engine, so that
	 * the changes that the sound thread makes to the music are registered with the engine
	 * scripts. In SCI0, we invoke this from kAnimate (which is called very often). SCI01
	 * and later have a specific callback function, cmdUpdateCues, which is called regularly
	 * by the engine scripts themselves, so the engine itself polls for changes to the music
	 */
	void updateSci0Cues();
#endif

private:
	typedef Common::Array<MusicEntryCommand *> SoundCommandContainer;
	SoundCommandContainer _soundCommands;
	ResourceManager *_resMan;
	SegManager *_segMan;
	Kernel *_kernel;
#ifdef USE_OLD_MUSIC_FUNCTIONS
	SfxState *_state;
	int _midiCmd, _controller, _param;
#else
	SciMusic *_music;
#endif
	AudioPlayer *_audio;
	SciVersion _soundVersion;
	int _argc;
	reg_t *_argv;	// for cmdFadeSound
	uint32 _midiCommand;	// for cmdSendMidi
	reg_t _acc;
	int _cmdUpdateCuesIndex;

	void cmdInitSound(reg_t obj, int16 value);
	void cmdPlaySound(reg_t obj, int16 value);
	void cmdDummy(reg_t obj, int16 value);
	void cmdMuteSound(reg_t obj, int16 value);
	void cmdPauseSound(reg_t obj, int16 value);
	void cmdResumeSound(reg_t obj, int16 value);
	void cmdStopSound(reg_t obj, int16 value);
	void cmdDisposeSound(reg_t obj, int16 value);
	void cmdMasterVolume(reg_t obj, int16 value);
	void cmdFadeSound(reg_t obj, int16 value);
	void cmdGetPolyphony(reg_t obj, int16 value);
	void cmdStopAllSounds(reg_t obj, int16 value);
	void cmdUpdateSound(reg_t obj, int16 value);
	void cmdUpdateCues(reg_t obj, int16 value);
	void cmdSendMidi(reg_t obj, int16 value);
	void cmdReverb(reg_t obj, int16 value);
	void cmdSetSoundHold(reg_t obj, int16 value);
	void cmdGetAudioCapability(reg_t obj, int16 value);
	void cmdSetSoundVolume(reg_t obj, int16 value);
	void cmdSetSoundPriority(reg_t obj, int16 value);
	void cmdSetSoundLoop(reg_t obj, int16 value);
	void cmdSuspendSound(reg_t obj, int16 value);

#ifdef USE_OLD_MUSIC_FUNCTIONS
	void changeSoundStatus(reg_t obj, int newStatus);
#endif
};

} // End of namespace Sci

#endif // SCI_SOUNDCMD_H
