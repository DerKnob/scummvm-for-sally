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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/m4/mads_logic.h $
 * $Id: mads_logic.h 48099 2010-02-21 01:18:03Z dreammaster $
 *
 * The MADS game logic is all hard-coded into the games, although for Rex at least
 * it seems to use only a fairly basic set of instructions and function calls, so it should be 
 * possible 
 */

#ifndef M4_MADS_LOGIC_H
#define M4_MADS_LOGIC_H

namespace M4 {

class MadsSceneLogic {
private:
	// Library interface methods
	uint16 loadSpriteSet(uint16 suffixNum, uint16 sepChar);
private:
	int _sceneNumber;
	uint16 _spriteIndexes[50];

	// Support functions
	const char *formAnimName(char sepChar, int16 suffixNum);
	void getSceneSpriteSet();
	void getAnimName();
public:
	void selectScene(int sceneNum);

	void setupScene();
	void enterScene();
	void doAction();
};

}

#endif
