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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/m4/mads_scene.h $
 * $Id: mads_scene.h 48172 2010-03-07 05:06:58Z dreammaster $
 *
 */

#ifndef M4_MADS_SCENE_H
#define M4_MADS_SCENE_H

#include "m4/scene.h"

namespace M4 {

#define INTERFACE_HEIGHT 106
#define MADS_SURFACE_HEIGHT 156

struct SpriteSlot {
	int16 spriteId;
	int16 scale;
	uint16 spriteListIndex;
};

struct DirtyArea {
	bool active;
	bool active2;
	Common::Rect bounds;
};

class MadsSceneInfo {
public:
	int sceneId;
	int artFileNum;
	int field_4;
	int width;
	int height;

	int objectCount;
	MadsObject objects[32];
	
	int walkSize;
	byte *walkData;

	MadsSceneInfo() { walkSize = 0; walkData = NULL; }
	~MadsSceneInfo() { delete walkData; }
	void load(int sceneId);	
};

#define TIMED_TEXT_SIZE 10
#define TEXT_DISPLAY_SIZE 40
#define TEXT_4A_SIZE 30

enum TalkTextFlags {TEXTFLAG_2 = 2, TEXTFLAG_4 = 4, TEXTFLAG_8 = 8, TEXTFLAG_40 = 0x40,
		TEXTFLAG_ACTIVE = 0x80};

struct TextDisplay {
	bool active;
	int spacing;
	int16 active2;
	Common::Rect bounds;
	int colour1, colour2;
	Font *font;
	char message[100];
};

struct TimedText {
	uint8 flags;
	int colour1;
	int colour2;
	Common::Point position;
	int textDisplayIndex;
	int unk4AIndex;
	uint32 timeout;
	uint32 frameTimer;
	bool field_1C;
	uint8 field_1D;
	uint16 actionNouns[3];
	char message[100];
};

struct Text4A {
	uint8 active;
	uint8 field25;
};

class MadsScreenText {
private:
	TextDisplay _textDisplay[TEXT_DISPLAY_SIZE];
	TimedText _timedText[TIMED_TEXT_SIZE];
	Text4A _text4A[TEXT_4A_SIZE];
	bool _abortTimedText;

	void addTimedText(TimedText *entry);
public:
	MadsScreenText();

	// TextDisplay list
	int add(const Common::Point &destPos, uint fontColours, int widthAdjust, const char *msg, Font *font);
	void setActive2(int16 idx) { _textDisplay[idx].active2 = -1; }
	// TimedText list
	int addTimed(const Common::Point &destPos, uint fontColours, uint flags, int vUnknown, uint32 timeout, const char *message);

	void draw(M4Surface *surface);
	void timedDisplay();
};

enum MadsActionMode {ACTMODE_NONE = 0, ACTMODE_VERB = 1, ACTMODE_OBJECT = 3, ACTMODE_TALK = 6};

class MadsAction {
private:
	char _statusText[100];
	int _currentHotspot;
	int _objectNameId;
	int _objectDescId;
	int _currentAction;
	int8 _flags1, _flags2;
	MadsActionMode _actionMode;
	int _articleNumber;
	bool _lookFlag;
	int _selectedRow;

	void appendVocab(int vocabId, bool capitalise = false);
public:
	MadsAction();

	void clear();
	void set();
};

typedef Common::Array<SpriteAsset *> SpriteAssetArray;

#define SPRITE_SLOTS_SIZE 50
#define DIRTY_AREA_SIZE 90

class MadsScene : public Scene {
private:
	MadsEngine *_vm;
	char _statusText[100];

	MadsSceneLogic _sceneLogic;
	MadsSceneInfo _sceneInfo;
	SpriteAsset *_playerSprites;
	SpriteAssetArray _sceneSprites;
	SpriteSlot _spriteSlots[50];
	MadsScreenText _textDisplay;
	DirtyArea _dirtyAreas[DIRTY_AREA_SIZE];
	int _spriteSlotsStart;

	void drawElements();
	void loadScene2(const char *aaName);
	void loadSceneTemporary();
	void clearAction();
	void appendActionVocab(int vocabId, bool capitalise);
	void setAction();
public:
	char _aaName[100];
	uint16 actionNouns[3];
public:
	MadsScene(MadsEngine *vm);

	// Methods that differ between engines
	virtual void loadScene(int sceneNumber);
	virtual void leaveScene();
	virtual void loadSceneCodes(int sceneNumber, int index = 0);
	virtual void show();
	virtual void checkHotspotAtMousePos(int x, int y);
	virtual void leftClick(int x, int y);
	virtual void rightClick(int x, int y);
	virtual void setAction(int action, int objectId = -1);
	virtual void setStatusText(const char *text);
	virtual void update();

	int loadSceneSpriteSet(const char *setName);
	void loadPlayerSprites(const char *prefix);

	MadsInterfaceView *getInterface() { return (MadsInterfaceView *)_interfaceSurface; };
};

} // End of namespace M4

#endif
