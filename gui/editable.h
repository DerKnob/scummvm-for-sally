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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/gui/editable.h $
 * $Id: editable.h 48101 2010-02-21 04:04:13Z bluddy $
 */

#ifndef GUI_EDITABLE_H
#define GUI_EDITABLE_H

#include "common/str.h"
#include "common/rect.h"
#include "gui/widget.h"
#include "gui/GuiManager.h"

namespace GUI {

/**
 * Base class for widgets which need to edit text, like ListWidget and
 * EditTextWidget.
 */
class EditableWidget : public Widget, public CommandSender {
public:
	typedef Common::String String;
protected:
	String		_editString;

	uint32		_cmd;

	bool		_caretVisible;
	uint32		_caretTime;
	int			_caretPos;

	bool		_caretInverse;

	int			_editScrollOffset;

	ThemeEngine::FontStyle  _font;

public:
	EditableWidget(GuiObject *boss, int x, int y, int w, int h, uint32 cmd = 0);
	EditableWidget(GuiObject *boss, const String &name, uint32 cmd = 0);
	virtual ~EditableWidget();

	void init();

	virtual void setEditString(const String &str);
	virtual const String &getEditString() const		{ return _editString; }

	virtual void handleTickle();
	virtual bool handleKeyDown(Common::KeyState state);

	virtual void reflowLayout();

protected:
	virtual void startEditMode() = 0;
	virtual void endEditMode() = 0;
	virtual void abortEditMode() = 0;

	virtual Common::Rect getEditRect() const = 0;
	virtual int getCaretOffset() const;
	void drawCaret(bool erase);
	bool setCaretPos(int newPos);
	bool adjustOffset();
	void makeCaretVisible();

	void defaultKeyDownHandler(Common::KeyState &state, bool &dirty, bool &forcecaret, bool &handled);

	void setFontStyle(ThemeEngine::FontStyle font) { _font = font; }

	virtual bool tryInsertChar(byte c, int pos);
};

} // End of namespace GUI

#endif
