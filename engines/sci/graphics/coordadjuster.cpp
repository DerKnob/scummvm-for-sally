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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/graphics/coordadjuster.cpp $
 * $Id: coordadjuster.cpp 47942 2010-02-06 21:26:30Z eriktorbjorn $
 *
 */

#include "common/util.h"

#include "sci/sci.h"
#include "sci/engine/state.h"
#include "sci/engine/selector.h"
#include "sci/graphics/coordadjuster.h"
#include "sci/graphics/ports.h"

namespace Sci {

GfxCoordAdjuster::GfxCoordAdjuster() {
}

GfxCoordAdjuster16::GfxCoordAdjuster16(GfxPorts *ports)
	: _ports(ports) {
}

GfxCoordAdjuster16::~GfxCoordAdjuster16() {
}

void GfxCoordAdjuster16::kernelGlobalToLocal(int16 &x, int16 &y, reg_t planeObject) {
	Port *curPort = _ports->getPort();
	x -= curPort->left;
	y -= curPort->top;
}

void GfxCoordAdjuster16::kernelLocalToGlobal(int16 &x, int16 &y, reg_t planeObject) {
	Port *curPort = _ports->getPort();
	x += curPort->left;
	y += curPort->top;
}

Common::Rect GfxCoordAdjuster16::onControl(Common::Rect rect) {
	Port *oldPort = _ports->setPort((Port *)_ports->_picWind);
	Common::Rect adjustedRect(rect.left, rect.top, rect.right, rect.bottom);

	adjustedRect.clip(_ports->getPort()->rect);
	_ports->offsetRect(adjustedRect);
	_ports->setPort(oldPort);
	return adjustedRect;
}

void GfxCoordAdjuster16::setCursorPos(Common::Point &pos) {
	pos.y += _ports->getPort()->top;
	pos.x += _ports->getPort()->left;
}

void GfxCoordAdjuster16::moveCursor(Common::Point &pos) {
	pos.y += _ports->_picWind->rect.top;
	pos.x += _ports->_picWind->rect.left;

	pos.y = CLIP<int16>(pos.y, _ports->_picWind->rect.top, _ports->_picWind->rect.bottom - 1);
	pos.x = CLIP<int16>(pos.x, _ports->_picWind->rect.left, _ports->_picWind->rect.right - 1);
}

Common::Rect GfxCoordAdjuster16::pictureGetDisplayArea() {
	Common::Rect displayArea(_ports->getPort()->rect.right, _ports->getPort()->rect.bottom);
	displayArea.moveTo(_ports->getPort()->left, _ports->getPort()->top);
	return displayArea;
}

#ifdef ENABLE_SCI32
GfxCoordAdjuster32::GfxCoordAdjuster32(SegManager *segMan)
	: _segMan(segMan) {
}

GfxCoordAdjuster32::~GfxCoordAdjuster32() {
}

void GfxCoordAdjuster32::kernelGlobalToLocal(int16 &x, int16 &y, reg_t planeObject) {
	//int16 resY = GET_SEL32V(_s->_segMan, planeObj, SELECTOR(resY));
	//int16 resX = GET_SEL32V(_s->_segMan, planeObj, SELECTOR(resX));
	//*x = ( *x * _screen->getWidth()) / resX;
	//*y = ( *y * _screen->getHeight()) / resY;
	x -= GET_SEL32V(_segMan, planeObject, SELECTOR(left));
	y -= GET_SEL32V(_segMan, planeObject, SELECTOR(top));
}
void GfxCoordAdjuster32::kernelLocalToGlobal(int16 &x, int16 &y, reg_t planeObject) {
	//int16 resY = GET_SEL32V(_s->_segMan, planeObj, SELECTOR(resY));
	//int16 resX = GET_SEL32V(_s->_segMan, planeObj, SELECTOR(resX));
	x += GET_SEL32V(_segMan, planeObject, SELECTOR(left));
	y += GET_SEL32V(_segMan, planeObject, SELECTOR(top));
	//*x = ( *x * resX) / _screen->getWidth();
	//*y = ( *y * resY) / _screen->getHeight();
}

Common::Rect GfxCoordAdjuster32::onControl(Common::Rect rect) {
	Common::Rect adjustedRect = rect;
	adjustedRect.translate(0, 10);
	return adjustedRect;
}

void GfxCoordAdjuster32::pictureSetDisplayArea(Common::Rect displayArea) {
	_pictureDisplayArea = displayArea;
}

Common::Rect GfxCoordAdjuster32::pictureGetDisplayArea() {
	return _pictureDisplayArea;
}
#endif

} // End of namespace Sci
