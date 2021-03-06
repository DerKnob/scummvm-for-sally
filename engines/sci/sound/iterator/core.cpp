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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/sound/iterator/core.cpp $
 * $Id: core.cpp 48046 2010-02-13 17:42:49Z fingolfin $
 *
 */

/* Sound subsystem core: Event handler, sound player dispatching */

#include "sci/sci.h"
#ifdef USE_OLD_MUSIC_FUNCTIONS

#include "sci/sound/iterator/core.h"
#include "sci/sound/iterator/iterator.h"
#include "sci/sound/drivers/mididriver.h"

#include "common/system.h"
#include "common/timer.h"

#include "sound/mixer.h"

namespace Sci {

/* Plays a song iterator that found a PCM through a PCM device, if possible
** Parameters: (SongIterator *) it: The iterator to play
**             (SongHandle) handle: Debug handle
** Returns   : (int) 0 if the effect will not be played, nonzero if it will
** This assumes that the last call to 'it->next()' returned SI_PCM.
*/
static int sfx_play_iterator_pcm(SongIterator *it, SongHandle handle);


#pragma mark -


class SfxPlayer {
public:
	/** Number of voices that can play simultaneously */
	int _polyphony;

protected:
	SciVersion _soundVersion;
	MidiPlayer *_mididrv;

	SongIterator *_iterator;
	Audio::Timestamp _wakeupTime;
	Audio::Timestamp _currentTime;
	uint32 _pauseTimeDiff;

	bool _paused;
	bool _iteratorIsDone;
	uint32 _tempo;

	Common::Mutex _mutex;
	int _volume;

	void play_song(SongIterator *it);
	static void player_timer_callback(void *refCon);

public:
	SfxPlayer(SciVersion soundVersion);
	~SfxPlayer();

	/**
	 * Initializes the player.
	 * @param resMan	a resource manager for driver initialization
	 * @param expected_latency	expected delay in between calls to 'maintenance' (in microseconds)
	 * @return	Common::kNoError on success, Common::kUnknownError on failure
	 */
	Common::Error init(ResourceManager *resMan, int expected_latency);

	/**
	 * Adds an iterator to the song player
	 * @param it		The iterator to play
	 * @param start_time	The time to assume as the time the first MIDI command executes at
	 * @return	Common::kNoError on success, Common::kUnknownError on failure
	 *
	 * The iterator should not be cloned (to avoid memory leaks) and
	 * may be modified according to the needs of the player.
	 * Implementors may use the 'sfx_iterator_combine()' function
	 * to add iterators onto their already existing iterators.
	 */
	Common::Error add_iterator(SongIterator *it, uint32 start_time);

	/**
	 * Stops the currently playing song and deletes the associated iterator.
	 * @return	Common::kNoError on success, Common::kUnknownError on failure
	 */
	Common::Error stop();

	/**
	 * Transmits a song iterator message to the active song.
	 * @param msg	the message to transmit
	 * @return	Common::kNoError on success, Common::kUnknownError on failure
	 */
	Common::Error iterator_message(const SongIterator::Message &msg);

	/**
	 * Pauses song playing.
	 * @return	Common::kNoError on success, Common::kUnknownError on failure
	 */
	Common::Error pause();

	/**
	 * Resumes song playing after a pause.
	 * @return	Common::kNoError on success, Common::kUnknownError on failure
	 */
	Common::Error resume();

	/**
	 * Pass a raw MIDI event to the synth.
	 * @param argc	length of buffer holding the midi event
	 * @param argv	the buffer itself
	 */
	void tell_synth(int buf_nr, byte *buf);

	void setVolume(int vol);

	int getVolume();
};

SfxPlayer::SfxPlayer(SciVersion soundVersion)
	: _soundVersion(soundVersion), _wakeupTime(0, SFX_TICKS_PER_SEC), _currentTime(0, 1)  {
	_polyphony = 0;

	_mididrv = 0;

	_iterator = NULL;
	_pauseTimeDiff = 0;

	_paused = false;
	_iteratorIsDone = false;
	_tempo = 0;

	_volume = 15;
}

SfxPlayer::~SfxPlayer() {
	if (_mididrv) {
		_mididrv->close();
		delete _mididrv;
	}
	delete _iterator;
	_iterator = NULL;
}

void SfxPlayer::play_song(SongIterator *it) {
	while (_iterator && _wakeupTime.msecsDiff(_currentTime) <= 0) {
		int delay;
		byte buf[8];
		int result;

		switch ((delay = songit_next(&(_iterator),
		                             buf, &result,
		                             IT_READER_MASK_ALL
		                             | IT_READER_MAY_FREE
		                             | IT_READER_MAY_CLEAN))) {

		case SI_FINISHED:
			delete _iterator;
			_iterator = NULL;
			_iteratorIsDone = true;
			return;

		case SI_IGNORE:
		case SI_LOOP:
		case SI_RELATIVE_CUE:
		case SI_ABSOLUTE_CUE:
			break;

		case SI_PCM:
			sfx_play_iterator_pcm(_iterator, 0);
			break;

		case 0:
			static_cast<MidiDriver *>(_mididrv)->send(buf[0], buf[1], buf[2]);

			break;

		default:
			_wakeupTime = _wakeupTime.addFrames(delay);
		}
	}
}

void SfxPlayer::tell_synth(int buf_nr, byte *buf) {
	byte op1 = (buf_nr < 2 ? 0 : buf[1]);
	byte op2 = (buf_nr < 3 ? 0 : buf[2]);

	static_cast<MidiDriver *>(_mididrv)->send(buf[0], op1, op2);
}

void SfxPlayer::player_timer_callback(void *refCon) {
	SfxPlayer *thePlayer = (SfxPlayer *)refCon;
	assert(refCon);
	Common::StackLock lock(thePlayer->_mutex);

	if (thePlayer->_iterator && !thePlayer->_iteratorIsDone && !thePlayer->_paused) {
		thePlayer->play_song(thePlayer->_iterator);
	}

	thePlayer->_currentTime = thePlayer->_currentTime.addFrames(1);
}

/* API implementation */

Common::Error SfxPlayer::init(ResourceManager *resMan, int expected_latency) {
	MidiDriverType musicDriver = MidiDriver::detectMusicDriver(MDT_PCSPK | MDT_ADLIB);

	switch (musicDriver) {
	case MD_ADLIB:
		// FIXME: There's no Amiga sound option, so we hook it up to AdLib
		if (g_sci->getPlatform() == Common::kPlatformAmiga)
			_mididrv = MidiPlayer_Amiga_create(_soundVersion);
		else
			_mididrv = MidiPlayer_AdLib_create(_soundVersion);
		break;
	case MD_PCJR:
		_mididrv = MidiPlayer_PCJr_create(_soundVersion);
		break;
	case MD_PCSPK:
		_mididrv = MidiPlayer_PCSpeaker_create(_soundVersion);
		break;
	default:
		break;
	}

	assert(_mididrv);

	_polyphony = _mididrv->getPolyphony();

	_tempo = _mididrv->getBaseTempo();
    uint32 time = g_system->getMillis();
	_currentTime = Audio::Timestamp(time, 1000000 / _tempo);
	_wakeupTime = Audio::Timestamp(time, SFX_TICKS_PER_SEC);

	_mididrv->setTimerCallback(this, player_timer_callback);
	_mididrv->open(resMan);
	_mididrv->setVolume(_volume);

	return Common::kNoError;
}

Common::Error SfxPlayer::add_iterator(SongIterator *it, uint32 start_time) {
	Common::StackLock lock(_mutex);
	SIMSG_SEND(it, SIMSG_SET_PLAYMASK(_mididrv->getPlayId()));
	SIMSG_SEND(it, SIMSG_SET_RHYTHM(_mididrv->hasRhythmChannel()));

	if (_iterator == NULL) {
		// Resync with clock
		_currentTime = Audio::Timestamp(g_system->getMillis(), 1000000 / _tempo);
		_wakeupTime = Audio::Timestamp(start_time, SFX_TICKS_PER_SEC);
	}

	_iterator = sfx_iterator_combine(_iterator, it);
	_iteratorIsDone = false;

	return Common::kNoError;
}

Common::Error SfxPlayer::stop() {
	debug(3, "Player: Stopping song iterator %p", (void *)_iterator);
	Common::StackLock lock(_mutex);
	delete _iterator;
	_iterator = NULL;
	for (int i = 0; i < MIDI_CHANNELS; i++)
		static_cast<MidiDriver *>(_mididrv)->send(0xb0 + i, SCI_MIDI_CHANNEL_NOTES_OFF, 0);

	return Common::kNoError;
}

Common::Error SfxPlayer::iterator_message(const SongIterator::Message &msg) {
	Common::StackLock lock(_mutex);
	if (!_iterator) {
		return Common::kUnknownError;
	}

	songit_handle_message(&_iterator, msg);

	return Common::kNoError;
}

Common::Error SfxPlayer::pause() {
	Common::StackLock lock(_mutex);

	_paused = true;
	_pauseTimeDiff = _wakeupTime.msecsDiff(_currentTime);

	_mididrv->playSwitch(false);

	return Common::kNoError;
}

Common::Error SfxPlayer::resume() {
	Common::StackLock lock(_mutex);

	_wakeupTime = Audio::Timestamp(_currentTime.msecs() + _pauseTimeDiff, SFX_TICKS_PER_SEC);
	_mididrv->playSwitch(true);
	_paused = false;

	return Common::kNoError;
}

void SfxPlayer::setVolume(int vol) {
	_mididrv->setVolume(vol);
}

int SfxPlayer::getVolume() {
	return _mididrv->getVolume();
}

#pragma mark -

void SfxState::sfx_reset_player() {
	if (_player)
		_player->stop();
}

void SfxState::sfx_player_tell_synth(int buf_nr, byte *buf) {
	if (_player)
		_player->tell_synth(buf_nr, buf);
}

int SfxState::sfx_get_player_polyphony() {
	if (_player)
		return _player->_polyphony;
	else
		return 0;
}

SfxState::SfxState() {
	_player = NULL;
	_it = NULL;
	_flags = 0;
	_song = NULL;
	_suspended = 0;
}

SfxState::~SfxState() {
}


void SfxState::freezeTime() {
	/* Freezes the top song delay time */
	const Audio::Timestamp ctime = Audio::Timestamp(g_system->getMillis(), SFX_TICKS_PER_SEC);
	Song *song = _song;

	while (song) {
		song->_delay = song->_wakeupTime.frameDiff(ctime);
		if (song->_delay < 0)
			song->_delay = 0;

		song = song->_nextPlaying;
	}
}

void SfxState::thawTime() {
	/* inverse of freezeTime() */
	const Audio::Timestamp ctime = Audio::Timestamp(g_system->getMillis(), SFX_TICKS_PER_SEC);
	Song *song = _song;

	while (song) {
		song->_wakeupTime = ctime.addFrames(song->_delay);

		song = song->_nextPlaying;
	}
}

#if 0
// Unreferenced - removed
static void _dump_playing_list(SfxState *self, char *msg) {
	Song *song = self->_song;

	fprintf(stderr, "[] Song list : [ ");
	song = *(self->_songlib.lib);
	while (song) {
		fprintf(stderr, "%08lx:%d ", song->handle, song->_status);
		song = song->_nextPlaying;
	}
	fprintf(stderr, "]\n");

	fprintf(stderr, "[] Play list (%s) : [ " , msg);

	while (song) {
		fprintf(stderr, "%08lx ", song->handle);
		song = song->_nextPlaying;
	}

	fprintf(stderr, "]\n");
}
#endif

#if 0
static void _dump_songs(SfxState *self) {
	Song *song = self->_song;

	fprintf(stderr, "Cue iterators:\n");
	song = *(self->_songlib.lib);
	while (song) {
		fprintf(stderr, "  **\tHandle %08x (p%d): status %d\n",
		        song->handle, song->_priority, song->_status);
		SIMSG_SEND(song->_it, SIMSG_PRINT(1));
		song = song->_next;
	}

	if (self->_player) {
		fprintf(stderr, "Audio iterator:\n");
		self->_player->iterator_message(SongIterator::Message(0, SIMSG_PRINT(1)));
	}
}
#endif

bool SfxState::isPlaying(Song *song) {
	Song *playing_song = _song;

	/*	_dump_playing_list(this, "is-playing");*/

	while (playing_song) {
		if (playing_song == song)
			return true;
		playing_song = playing_song->_nextPlaying;
	}
	return false;
}

void SfxState::setSongStatus(Song *song, int status) {
	const Audio::Timestamp ctime = Audio::Timestamp(g_system->getMillis(), SFX_TICKS_PER_SEC);

	switch (status) {

	case SOUND_STATUS_STOPPED:
		// Reset
		song->_it->init();
		break;

	case SOUND_STATUS_SUSPENDED:
	case SOUND_STATUS_WAITING:
		if (song->_status == SOUND_STATUS_PLAYING) {
			// Update delay, set wakeup_time
			song->_delay += song->_wakeupTime.frameDiff(ctime);
			song->_wakeupTime = ctime;
		}
		if (status == SOUND_STATUS_SUSPENDED)
			break;

		/* otherwise... */

	case SOUND_STATUS_PLAYING:
		if (song->_status == SOUND_STATUS_STOPPED) {
			// Starting anew
			song->_wakeupTime = ctime;
		}

		if (isPlaying(song))
			status = SOUND_STATUS_PLAYING;
		else
			status = SOUND_STATUS_WAITING;
		break;

	default:
		fprintf(stderr, "%s L%d: Attempt to set invalid song"
		        " state %d!\n", __FILE__, __LINE__, status);
		return;

	}
	song->_status = status;
}

/* Update internal state iff only one song may be played */
void SfxState::updateSingleSong() {
	Song *newsong = _songlib.findFirstActive();

	if (newsong != _song) {
		freezeTime(); /* Store song delay time */

		if (_player)
			_player->stop();

		if (newsong) {
			if (!newsong->_it)
				return; /* Restore in progress and not ready for this yet */

			/* Change song */
			if (newsong->_status == SOUND_STATUS_WAITING)
				setSongStatus(newsong, SOUND_STATUS_PLAYING);

			/* Change instrument mappings */
		} else {
			/* Turn off sound */
		}
		if (_song) {
			if (_song->_status == SOUND_STATUS_PLAYING)
				setSongStatus(newsong, SOUND_STATUS_WAITING);
		}

		Common::String debugMessage = "[SFX] Changing active song:";
		if (!_song) {
			debugMessage += " New song:";
		} else {
			char tmp[50];
			sprintf(tmp, " pausing %08lx, now playing ", _song->_handle);
			debugMessage += tmp;
		}

		if (newsong) {
			char tmp[20];
			sprintf(tmp, "%08lx\n", newsong->_handle);
			debugMessage += tmp;
		} else {
			debugMessage += " none\n";
		}

		debugC(2, kDebugLevelSound, "%s", debugMessage.c_str());

		_song = newsong;
		thawTime(); /* Recover song delay time */

		if (newsong && _player) {
			SongIterator *clonesong = newsong->_it->clone(newsong->_delay);

			_player->add_iterator(clonesong, newsong->_wakeupTime.msecs());
		}
	}
}


void SfxState::updateMultiSong() {
	Song *oldfirst = _song;
	Song *oldseeker;
	Song *newsong = _songlib.findFirstActive();
	Song *newseeker;
	Song not_playing_anymore; /* Dummy object, referenced by
				    ** songs which are no longer
				    ** active.  */

	/*	_dump_playing_list(this, "before");*/
	freezeTime(); /* Store song delay time */

	// WORKAROUND: sometimes, newsong can be NULL (e.g. in SQ4).
	// Handle this here, so that we avoid a crash
	if (!newsong) {
		// Iterators should get freed when there's only one song left playing
		if(oldfirst && oldfirst->_status == SOUND_STATUS_STOPPED) {
			debugC(2, kDebugLevelSound, "[SFX] Stopping song %lx", oldfirst->_handle);
			if (_player && oldfirst->_it)
				_player->iterator_message(SongIterator::Message(oldfirst->_it->ID, SIMSG_STOP));
		}
		return;
	}

	for (newseeker = newsong; newseeker;
	        newseeker = newseeker->_nextPlaying) {
		if (!newseeker || !newseeker->_it)
			return; /* Restore in progress and not ready for this yet */
	}

	/* First, put all old songs into the 'stopping' list and
	** mark their 'next-playing' as not_playing_anymore.  */
	for (oldseeker = oldfirst; oldseeker;
	        oldseeker = oldseeker->_nextStopping) {
		oldseeker->_nextStopping = oldseeker->_nextPlaying;
		oldseeker->_nextPlaying = &not_playing_anymore;

		if (oldseeker == oldseeker->_nextPlaying) {
			error("updateMultiSong() failed. Breakpoint in %s, line %d", __FILE__, __LINE__);
		}
	}

	/* Second, re-generate the new song queue. */
	for (newseeker = newsong; newseeker; newseeker = newseeker->_nextPlaying) {
		newseeker->_nextPlaying = _songlib.findNextActive(newseeker);

		if (newseeker == newseeker->_nextPlaying) {
			error("updateMultiSong() failed. Breakpoint in %s, line %d", __FILE__, __LINE__);
		}
	}
	/* We now need to update the currently playing song list, because we're
	** going to use some functions that require this list to be in a sane
	** state (particularly isPlaying(), indirectly */
	_song = newsong;

	/* Third, stop all old songs */
	for (oldseeker = oldfirst; oldseeker;
	        oldseeker = oldseeker->_nextStopping)
		if (oldseeker->_nextPlaying == &not_playing_anymore) {
			setSongStatus(oldseeker, SOUND_STATUS_SUSPENDED);
			debugC(2, kDebugLevelSound, "[SFX] Stopping song %lx", oldseeker->_handle);

			if (_player && oldseeker->_it)
				_player->iterator_message(SongIterator::Message(oldseeker->_it->ID, SIMSG_STOP));
			oldseeker->_nextPlaying = NULL; /* Clear this pointer; we don't need the tag anymore */
		}

	for (newseeker = newsong; newseeker; newseeker = newseeker->_nextPlaying) {
		if (newseeker->_status != SOUND_STATUS_PLAYING && _player) {
			debugC(2, kDebugLevelSound, "[SFX] Adding song %lx", newseeker->_it->ID);

			SongIterator *clonesong = newseeker->_it->clone(newseeker->_delay);
			_player->add_iterator(clonesong, g_system->getMillis());
		}
		setSongStatus(newseeker, SOUND_STATUS_PLAYING);
	}

	_song = newsong;
	thawTime();
	/*	_dump_playing_list(this, "after");*/
}

/* Update internal state */
void SfxState::update() {
	if (_flags & SFX_STATE_FLAG_MULTIPLAY)
		updateMultiSong();
	else
		updateSingleSong();
}

static int sfx_play_iterator_pcm(SongIterator *it, SongHandle handle) {
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Playing PCM: %08lx\n", handle);
#endif
	if (g_system->getMixer()->isReady()) {
		Audio::AudioStream *newfeed = it->getAudioStream();
		if (newfeed) {
			g_system->getMixer()->playInputStream(Audio::Mixer::kSFXSoundType, 0, newfeed);
			return 1;
		}
	}
	return 0;
}

#define DELAY (1000000 / SFX_TICKS_PER_SEC)

void SfxState::sfx_init(ResourceManager *resMan, int flags, SciVersion soundVersion) {
	_songlib._lib = 0;
	_song = NULL;
	_flags = flags;

	_player = NULL;

	if (flags & SFX_STATE_FLAG_NOSOUND) {
		warning("[SFX] Sound disabled");
		return;
	}

#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Initialising: flags=%x\n", flags);
#endif

	/*-------------------*/
	/* Initialise player */
	/*-------------------*/

	if (!resMan) {
		warning("[SFX] Warning: No resource manager present, cannot initialise player");
		return;
	}

	_player = new SfxPlayer(soundVersion);

	if (!_player) {
		warning("[SFX] No song player found");
		return;
	}

	if (_player->init(resMan, DELAY / 1000)) {
		warning("[SFX] Song player reported error, disabled");
		delete _player;
		_player = NULL;
	}

	_resMan = resMan;
}

void SfxState::sfx_exit() {
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Uninitialising\n");
#endif

	delete _player;
	_player = 0;

	g_system->getMixer()->stopAll();

	_songlib.freeSounds();
}

void SfxState::sfx_suspend(bool suspend) {
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Suspending? = %d\n", suspend);
#endif
	if (suspend && (!_suspended)) {
		/* suspend */

		freezeTime();
		if (_player)
			_player->pause();
		/* Suspend song player */

	} else if (!suspend && (_suspended)) {
		/* unsuspend */

		thawTime();
		if (_player)
			_player->resume();

		/* Unsuspend song player */
	}

	_suspended = suspend;
}

int SfxState::sfx_poll(SongHandle *handle, int *cue) {
	if (!_song)
		return 0; /* No milk today */

	*handle = _song->_handle;

#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Polling any (%08lx)\n", *handle);
#endif
	return sfx_poll_specific(*handle, cue);
}

int SfxState::sfx_poll_specific(SongHandle handle, int *cue) {
	const Audio::Timestamp ctime = Audio::Timestamp(g_system->getMillis(), SFX_TICKS_PER_SEC);
	Song *song = _song;

	while (song && song->_handle != handle)
		song = song->_nextPlaying;

	if (!song)
		return 0; /* Song not playing */

	debugC(2, kDebugLevelSound, "[SFX:CUE] Polled song %08lx ", handle);

	while (1) {
		if (song->_wakeupTime.frameDiff(ctime) > 0)
			return 0; /* Patience, young hacker! */

		byte buf[8];
		int result = songit_next(&(song->_it), buf, cue, IT_READER_MASK_ALL);

		switch (result) {

		case SI_FINISHED:
			setSongStatus(song, SOUND_STATUS_STOPPED);
			update();
			/* ...fall through... */
		case SI_LOOP:
		case SI_RELATIVE_CUE:
		case SI_ABSOLUTE_CUE:
			if (result == SI_FINISHED)
				debugC(2, kDebugLevelSound, " => finished");
			else {
				if (result == SI_LOOP)
					debugC(2, kDebugLevelSound, " => Loop: %d (0x%x)", *cue, *cue);
				else
					debugC(2, kDebugLevelSound, " => Cue: %d (0x%x)", *cue, *cue);

			}
			return result;

		default:
			if (result > 0)
				song->_wakeupTime = song->_wakeupTime.addFrames(result);

			/* Delay */
			break;
		}
	}

}


/*****************/
/*  Song basics  */
/*****************/

void SfxState::sfx_add_song(SongIterator *it, int priority, SongHandle handle, int number) {
	Song *song = _songlib.findSong(handle);

#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Adding song: %08lx at %d, it=%p\n", handle, priority, it);
#endif
	if (!it) {
		error("[SFX] Attempt to add empty song with handle %08lx", handle);
		return;
	}

	it->init();

	/* If we're already playing this, stop it */
	/* Tell player to shut up */
//	_dump_songs(this);

	if (_player)
		_player->iterator_message(SongIterator::Message(handle, SIMSG_STOP));

	if (song) {
		setSongStatus( song, SOUND_STATUS_STOPPED);

		fprintf(stderr, "Overwriting old song (%08lx) ...\n", handle);
		if (song->_status == SOUND_STATUS_PLAYING || song->_status == SOUND_STATUS_SUSPENDED) {
			delete it;
			error("Unexpected (error): Song %ld still playing/suspended (%d)",
			        handle, song->_status);
			return;
		} else {
			_songlib.removeSong(handle); /* No duplicates */
		}

	}

	song = new Song(handle, it, priority);
	song->_resourceNum = number;
	song->_hold = 0;
	song->_loops = 0;
	song->_wakeupTime = Audio::Timestamp(g_system->getMillis(), SFX_TICKS_PER_SEC);
	_songlib.addSong(song);
	_song = NULL; /* As above */
	update();

	return;
}

void SfxState::sfx_remove_song(SongHandle handle) {
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Removing song: %08lx\n", handle);
#endif
	if (_song && _song->_handle == handle)
		_song = NULL;

	_songlib.removeSong(handle);
	update();
}



/**********************/
/* Song modifications */
/**********************/

#define ASSERT_SONG(s) if (!(s)) { warning("Looking up song handle %08lx failed in %s, L%d", handle, __FILE__, __LINE__); return; }

void SfxState::sfx_song_set_status(SongHandle handle, int status) {
	Song *song = _songlib.findSong(handle);
	ASSERT_SONG(song);
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Setting song status to %d"
	        " (0:stop, 1:play, 2:susp, 3:wait): %08lx\n", status, handle);
#endif

	setSongStatus(song, status);

	update();
}

void SfxState::sfx_song_set_fade(SongHandle handle, fade_params_t *params) {
#ifdef DEBUG_SONG_API
	static const char *stopmsg[] = {"??? Should not happen", "Do not stop afterwards", "Stop afterwards"};
#endif
	Song *song = _songlib.findSong(handle);

	ASSERT_SONG(song);

#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Setting fade params of %08lx to "
	        "final volume %d in steps of %d per %d ticks. %s.",
	        handle, fade->final_volume, fade->step_size, fade->ticks_per_step,
	        stopmsg[fade->action]);
#endif

	SIMSG_SEND_FADE(song->_it, params);

	update();
}

void SfxState::sfx_song_renice(SongHandle handle, int priority) {
	Song *song = _songlib.findSong(handle);
	ASSERT_SONG(song);
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Renicing song %08lx to %d\n",
	        handle, priority);
#endif

	song->_priority = priority;

	update();
}

void SfxState::sfx_song_set_loops(SongHandle handle, int loops) {
	Song *song = _songlib.findSong(handle);
	SongIterator::Message msg = SongIterator::Message(handle, SIMSG_SET_LOOPS(loops));
	ASSERT_SONG(song);

	song->_loops = loops;
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Setting loops on %08lx to %d\n",
	        handle, loops);
#endif
	songit_handle_message(&(song->_it), msg);

	if (_player/* && _player->send_iterator_message*/)
		/* FIXME: The above should be optional! */
		_player->iterator_message(msg);
}

void SfxState::sfx_song_set_hold(SongHandle handle, int hold) {
	Song *song = _songlib.findSong(handle);
	SongIterator::Message msg = SongIterator::Message(handle, SIMSG_SET_HOLD(hold));
	ASSERT_SONG(song);

	song->_hold = hold;
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] Setting hold on %08lx to %d\n",
	        handle, hold);
#endif
	songit_handle_message(&(song->_it), msg);

	if (_player/* && _player->send_iterator_message*/)
		/* FIXME: The above should be optional! */
		_player->iterator_message(msg);
}

/* Different from the one in iterator.c */
static const int MIDI_cmdlen[16] = {0, 0, 0, 0, 0, 0, 0, 0,
                                    3, 3, 0, 3, 2, 0, 3, 0
                                   };

static const SongHandle midi_send_base = 0xffff0000;

Common::Error SfxState::sfx_send_midi(SongHandle handle, int channel,
	int command, int arg1, int arg2) {
	byte buffer[5];

	/* Yes, in that order. SCI channel mutes are actually done via
	   a counting semaphore. 0 means to decrement the counter, 1
	   to increment it. */
	static const char *channel_state[] = {"ON", "OFF"};

	if (command == 0xb0 &&
	        arg1 == SCI_MIDI_CHANNEL_MUTE) {
		warning("TODO: channel mute (channel %d %s)", channel, channel_state[arg2]);
		/* We need to have a GET_PLAYMASK interface to use
		   here. SET_PLAYMASK we've got.
		*/
		return Common::kNoError;
	}

	buffer[0] = channel | command; /* No channel remapping yet */

	switch (command) {
	case 0x80 :
	case 0x90 :
	case 0xb0 :
		buffer[1] = arg1 & 0xff;
		buffer[2] = arg2 & 0xff;
		break;
	case 0xc0 :
		buffer[1] = arg1 & 0xff;
		break;
	case 0xe0 :
		buffer[1] = (arg1 & 0x7f) | 0x80;
		buffer[2] = (arg1 & 0xff00) >> 7;
		break;
	default:
		warning("Unexpected explicit MIDI command %02x", command);
		return Common::kUnknownError;
	}

	if (_player)
		_player->tell_synth(MIDI_cmdlen[command >> 4], buffer);
	return Common::kNoError;
}

int SfxState::sfx_getVolume() {
	return _player->getVolume();
}

void SfxState::sfx_setVolume(int volume) {
	_player->setVolume(volume);
}

void SfxState::sfx_all_stop() {
#ifdef DEBUG_SONG_API
	fprintf(stderr, "[sfx-core] All stop\n");
#endif

	_songlib.freeSounds();
	update();
}

} // End of namespace Sci

#endif	// USE_OLD_MUSIC_FUNCTIONS
