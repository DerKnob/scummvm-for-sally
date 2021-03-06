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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/tools/create_kyradat/extract.h $
 * $Id: extract.h 47476 2010-01-23 15:00:11Z strangerke $
 *
 */

#ifndef CREATE_KYRADAT_EXTRACT_H
#define CREATE_KYRADAT_EXTRACT_H

#include "create_kyradat.h"
#include "pak.h"
#include "util.h"

enum kExtractType {
	kTypeStringList = 0,
	kTypeRoomList,
	kTypeShapeList,
	kTypeRawData,
	kTypeAmigaSfxTable,
	kTypeTownsWDSfxTable,

	k2TypeSeqData,
	k2TypeShpDataV1,
	k2TypeShpDataV2,
	k2TypeSoundList,
	k2TypeLangSoundList,
	k2TypeSize10StringList,
	k2TypeSfxList,

	k3TypeRaw16to8,
	k3TypeShpData,

	kLolTypeRaw16,
	kLolTypeRaw32,
	kLolTypeButtonDef,
	kLolTypeCharData,
	kLolTypeSpellData,
	kLolTypeCompassData,
	kLolTypeFlightShpData
};

struct ExtractInformation {
	int game;
	int platform;
	int lang;
	int special;
};

struct ExtractType {
	int type;
	bool (*extract)(PAKFile &out, const ExtractInformation *info, const byte *data, const uint32 size, const char *filename, int id);
};

const ExtractType *findExtractType(const int type);
byte getTypeID(int type);

#endif

