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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/gob/draw_bargon.cpp $
 * $Id: draw_bargon.cpp 45616 2009-11-02 21:54:57Z fingolfin $
 *
 */

#include "gob/gob.h"
#include "gob/draw.h"
#include "gob/global.h"
#include "gob/video.h"

namespace Gob {

Draw_Bargon::Draw_Bargon(GobEngine *vm) : Draw_v2(vm) {
}

void Draw_Bargon::initScreen() {
	_vm->_global->_videoMode = 0x14;
	_vm->_video->_surfWidth = 640;
	_vm->_video->initPrimary(_vm->_global->_videoMode);

	Draw_v2::initScreen();
}

} // End of namespace Gob
