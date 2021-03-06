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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/graphics/animate.cpp $
 * $Id: animate.cpp 48050 2010-02-13 17:45:40Z fingolfin $
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
#include "sci/graphics/cursor.h"
#include "sci/graphics/ports.h"
#include "sci/graphics/paint16.h"
#include "sci/graphics/view.h"
#include "sci/graphics/screen.h"
#include "sci/graphics/transitions.h"
#include "sci/graphics/animate.h"

namespace Sci {

GfxAnimate::GfxAnimate(EngineState *state, GfxCache *cache, GfxPorts *ports, GfxPaint16 *paint16, GfxScreen *screen, GfxPalette *palette, GfxCursor *cursor, GfxTransitions *transitions)
	: _s(state), _cache(cache), _ports(ports), _paint16(paint16), _screen(screen), _palette(palette), _cursor(cursor), _transitions(transitions) {
	init();
}

GfxAnimate::~GfxAnimate() {
	free(_listData);
	free(_lastCastData);
}

void GfxAnimate::init() {
	_listData = NULL;
	_listCount = 0;
	_lastCastData = NULL;
	_lastCastCount = 0;

	_ignoreFastCast = false;
	// fastCast object is not found in any SCI games prior SCI1
	if (getSciVersion() <= SCI_VERSION_01)
		_ignoreFastCast = true;
	// Also if fastCast object exists at gamestartup, we can assume that the interpreter doesnt do kAnimate aborts
	//  (found in larry 1)
	if (!_s->_segMan->findObjectByName("fastCast").isNull())
		_ignoreFastCast = true;
}

void GfxAnimate::disposeLastCast() {
	_lastCastCount = 0;
}

bool GfxAnimate::invoke(List *list, int argc, reg_t *argv) {
	reg_t curAddress = list->first;
	Node *curNode = _s->_segMan->lookupNode(curAddress);
	reg_t curObject;
	uint16 signal;

	while (curNode) {
		curObject = curNode->value;

		if (!_ignoreFastCast) {
			// Check if the game has a fastCast object set
			//  if we don't abort kAnimate processing, at least in kq5 there will be animation cels drawn into speech boxes.
			reg_t global84 = _s->script_000->_localsBlock->_locals[84];

			if (!global84.isNull()) {
				if (!strcmp(_s->_segMan->getObjectName(global84), "fastCast"))
					return false;
			}
		}

		signal = GET_SEL32V(_s->_segMan, curObject, SELECTOR(signal));
		if (!(signal & kSignalFrozen)) {
			// Call .doit method of that object
			invoke_selector(_s, curObject, g_sci->getKernel()->_selectorCache.doit, kContinueOnInvalidSelector, argc, argv, 0);
			// Lookup node again, since the nodetable it was in may have been reallocated
			curNode = _s->_segMan->lookupNode(curAddress);
		}
		curAddress = curNode->succ;
		curNode = _s->_segMan->lookupNode(curAddress);
	}
	return true;
}

bool sortHelper(const AnimateEntry* entry1, const AnimateEntry* entry2) {
	return (entry1->y == entry2->y) ? (entry1->z < entry2->z) : (entry1->y < entry2->y);
}

void GfxAnimate::makeSortedList(List *list) {
	reg_t curAddress = list->first;
	Node *curNode = _s->_segMan->lookupNode(curAddress);
	reg_t curObject;
	AnimateEntry *listEntry;
	int16 listNr, listCount = 0;

	// Count the list entries
	while (curNode) {
		listCount++;
		curAddress = curNode->succ;
		curNode = _s->_segMan->lookupNode(curAddress);
	}

	_list.clear();

	// No entries -> exit immediately
	if (listCount == 0)
		return;

	// Adjust list size, if needed
	if ((_listData == NULL) || (_listCount < listCount)) {
		free(_listData);
		_listData = (AnimateEntry *)malloc(listCount * sizeof(AnimateEntry));
		if (!_listData)
			error("Could not allocate memory for _listData");
		_listCount = listCount;

		free(_lastCastData);
		_lastCastData = (AnimateEntry *)malloc(listCount * sizeof(AnimateEntry));
		if (!_lastCastData)
			error("Could not allocate memory for _lastCastData");
		_lastCastCount = 0;
	}

	// Fill the list
	curAddress = list->first;
	curNode = _s->_segMan->lookupNode(curAddress);
	listEntry = _listData;
	for (listNr = 0; listNr < listCount; listNr++) {
		curObject = curNode->value;
		listEntry->object = curObject;

		// Get data from current object
		listEntry->viewId = GET_SEL32V(_s->_segMan, curObject, SELECTOR(view));
		listEntry->loopNo = GET_SEL32V(_s->_segMan, curObject, SELECTOR(loop));
		listEntry->celNo = GET_SEL32V(_s->_segMan, curObject, SELECTOR(cel));
		listEntry->paletteNo = GET_SEL32V(_s->_segMan, curObject, SELECTOR(palette));
		listEntry->x = GET_SEL32V(_s->_segMan, curObject, SELECTOR(x));
		listEntry->y = GET_SEL32V(_s->_segMan, curObject, SELECTOR(y));
		listEntry->z = GET_SEL32V(_s->_segMan, curObject, SELECTOR(z));
		listEntry->priority = GET_SEL32V(_s->_segMan, curObject, SELECTOR(priority));
		listEntry->signal = GET_SEL32V(_s->_segMan, curObject, SELECTOR(signal));
		if (getSciVersion() >= SCI_VERSION_1_1) {
			// Cel scaling
			listEntry->scaleSignal = GET_SEL32V(_s->_segMan, curObject, SELECTOR(scaleSignal));
			if (listEntry->scaleSignal & kScaleSignalDoScaling) {
				listEntry->scaleX = GET_SEL32V(_s->_segMan, curObject, SELECTOR(scaleX));
				listEntry->scaleY = GET_SEL32V(_s->_segMan, curObject, SELECTOR(scaleY));
			} else {
				listEntry->scaleX = 128;
				listEntry->scaleY = 128;
			}
			// TODO
			// On scaleSignal bit 1 sierra sci does some stuff with global var 2, current Port
			//  and some other stuff and sets scaleX/Y accordingly. It seems this functionality is needed in at
			//  least sq5 right when starting the game before wilco exists the room. Currently we dont get scaling
			//  but sierra sci does scaling there. I dont fully understand the code yet, that's why i didnt implement
			//  anything.
		} else {
			listEntry->scaleSignal = 0;
			listEntry->scaleX = 128;
			listEntry->scaleY = 128;
		}
		// listEntry->celRect is filled in AnimateFill()
		listEntry->showBitsFlag = false;

		_list.push_back(listEntry);

		listEntry++;
		curAddress = curNode->succ;
		curNode = _s->_segMan->lookupNode(curAddress);
	}

	// Now sort the list according y and z (descending)
	AnimateList::iterator listBegin = _list.begin();
	AnimateList::iterator listEnd = _list.end();

	Common::sort(_list.begin(), _list.end(), sortHelper);
}

void GfxAnimate::fill(byte &old_picNotValid) {
	reg_t curObject;
	AnimateEntry *listEntry;
	uint16 signal;
	GfxView *view = NULL;
	AnimateList::iterator listIterator;
	AnimateList::iterator listEnd = _list.end();

	listIterator = _list.begin();
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;

		// Get the corresponding view
		view = _cache->getView(listEntry->viewId);

		// adjust loop and cel, if any of those is invalid
		if (listEntry->loopNo >= view->getLoopCount()) {
			listEntry->loopNo = 0;
			PUT_SEL32V(_s->_segMan, curObject, SELECTOR(loop), listEntry->loopNo);
		}
		if (listEntry->celNo >= view->getCelCount(listEntry->loopNo)) {
			listEntry->celNo = 0;
			PUT_SEL32V(_s->_segMan, curObject, SELECTOR(cel), listEntry->celNo);
		}

		// Create rect according to coordinates and given cel
		if (listEntry->scaleSignal & kScaleSignalDoScaling) {
			view->getCelScaledRect(listEntry->loopNo, listEntry->celNo, listEntry->x, listEntry->y, listEntry->z, listEntry->scaleX, listEntry->scaleY, &listEntry->celRect);
		} else {
			view->getCelRect(listEntry->loopNo, listEntry->celNo, listEntry->x, listEntry->y, listEntry->z, &listEntry->celRect);
		}
		PUT_SEL32V(_s->_segMan, curObject, SELECTOR(nsLeft), listEntry->celRect.left);
		PUT_SEL32V(_s->_segMan, curObject, SELECTOR(nsTop), listEntry->celRect.top);
		PUT_SEL32V(_s->_segMan, curObject, SELECTOR(nsRight), listEntry->celRect.right);
		PUT_SEL32V(_s->_segMan, curObject, SELECTOR(nsBottom), listEntry->celRect.bottom);

		signal = listEntry->signal;

		// Calculate current priority according to y-coordinate
		if (!(signal & kSignalFixedPriority)) {
			listEntry->priority = _ports->kernelCoordinateToPriority(listEntry->y);
			PUT_SEL32V(_s->_segMan, curObject, SELECTOR(priority), listEntry->priority);
		}

		if (signal & kSignalNoUpdate) {
			if (signal & (kSignalForceUpdate | kSignalViewUpdated)
				|| (signal & kSignalHidden && !(signal & kSignalRemoveView))
				|| (!(signal & kSignalHidden) && signal & kSignalRemoveView)
				|| (signal & kSignalAlwaysUpdate))
				old_picNotValid++;
			signal &= 0xFFFF ^ kSignalStopUpdate;
		} else {
			if (signal & kSignalStopUpdate || signal & kSignalAlwaysUpdate)
				old_picNotValid++;
			signal &= 0xFFFF ^ kSignalForceUpdate;
		}
		listEntry->signal = signal;

		listIterator++;
	}
}

void GfxAnimate::update() {
	reg_t curObject;
	AnimateEntry *listEntry;
	uint16 signal;
	reg_t bitsHandle;
	Common::Rect rect;
	AnimateList::iterator listIterator;
	AnimateList::iterator listBegin = _list.begin();
	AnimateList::iterator listEnd = _list.end();

	// Remove all no-update cels, if requested
	listIterator = _list.reverse_begin();
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;
		signal = listEntry->signal;

		if (signal & kSignalNoUpdate) {
			if (!(signal & kSignalRemoveView)) {
				bitsHandle = GET_SEL32(_s->_segMan, curObject, SELECTOR(underBits));
				if (_screen->_picNotValid != 1) {
					_paint16->bitsRestore(bitsHandle);
					listEntry->showBitsFlag = true;
				} else	{
					_paint16->bitsFree(bitsHandle);
				}
				PUT_SEL32V(_s->_segMan, curObject, SELECTOR(underBits), 0);
			}
			signal &= 0xFFFF ^ kSignalForceUpdate;
			signal &= signal & kSignalViewUpdated ? 0xFFFF ^ (kSignalViewUpdated | kSignalNoUpdate) : 0xFFFF;
		} else if (signal & kSignalStopUpdate) {
			signal =  (signal & (0xFFFF ^ kSignalStopUpdate)) | kSignalNoUpdate;
		}
		listEntry->signal = signal;
		listIterator--;
	}

	// Draw always-update cels
	listIterator = listBegin;
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;
		signal = listEntry->signal;

		if (signal & kSignalAlwaysUpdate) {
			// draw corresponding cel
			_paint16->drawCel(listEntry->viewId, listEntry->loopNo, listEntry->celNo, listEntry->celRect, listEntry->priority, listEntry->paletteNo, listEntry->scaleX, listEntry->scaleY);
			listEntry->showBitsFlag = true;

			signal &= 0xFFFF ^ (kSignalStopUpdate | kSignalViewUpdated | kSignalNoUpdate | kSignalForceUpdate);
			if ((signal & kSignalIgnoreActor) == 0) {
				rect = listEntry->celRect;
				rect.top = CLIP<int16>(_ports->kernelPriorityToCoordinate(listEntry->priority) - 1, rect.top, rect.bottom - 1);
				_paint16->fillRect(rect, SCI_SCREEN_MASK_CONTROL, 0, 0, 15);
			}
			listEntry->signal = signal;
		}
		listIterator++;
	}

	// Saving background for all NoUpdate-cels
	listIterator = listBegin;
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;
		signal = listEntry->signal;

		if (signal & kSignalNoUpdate) {
			if (signal & kSignalHidden) {
				signal |= kSignalRemoveView;
			} else {
				signal &= 0xFFFF ^ kSignalRemoveView;
				if (signal & kSignalIgnoreActor)
					bitsHandle = _paint16->bitsSave(listEntry->celRect, SCI_SCREEN_MASK_VISUAL|SCI_SCREEN_MASK_PRIORITY);
				else
					bitsHandle = _paint16->bitsSave(listEntry->celRect, SCI_SCREEN_MASK_ALL);
				PUT_SEL32(_s->_segMan, curObject, SELECTOR(underBits), bitsHandle);
			}
			listEntry->signal = signal;
		}
		listIterator++;
	}

	// Draw NoUpdate cels
	listIterator = listBegin;
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;
		signal = listEntry->signal;

		if (signal & kSignalNoUpdate && !(signal & kSignalHidden)) {
			// draw corresponding cel
			_paint16->drawCel(listEntry->viewId, listEntry->loopNo, listEntry->celNo, listEntry->celRect, listEntry->priority, listEntry->paletteNo, listEntry->scaleX, listEntry->scaleY);
			listEntry->showBitsFlag = true;

			if ((signal & kSignalIgnoreActor) == 0) {
				rect = listEntry->celRect;
				rect.top = CLIP<int16>(_ports->kernelPriorityToCoordinate(listEntry->priority) - 1, rect.top, rect.bottom - 1);
				_paint16->fillRect(rect, SCI_SCREEN_MASK_CONTROL, 0, 0, 15);
			}
		}
		listIterator++;
	}
}

void GfxAnimate::drawCels() {
	reg_t curObject;
	AnimateEntry *listEntry;
	AnimateEntry *lastCastEntry = _lastCastData;
	uint16 signal;
	reg_t bitsHandle;
	AnimateList::iterator listIterator;
	AnimateList::iterator listEnd = _list.end();
	_lastCastCount = 0;

	listIterator = _list.begin();
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;
		signal = listEntry->signal;

		if (!(signal & (kSignalNoUpdate | kSignalHidden | kSignalAlwaysUpdate))) {
			// Save background
			bitsHandle = _paint16->bitsSave(listEntry->celRect, SCI_SCREEN_MASK_ALL);
			PUT_SEL32(_s->_segMan, curObject, SELECTOR(underBits), bitsHandle);

			// draw corresponding cel
			_paint16->drawCel(listEntry->viewId, listEntry->loopNo, listEntry->celNo, listEntry->celRect, listEntry->priority, listEntry->paletteNo, listEntry->scaleX, listEntry->scaleY);
			listEntry->showBitsFlag = true;

			if (signal & kSignalRemoveView) {
				signal &= 0xFFFF ^ kSignalRemoveView;
			}
			listEntry->signal = signal;

			// Remember that entry in lastCast
			memcpy(lastCastEntry, listEntry, sizeof(AnimateEntry));
			lastCastEntry++; _lastCastCount++;
		}
		listIterator++;
	}
}

void GfxAnimate::updateScreen(byte oldPicNotValid) {
	reg_t curObject;
	AnimateEntry *listEntry;
	uint16 signal;
	AnimateList::iterator listIterator;
	AnimateList::iterator listEnd = _list.end();
	Common::Rect lsRect;
	Common::Rect workerRect;

	listIterator = _list.begin();
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;
		signal = listEntry->signal;

		if (listEntry->showBitsFlag || !(signal & (kSignalRemoveView | kSignalNoUpdate) ||
										(!(signal & kSignalRemoveView) && (signal & kSignalNoUpdate) && oldPicNotValid))) {
			lsRect.left = GET_SEL32V(_s->_segMan, curObject, SELECTOR(lsLeft));
			lsRect.top = GET_SEL32V(_s->_segMan, curObject, SELECTOR(lsTop));
			lsRect.right = GET_SEL32V(_s->_segMan, curObject, SELECTOR(lsRight));
			lsRect.bottom = GET_SEL32V(_s->_segMan, curObject, SELECTOR(lsBottom));

			workerRect = lsRect;
			workerRect.clip(listEntry->celRect);

			if (!workerRect.isEmpty()) {
				workerRect = lsRect;
				workerRect.extend(listEntry->celRect);
			} else {
				_paint16->bitsShow(lsRect);
				workerRect = listEntry->celRect;
			}
			PUT_SEL32V(_s->_segMan, curObject, SELECTOR(lsLeft), workerRect.left);
			PUT_SEL32V(_s->_segMan, curObject, SELECTOR(lsTop), workerRect.top);
			PUT_SEL32V(_s->_segMan, curObject, SELECTOR(lsRight), workerRect.right);
			PUT_SEL32V(_s->_segMan, curObject, SELECTOR(lsBottom), workerRect.bottom);
			_paint16->bitsShow(workerRect);

			if (signal & kSignalHidden) {
				listEntry->signal |= kSignalRemoveView;
			}
		}

		listIterator++;
	}
	// use this for debug purposes
	// _screen->copyToScreen();
}

void GfxAnimate::restoreAndDelete(int argc, reg_t *argv) {
	reg_t curObject;
	AnimateEntry *listEntry;
	uint16 signal;
	AnimateList::iterator listIterator;
	AnimateList::iterator listEnd = _list.end();


	// This has to be done in a separate loop. At least in sq1 some .dispose modifies FIXEDLOOP flag in signal for
	//  another object. In that case we would overwrite the new signal with our version of the old signal
	listIterator = _list.begin();
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;
		signal = listEntry->signal;

		// Finally update signal
		PUT_SEL32V(_s->_segMan, curObject, SELECTOR(signal), signal);
		listIterator++;
	}

	listIterator = _list.reverse_begin();
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;
		// We read out signal here again, this is not by accident but to ensure that we got an up-to-date signal
		signal = GET_SEL32V(_s->_segMan, curObject, SELECTOR(signal));

		if ((signal & (kSignalNoUpdate | kSignalRemoveView)) == 0) {
			_paint16->bitsRestore(GET_SEL32(_s->_segMan, curObject, SELECTOR(underBits)));
			PUT_SEL32V(_s->_segMan, curObject, SELECTOR(underBits), 0);
		}

		if (signal & kSignalDisposeMe) {
			// Call .delete_ method of that object
			invoke_selector(_s, curObject, g_sci->getKernel()->_selectorCache.delete_, kContinueOnInvalidSelector, argc, argv, 0);
		}
		listIterator--;
	}
}

void GfxAnimate::reAnimate(Common::Rect rect) {
	AnimateEntry *lastCastEntry;
	uint16 lastCastCount;

	if (_lastCastCount > 0) {
		lastCastEntry = _lastCastData;
		lastCastCount = _lastCastCount;
		while (lastCastCount > 0) {
			lastCastEntry->castHandle = _paint16->bitsSave(lastCastEntry->celRect, SCI_SCREEN_MASK_VISUAL|SCI_SCREEN_MASK_PRIORITY);
			_paint16->drawCel(lastCastEntry->viewId, lastCastEntry->loopNo, lastCastEntry->celNo, lastCastEntry->celRect, lastCastEntry->priority, lastCastEntry->paletteNo, lastCastEntry->scaleX, lastCastEntry->scaleY);
			lastCastEntry++; lastCastCount--;
		}
		_paint16->bitsShow(rect);
		// restoring
		lastCastCount = _lastCastCount;
		while (lastCastCount > 0) {
			lastCastEntry--;
			_paint16->bitsRestore(lastCastEntry->castHandle);
			lastCastCount--;
		}
	} else {
		_paint16->bitsShow(rect);
	}

	/*
	if (!_lastCast->isEmpty()) {
		HEAPHANDLE hnode = _lastCast->getFirst();
		sciCast *pCast;
		CResView *res;
		while (hnode) {
			pCast = (sciCast *)heap2Ptr(hnode);
			res = (CResView *)ResMgr.ResLoad(SCI_RES_VIEW, pCast->view);
			pCast->hSaved = _gfx->SaveBits(pCast->rect, 3);
			res->drawCel(pCast->loop, pCast->cel, &pCast->rect, pCast->z, pCast->pal);
			hnode = pCast->node.next;
		}
		_gfx->BitsShow(rect);
		// restoring
		hnode = _lastCast->getLast();
		while (hnode) {
			pCast = (sciCast *)heap2Ptr(hnode);
			_gfx->BitsShow(pCast->hSaved);
			hnode = pCast->node.prev;
		}
	*/
}

void GfxAnimate::addToPicDrawCels() {
	reg_t curObject;
	AnimateEntry *listEntry;
	GfxView *view = NULL;
	AnimateList::iterator listIterator;
	AnimateList::iterator listEnd = _list.end();

	listIterator = _list.begin();
	while (listIterator != listEnd) {
		listEntry = *listIterator;
		curObject = listEntry->object;

		if (listEntry->priority == -1)
			listEntry->priority = _ports->kernelCoordinateToPriority(listEntry->y);

		// Get the corresponding view
		view = _cache->getView(listEntry->viewId);

		// Create rect according to coordinates and given cel
		view->getCelRect(listEntry->loopNo, listEntry->celNo, listEntry->x, listEntry->y, listEntry->z, &listEntry->celRect);

		// draw corresponding cel
		_paint16->drawCel(listEntry->viewId, listEntry->loopNo, listEntry->celNo, listEntry->celRect, listEntry->priority, listEntry->paletteNo);
		if ((listEntry->signal & kSignalIgnoreActor) == 0) {
			listEntry->celRect.top = CLIP<int16>(_ports->kernelPriorityToCoordinate(listEntry->priority) - 1, listEntry->celRect.top, listEntry->celRect.bottom - 1);
			_paint16->fillRect(listEntry->celRect, SCI_SCREEN_MASK_CONTROL, 0, 0, 15);
		}

		listIterator++;
	}
}

void GfxAnimate::addToPicDrawView(GuiResourceId viewId, int16 loopNo, int16 celNo, int16 leftPos, int16 topPos, int16 priority, int16 control) {
	GfxView *view = _cache->getView(viewId);
	Common::Rect celRect;

	// Create rect according to coordinates and given cel
	view->getCelRect(loopNo, celNo, leftPos, topPos, priority, &celRect);
	_paint16->drawCel(view, loopNo, celNo, celRect, priority, 0);
}


void GfxAnimate::animateShowPic() {
	Port *picPort = _ports->_picWind;
	Common::Rect picRect = picPort->rect;
	bool previousCursorState = _cursor->isVisible();

	if (previousCursorState)
		_cursor->kernelHide();
	// Adjust picRect to become relative to screen
	picRect.translate(picPort->left, picPort->top);
	_transitions->doit(picRect);
	if (previousCursorState)
		_cursor->kernelShow();

	// We set SCI1.1 priority band information here
	_ports->priorityBandsRecall();
}

void GfxAnimate::kernelAnimate(reg_t listReference, bool cycle, int argc, reg_t *argv) {
	byte old_picNotValid = _screen->_picNotValid;

	if (listReference.isNull()) {
		disposeLastCast();
		if (_screen->_picNotValid)
			animateShowPic();
		return;
	}

	List *list = _s->_segMan->lookupList(listReference);
	if (!list)
		error("kAnimate called with non-list as parameter");

	if (cycle) {
		if (!invoke(list, argc, argv))
			return;
	}

	Port *oldPort = _ports->setPort((Port *)_ports->_picWind);
	disposeLastCast();

	makeSortedList(list);
	fill(old_picNotValid);

	if (old_picNotValid) {
		_ports->beginUpdate(_ports->_picWind);
		update();
		_ports->endUpdate(_ports->_picWind);
	}

	drawCels();

	if (_screen->_picNotValid)
		animateShowPic();

	updateScreen(old_picNotValid);
	restoreAndDelete(argc, argv);

	if (getLastCastCount() > 1)
		_s->_throttleTrigger = true;

	_ports->setPort(oldPort);
}

void GfxAnimate::addToPicSetPicNotValid() {
	if (getSciVersion() <= SCI_VERSION_1_EARLY)
		_screen->_picNotValid = 1;
	else
		_screen->_picNotValid = 2;
}

void GfxAnimate::kernelAddToPicList(reg_t listReference, int argc, reg_t *argv) {
	List *list;

	_ports->setPort((Port *)_ports->_picWind);

	list = _s->_segMan->lookupList(listReference);
	if (!list)
		error("kAddToPic called with non-list as parameter");

	makeSortedList(list);
	addToPicDrawCels();

	addToPicSetPicNotValid();
}

void GfxAnimate::kernelAddToPicView(GuiResourceId viewId, int16 loopNo, int16 celNo, int16 leftPos, int16 topPos, int16 priority, int16 control) {
	_ports->setPort((Port *)_ports->_picWind);
	addToPicDrawView(viewId, loopNo, celNo, leftPos, topPos, priority, control);
	addToPicSetPicNotValid();
}

} // End of namespace Sci
