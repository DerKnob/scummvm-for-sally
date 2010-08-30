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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/graphics/compare.h $
 * $Id: compare.h 47920 2010-02-05 22:56:05Z fingolfin $
 *
 */

#ifndef SCI_GRAPHICS_GFX_H
#define SCI_GRAPHICS_GFX_H

#include "sci/graphics/gui.h"

#include "common/hashmap.h"

namespace Sci {

class Screen;

/**
 * Compare class, handles compare operations graphic-wise (like when checking control screen for a pattern etc.)
 */
class GfxCompare {
public:
	GfxCompare(SegManager *segMan, Kernel *kernel, GfxCache *cache, GfxScreen *screen, GfxCoordAdjuster *coordAdjuster);
	~GfxCompare();

	uint16 kernelOnControl(byte screenMask, const Common::Rect &rect);
	void kernelSetNowSeen(reg_t objectReference);
	bool kernelCanBeHere(reg_t curObject, reg_t listReference);
	bool kernelIsItSkip(GuiResourceId viewId, int16 loopNo, int16 celNo, Common::Point position);
	void kernelBaseSetter(reg_t object);

private:
	SegManager *_segMan;
	Kernel *_kernel;
	GfxCache *_cache;
	GfxScreen *_screen;
	GfxCoordAdjuster *_coordAdjuster;

	uint16 isOnControl(uint16 screenMask, const Common::Rect &rect);
	
	/**
	 * This function checks whether any of the objects in the given list,
	 * *different* from checkObject, has a brRect which is contained inside
	 * checkRect.
	 */
	bool canBeHereCheckRectList(reg_t checkObject, const Common::Rect &checkRect, List *list);
};

} // End of namespace Sci

#endif