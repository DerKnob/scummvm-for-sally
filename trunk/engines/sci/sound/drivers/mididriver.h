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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/sound/drivers/mididriver.h $
 * $Id: mididriver.h 48086 2010-02-17 23:38:43Z fingolfin $
 *
 */

#ifndef SCI_SFX_SOFTSEQ_MIDIDRIVER_H
#define SCI_SFX_SOFTSEQ_MIDIDRIVER_H

#include "sci/sci.h"
#include "sound/mididrv.h"
#include "common/error.h"

namespace Sci {

class ResourceManager;

enum {
	MIDI_CHANNELS = 16,
	MIDI_PROP_MASTER_VOLUME = 0
};


#define MIDI_RHYTHM_CHANNEL 9

/* Special SCI sound stuff */

#define SCI_MIDI_TIME_EXPANSION_PREFIX 0xF8
#define SCI_MIDI_TIME_EXPANSION_LENGTH 240

#define SCI_MIDI_EOT 0xFC
#define SCI_MIDI_SET_SIGNAL 0xCF
#define SCI_MIDI_SET_POLYPHONY 0x4B
#define SCI_MIDI_RESET_ON_SUSPEND 0x4C
#define SCI_MIDI_CHANNEL_MUTE 0x4E
#define SCI_MIDI_SET_REVERB 0x50
#define SCI_MIDI_HOLD 0x52
#define SCI_MIDI_CUMULATIVE_CUE 0x60
#define SCI_MIDI_CHANNEL_SOUND_OFF 0x78 /* all-sound-off for Bn */
#define SCI_MIDI_CHANNEL_NOTES_OFF 0x7B /* all-notes-off for Bn */

#define SCI_MIDI_SET_SIGNAL_LOOP 0x7F
/* If this is the parameter of 0xCF, the loop point is set here */

#define SCI_MIDI_CONTROLLER(status) ((status & 0xF0) == 0xB0)

class MidiPlayer : public MidiDriver {
protected:
	MidiDriver *_driver;
	byte _reverb;

public:
	MidiPlayer(SciVersion version) : _reverb(0), _version(version) { }

	int open() {
		ResourceManager *resMan = g_sci->getResMan();	// HACK
		return open(resMan);
	}
	virtual int open(ResourceManager *resMan) { return _driver->open(); }
	virtual void close() { _driver->close(); }
	virtual void send(uint32 b) { _driver->send(b); }
	uint32 getBaseTempo() { return _driver->getBaseTempo(); }
	virtual bool hasRhythmChannel() const = 0;
	MidiChannel *allocateChannel() { return _driver->allocateChannel(); }
	MidiChannel *getPercussionChannel() { return _driver->getPercussionChannel(); }
	virtual void setTimerCallback(void *timer_param, Common::TimerManager::TimerProc timer_proc) { _driver->setTimerCallback(timer_param, timer_proc); }

	virtual byte getPlayId() = 0;
	virtual int getPolyphony() const = 0;

	virtual void setVolume(byte volume) {
		if(_driver)
			_driver->property(MIDI_PROP_MASTER_VOLUME, volume);
	}

	virtual int getVolume() {
		return _driver ? _driver->property(MIDI_PROP_MASTER_VOLUME, 0xffff) : 0;
	}

	virtual byte getReverb() { return _reverb; }
	virtual void setReverb(byte reverb) { _reverb = reverb; }

	virtual void playSwitch(bool play) {
		if (!play) {
			// Send "All Sound Off" on all channels
			for (int i = 0; i < MIDI_CHANNELS; ++i)
				_driver->send(0xb0 + i, SCI_MIDI_CHANNEL_NOTES_OFF, 0);
		}
	}

protected:
	SciVersion _version;
};

extern MidiPlayer *MidiPlayer_AdLib_create(SciVersion version);
extern MidiPlayer *MidiPlayer_Amiga_create(SciVersion version);
extern MidiPlayer *MidiPlayer_PCJr_create(SciVersion version);
extern MidiPlayer *MidiPlayer_PCSpeaker_create(SciVersion version);
extern MidiPlayer *MidiPlayer_Midi_create(SciVersion version);
extern MidiPlayer *MidiPlayer_Fb01_create(SciVersion version);

} // End of namespace Sci

#endif // SCI_SFX_SOFTSEQ_MIDIDRIVER_H