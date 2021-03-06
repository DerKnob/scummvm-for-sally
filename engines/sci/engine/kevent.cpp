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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/engine/kevent.cpp $
 * $Id: kevent.cpp 48049 2010-02-13 17:44:58Z fingolfin $
 *
 */

#include "sci/sci.h"
#include "sci/engine/features.h"
#include "sci/engine/state.h"
#include "sci/engine/selector.h"
#include "sci/engine/kernel.h"
#include "sci/console.h"
#include "sci/debug.h"	// for g_debug_simulated_key
#include "sci/event.h"
#include "sci/graphics/gui.h"
#include "sci/graphics/gui32.h"
#include "sci/graphics/coordadjuster.h"
#include "sci/graphics/cursor.h"

namespace Sci {

#define SCI_VARIABLE_GAME_SPEED 3

reg_t kGetEvent(EngineState *s, int argc, reg_t *argv) {
	int mask = argv[0].toUint16();
	reg_t obj = argv[1];
	sciEvent curEvent;
	int oldx, oldy;
	int modifier_mask = getSciVersion() <= SCI_VERSION_01 ? SCI_KEYMOD_ALL : SCI_KEYMOD_NO_FOOLOCK;
	SegManager *segMan = s->_segMan;
	Common::Point mousePos;

	mousePos = g_sci->_gfxCursor->getPosition();

	// If there's a simkey pending, and the game wants a keyboard event, use the
	// simkey instead of a normal event
	if (g_debug_simulated_key && (mask & SCI_EVENT_KEYBOARD)) {
		PUT_SEL32V(segMan, obj, SELECTOR(type), SCI_EVENT_KEYBOARD); // Keyboard event
		PUT_SEL32V(segMan, obj, SELECTOR(message), g_debug_simulated_key);
		PUT_SEL32V(segMan, obj, SELECTOR(modifiers), SCI_KEYMOD_NUMLOCK); // Numlock on
		PUT_SEL32V(segMan, obj, SELECTOR(x), mousePos.x);
		PUT_SEL32V(segMan, obj, SELECTOR(y), mousePos.y);
		g_debug_simulated_key = 0;
		return make_reg(0, 1);
	}

	oldx = mousePos.x;
	oldy = mousePos.y;
	curEvent = s->_event->get(mask);

	if (s->_voc)
		s->_voc->parser_event = NULL_REG; // Invalidate parser event

	PUT_SEL32V(segMan, obj, SELECTOR(x), mousePos.x);
	PUT_SEL32V(segMan, obj, SELECTOR(y), mousePos.y);

	//s->_gui->moveCursor(s->gfx_state->pointer_pos.x, s->gfx_state->pointer_pos.y);

	switch (curEvent.type) {
	case SCI_EVENT_QUIT:
		quit_vm();
		break;

	case SCI_EVENT_KEYBOARD:
		PUT_SEL32V(segMan, obj, SELECTOR(type), SCI_EVENT_KEYBOARD); // Keyboard event
		s->r_acc = make_reg(0, 1);

		PUT_SEL32V(segMan, obj, SELECTOR(message), curEvent.character);
		// We only care about the translated character
		PUT_SEL32V(segMan, obj, SELECTOR(modifiers), curEvent.modifiers & modifier_mask);
		break;

	case SCI_EVENT_MOUSE_RELEASE:
	case SCI_EVENT_MOUSE_PRESS:

		// track left buttton clicks, if requested
		if (curEvent.type == SCI_EVENT_MOUSE_PRESS && curEvent.data == 1 && g_debug_track_mouse_clicks) {
			g_sci->getSciDebugger()->DebugPrintf("Mouse clicked at %d, %d\n",
						mousePos.x, mousePos.y);
		}

		if (mask & curEvent.type) {
			int extra_bits = 0;

			switch (curEvent.data) {
			case 2:
				extra_bits = SCI_KEYMOD_LSHIFT | SCI_KEYMOD_RSHIFT;
				break;
			case 3:
				extra_bits = SCI_KEYMOD_CTRL;
			default:
				break;
			}

			PUT_SEL32V(segMan, obj, SELECTOR(type), curEvent.type);
			PUT_SEL32V(segMan, obj, SELECTOR(message), 0);
			PUT_SEL32V(segMan, obj, SELECTOR(modifiers), (curEvent.modifiers | extra_bits) & modifier_mask);
			s->r_acc = make_reg(0, 1);
		}
		break;

	default:
		s->r_acc = NULL_REG; // Unknown or no event
	}

	if ((s->r_acc.offset) && (g_debugState.stopOnEvent)) {
		g_debugState.stopOnEvent = false;

		// A SCI event occured, and we have been asked to stop, so open the debug console
		Console *con = g_sci->getSciDebugger();
		con->DebugPrintf("SCI event occured: ");
		switch (curEvent.type) {
		case SCI_EVENT_QUIT:
			con->DebugPrintf("quit event\n");
			break;
		case SCI_EVENT_KEYBOARD:
			con->DebugPrintf("keyboard event\n");
			break;
		case SCI_EVENT_MOUSE_RELEASE:
		case SCI_EVENT_MOUSE_PRESS:
			con->DebugPrintf("mouse click event\n");
			break;
		default:
			con->DebugPrintf("unknown or no event (event type %d)\n", curEvent.type);
		}

		con->attach();
		con->onFrame();
	}

#ifndef USE_OLD_MUSIC_FUNCTIONS
	if (g_sci->_features->detectDoSoundType() <= SCI_VERSION_0_LATE) {
		// If we're running a SCI0 game, update the sound cues, to compensate
		// for the fact that SCI0 does not poll to update the sound cues itself,
		// like SCI01 and later do with cmdUpdateSoundCues. kGetEvent is called
		// quite often, so emulate the SCI01 behavior of cmdUpdateSoundCues with
		// this call
		s->_soundCmd->updateSci0Cues();
	}
#endif

	return s->r_acc;
}

reg_t kMapKeyToDir(EngineState *s, int argc, reg_t *argv) {
	reg_t obj = argv[0];
	SegManager *segMan = s->_segMan;

	if (GET_SEL32V(segMan, obj, SELECTOR(type)) == SCI_EVENT_KEYBOARD) { // Keyboard
		int mover = -1;
		switch (GET_SEL32V(segMan, obj, SELECTOR(message))) {
		case SCI_KEY_HOME:
			mover = 8;
			break;
		case SCI_KEY_UP:
			mover = 1;
			break;
		case SCI_KEY_PGUP:
			mover = 2;
			break;
		case SCI_KEY_LEFT:
			mover = 7;
			break;
		case SCI_KEY_CENTER:
		case 76:
			mover = 0;
			break;
		case SCI_KEY_RIGHT:
			mover = 3;
			break;
		case SCI_KEY_END:
			mover = 6;
			break;
		case SCI_KEY_DOWN:
			mover = 5;
			break;
		case SCI_KEY_PGDOWN:
			mover = 4;
			break;
		default:
			break;
		}

		if (mover >= 0) {
			PUT_SEL32V(segMan, obj, SELECTOR(type), SCI_EVENT_JOYSTICK);
			PUT_SEL32V(segMan, obj, SELECTOR(message), mover);
			return make_reg(0, 1);
		} else
			return NULL_REG;
	}

	return s->r_acc;
}

reg_t kGlobalToLocal(EngineState *s, int argc, reg_t *argv) {
	reg_t obj = argc ? argv[0] : NULL_REG; // Can this really happen? Lars
	reg_t planeObject = argc > 1 ? argv[1] : NULL_REG; // SCI32
	SegManager *segMan = s->_segMan;

	if (obj.segment) {
		int16 x = GET_SEL32V(segMan, obj, SELECTOR(x));
		int16 y = GET_SEL32V(segMan, obj, SELECTOR(y));

		g_sci->_gfxCoordAdjuster->kernelGlobalToLocal(x, y, planeObject);

		PUT_SEL32V(segMan, obj, SELECTOR(x), x);
		PUT_SEL32V(segMan, obj, SELECTOR(y), y);
	}

	return s->r_acc;

}

reg_t kLocalToGlobal(EngineState *s, int argc, reg_t *argv) {
	reg_t obj = argc ? argv[0] : NULL_REG; // Can this really happen? Lars
	reg_t planeObject = argc > 1 ? argv[1] : NULL_REG; // SCI32
	SegManager *segMan = s->_segMan;

	if (obj.segment) {
		int16 x = GET_SEL32V(segMan, obj, SELECTOR(x));
		int16 y = GET_SEL32V(segMan, obj, SELECTOR(y));

		g_sci->_gfxCoordAdjuster->kernelLocalToGlobal(x, y, planeObject);

		PUT_SEL32V(segMan, obj, SELECTOR(x), x);
		PUT_SEL32V(segMan, obj, SELECTOR(y), y);
	}

	return s->r_acc;
}

reg_t kJoystick(EngineState *s, int argc, reg_t *argv) {
	// Subfunction 12 sets/gets joystick repeat rate
	debug(5, "Unimplemented syscall 'Joystick()'");
	return NULL_REG;
}

} // End of namespace Sci
