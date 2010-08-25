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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/backends/platform/wince/CEgui/PanelItem.cpp $
 * $Id: PanelItem.cpp 28966 2007-09-19 08:40:12Z peres001 $
 *
 */

#include "PanelItem.h"

namespace CEGUI {

	PanelItem::PanelItem(WORD reference) : GUIElement() {
		setBackground(reference);
		_panel = NULL;
	}


	PanelItem::~PanelItem() {
	}

	bool PanelItem::action(int x, int y, bool pushed) {
		return false;
	}

	void PanelItem::setPanel(Panel *panel) {
		_panel = panel;
	}
}

