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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/teenagent/objects.h $
 * $Id: objects.h 48372 2010-03-23 21:23:56Z megath $
 */


#ifndef TEENAGENT_OBJECTS_H
#define TEENAGENT_OBJECTS_H

#include "common/noncopyable.h"
#include "common/rect.h"
#include "graphics/surface.h"

namespace TeenAgent {

enum {kActorUp = 1, kActorRight = 2, kActorDown = 3, kActorLeft = 4 };

struct Rect {
	int16 left, top, right, bottom;

	inline Rect() : left(0), top(0), right(0), bottom(0), _base(NULL) {}
	inline Rect(const Common::Rect &r) : left(r.left), top(r.top), right(r.right), bottom(r.bottom), _base(NULL) {}
	inline Rect(uint16 l, uint16 t, uint16 r, uint16 b) : left(l), top(t), right(r), bottom(b), _base(NULL) {}

	inline bool in(const Common::Point &point) const {
		return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
	}

	inline Common::Point center() const {
		return Common::Point((right + left) / 2, (bottom + top) / 2);
	}

	inline bool valid() const {
		return left >= 0 && left < 320 && right >= 0 && right < 320 && top >= 0 && top < 200 && bottom >= 0 && bottom < 200;
	}

	void render(Graphics::Surface *surface, uint8 color) const;

	void dump(int level = 0) const {
		debug(level, "rect[%u, %u, %u, %u]", left, top, right, bottom);
	}

	inline void clear() {
		left = top = right = bottom = 0;
	}

	void load(byte *src); //8 bytes
	void save() const;

	inline bool intersects_hline(int x1, int x2, int y) const {
		if (x1 > x2)
			SWAP(x1, x2);
		return y >= top && y <= bottom && x1 <= right && x2 >= left;
	}

	inline bool intersects_vline(int x, int y1, int y2) const {
		if (y1 > y2)
			SWAP(y1, y2);
		return x >= left && x <= right && y1 <= bottom && y2 >= top;
	}

	inline bool contains(const Rect & rect) const {
		return rect.left >= left && rect.right <= right && rect.top >= top && rect.bottom <= bottom;
	}

	static inline bool inside(int x, int a, int b) {
		if (a > b)
			SWAP(a, b);
		return x >= a && x <= b;
	}

	int intersects_line(const Common::Point &a, const Common::Point &b) const {
		int dy = b.y - a.y, dx = b.x - a.x;

		int mask = 0; //orientation bitmask: 1 - top, 2 - right, 3 - bottom, 4 - left

		if (dx != 0) {
			int yl = (left - a.x) * dy / dx + a.y;
			if (yl > top && yl < bottom && inside(yl, a.y, b.y) && inside(left, a.x, b.x)) {
				//c[idx++] = Common::Point(left, yl);
				mask |= 8;
			}
			int yr = (right - a.x) * dy / dx + a.y;
			if (yr > top && yr < bottom && inside(yr, a.y, b.y) && inside(right, a.x, b.x)) {
				//c[idx++] = Common::Point(right, yr);
				mask |= 2;
			}
		}

		if (dy != 0) {
			int xt = (top - a.y) * dx / dy + a.x;
			if (xt > left && xt < right && inside(xt, a.x, b.x) && inside(top, a.y, b.y)) {
				//assert(idx < 2);
				//c[idx++] = Common::Point(xt, top);
				mask |= 1;
			}
			int xb = (bottom - a.y) * dx / dy + a.x;
			if (xb > left && xb < right && inside(xb, a.x, b.x) && inside(bottom, a.y, b.y)) {
				//assert(idx < 2);
				//c[idx++] = Common::Point(xb, bottom);
				mask |= 4;
			}
		}
		return mask;
	}

	void side(Common::Point &p1, Common::Point &p2, int o, const Common::Point &nearest) const {
		switch(o) {
		case kActorLeft:
			p1 = Common::Point(left, top);
			p2 = Common::Point(left, bottom);
			break;

		case kActorRight:
			p1 = Common::Point(right, top);
			p2 = Common::Point(right, bottom);
			break;

		case kActorUp:
			p1 = Common::Point(left, top);
			p2 = Common::Point(right, top);
			break;

		case kActorDown:
			p1 = Common::Point(left, bottom);
			p2 = Common::Point(right, bottom);
			break;

		default:
			p1 = Common::Point();
			p2 = Common::Point();
		}
		if (p1.sqrDist(nearest) >= p2.sqrDist(nearest))
			SWAP(p1, p2);
	}

protected:
	byte * _base;
};

struct Object {

	byte id; //0
	Rect rect; //1
	Rect actor_rect; //9
	byte actor_orientation; //17
	byte enabled; //18
	//19
	Common::String name, description;

	Object(): _base(NULL) {}
	void dump(int level = 0) const;
	void setName(const Common::String &name);
	void load(byte *addr);
	void save() const;

	static Common::String parse_description(const char *name);

protected:
	byte * _base;
};

struct InventoryObject {
	byte id;
	byte animated;
	Common::String name, description;

	InventoryObject(): id(0), animated(0), _base(0) {}
	void load(byte *addr);

protected:
	byte * _base;
};

struct UseHotspot {
	byte inventory_id;
	byte object_id;
	byte orientation;
	uint16 actor_x, actor_y;
	uint16 callback;
	void load(byte *src);
	void dump(int level = 0) const;
};

struct Walkbox {
	byte type;
	byte orientation;
	Rect rect;
	byte side_hint[4];

	Walkbox() : _base(NULL) {}
	void dump(int level = 0) const;
	void load(byte *src);
	void save() const;

protected:
	byte * _base;
};

struct FadeType {
	Rect rect;
	byte value;

	void load(byte *src);
};

template<typename T>
class ScopedPtr : Common::NonCopyable {
public:
	typedef T ValueType;
	typedef T *PointerType;
	typedef T &ReferenceType;

	explicit ScopedPtr(PointerType o = 0) : _pointer(o) {}

	ReferenceType operator*() const { return *_pointer; }
	PointerType operator->() const { return _pointer; }
	operator PointerType() const { return _pointer; }

	/**
	 * Implicit conversion operator to bool for convenience, to make
	 * checks like "if (scopedPtr) ..." possible.
	 */
	operator bool() const { return _pointer != 0; }

	~ScopedPtr() { 
		delete _pointer;
	}

	/**
	 * Resets the pointer with the new value. Old object will be destroyed
	 */
	void reset(PointerType o = 0) {
		delete _pointer;
		_pointer = o;
	}

	/**
	 * Returns the plain pointer value. 
	 *
	 * @return the pointer the ScopedPtr manages
	 */
	PointerType get() const { return _pointer; }

	/**
	 * Returns the plain pointer value and releases ScopedPtr. 
	 * After release() call you need to delete object yourself
	 *
	 * @return the pointer the ScopedPtr manages
	 */
	PointerType release() {
		PointerType r = _pointer;
		_pointer = 0;
		return r;
	}

private:
	PointerType _pointer;
};

//\todo move it to util.h?
template<typename T> inline T SIGN (T x) { return (x > 0)? 1: ((x < 0)? -1: 0); }

} // End of namespace TeenAgent

#endif
