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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/backends/vkeybd/image-map.h $
 * $Id: image-map.h 40867 2009-05-24 15:17:42Z lordhoto $
 *
 */

#ifndef COMMON_IMAGEMAP_H
#define COMMON_IMAGEMAP_H

#ifdef ENABLE_VKEYBD

#include "common/scummsys.h"
#include "common/hashmap.h"
#include "common/hash-str.h"

namespace Common {

struct Polygon;

class ImageMap {

public:

	~ImageMap();

	Polygon *createArea(const String& id);
	void removeArea(const String& id);
	void removeAllAreas();
	String findMapArea(int16 x, int16 y);

protected:
	HashMap<String, Polygon *> _areas;
};


} // End of namespace Common

#endif // #ifdef ENABLE_VKEYBD

#endif // #ifndef COMMON_IMAGEMAP_H
