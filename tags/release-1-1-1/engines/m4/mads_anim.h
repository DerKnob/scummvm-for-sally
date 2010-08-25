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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/m4/mads_anim.h $
 * $Id: mads_anim.h 47733 2010-01-31 00:47:29Z dreammaster $
 *
 */

#ifndef M4_MADS_ANIM_H
#define M4_MADS_ANIM_H

#include "m4/viewmgr.h"
#include "m4/compression.h"

namespace M4 {

enum SceneTransition {
	kTransitionNone = 0,
	kTransitionFadeIn = 1,
	kTransitionFadeIn2 = 2,
	kTransitionBoxInBottomLeft = 3,
	kTransitionBoxInBottomRight = 4,
	kTransitionBoxInTopLeft = 5,
	kTransitionBoxInTopRight = 6,
	kTransitionPanLeftToRight = 7,
	kTransitionPanRightToLeft = 8,
	kTransitionCircleIn = 9
};

typedef void (*TextviewCallback)(MadsM4Engine *vm);

class TextviewView : public View {
private:
	bool _animating;

	char _resourceName[80];
	Common::SeekableReadStream *_script;
	uint16 _spareScreens[10];
	M4Surface *_spareScreen;
	RGBList *_bgCurrent, *_bgSpare;
	int _translationX;
	int _panX, _panY, _panSpeed;
	int _panCountdown;
	char _currentLine[80];
	uint32 _scrollTimeout;
	int _scrollCount;
	int _lineY;
	M4Surface _bgSurface;
	M4Surface _textSurface;
	TextviewCallback _callback;
	bool _soundDriverLoaded;
	bool _processEvents;

	void reset();
	void processLines();
	void processCommand();
	void processText();
	int getParameter(char **paramP);
public:
	TextviewView(MadsM4Engine *vm);
	~TextviewView();

	void setScript(const char *resourceName, TextviewCallback callback);
	bool isAnimating() { return _animating; }
	void scriptDone();

	bool onEvent(M4EventType eventType, int32 param1, int x, int y, bool &captureEvents);
	void updateState();
};

typedef void (*AnimviewCallback)(MadsM4Engine *vm);

class AAFile : public MadsPack {
public:
	AAFile(const char *resourceName, MadsM4Engine* vm);

	uint16 seriesCount;
	uint16 frameCount;
	uint16 frameEntryCount;
	uint8 flags;
	uint16 roomNumber;
	uint16 frameTicks;
	Common::StringList filenames;
	Common::String lbmFilename;
	Common::String spritesFilename;
	Common::String soundName;
	Common::String fontResource;
};

enum AAFlags {AA_HAS_FONT = 0x20, AA_HAS_SOUND = 0x8000};

class AnimviewView : public View {
private:
	char _resourceName[80];
	Common::SeekableReadStream *_script;
	uint32 _previousUpdate;
	char _currentLine[80];
	M4Surface _bgSurface;
	AnimviewCallback _callback;
	bool _soundDriverLoaded;
	RGBList *_palData;
	int _transition;

	void reset();
	void readNextCommand();
	void processCommand();
public:
	AnimviewView(MadsM4Engine *vm);
	~AnimviewView();

	void setScript(const char *resourceName, AnimviewCallback callback);
	void scriptDone();

	bool onEvent(M4EventType eventType, int32 param1, int x, int y, bool &captureEvents);
	void updateState();
};

}

#endif
