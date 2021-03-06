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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/gui/dialog.h $
 * $Id: dialog.h 41311 2009-06-06 23:22:48Z fingolfin $
 */

#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#include "common/scummsys.h"
#include "common/str.h"

#include "gui/object.h"
#include "gui/widget.h"

namespace GUI {

class GuiManager;
class PopUpWidget;

// Some "common" commands sent to handleCommand()
enum {
	kCloseCmd = 'clos'
};

class Dialog : public GuiObject {
	friend class GuiManager;
protected:
	Widget	*_mouseWidget;
	Widget  *_focusedWidget;
	Widget  *_dragWidget;
	bool	_visible;

	ThemeEngine::DialogBackground _backgroundType;

private:
	int		_result;

public:
	Dialog(int x, int y, int w, int h);
	Dialog(const Common::String &name);

	virtual int runModal();

	bool	isVisible() const	{ return _visible; }

	void	releaseFocus();
	void	setFocusWidget(Widget *widget);
	Widget *getFocusWidget() { return _focusedWidget; }

	virtual void reflowLayout();

protected:
	virtual void open();
	virtual void close();

	virtual void draw();
	virtual void drawDialog();

	virtual void handleTickle(); // Called periodically (in every guiloop() )
	virtual void handleMouseDown(int x, int y, int button, int clickCount);
	virtual void handleMouseUp(int x, int y, int button, int clickCount);
	virtual void handleMouseWheel(int x, int y, int direction);
	virtual void handleKeyDown(Common::KeyState state);
	virtual void handleKeyUp(Common::KeyState state);
	virtual void handleMouseMoved(int x, int y, int button);
	virtual void handleCommand(CommandSender *sender, uint32 cmd, uint32 data);

	Widget *findWidget(int x, int y); // Find the widget at pos x,y if any
	Widget *findWidget(const char *name);
	void removeWidget(Widget *widget);

	void setResult(int result) { _result = result; }
	int getResult() const { return _result; }
};

} // End of namespace GUI

#endif
