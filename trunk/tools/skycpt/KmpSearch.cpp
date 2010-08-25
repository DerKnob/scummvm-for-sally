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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/tools/skycpt/KmpSearch.cpp $
 * $Id: KmpSearch.cpp 47476 2010-01-23 15:00:11Z strangerke $
 *
 */

#include "stdafx.h"
#include "KmpSearch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void KmpSearch::init(const char *subStr) {
	strcpy(_subStr, subStr);
}

const char *KmpSearch::search(const char *str) {
	return strstr(str, _subStr);
}