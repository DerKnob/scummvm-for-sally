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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/drascula/resource.cpp $
 * $Id: resource.cpp 48834 2010-04-28 20:55:52Z eriktorbjorn $
 *
 */

#include "drascula/drascula.h"
#include "common/unarj.h"

namespace Drascula {

ArchiveMan::ArchiveMan() {
	_fallBack = false;
}

ArchiveMan::~ArchiveMan() {
}

void ArchiveMan::registerArchive(const Common::String &filename, int priority) {
	add(filename, Common::makeArjArchive(filename), priority);
}

Common::SeekableReadStream *ArchiveMan::open(const Common::String &filename) {
	if (_fallBack && SearchMan.hasFile(filename)) {
		return SearchMan.createReadStreamForMember(filename);
	}

	return createReadStreamForMember(filename);
}




TextResourceParser::TextResourceParser(Common::SeekableReadStream *stream, DisposeAfterUse::Flag dispose) :
	_stream(stream), _dispose(dispose) {

	// NOTE: strangely enough, the code before this refactoring used the size of
	// the stream as a fixed maximum length for the parser. Using an updated 
	// (size-pos) would make more sense to me, but let's see what the experts say.
	_maxLen = _stream->size();
}

TextResourceParser::~TextResourceParser() {
	if (_dispose == DisposeAfterUse::YES) {
		delete _stream;
	}
}

void TextResourceParser::getLine(char *buf) {
	byte c;
	char *b;

	for (;;) {
		b = buf;
		while (true) {
			c = ~_stream->readByte();
			if (_stream->eos()) break;

			if (c == '\r')
				continue;
			if (c == '\n' || b - buf >= (_maxLen - 1))
				break;
			*b++ = c;
		}
		*b = '\0';
		if (_stream->eos() && b == buf)
			return;
		if (b != buf)
			break;
	}
}

void TextResourceParser::parseInt(int &result) {
	char buf[256];
	getLine(buf);
	sscanf(buf, "%d", &result);
}

void TextResourceParser::parseString(char* result) {
	char buf[256];
	getLine(buf);
	sscanf(buf, "%s", result);
}



} // End of namespace Drascula

