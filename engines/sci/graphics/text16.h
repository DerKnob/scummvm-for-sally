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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/graphics/text16.h $
 * $Id: text16.h 47904 2010-02-05 15:48:45Z m_kiewitz $
 *
 */

#ifndef SCI_GRAPHICS_TEXT16_H
#define SCI_GRAPHICS_TEXT16_H

namespace Sci {

#define SCI_TEXT16_ALIGNMENT_RIGHT -1
#define SCI_TEXT16_ALIGNMENT_CENTER 1
#define SCI_TEXT16_ALIGNMENT_LEFT	0

class GfxPorts;
class GfxPaint16;
class GfxScreen;
class GfxFont;
/**
 * Text16 class, handles text calculation and displaying of text for SCI0->SCI1.1 games
 */
class GfxText16 {
public:
	GfxText16(ResourceManager *_resMan, GfxCache *fonts, GfxPorts *ports, GfxPaint16 *paint16, GfxScreen *screen);
	~GfxText16();

	GuiResourceId GetFontId();
	GfxFont *GetFont();
	void SetFont(GuiResourceId fontId);

	void CodeSetFonts(int argc, reg_t *argv);
	void CodeSetColors(int argc, reg_t *argv);
	int16 CodeProcessing(const char *&text, GuiResourceId orgFontId, int16 orgPenColor);

	void ClearChar(int16 chr);

	int16 GetLongest(const char *text, int16 maxWidth, GuiResourceId orgFontId);
	void Width(const char *text, int16 from, int16 len, GuiResourceId orgFontId, int16 &textWidth, int16 &textHeight);
	void StringWidth(const char *str, GuiResourceId orgFontId, int16 &textWidth, int16 &textHeight);
	void ShowString(const char *str, GuiResourceId orgFontId, int16 orgPenColor);
	void DrawString(const char *str, GuiResourceId orgFontId, int16 orgPenColor);
	int16 Size(Common::Rect &rect, const char *str, GuiResourceId fontId, int16 maxWidth);
	void Draw(const char *text, int16 from, int16 len, GuiResourceId orgFontId, int16 orgPenColor);
	void Show(const char *text, int16 from, int16 len, GuiResourceId orgFontId, int16 orgPenColor);
	void Box(const char *text, int16 bshow, const Common::Rect &rect, TextAlignment alignment, GuiResourceId fontId);
	void Draw_String(const char *text);

	GfxFont *_font;

private:
	void init();

	ResourceManager *_resMan;
	GfxCache *_cache;
	GfxPorts *_ports;
	GfxPaint16 *_paint16;
	GfxScreen *_screen;

	int _codeFontsCount;
	GuiResourceId *_codeFonts;
	int _codeColorsCount;
	uint16 *_codeColors;
};

} // End of namespace Sci

#endif
