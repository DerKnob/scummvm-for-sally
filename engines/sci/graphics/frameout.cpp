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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/graphics/frameout.cpp $
 * $Id: frameout.cpp 48046 2010-02-13 17:42:49Z fingolfin $
 *
 */

#include "common/util.h"
#include "common/stack.h"
#include "graphics/primitives.h"

#include "sci/sci.h"
#include "sci/engine/state.h"
#include "sci/engine/selector.h"
#include "sci/engine/vm.h"
#include "sci/graphics/cache.h"
#include "sci/graphics/coordadjuster.h"
#include "sci/graphics/font.h"
#include "sci/graphics/view.h"
#include "sci/graphics/screen.h"
#include "sci/graphics/paint32.h"
#include "sci/graphics/picture.h"
#include "sci/graphics/frameout.h"

namespace Sci {

GfxFrameout::GfxFrameout(SegManager *segMan, ResourceManager *resMan, GfxCoordAdjuster *coordAdjuster, GfxCache *cache, GfxScreen *screen, GfxPalette *palette, GfxPaint32 *paint32)
	: _segMan(segMan), _resMan(resMan), _cache(cache), _screen(screen), _palette(palette), _paint32(paint32) {

	_coordAdjuster = (GfxCoordAdjuster32 *)coordAdjuster;
	_highPlanePri = 0;
}

GfxFrameout::~GfxFrameout() {
}

void GfxFrameout::kernelAddPlane(reg_t object) {
	_planes.push_back(object);
	int16 planePri = GET_SEL32V(_segMan, object, SELECTOR(priority)) & 0xFFFF;
	if (planePri > _highPlanePri)
		_highPlanePri = planePri;
}

void GfxFrameout::kernelUpdatePlane(reg_t object) {
}

void GfxFrameout::kernelDeletePlane(reg_t object) {
	for (uint32 planeNr = 0; planeNr < _planes.size(); planeNr++) {
		if (_planes[planeNr] == object) {
			_planes.remove_at(planeNr);
			break;
		}
	}

	// Recalculate highPlanePri
	_highPlanePri = 0;

	for (uint32 planeNr = 0; planeNr < _planes.size(); planeNr++) {
		int16 planePri = GET_SEL32V(_segMan, _planes[planeNr], SELECTOR(priority)) & 0xFFFF;
		if (planePri > _highPlanePri)
			_highPlanePri = planePri;
	}
}

void GfxFrameout::kernelAddScreenItem(reg_t object) {
	_screenItems.push_back(object);
	warning("addScreenItem %X:%X (%s)", object.segment, object.offset, _segMan->getObjectName(object));
}

void GfxFrameout::kernelDeleteScreenItem(reg_t object) {
	for (uint32 itemNr = 0; itemNr < _screenItems.size(); itemNr++) {
		if (_screenItems[itemNr] == object) {
			_screenItems.remove_at(itemNr);
			return;
		}
	}
}

int16 GfxFrameout::kernelGetHighPlanePri() {
	return _highPlanePri;
}

bool sortHelper(const FrameoutEntry* entry1, const FrameoutEntry* entry2) {
	return (entry1->priority == entry2->priority) ? (entry1->y < entry2->y) : (entry1->priority < entry2->priority);
}

void GfxFrameout::kernelFrameout() {
	int16 itemCount = 0;
	reg_t planeObject;
	GuiResourceId planePictureNr;
	GfxPicture *planePicture = 0;
	int16 planePictureCels = 0;
	int16 planePictureCel;
	int16 planePriority;
	Common::Rect planeRect;
	int16 planeResY, planeResX;
	byte planeBack;

	reg_t itemObject;
	reg_t itemPlane;

	FrameoutEntry *itemData;
	FrameoutList itemList;
	FrameoutEntry *itemEntry;

	// Allocate enough space for all screen items
	itemData = (FrameoutEntry *)malloc(_screenItems.size() * sizeof(FrameoutEntry));

	for (uint32 planeNr = 0; planeNr < _planes.size(); planeNr++) {
		planeObject = _planes[planeNr];
		planePriority = GET_SEL32V(_segMan, planeObject, SELECTOR(priority));

		if (planePriority == -1) // Plane currently not meant to be shown
			continue;

		planeRect.top = GET_SEL32V(_segMan, planeObject, SELECTOR(top));
		planeRect.left = GET_SEL32V(_segMan, planeObject, SELECTOR(left));
		planeRect.bottom = GET_SEL32V(_segMan, planeObject, SELECTOR(bottom));
		planeRect.right = GET_SEL32V(_segMan, planeObject, SELECTOR(right));
		planeResY = GET_SEL32V(_segMan, planeObject, SELECTOR(resY));
		planeResX = GET_SEL32V(_segMan, planeObject, SELECTOR(resX));

		planeRect.top = (planeRect.top * _screen->getHeight()) / planeResY;
		planeRect.left = (planeRect.left * _screen->getWidth()) / planeResX;
		planeRect.bottom = (planeRect.bottom * _screen->getHeight()) / planeResY;
		planeRect.right = (planeRect.right * _screen->getWidth()) / planeResX;

		planeBack = GET_SEL32V(_segMan, planeObject, SELECTOR(back));
		if (planeBack) {
			_paint32->fillRect(planeRect, planeBack);
		}

		planePictureNr = GET_SEL32V(_segMan, planeObject, SELECTOR(picture));
		if ((planePictureNr != 0xFFFF) && (planePictureNr != 0xFFFE)) {
			planePicture = new GfxPicture(_resMan, _coordAdjuster, 0, _screen, _palette, planePictureNr, false);
			planePictureCels = planePicture->getSci32celCount();

			_coordAdjuster->pictureSetDisplayArea(planeRect);
		}

		// Fill our itemlist for this plane
		itemCount = 0;
		itemEntry = itemData;
		for (uint32 itemNr = 0; itemNr < _screenItems.size(); itemNr++) {
			itemObject = _screenItems[itemNr];
			itemPlane = GET_SEL32(_segMan, itemObject, SELECTOR(plane));
			if (planeObject == itemPlane) {
				// Found an item on current plane
				itemEntry->viewId = GET_SEL32V(_segMan, itemObject, SELECTOR(view));
				itemEntry->loopNo = GET_SEL32V(_segMan, itemObject, SELECTOR(loop));
				itemEntry->celNo = GET_SEL32V(_segMan, itemObject, SELECTOR(cel));
				itemEntry->x = GET_SEL32V(_segMan, itemObject, SELECTOR(x));
				itemEntry->y = GET_SEL32V(_segMan, itemObject, SELECTOR(y));
				itemEntry->z = GET_SEL32V(_segMan, itemObject, SELECTOR(z));
				itemEntry->priority = GET_SEL32V(_segMan, itemObject, SELECTOR(priority));
				itemEntry->signal = GET_SEL32V(_segMan, itemObject, SELECTOR(signal));
				itemEntry->scaleX = GET_SEL32V(_segMan, itemObject, SELECTOR(scaleX));
				itemEntry->scaleY = GET_SEL32V(_segMan, itemObject, SELECTOR(scaleY));
				itemEntry->object = itemObject;

				itemEntry->y = ((itemEntry->y * _screen->getHeight()) / planeResY);
				itemEntry->x = ((itemEntry->x * _screen->getWidth()) / planeResX);
				itemEntry->y += planeRect.top;
				itemEntry->x += planeRect.left;

				if (itemEntry->priority == 0)
					itemEntry->priority = itemEntry->y;

				itemList.push_back(itemEntry);
				itemEntry++;
				itemCount++;
			}
		}

		// Now sort our itemlist
		Common::sort(itemList.begin(), itemList.end(), sortHelper);

		// Now display itemlist
		planePictureCel = 0;

		itemEntry = itemData;
		FrameoutList::iterator listIterator = itemList.begin();
		FrameoutList::iterator listEnd = itemList.end();
		while (listIterator != listEnd) {
			itemEntry = *listIterator;
			if (planePicture) {
				while ((planePictureCel <= itemEntry->priority) && (planePictureCel < planePictureCels)) {
					planePicture->drawSci32Vga(planePictureCel);
					planePictureCel++;
				}
			}
			if (itemEntry->viewId != 0xFFFF) {
				GfxView *view = _cache->getView(itemEntry->viewId);

				if ((itemEntry->scaleX == 128) && (itemEntry->scaleY == 128)) {
					view->getCelRect(itemEntry->loopNo, itemEntry->celNo, itemEntry->x, itemEntry->y, itemEntry->z, &itemEntry->celRect);
				} else
					view->getCelScaledRect(itemEntry->loopNo, itemEntry->celNo, itemEntry->x, itemEntry->y, itemEntry->z, itemEntry->scaleX, itemEntry->scaleY, &itemEntry->celRect);

				if (itemEntry->celRect.top < 0 || itemEntry->celRect.top >= _screen->getHeight()) {
					listIterator++;
					continue;
				}

				if (itemEntry->celRect.left < 0 || itemEntry->celRect.left >= _screen->getWidth()) {
					listIterator++;
					continue;
				}

				Common::Rect clipRect;
				clipRect = itemEntry->celRect;
				clipRect.clip(planeRect);

				if ((itemEntry->scaleX == 128) && (itemEntry->scaleY == 128))
					view->draw(itemEntry->celRect, clipRect, clipRect, itemEntry->loopNo, itemEntry->celNo, 255, 0, false);
				else
					view->drawScaled(itemEntry->celRect, clipRect, clipRect, itemEntry->loopNo, itemEntry->celNo, 255, itemEntry->scaleX, itemEntry->scaleY);
			} else {
				// Most likely a text entry
				// This draws text the "SCI0-SCI11" way. In SCI2, text is prerendered in kCreateTextBitmap
				// TODO: rewrite this the "SCI2" way (i.e. implement the text buffer to draw inside kCreateTextBitmap)
				// This doesn't work for SCI2.1 games...
				if (getSciVersion() == SCI_VERSION_2) {
					Kernel *kernel = g_sci->getKernel();
					if (lookup_selector(_segMan, itemEntry->object, kernel->_selectorCache.text, NULL, NULL) == kSelectorVariable) {
						Common::String text = _segMan->getString(GET_SEL32(_segMan, itemEntry->object, SELECTOR(text)));
						int16 fontRes = GET_SEL32V(_segMan, itemEntry->object, SELECTOR(font));
						GfxFont *font = new GfxFont(_resMan, _screen, fontRes);
						bool dimmed = GET_SEL32V(_segMan, itemEntry->object, SELECTOR(dimmed));
						uint16 foreColor = GET_SEL32V(_segMan, itemEntry->object, SELECTOR(fore));
						uint16 curX = itemEntry->x;
						uint16 curY = itemEntry->y;
						for (uint32 i = 0; i < text.size(); i++) {
							// TODO: proper text splitting... this is a hack
							if ((text[i] == ' ' && i > 0 && text[i - i] == ' ') || text[i] == '\n' || 
								(curX + font->getCharWidth(text[i]) > _screen->getWidth())) {
								curY += font->getCharHeight('A');
								curX = itemEntry->x;
							}
							font->draw(text[i], curY, curX, foreColor, dimmed);
							curX += font->getCharWidth(text[i]);
						}
						delete font;
					}
				}
			}
			listIterator++;
		}
		if (planePicture) {
			while (planePictureCel < planePictureCels) {
				planePicture->drawSci32Vga(planePictureCel);
				planePictureCel++;
			}
			delete planePicture;
			planePicture = 0;
		}
	}
	free(itemData);
	_screen->copyToScreen();
}

} // End of namespace Sci
