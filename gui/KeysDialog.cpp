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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/gui/KeysDialog.cpp $
 * $Id: KeysDialog.cpp 35416 2008-12-17 21:19:46Z anotherguest $
 *
 */

#include "gui/KeysDialog.h"
#include "gui/Actions.h"
#include <SDL_keyboard.h>

#ifdef _WIN32_WCE
#include "CEDevice.h"
#endif

namespace GUI {

enum {
	kMapCmd	= 'map ',
	kOKCmd	= 'ok  '
};

KeysDialog::KeysDialog(const Common::String &title)
	: GUI::Dialog("KeysDialog") {

	new ButtonWidget(this, "KeysDialog.Map", "Map", kMapCmd, 0);
	new ButtonWidget(this, "KeysDialog.Ok", "OK", kOKCmd, 0);
	new ButtonWidget(this, "KeysDialog.Cancel", "Cancel", kCloseCmd, 0);

	_actionsList = new ListWidget(this, "KeysDialog.List");
	_actionsList->setNumberingMode(kListNumberingZero);

	_actionTitle = new StaticTextWidget(this, "KeysDialog.Action", title);
	_keyMapping = new StaticTextWidget(this, "KeysDialog.Mapping", "Select an action and click 'Map'");

	_actionTitle->setFlags(WIDGET_CLEARBG);
	_keyMapping->setFlags(WIDGET_CLEARBG);

	// Get actions names
	Common::StringList l;

	for (int i = 0; i < Actions::Instance()->size(); i++)
		l.push_back(Actions::Instance()->actionName((ActionType)i));

	_actionsList->setList(l);

	_actionSelected = -1;
	Actions::Instance()->beginMapping(false);
}

void KeysDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {

	case kListSelectionChangedCmd:
		if (_actionsList->getSelected() >= 0) {
			char selection[100];

			uint16 key = Actions::Instance()->getMapping(_actionsList->getSelected());
#ifdef __SYMBIAN32__
			// ScummVM mappings for F1-F9 are different from SDL so remap back to sdl
			if (key >= Common::ASCII_F1 && key <= Common::ASCII_F9)
				key = key - Common::ASCII_F1 + SDLK_F1;
#endif
			if (key != 0)
				sprintf(selection, "Associated key : %s", SDL_GetKeyName((SDLKey)key));
			else
				sprintf(selection, "Associated key : none");

			_keyMapping->setLabel(selection);
			_keyMapping->draw();
		}
		break;
	case kMapCmd:
		if (_actionsList->getSelected() < 0) {
				_actionTitle->setLabel("Please select an action");
		} else {
			char selection[100];

			_actionSelected = _actionsList->getSelected();
			uint16 key = Actions::Instance()->getMapping(_actionSelected);
#ifdef __SYMBIAN32__
			// ScummVM mappings for F1-F9 are different from SDL so remap back to sdl
			if (key >= Common::ASCII_F1 && key <= Common::ASCII_F9)
				key = key - Common::ASCII_F1 + SDLK_F1;
#endif
			if (key != 0)
				sprintf(selection, "Associated key : %s", SDL_GetKeyName((SDLKey)key));
			else
				sprintf(selection, "Associated key : none");

			_actionTitle->setLabel("Press the key to associate");
			_keyMapping->setLabel(selection);
			_keyMapping->draw();
			Actions::Instance()->beginMapping(true);
			_actionsList->setEnabled(false);
		}
		_actionTitle->draw();
		break;
	case kOKCmd:
		Actions::Instance()->saveMapping();
		close();
		break;
	case kCloseCmd:
		Actions::Instance()->loadMapping();
		close();
		break;
	}
}

void KeysDialog::handleKeyDown(Common::KeyState state){
	if (!Actions::Instance()->mappingActive())
		Dialog::handleKeyDown(state);
}

void KeysDialog::handleKeyUp(Common::KeyState state) {
#ifdef __SYMBIAN32__
	if (Actions::Instance()->mappingActive()) {
#else
	if (state.flags == 0xff  && Actions::Instance()->mappingActive()) {	// GAPI key was selected
#endif
		char selection[100];

		Actions::Instance()->setMapping((ActionType)_actionSelected, state.ascii);

		if (state.ascii != 0)
			sprintf(selection, "Associated key : %s", SDL_GetKeyName((SDLKey) state.keycode));
		else
			sprintf(selection, "Associated key : none");

		_actionTitle->setLabel("Choose an action to map");
		_keyMapping->setLabel(selection);
		_keyMapping->draw();
		_actionTitle->draw();
		_actionSelected = -1;
		_actionsList->setEnabled(true);
		Actions::Instance()->beginMapping(false);
	} else
		Dialog::handleKeyUp(state);
}

} // namespace GUI
