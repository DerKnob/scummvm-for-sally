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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/graphics/animate.h $
 * $Id: animate.h 47904 2010-02-05 15:48:45Z m_kiewitz $
 *
 */

#ifndef SCI_GRAPHICS_ANIMATE_H
#define SCI_GRAPHICS_ANIMATE_H

#include "sci/graphics/helpers.h"

namespace Sci {

// Flags for the signal selector
enum ViewSignals {
	kSignalStopUpdate    = 0x0001,
	kSignalViewUpdated   = 0x0002,
	kSignalNoUpdate      = 0x0004,
	kSignalHidden        = 0x0008,
	kSignalFixedPriority = 0x0010,
	kSignalAlwaysUpdate  = 0x0020,
	kSignalForceUpdate   = 0x0040,
	kSignalRemoveView    = 0x0080,
	kSignalFrozen        = 0x0100,
	kSignalExtraActor	 = 0x0200, // unused by us, defines all actors that may be included into the background if speed is too slow
	kSignalHitObstacle	 = 0x0400, // used in the actor movement code by kDoBresen()
	kSignalDoesntTurn	 = 0x0800, // used by _k_dirloop() to determine if an actor can turn or not
	kSignalNoCycler		 = 0x1000, // unused by us
	kSignalIgnoreHorizon = 0x2000, // unused by us, defines actor that can ignore horizon
	kSignalIgnoreActor   = 0x4000,
	kSignalDisposeMe     = 0x8000
};

enum ViewScaleSignals {
	kScaleSignalDoScaling	= 0x0001, // enables scaling when drawing that cel (involves scaleX and scaleY)
	kScaleSignalUnknown1	= 0x0002, // seems to do something with globalvar 2, sets scaleX/scaleY
	kScaleSignalUnknown2	= 0x0004 // really unknown
};

struct AnimateEntry {
	reg_t object;
	GuiResourceId viewId;
	int16 loopNo;
	int16 celNo;
	int16 paletteNo;
	int16 x, y, z;
	int16 priority;
	uint16 signal;
	uint16 scaleSignal;
	int16 scaleX;
	int16 scaleY;
	Common::Rect celRect;
	bool showBitsFlag;
	reg_t castHandle;
};
typedef Common::List<AnimateEntry *> AnimateList;

class GfxCache;
class GfxCursor;
class GfxPorts;
class GfxPaint16;
class GfxScreen;
class GfxPalette;
class GfxTransitions;
/**
 * Animate class, kAnimate and relevant functions for SCI16 (SCI0-SCI1.1) games
 */
class GfxAnimate {
public:
	GfxAnimate(EngineState *state, GfxCache *cache, GfxPorts *ports, GfxPaint16 *paint16, GfxScreen *screen, GfxPalette *palette, GfxCursor *cursor, GfxTransitions *transitions);
	virtual ~GfxAnimate();

	// FIXME: Don't store EngineState
	void resetEngineState(EngineState *newState) { _s = newState; }

	void disposeLastCast();
	bool invoke(List *list, int argc, reg_t *argv);
	void makeSortedList(List *list);
	void fill(byte &oldPicNotValid);
	void update();
	void drawCels();
	void updateScreen(byte oldPicNotValid);
	void restoreAndDelete(int argc, reg_t *argv);
	void reAnimate(Common::Rect rect);
	void addToPicDrawCels();
	void addToPicDrawView(GuiResourceId viewId, int16 loopNo, int16 celNo, int16 leftPos, int16 topPos, int16 priority, int16 control);

	uint16 getLastCastCount() { return _lastCastCount; };

	virtual void kernelAnimate(reg_t listReference, bool cycle, int argc, reg_t *argv);
	virtual void kernelAddToPicList(reg_t listReference, int argc, reg_t *argv);
	virtual void kernelAddToPicView(GuiResourceId viewId, int16 loopNo, int16 celNo, int16 leftPos, int16 topPos, int16 priority, int16 control);

private:
	void init();

	void addToPicSetPicNotValid();
	void animateShowPic();

	EngineState *_s;
	GfxCache *_cache;
	GfxPorts *_ports;
	GfxPaint16 *_paint16;
	GfxScreen *_screen;
	GfxPalette *_palette;
	GfxCursor *_cursor;
	GfxTransitions *_transitions;

	uint16 _listCount;
	AnimateEntry *_listData;
	AnimateList _list;

	uint16 _lastCastCount;
	AnimateEntry *_lastCastData;

	bool _ignoreFastCast;
};

} // End of namespace Sci

#endif
