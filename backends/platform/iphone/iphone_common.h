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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/backends/platform/iphone/iphone_common.h $
 * $Id: iphone_common.h 48478 2010-04-02 23:23:03Z vinterstum $
 *
 */


enum InputEvent {
	kInputMouseDown,
	kInputMouseUp,
	kInputMouseDragged,
	kInputMouseSecondDragged,
	kInputMouseSecondDown,
	kInputMouseSecondUp,
	kInputOrientationChanged,
	kInputKeyPressed,
	kInputApplicationSuspended,
	kInputApplicationResumed,
	kInputSwipe
};

enum ScreenOrientation {
	kScreenOrientationPortrait,
	kScreenOrientationLandscape,
	kScreenOrientationFlippedLandscape
};

typedef enum
{
	kUIViewSwipeUp = 1,
	kUIViewSwipeDown = 2,
	kUIViewSwipeLeft = 4,
	kUIViewSwipeRight = 8
} UIViewSwipeDirection;

#ifdef IPHONE_OFFICIAL
void iphone_main(int argc, char **argv);
#endif

// We need this to be able to call functions from/in Objective-C.
#ifdef  __cplusplus
extern "C" {
#endif

// On the C++ side
#ifndef IPHONE_OFFICIAL
void iphone_main(int argc, char *argv[]);
#endif

// On the ObjC side
void iPhone_updateScreen();
void iPhone_updateScreenRect(unsigned short* screen, int x1, int y1, int x2, int y2);
void iPhone_initSurface(int width, int height);
bool iPhone_fetchEvent(int *outEvent, float *outX, float *outY);
const char* iPhone_getDocumentsDir();

#ifdef __cplusplus
}
#endif
