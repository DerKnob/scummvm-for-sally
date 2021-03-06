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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/engine/seg_manager.cpp $
 * $Id: seg_manager.cpp 47836 2010-02-03 01:34:15Z fingolfin $
 *
 */

#include "sci/sci.h"
#include "sci/engine/seg_manager.h"
#include "sci/engine/state.h"

namespace Sci {


enum {
	DEFAULT_SCRIPTS = 32,
	DEFAULT_OBJECTS = 8,			///< default number of objects per script
	DEFAULT_OBJECTS_INCREMENT = 4	///< Number of additional objects to instantiate if we're running out of them
};

SegManager::SegManager(ResourceManager *resMan) {
	_heap.push_back(0);

	Clones_seg_id = 0;
	Lists_seg_id = 0;
	Nodes_seg_id = 0;
	Hunks_seg_id = 0;

#ifdef ENABLE_SCI32
	Arrays_seg_id = 0;
	String_seg_id = 0;
#endif

	_exportsAreWide = false;
	_resMan = resMan;

	createClassTable();
}

// Destroy the object, free the memorys if allocated before
SegManager::~SegManager() {
	resetSegMan();
}

void SegManager::resetSegMan() {
	// Free memory
	for (uint i = 0; i < _heap.size(); i++) {
		if (_heap[i])
			deallocate(i, false);
	}

	_heap.clear();

	// And reinitialize
	_heap.push_back(0);

	Clones_seg_id = 0;
	Lists_seg_id = 0;
	Nodes_seg_id = 0;
	Hunks_seg_id = 0;

	// Reinitialize class table
	_classtable.clear();
	createClassTable();
}

SegmentId SegManager::findFreeSegment() const {
	// FIXME: This is a very crude approach: We find a free segment id by scanning
	// from the start. This can be slow if the number of segments becomes large.
	// Optimizations are possible and easy, but I refrain from doing any until this
	// refactoring is complete.
	// One very simple yet probably effective approach would be to add
	// "int firstFreeSegment", which is updated when allocating/freeing segments.
	// There are some scenarios where it performs badly, but we should first check
	// whether those occur at all. If so, there are easy refinements that deal
	// with this. Finally, we could switch to a more elaborate scheme,
	// such as keeping track of all free segments. But I doubt this is necessary.
	uint seg = 1;
	while (seg < _heap.size() && _heap[seg]) {
		++seg;
	}
	assert(seg < 65536);
	return seg;
}

SegmentObj *SegManager::allocSegment(SegmentObj *mem, SegmentId *segid) {
	// Find a free segment
	SegmentId id = findFreeSegment();
	if (segid)
		*segid = id;

	if (!mem)
		error("SegManager: invalid mobj");

	// ... and put it into the (formerly) free segment.
	if (id >= (int)_heap.size()) {
		assert(id == (int)_heap.size());
		_heap.push_back(0);
	}
	_heap[id] = mem;

	return mem;
}

Script *SegManager::allocateScript(int script_nr, SegmentId *segid) {
	// Check if the script already has an allocated segment. If it
	// does, return that segment.
	*segid = _scriptSegMap.getVal(script_nr, 0);
	if (*segid > 0) {
		return (Script *)_heap[*segid];
	}

	// allocate the SegmentObj
	SegmentObj *mem = allocSegment(new Script(), segid);

	// Add the script to the "script id -> segment id" hashmap
	_scriptSegMap[script_nr] = *segid;

	return (Script *)mem;
}

int SegManager::deallocate(SegmentId seg, bool recursive) {
	SegmentObj *mobj;
	VERIFY(check(seg), "invalid seg id");

	mobj = _heap[seg];

	if (mobj->getType() == SEG_TYPE_SCRIPT) {
		Script *scr = (Script *)mobj;
		_scriptSegMap.erase(scr->_nr);
		if (recursive && scr->_localsSegment)
			deallocate(scr->_localsSegment, recursive);
	}

	delete mobj;
	_heap[seg] = NULL;

	return 1;
}

bool SegManager::isHeapObject(reg_t pos) {
	Object *obj = getObject(pos);
	if (obj == NULL || (obj && obj->isFreed()))
		return false;
	Script *scr = getScriptIfLoaded(pos.segment);
	return !(scr && scr->_markedAsDeleted);
}

void SegManager::deallocateScript(int script_nr) {
	SegmentId seg = getScriptSegment(script_nr);
	deallocate(seg, true);
}

Script *SegManager::getScript(const SegmentId seg) {
	if (seg < 1 || (uint)seg >= _heap.size()) {
		error("SegManager::getScript(): seg id %x out of bounds", seg);
	}
	if (!_heap[seg]) {
		error("SegManager::getScript(): seg id %x is not in memory", seg);
	}
	if (_heap[seg]->getType() != SEG_TYPE_SCRIPT) {
		error("SegManager::getScript(): seg id %x refers to type %d != SEG_TYPE_SCRIPT", seg, _heap[seg]->getType());
	}
	return (Script *)_heap[seg];
}

Script *SegManager::getScriptIfLoaded(const SegmentId seg) {
	if (seg < 1 || (uint)seg >= _heap.size() || !_heap[seg] || _heap[seg]->getType() != SEG_TYPE_SCRIPT)
		return 0;
	return (Script *)_heap[seg];
}

SegmentId SegManager::findSegmentByType(int type) {
	for (uint i = 0; i < _heap.size(); i++)
		if (_heap[i] && _heap[i]->getType() == type)
			return i;
	return 0;
}

SegmentObj *SegManager::getSegmentObj(SegmentId seg) {
	if (seg < 1 || (uint)seg >= _heap.size() || !_heap[seg])
		return 0;
	return _heap[seg];
}

SegmentType SegManager::getSegmentType(SegmentId seg) {
	if (seg < 1 || (uint)seg >= _heap.size() || !_heap[seg])
		return SEG_TYPE_INVALID;
	return _heap[seg]->getType();
}

Object *SegManager::getObject(reg_t pos) {
	SegmentObj *mobj = getSegmentObj(pos.segment);
	Object *obj = NULL;

	if (mobj != NULL) {
		if (mobj->getType() == SEG_TYPE_CLONES) {
			CloneTable *ct = (CloneTable *)mobj;
			if (ct->isValidEntry(pos.offset))
				obj = &(ct->_table[pos.offset]);
			else
				warning("getObject(): Trying to get an invalid object");
		} else if (mobj->getType() == SEG_TYPE_SCRIPT) {
			Script *scr = (Script *)mobj;
			if (pos.offset <= scr->_bufSize && pos.offset >= -SCRIPT_OBJECT_MAGIC_OFFSET
			        && RAW_IS_OBJECT(scr->_buf + pos.offset)) {
				obj = scr->getObject(pos.offset);
			}
		}
	}

	return obj;
}

const char *SegManager::getObjectName(reg_t pos) {
	Object *obj = getObject(pos);
	if (!obj)
		return "<no such object>";

	reg_t nameReg = obj->getNameSelector();
	if (nameReg.isNull())
		return "<no name>";

	const char *name = derefString(nameReg);
	if (!name)
		return "<invalid name>";

	return name;
}

reg_t SegManager::findObjectByName(const Common::String &name, int index) {
	reg_t retVal = NULL_REG;

	// Now all values are available; iterate over all objects.
	int timesFound = 0;
	for (uint i = 0; i < _heap.size(); i++) {
		SegmentObj *mobj = _heap[i];
		int idx = 0;
		int max_index = 0;
		ObjMap::iterator it;
		Script *scr = 0;
		CloneTable *ct = 0;

		if (mobj) {
			if (mobj->getType() == SEG_TYPE_SCRIPT) {
				scr = (Script *)mobj;
				max_index = scr->_objects.size();
				it = scr->_objects.begin();
			} else if (mobj->getType() == SEG_TYPE_CLONES) {
				ct = (CloneTable *)mobj;
				max_index = ct->_table.size();
			}
		}

		// It's a script or a clone table, scan all objects in it
		for (; idx < max_index; ++idx) {
			Object *obj = NULL;
			reg_t objpos;
			objpos.offset = 0;
			objpos.segment = i;

			if (mobj->getType() == SEG_TYPE_SCRIPT) {
				obj = &(it->_value);
				objpos.offset = obj->getPos().offset;
				++it;
			} else if (mobj->getType() == SEG_TYPE_CLONES) {
				if (!ct->isValidEntry(idx))
					continue;
				obj = &(ct->_table[idx]);
				objpos.offset = idx;
			}

			const char *objname = getObjectName(objpos);
			if (name == objname) {
				// Found a match!
				if ((index < 0) && (timesFound > 0)) {
					if (timesFound == 1) {
						// First time we realized the ambiguity
						printf("Ambiguous:\n");
						printf("  %3x: [%04x:%04x] %s\n", 0, PRINT_REG(retVal), name.c_str());
					}
					printf("  %3x: [%04x:%04x] %s\n", timesFound, PRINT_REG(objpos), name.c_str());
				}
				if (index < 0 || timesFound == index)
					retVal = objpos;
				++timesFound;
			}
		}

	}

	if (!timesFound)
		return NULL_REG;

	if (timesFound > 1 && index < 0) {
		printf("Ambiguous: Aborting.\n");
		return NULL_REG; // Ambiguous
	}

	if (timesFound <= index)
		return NULL_REG; // Not found

	return retVal;
}

// validate the seg
// return:
//	false - invalid seg
//	true  - valid seg
bool SegManager::check(SegmentId seg) {
	if (seg < 1 || (uint)seg >= _heap.size()) {
		return false;
	}
	if (!_heap[seg]) {
		warning("SegManager: seg %x is removed from memory, but not removed from hash_map", seg);
		return false;
	}
	return true;
}

void SegManager::setExportAreWide(bool flag) {
	_exportsAreWide = flag;
}

// return the seg if script_id is valid and in the map, else 0
SegmentId SegManager::getScriptSegment(int script_id) const {
	return _scriptSegMap.getVal(script_id, 0);
}

SegmentId SegManager::getScriptSegment(int script_nr, ScriptLoadType load) {
	SegmentId segment;

	if ((load & SCRIPT_GET_LOAD) == SCRIPT_GET_LOAD)
		script_instantiate(_resMan, this, script_nr);

	segment = getScriptSegment(script_nr);

	if (segment > 0) {
		if ((load & SCRIPT_GET_LOCK) == SCRIPT_GET_LOCK)
			getScript(segment)->incrementLockers();
	}
	return segment;
}

LocalVariables *SegManager::allocLocalsSegment(Script *scr, int count) {
	if (!count) { // No locals
		scr->_localsSegment = 0;
		scr->_localsBlock = NULL;
		return NULL;
	} else {
		LocalVariables *locals;

		if (scr->_localsSegment) {
			locals = (LocalVariables *)_heap[scr->_localsSegment];
			VERIFY(locals != NULL, "Re-used locals segment was NULL'd out");
			VERIFY(locals->getType() == SEG_TYPE_LOCALS, "Re-used locals segment did not consist of local variables");
			VERIFY(locals->script_id == scr->_nr, "Re-used locals segment belonged to other script");
		} else
			locals = (LocalVariables *)allocSegment(new LocalVariables(), &scr->_localsSegment);

		scr->_localsBlock = locals;
		locals->script_id = scr->_nr;
		locals->_locals.resize(count);

		return locals;
	}
}

DataStack *SegManager::allocateStack(int size, SegmentId *segid) {
	SegmentObj *mobj = allocSegment(new DataStack(), segid);
	DataStack *retval = (DataStack *)mobj;

	retval->_entries = (reg_t *)calloc(size, sizeof(reg_t));
	retval->_capacity = size;

	return retval;
}

SystemStrings *SegManager::allocateSysStrings(SegmentId *segid) {
	return (SystemStrings *)allocSegment(new SystemStrings(), segid);
}

void SegManager::freeHunkEntry(reg_t addr) {
	HunkTable *ht = (HunkTable *)GET_SEGMENT(*this, addr.segment, SEG_TYPE_HUNK);

	if (!ht) {
		warning("Attempt to free Hunk from address %04x:%04x: Invalid segment type", PRINT_REG(addr));
		return;
	}

	ht->freeEntry(addr.offset);
}

Hunk *SegManager::allocateHunkEntry(const char *hunk_type, int size, reg_t *addr) {
	HunkTable *table;
	int offset;

	if (!Hunks_seg_id)
		allocSegment(new HunkTable(), &(Hunks_seg_id));
	table = (HunkTable *)_heap[Hunks_seg_id];

	offset = table->allocEntry();

	*addr = make_reg(Hunks_seg_id, offset);
	Hunk *h = &(table->_table[offset]);

	if (!h)
		return NULL;

	h->mem = malloc(size);
	h->size = size;
	h->type = hunk_type;

	return h;
}

Clone *SegManager::allocateClone(reg_t *addr) {
	CloneTable *table;
	int offset;

	if (!Clones_seg_id)
		table = (CloneTable *)allocSegment(new CloneTable(), &(Clones_seg_id));
	else
		table = (CloneTable *)_heap[Clones_seg_id];

	offset = table->allocEntry();

	*addr = make_reg(Clones_seg_id, offset);
	return &(table->_table[offset]);
}

void SegManager::reconstructClones() {
	for (uint i = 0; i < _heap.size(); i++) {
		if (_heap[i]) {
			SegmentObj *mobj = _heap[i];
			if (mobj->getType() == SEG_TYPE_CLONES) {
				CloneTable *ct = (CloneTable *)mobj;

				for (uint j = 0; j < ct->_table.size(); j++) {
					// Check if the clone entry is used
					uint entryNum = (uint)ct->first_free;
					bool isUsed = true;
					while (entryNum != ((uint) CloneTable::HEAPENTRY_INVALID)) {
						if (entryNum == j) {
							isUsed = false;
							break;
						}
						entryNum = ct->_table[entryNum].next_free;
					}

					if (!isUsed)
						continue;

					CloneTable::Entry &seeker = ct->_table[j];
					Object *baseObj = getObject(seeker.getSpeciesSelector());
					seeker.cloneFromObject(baseObj);
					if (!baseObj)
						warning("Clone entry without a base class: %d", j);
				}	// end for
			}	// end if
		}	// end if
	}	// end for
}

List *SegManager::allocateList(reg_t *addr) {
	ListTable *table;
	int offset;

	if (!Lists_seg_id)
		allocSegment(new ListTable(), &(Lists_seg_id));
	table = (ListTable *)_heap[Lists_seg_id];

	offset = table->allocEntry();

	*addr = make_reg(Lists_seg_id, offset);
	return &(table->_table[offset]);
}

Node *SegManager::allocateNode(reg_t *addr) {
	NodeTable *table;
	int offset;

	if (!Nodes_seg_id)
		allocSegment(new NodeTable(), &(Nodes_seg_id));
	table = (NodeTable *)_heap[Nodes_seg_id];

	offset = table->allocEntry();

	*addr = make_reg(Nodes_seg_id, offset);
	return &(table->_table[offset]);
}

List *SegManager::lookupList(reg_t addr) {
	if (getSegmentType(addr.segment) != SEG_TYPE_LISTS) {
		warning("Attempt to use non-list %04x:%04x as list", PRINT_REG(addr));
		return NULL;
	}

	ListTable *lt = (ListTable *)_heap[addr.segment];

	if (!lt->isValidEntry(addr.offset)) {
		warning("Attempt to use non-list %04x:%04x as list", PRINT_REG(addr));
		return NULL;
	}

	return &(lt->_table[addr.offset]);
}

Node *SegManager::lookupNode(reg_t addr) {
	if (addr.isNull())
		return NULL; // Non-error null

	if (getSegmentType(addr.segment) != SEG_TYPE_NODES) {
		// FIXME: This occurs right at the beginning of SQ4, when walking north from the first screen. It doesn't
		// seem to have any apparent ill-effects, though, so it's been changed to non-fatal, for now
		//error("%s, L%d: Attempt to use non-node %04x:%04x as list node", __FILE__, __LINE__, PRINT_REG(addr));
		warning("Attempt to use non-node %04x:%04x as list node", PRINT_REG(addr));
		return NULL;
	}

	NodeTable *nt = (NodeTable *)_heap[addr.segment];

	if (!nt->isValidEntry(addr.offset)) {
		warning("Attempt to use non-node %04x:%04x as list node", PRINT_REG(addr));
		return NULL;
	}

	return &(nt->_table[addr.offset]);
}

SegmentRef SegManager::dereference(reg_t pointer) {
	SegmentRef ret;

	if (!pointer.segment || (pointer.segment >= _heap.size()) || !_heap[pointer.segment]) {
		// This occurs in KQ5CD when interacting with certain objects
		warning("SegManager::dereference(): Attempt to dereference invalid pointer %04x:%04x", PRINT_REG(pointer));
		return ret; /* Invalid */
	}

	SegmentObj *mobj = _heap[pointer.segment];
	return mobj->dereference(pointer);
}

static void *derefPtr(SegManager *segMan, reg_t pointer, int entries, bool wantRaw) {
	SegmentRef ret = segMan->dereference(pointer);

	if (!ret.isValid())
		return NULL;

	if (ret.isRaw != wantRaw) {
		warning("Dereferencing pointer %04x:%04x (type %d) which is %s, but expected %s", PRINT_REG(pointer),
			segMan->_heap[pointer.segment]->getType(),
			ret.isRaw ? "raw" : "not raw",
			wantRaw ? "raw" : "not raw");
	}

	if (!wantRaw && ret.skipByte) {
		warning("Unaligned pointer read: %04x:%04x expected with word alignment", PRINT_REG(pointer));
		return NULL;
	}

	if (entries > ret.maxSize) {
		warning("Trying to dereference pointer %04x:%04x beyond end of segment", PRINT_REG(pointer));
		return NULL;
	}

	if (ret.isRaw)
		return ret.raw;
	else
		return ret.reg;
}

byte *SegManager::derefBulkPtr(reg_t pointer, int entries) {
	return (byte *)derefPtr(this, pointer, entries, true);
}

reg_t *SegManager::derefRegPtr(reg_t pointer, int entries) {
	return (reg_t *)derefPtr(this, pointer, 2*entries, false);
}

char *SegManager::derefString(reg_t pointer, int entries) {
	return (char *)derefPtr(this, pointer, entries, true);
}

// Helper functions for getting/setting characters in string fragments
static inline char getChar(const SegmentRef &ref, uint offset) {
	if (ref.skipByte)
		offset++;

	reg_t val = ref.reg[offset / 2];

	if (val.segment != 0)
		warning("Attempt to read character from non-raw data");

	return (offset & 1 ? val.offset >> 8 : val.offset & 0xff);
}

static inline void setChar(const SegmentRef &ref, uint offset, char value) {
	if (ref.skipByte)
		offset++;

	reg_t *val = ref.reg + offset / 2;

	val->segment = 0;

	if (offset & 1)
		val->offset = (val->offset & 0x00ff) | (value << 8);
	else
		val->offset = (val->offset & 0xff00) | value;
}

// TODO: memcpy, strcpy and strncpy could maybe be folded into a single function
void SegManager::strncpy(reg_t dest, const char* src, size_t n) {
	SegmentRef dest_r = dereference(dest);
	if (!dest_r.isValid()) {
		warning("Attempt to strncpy to invalid pointer %04x:%04x", PRINT_REG(dest));
		return;
	}

	if (dest_r.isRaw) {
		// raw -> raw
		if (n == 0xFFFFFFFFU)
			::strcpy((char *)dest_r.raw, src);
		else
			::strncpy((char *)dest_r.raw, src, n);
	} else {
		// raw -> non-raw
		for (uint i = 0; i < n; i++) {
			setChar(dest_r, i, src[i]);
			if (!src[i])
				break;
		}
	}
}

void SegManager::strncpy(reg_t dest, reg_t src, size_t n) {
	SegmentRef dest_r = dereference(dest);
	const SegmentRef src_r = dereference(src);
	if (!src_r.isValid()) {
		warning("Attempt to strncpy from invalid pointer %04x:%04x", PRINT_REG(src));

		// Clear target string instead.
		if (n > 0)
			strcpy(dest, "");
		return;
	}

	if (!dest_r.isValid()) {
		warning("Attempt to strncpy to invalid pointer %04x:%04x", PRINT_REG(dest));
		return;
	}


	if (src_r.isRaw) {
		// raw -> *
		strncpy(dest, (const char*)src_r.raw, n);
	} else if (dest_r.isRaw && !src_r.isRaw) {
		// non-raw -> raw
		for (uint i = 0; i < n; i++) {
			char c = getChar(src_r, i);
			dest_r.raw[i] = c;
			if (!c)
				break;
		}
	} else {
		// non-raw -> non-raw
		for (uint i = 0; i < n; i++) {
			char c = getChar(src_r, i);
			setChar(dest_r, i, c);
			if (!c)
				break;
		}
	}
}

void SegManager::strcpy(reg_t dest, const char* src) {
	strncpy(dest, src, 0xFFFFFFFFU);
}

void SegManager::strcpy(reg_t dest, reg_t src) {
	strncpy(dest, src, 0xFFFFFFFFU);
}

void SegManager::memcpy(reg_t dest, const byte* src, size_t n) {
	SegmentRef dest_r = dereference(dest);
	if (!dest_r.isValid()) {
		warning("Attempt to memcpy to invalid pointer %04x:%04x", PRINT_REG(dest));
		return;
	}
	if ((int)n > dest_r.maxSize) {
		warning("Trying to dereference pointer %04x:%04x beyond end of segment", PRINT_REG(dest));
		return;
	}

	if (dest_r.isRaw) {
		// raw -> raw
		::memcpy((char*)dest_r.raw, src, n);
	} else {
		// raw -> non-raw
		for (uint i = 0; i < n; i++)
			setChar(dest_r, i, src[i]);
	}
}

void SegManager::memcpy(reg_t dest, reg_t src, size_t n) {
	SegmentRef dest_r = dereference(dest);
	const SegmentRef src_r = dereference(src);
	if (!dest_r.isValid()) {
		warning("Attempt to memcpy to invalid pointer %04x:%04x", PRINT_REG(dest));
		return;
	}
	if ((int)n > dest_r.maxSize) {
		warning("Trying to dereference pointer %04x:%04x beyond end of segment", PRINT_REG(dest));
		return;
	}
	if (!src_r.isValid()) {
		warning("Attempt to memcpy from invalid pointer %04x:%04x", PRINT_REG(src));
		return;
	}
	if ((int)n > src_r.maxSize) {
		warning("Trying to dereference pointer %04x:%04x beyond end of segment", PRINT_REG(src));
		return;
	}

	if (src_r.isRaw) {
		// raw -> *
		memcpy(dest, src_r.raw, n);
	} else if (dest_r.isRaw) {
		// * -> raw
		memcpy(dest_r.raw, src, n);
	} else {
		// non-raw -> non-raw
		for (uint i = 0; i < n; i++) {
			char c = getChar(src_r, i);
			setChar(dest_r, i, c);
		}
	}
}

void SegManager::memcpy(byte *dest, reg_t src, size_t n) {
	const SegmentRef src_r = dereference(src);
	if (!src_r.isValid()) {
		warning("Attempt to memcpy from invalid pointer %04x:%04x", PRINT_REG(src));
		return;
	}
	if ((int)n > src_r.maxSize) {
		warning("Trying to dereference pointer %04x:%04x beyond end of segment", PRINT_REG(src));
		return;
	}

	if (src_r.isRaw) {
		// raw -> raw
		::memcpy(dest, src_r.raw, n);
	} else {
		// non-raw -> raw
		for (uint i = 0; i < n; i++) {
			char c = getChar(src_r, i);
			dest[i] = c;
		}
	}
}

size_t SegManager::strlen(reg_t str) {
	SegmentRef str_r = dereference(str);
	if (!str_r.isValid()) {
		warning("Attempt to call strlen on invalid pointer %04x:%04x", PRINT_REG(str));
		return 0;
	}

	if (str_r.isRaw) {
		return ::strlen((const char *)str_r.raw);
	} else {
		int i = 0;
		while (getChar(str_r, i))
			i++;
		return i;
	}
}


Common::String SegManager::getString(reg_t pointer, int entries) {
	Common::String ret;
	SegmentRef src_r = dereference(pointer);
	if (!src_r.isValid()) {
		warning("SegManager::getString(): Attempt to dereference invalid pointer %04x:%04x", PRINT_REG(pointer));
		return ret;
	}
	if (entries > src_r.maxSize) {
		warning("Trying to dereference pointer %04x:%04x beyond end of segment", PRINT_REG(pointer));
		return ret;
	}
	if (src_r.isRaw)
		ret = (char *)src_r.raw;
	else {
		uint i = 0;
		for (;;) {
			char c = getChar(src_r, i);

			if (!c)
				break;

			i++;
			ret += c;
		};
	}
	return ret;
}


byte *SegManager::allocDynmem(int size, const char *descr, reg_t *addr) {
	SegmentId seg;
	SegmentObj *mobj = allocSegment(new DynMem(), &seg);
	*addr = make_reg(seg, 0);

	DynMem &d = *(DynMem *)mobj;

	d._size = size;

	if (size == 0)
		d._buf = NULL;
	else
		d._buf = (byte *)malloc(size);

	d._description = descr;

	return (byte *)(d._buf);
}

int SegManager::freeDynmem(reg_t addr) {
	if (addr.segment < 1 || addr.segment >= _heap.size() || !_heap[addr.segment] || _heap[addr.segment]->getType() != SEG_TYPE_DYNMEM)
		return 1; // error

	deallocate(addr.segment, true);

	return 0; // OK
}

#ifdef ENABLE_SCI32
SciArray<reg_t> *SegManager::allocateArray(reg_t *addr) {
	ArrayTable *table;
	int offset;

	if (!Arrays_seg_id) {
		table = (ArrayTable *)allocSegment(new ArrayTable(), &(Arrays_seg_id));
	} else
		table = (ArrayTable *)_heap[Arrays_seg_id];

	offset = table->allocEntry();

	*addr = make_reg(Arrays_seg_id, offset);
	return &(table->_table[offset]);
}

SciArray<reg_t> *SegManager::lookupArray(reg_t addr) {
	if (_heap[addr.segment]->getType() != SEG_TYPE_ARRAY)
		error("Attempt to use non-array %04x:%04x as array", PRINT_REG(addr));

	ArrayTable *arrayTable = (ArrayTable *)_heap[addr.segment];

	if (!arrayTable->isValidEntry(addr.offset))
		error("Attempt to use non-array %04x:%04x as array", PRINT_REG(addr));

	return &(arrayTable->_table[addr.offset]);
}

void SegManager::freeArray(reg_t addr) {
	if (_heap[addr.segment]->getType() != SEG_TYPE_ARRAY)
		error("Attempt to use non-array %04x:%04x as array", PRINT_REG(addr));

	ArrayTable *arrayTable = (ArrayTable *)_heap[addr.segment];

	if (!arrayTable->isValidEntry(addr.offset))
		error("Attempt to use non-array %04x:%04x as array", PRINT_REG(addr));

	arrayTable->_table[addr.offset].destroy();
	arrayTable->freeEntry(addr.offset);
}

SciString *SegManager::allocateString(reg_t *addr) {
	StringTable *table;
	int offset;

	if (!String_seg_id) {
		table = (StringTable *)allocSegment(new StringTable(), &(String_seg_id));
	} else
		table = (StringTable *)_heap[String_seg_id];

	offset = table->allocEntry();

	*addr = make_reg(String_seg_id, offset);
	return &(table->_table[offset]);
}

SciString *SegManager::lookupString(reg_t addr) {
	if (_heap[addr.segment]->getType() != SEG_TYPE_STRING)
		error("lookupString: Attempt to use non-string %04x:%04x as string", PRINT_REG(addr));

	StringTable *stringTable = (StringTable *)_heap[addr.segment];

	if (!stringTable->isValidEntry(addr.offset))
		error("lookupString: Attempt to use non-string %04x:%04x as string", PRINT_REG(addr));

	return &(stringTable->_table[addr.offset]);
}

void SegManager::freeString(reg_t addr) {
	if (_heap[addr.segment]->getType() != SEG_TYPE_STRING)
		error("freeString: Attempt to use non-string %04x:%04x as string", PRINT_REG(addr));

	StringTable *stringTable = (StringTable *)_heap[addr.segment];

	if (!stringTable->isValidEntry(addr.offset))
		error("freeString: Attempt to use non-string %04x:%04x as string", PRINT_REG(addr));

	stringTable->_table[addr.offset].destroy();
	stringTable->freeEntry(addr.offset);
}

#endif

} // End of namespace Sci
