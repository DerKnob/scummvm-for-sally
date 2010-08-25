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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/mohawk/riven_cursors.h $
 * $Id: riven_cursors.h 47541 2010-01-25 01:39:44Z lordhoto $
 *
 */

namespace Mohawk {

//////////////////////////////////////////////
// Cursors and Cursor Palettes
//////////////////////////////////////////////

////////////////////////////////////////
// Zip Mode Cursor (16x16):
//    Shown when a zip mode spot is active
//
//    0 = Transparent
//    1 = Black		(0x000000)
//    2 = Yellow	(0xDCFF00)
////////////////////////////////////////
static const byte zipModeCursor[] = {
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 1, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 1, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 1, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 1, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 1, 2, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 1, 2, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};


////////////////////////////////////////
// Zip Mode Cursor Palette:
//    Palette For The Zip Mode Cursor
////////////////////////////////////////
static const byte zipModeCursorPalette[] = {
	0x00, 0x00, 0x00,	0x00,		// Black
	0xDC, 0xFF, 0x00,	0x00,		// Yellow
};


////////////////////////////////////////
// Hand Over Object Cursor (16x16):
//    Shown when over a hotspot that's interactive
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte objectHandCursor[] = {
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 0, 1, 2, 3, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 1, 2, 3, 1, 1, 2, 3, 1, 2, 3, 1, 0, 0, 0,
	0, 0, 1, 2, 3, 1, 1, 4, 3, 1, 2, 3, 1, 0, 1, 0,
	0, 0, 0, 1, 2, 3, 1, 2, 3, 1, 4, 3, 1, 1, 2, 1,
	0, 0, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1,
	0, 1, 1, 0, 1, 2, 2, 2, 2, 2, 2, 3, 1, 2, 3, 1,
	1, 2, 2, 1, 1, 2, 2, 2, 4, 2, 4, 2, 2, 4, 2, 1,
	1, 3, 4, 2, 1, 2, 4, 2, 2, 2, 2, 2, 4, 4, 1, 0,
	0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 3, 1, 0,
	0, 0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 4, 4, 3, 1, 0,
	0, 0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 4, 3, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 1, 0, 0,
	0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 4, 3, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 4, 3, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 4, 3, 1, 0, 0, 0
};


////////////////////////////////////////
// Grabbing Hand Cursor (13x13):
//    Shown when interacting with an object
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte grabbingHandCursor[] = {
	0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 1, 1, 2, 3, 1, 1, 1, 0, 0, 0,
	0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 1, 0,
	0, 1, 2, 2, 2, 2, 2, 2, 2, 3, 1, 2, 1,
	0, 1, 1, 2, 2, 2, 4, 2, 4, 2, 2, 4, 1,
	1, 2, 1, 2, 4, 2, 2, 2, 2, 2, 4, 4, 1,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 3, 1,
	1, 3, 2, 2, 2, 2, 2, 2, 2, 4, 4, 3, 1,
	1, 3, 2, 2, 2, 2, 2, 2, 2, 4, 3, 1, 0,
	0, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 1, 0,
	0, 0, 1, 2, 2, 2, 2, 2, 4, 3, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 4, 3, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 4, 3, 1, 0, 0
};


////////////////////////////////////////
// Standard Hand Cursor (15x16):
//    Standard Cursor
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte standardHandCursor[] = {
	0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 4, 4, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 3, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 4, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 4, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 4, 1, 0, 0, 0, 0, 0, 0,
	0, 1, 1, 0, 0, 1, 2, 4, 1, 1, 1, 1, 1, 0, 0,
	1, 4, 2, 1, 0, 1, 2, 4, 1, 4, 1, 4, 1, 1, 1,
	0, 1, 3, 2, 1, 1, 2, 4, 1, 4, 1, 4, 1, 4, 1,
	0, 0, 1, 4, 2, 1, 2, 2, 4, 2, 4, 2, 1, 4, 1,
	0, 0, 1, 4, 2, 1, 2, 2, 2, 2, 2, 2, 2, 3, 1,
	0, 0, 0, 1, 4, 2, 2, 2, 2, 2, 2, 2, 4, 3, 1,
	0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 4, 3, 1,
	0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 3, 1, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 4, 3, 1, 0,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 4, 3, 1, 0
};


////////////////////////////////////////
// Pointing Left Cursor (15x13):
//    Cursor When Over A Hotspot That Allows You To Move Left
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte pointingLeftCursor[] = {
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 3, 2, 2, 2, 1, 1, 0, 0,
	0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 3, 1, 1,
	1, 4, 2, 2, 2, 2, 1, 2, 3, 2, 2, 2, 4, 4, 4,
	1, 4, 4, 4, 4, 4, 1, 2, 1, 3, 4, 2, 2, 2, 2,
	0, 1, 1, 1, 1, 1, 1, 2, 1, 3, 3, 4, 2, 2, 2,
	0, 0, 0, 0, 0, 1, 1, 1, 1, 3, 3, 3, 4, 4, 2,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 4, 1, 3, 4, 2, 2,
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 3, 4, 2, 2,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 4, 2, 4,
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0
};


////////////////////////////////////////
// Pointing Right Cursor (15x13):
//    Cursor When Over A Hotspot That Allows You To Move Right
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte pointingRightCursor[] = {
	0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 1, 2, 2, 2, 3, 1, 0, 0, 0, 0, 0, 0,
	1, 1, 3, 4, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0,
	4, 4, 4, 2, 2, 2, 3, 2, 1, 4, 4, 4, 4, 4, 1,
	2, 2, 2, 2, 4, 3, 1, 2, 1, 2, 2, 2, 2, 4, 1,
	2, 2, 2, 4, 3, 3, 1, 2, 1, 1, 1, 1, 1, 1, 0,
	2, 4, 4, 3, 3, 3, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	2, 2, 4, 3, 1, 4, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	2, 2, 4, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	4, 2, 4, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	1, 4, 4, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	0, 1, 1, 1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0
};


////////////////////////////////////////
// Pointing Down Cursor (Palm Up)(13x16):
//    Cursor When Over A Hotspot That Allows You To Move Down
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte pointingDownCursorPalmUp[] = {
	0, 0, 1, 4, 2, 2, 2, 2, 2, 4, 1, 0, 0,
	0, 0, 1, 4, 2, 2, 4, 2, 2, 2, 4, 1, 0,
	0, 1, 3, 4, 2, 2, 4, 4, 4, 4, 4, 1, 0,
	0, 1, 4, 2, 2, 4, 3, 3, 3, 1, 1, 1, 1,
	1, 2, 2, 2, 4, 3, 3, 1, 1, 2, 1, 2, 1,
	1, 2, 2, 2, 3, 3, 3, 4, 1, 2, 1, 2, 1,
	1, 2, 2, 3, 1, 1, 1, 2, 1, 2, 1, 2, 1,
	1, 3, 2, 2, 2, 2, 1, 2, 1, 2, 1, 2, 1,
	0, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1,
	0, 0, 1, 2, 4, 1, 1, 1, 1, 1, 1, 0, 0,
	0, 0, 1, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 2, 4, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 4, 4, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
};


////////////////////////////////////////
// Pointing Up Cursor (Palm Up)(13x16):
//    Cursor When Over A Hotspot That Allows You To Move Up
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte pointingUpCursorPalmUp[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 4, 4, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 4, 1, 0, 0,
	0, 0, 1, 1, 1, 1, 1, 1, 2, 4, 1, 0, 0,
	1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 0,
	1, 2, 1, 2, 1, 2, 1, 2, 2, 2, 2, 3, 1,
	1, 2, 1, 2, 1, 2, 1, 1, 1, 3, 2, 2, 1,
	1, 2, 1, 2, 1, 4, 3, 3, 3, 2, 2, 2, 1,
	1, 2, 1, 2, 1, 1, 3, 3, 4, 2, 2, 2, 1,
	1, 1, 1, 1, 3, 3, 3, 4, 2, 2, 4, 1, 0,
	0, 1, 4, 4, 4, 4, 4, 2, 2, 4, 3, 1, 0,
	0, 1, 4, 2, 2, 2, 4, 2, 2, 4, 1, 0, 0,
	0, 0, 1, 4, 2, 2, 2, 2, 2, 4, 1, 0, 0
};


////////////////////////////////////////
// Pointing Left Cursor (Bent)(15x13):
//    Cursor When Over A Hotspot That Allows You To Turn Left 180 Degrees
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte pointingLeftCursorBent[] = {
	0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 3, 2, 2, 2, 1, 1, 0, 0,
	0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 3, 1, 1,
	1, 3, 2, 4, 4, 2, 1, 2, 3, 3, 2, 2, 4, 4, 4,
	1, 2, 4, 3, 3, 4, 1, 2, 1, 3, 4, 2, 2, 2, 2,
	1, 4, 4, 1, 1, 1, 1, 2, 1, 1, 3, 4, 2, 2, 2,
	1, 1, 1, 0, 0, 1, 1, 1, 1, 3, 3, 3, 4, 4, 2,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 4, 1, 3, 4, 3, 2,
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 3, 4, 2, 2,
	0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 4, 2, 4,
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 4, 4, 1,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0
};


////////////////////////////////////////
// Pointing Right Cursor (Bent)(15x13):
//    Cursor When Over A Hotspot That Allows You To Turn Right 180 Degrees
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte pointingRightCursorBent[] = {
	0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 1, 2, 2, 2, 3, 1, 0, 0, 0, 0, 0, 0,
	1, 1, 3, 4, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0,
	4, 4, 4, 2, 2, 3, 3, 2, 1, 2, 4, 4, 2, 3, 1,
	2, 2, 2, 2, 4, 3, 1, 2, 1, 4, 3, 3, 4, 2, 1,
	2, 2, 2, 4, 3, 1, 1, 2, 1, 1, 1, 1, 4, 4, 1,
	2, 4, 4, 3, 3, 3, 1, 1, 1, 1, 0, 0, 1, 1, 1,
	2, 3, 4, 3, 1, 4, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	2, 2, 4, 3, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	4, 2, 4, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	1, 4, 4, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	0, 1, 1, 1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0
};


////////////////////////////////////////
// Pointing Down Cursor (Palm Down)(15x16):
//    Similar to Standard Cursor
//
//    0 = Transparent
//    1 = Black 		(0x000000)
//    2 = Light Peach 	(0xEDCD96)
//    3 = Brown			(0x8A672F)
//    4 = Dark Peach	(0xE89A62)
////////////////////////////////////////
static const byte pointingDownCursorPalmDown[] = {
	0, 1, 3, 4, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	0, 1, 3, 4, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	0, 1, 3, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0,
	1, 3, 4, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0,
	1, 3, 4, 2, 2, 2, 2, 2, 2, 2, 4, 1, 0, 0, 0,
	1, 3, 2, 3, 2, 2, 2, 2, 2, 1, 2, 4, 1, 0, 0,
	1, 4, 1, 2, 2, 3, 2, 3, 2, 1, 2, 4, 1, 0, 0,
	1, 4, 1, 4, 1, 4, 1, 4, 4, 1, 1, 2, 3, 1, 0,
	0, 1, 1, 4, 1, 4, 1, 4, 2, 1, 0, 1, 2, 4, 1,
	0, 0, 1, 1, 1, 1, 1, 4, 2, 1, 0, 0, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 1, 4, 4, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 4, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 4, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 3, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 4, 4, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0
};

////////////////////////////////////////
// Hand Cursor Palette:
//    Palette For All Hand Cursors
////////////////////////////////////////
static const byte handCursorPalette[] = {
	0x00, 0x00, 0x00,	0x00,		// Black
	0xED, 0xCD, 0x96,	0x00,		// Light Peach
	0x8A, 0x67, 0x2F,	0x00,		// Brown
	0xE8, 0x9A, 0x62,	0x00		// Dark Peach
};


////////////////////////////////////////
// Pellet Cursor (8x8):
//    Cursor When Using The Pellet In The Frog Trap
//
//    0 = Transparent
//    1 = Light Olive Green 	(0x5D6730)
//    2 = Maroon				(0x5E3333)
//    3 = Light Gray			(0x555555)
//    4 = Medium Gray			(0x444444)
//    5 = Dark Gray				(0x333333)
//    6 = Dark Green			(0x2D3300)
//    7 = Darkest Gray			(0x222222)
////////////////////////////////////////
static const byte pelletCursor[] = {
	0, 0, 1, 1, 2, 3, 0, 0,
	0, 2, 1, 4, 1, 2, 5, 0,
	4, 1, 4, 1, 2, 1, 5, 4,
	4, 2, 1, 2, 1, 1, 2, 6,
	6, 4, 2, 1, 4, 4, 1, 5,
	5, 6, 5, 2, 1, 2, 4, 4,
	0, 7, 5, 5, 4, 2, 5, 0,
	0, 0, 5, 6, 6, 5, 0, 0
};

////////////////////////////////////////
// Pellet Cursor Palette:
//    Palette For The Pellet Cursor
////////////////////////////////////////
static const byte pelletCursorPalette[] = {
	0x5D, 0x67, 0x30,	0x00,
	0x5E, 0x33, 0x33,	0x00,
	0x55, 0x55, 0x55,	0x00,
	0x44, 0x44, 0x44,	0x00,
	0x33, 0x33, 0x33,	0x00,
	0x2D, 0x33, 0x00,	0x00,
	0x22, 0x22, 0x22,	0x00
};

} // End of namespace Mohawk
