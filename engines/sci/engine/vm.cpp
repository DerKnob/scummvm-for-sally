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
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-1-1/engines/sci/engine/vm.cpp $
 * $Id: vm.cpp 48050 2010-02-13 17:45:40Z fingolfin $
 *
 */

#include "common/debug.h"
#include "common/stack.h"
#include "common/config-manager.h"

#include "sci/sci.h"
#include "sci/console.h"
#include "sci/debug.h"	// for g_debugState
#include "sci/resource.h"
#include "sci/engine/features.h"
#include "sci/engine/state.h"
#include "sci/engine/kernel.h"
#include "sci/engine/kernel_types.h"
#include "sci/engine/seg_manager.h"
#include "sci/engine/script.h"
#include "sci/engine/gc.h"

namespace Sci {

const reg_t NULL_REG = {0, 0};
const reg_t SIGNAL_REG = {0, SIGNAL_OFFSET};

//#define VM_DEBUG_SEND

ScriptState scriptState;	// FIXME: Avoid non-const global vars
int g_loadFromLauncher;	// FIXME: Avoid non-const global vars

int script_abort_flag = 0; // Set to 1 to abort execution. Set to 2 to force a replay afterwards	// FIXME: Avoid non-const global vars
int script_step_counter = 0; // Counts the number of steps executed	// FIXME: Avoid non-const global vars
int script_gc_interval = GC_INTERVAL; // Number of steps in between gcs	// FIXME: Avoid non-const global vars

static bool breakpointWasHit = false;	// FIXME: Avoid non-const global vars


#define SCI_XS_CALLEE_LOCALS ((SegmentId)-1)

/**
 * Adds an entry to the top of the execution stack.
 *
 * @param[in] s				The state with which to execute
 * @param[in] pc			The initial program counter
 * @param[in] sp			The initial stack pointer
 * @param[in] objp			Pointer to the beginning of the current object
 * @param[in] argc			Number of parameters to call with
 * @param[in] argp			Heap pointer to the first parameter
 * @param[in] selector		The selector by which it was called or
 *							NULL_SELECTOR if n.a. For debugging.
 * @param[in] sendp			Pointer to the object which the message was
 * 							sent to. Equal to objp for anything but super.
 * @param[in] origin		Number of the execution stack element this
 * 							entry was created by (usually the current TOS
 * 							number, except for multiple sends).
 * @param[in] local_segment	The segment to use for local variables,
 *							or SCI_XS_CALLEE_LOCALS to use obj's segment.
 * @return 					A pointer to the new exec stack TOS entry
 */
static ExecStack *add_exec_stack_entry(Common::List<ExecStack> &execStack, reg_t pc, StackPtr sp,
		reg_t objp, int argc, StackPtr argp, Selector selector,
		reg_t sendp, int origin, SegmentId local_segment);


/**
 * Adds one varselector access to the execution stack.
 * This function is called from send_selector only.
 * @param[in] s			The EngineState to use
 * @param[in] objp		Pointer to the object owning the selector
 * @param[in] argc		1 for writing, 0 for reading
 * @param[in] argp		Pointer to the address of the data to write -2
 * @param[in] selector	Selector name
 * @param[in] address	Heap address of the selector
 * @param[in] origin	Stack frame which the access originated from
 * @return 				Pointer to the new exec-TOS element
 */
static ExecStack *add_exec_stack_varselector(Common::List<ExecStack> &execStack, reg_t objp, int argc,
		StackPtr argp, Selector selector, const ObjVarRef& address,
		int origin);




// validation functionality

#ifndef DISABLE_VALIDATIONS

static reg_t &validate_property(Object *obj, int index) {
	// A static dummy reg_t, which we return if obj or index turn out to be
	// invalid. Note that we cannot just return NULL_REG, because client code
	// may modify the value of the returned reg_t.
	static reg_t dummyReg = NULL_REG;

	if (!obj) {
		debugC(2, kDebugLevelVM, "[VM] Sending to disposed object!");
		return dummyReg;
	}

	if (index < 0 || (uint)index >= obj->getVarCount()) {
		debugC(2, kDebugLevelVM, "[VM] Invalid property #%d (out of [0..%d]) requested!",
			index, obj->getVarCount());
		return dummyReg;
	}

	return obj->_variables[index];
}

static StackPtr validate_stack_addr(EngineState *s, StackPtr sp) {
	if (sp >= s->stack_base && sp < s->stack_top)
		return sp;

	error("[VM] Stack index %d out of valid range [%d..%d]",
		(int)(sp - s->stack_base), 0, (int)(s->stack_top - s->stack_base - 1));
	return 0;
}

static int validate_arithmetic(reg_t reg) {
	if (reg.segment) {
		warning("[VM] Attempt to read arithmetic value from non-zero segment [%04x]", reg.segment);
		return 0;
	}

	return reg.offset;
}

static int signed_validate_arithmetic(reg_t reg) {
	if (reg.segment) {
		warning("[VM] Attempt to read arithmetic value from non-zero segment [%04x]", reg.segment);
		return 0;
	}

	return (int16)reg.offset;
}

static bool validate_variable(reg_t *r, reg_t *stack_base, int type, int max, int index, int line) {
	const char *names[4] = {"global", "local", "temp", "param"};

	if (index < 0 || index >= max) {
		Common::String txt = Common::String::printf(
							"[VM] Attempt to use invalid %s variable %04x ",
							names[type], index);
		if (max == 0)
			txt += "(variable type invalid)";
		else
			txt += Common::String::printf("(out of range [%d..%d])", 0, max - 1);

		if (type == VAR_PARAM || type == VAR_TEMP) {
			int total_offset = r - stack_base;
			if (total_offset < 0 || total_offset >= VM_STACK_SIZE) {
				warning("%s", txt.c_str());
				warning("[VM] Access would be outside even of the stack (%d); access denied", total_offset);
				return false;
			} else {
				debugC(2, kDebugLevelVM, "%s", txt.c_str());
				debugC(2, kDebugLevelVM, "[VM] Access within stack boundaries; access granted.");
				return true;
			}
		}
		return false;
	}

	return true;
}

static reg_t validate_read_var(reg_t *r, reg_t *stack_base, int type, int max, int index, int line, reg_t default_value) {
	if (validate_variable(r, stack_base, type, max, index, line))
		return r[index];
	else
		return default_value;
}

static void validate_write_var(reg_t *r, reg_t *stack_base, int type, int max, int index, int line, reg_t value, SegManager *segMan, Kernel *kernel) {
	if (validate_variable(r, stack_base, type, max, index, line)) {

		// WORKAROUND: This code is needed to work around a probable script bug, or a
		// limitation of the original SCI engine, which can be observed in LSL5.
		//
		// In some games, ego walks via the "Grooper" object, in particular its "stopGroop"
		// child. In LSL5, during the game, ego is swapped from Larry to Patti. When this
		// happens in the original interpreter, the new actor is loaded in the same memory
		// location as the old one, therefore the client variable in the stopGroop object
		// points to the new actor. This is probably why the reference of the stopGroop
		// object is never updated (which is why I mentioned that this is either a script
		// bug or some kind of limitation).
		//
		// In our implementation, each new object is loaded in a different memory location,
		// and we can't overwrite the old one. This means that in our implementation,
		// whenever ego is changed, we need to update the "client" variable of the
		// stopGroop object, which points to ego, to the new ego object. If this is not
		// done, ego's movement will not be updated properly, so the result is
		// unpredictable (for example in LSL5, Patti spins around instead of walking).
		if (index == 0 && type == VAR_GLOBAL) {	// global 0 is ego
			reg_t stopGroopPos = segMan->findObjectByName("stopGroop");
			if (!stopGroopPos.isNull()) {	// does the game have a stopGroop object?
				// Find the "client" member variable of the stopGroop object, and update it
				ObjVarRef varp;
				if (lookup_selector(segMan, stopGroopPos, kernel->_selectorCache.client, &varp, NULL) == kSelectorVariable) {
					reg_t *clientVar = varp.getPointer(segMan);
					*clientVar = value;
				}
			}
		}

		r[index] = value;
	}
}

#else
// Non-validating alternatives

#  define validate_stack_addr(s, sp) sp
#  define validate_arithmetic(r) ((r).offset)
#  define signed_validate_arithmetic(r) ((int16)(r).offset)
#  define validate_variable(r, sb, t, m, i, l)
#  define validate_read_var(r, sb, t, m, i, l, dv) ((r)[i])
#  define validate_write_var(r, sb, t, m, i, l, v, sm, k) ((r)[i] = (v))
#  define validate_property(o, p) ((o)->_variables[p])

#endif

#define READ_VAR(type, index, def) validate_read_var(scriptState.variables[type], s->stack_base, type, scriptState.variables_max[type], index, __LINE__, def)
#define WRITE_VAR(type, index, value) validate_write_var(scriptState.variables[type], s->stack_base, type, scriptState.variables_max[type], index, __LINE__, value, s->_segMan, g_sci->getKernel())
#define WRITE_VAR16(type, index, value) WRITE_VAR(type, index, make_reg(0, value));

#define ACC_ARITHMETIC_L(op) make_reg(0, (op validate_arithmetic(s->r_acc)))
#define ACC_AUX_LOAD() aux_acc = signed_validate_arithmetic(s->r_acc)
#define ACC_AUX_STORE() s->r_acc = make_reg(0, aux_acc)

#define OBJ_PROPERTY(o, p) (validate_property(o, p))

// Operating on the stack
// 16 bit:
#define PUSH(v) PUSH32(make_reg(0, v))
#define POP() (validate_arithmetic(POP32()))
// 32 bit:
#define PUSH32(a) (*(validate_stack_addr(s, (scriptState.xs->sp)++)) = (a))
#define POP32() (*(validate_stack_addr(s, --(scriptState.xs->sp))))

ExecStack *execute_method(EngineState *s, uint16 script, uint16 pubfunct, StackPtr sp, reg_t calling_obj, uint16 argc, StackPtr argp) {
	int seg = s->_segMan->getScriptSegment(script);
	Script *scr = s->_segMan->getScriptIfLoaded(seg);

	if (!scr || scr->isMarkedAsDeleted()) // Script not present yet?
		seg = script_instantiate(g_sci->getResMan(), s->_segMan, script);

	const int temp = s->_segMan->validateExportFunc(pubfunct, seg);
	if (!temp) {
#ifdef ENABLE_SCI32
		// HACK: Temporarily switch to a warning in SCI32 games until we can figure out why Torin has
		// an invalid exported function.
		if (getSciVersion() >= SCI_VERSION_2)
			warning("Request for invalid exported function 0x%x of script 0x%x", pubfunct, script);
		else
#endif
			error("Request for invalid exported function 0x%x of script 0x%x", pubfunct, script);
		return NULL;
	}

	// Check if a breakpoint is set on this method
	if (s->_activeBreakpointTypes & BREAK_EXPORT) {
		uint32 bpaddress;

		bpaddress = (script << 16 | pubfunct);

		Common::List<Breakpoint>::const_iterator bp;
		for (bp = s->_breakpoints.begin(); bp != s->_breakpoints.end(); ++bp) {
			if (bp->type == BREAK_EXPORT && bp->address == bpaddress) {
				Console *con = g_sci->getSciDebugger();
				con->DebugPrintf("Break on script %d, export %d\n", script, pubfunct);
				g_debugState.debugging = true;
				breakpointWasHit = true;
				break;
			}
		}
	}

	return add_exec_stack_entry(s->_executionStack, make_reg(seg, temp), sp, calling_obj, argc, argp, -1, calling_obj, s->_executionStack.size()-1, seg);
}


static void _exec_varselectors(EngineState *s) {
	// Executes all varselector read/write ops on the TOS
	while (!s->_executionStack.empty() && s->_executionStack.back().type == EXEC_STACK_TYPE_VARSELECTOR) {
		ExecStack &xs = s->_executionStack.back();
		reg_t *var = xs.getVarPointer(s->_segMan);
		if (!var) {
			warning("Invalid varselector exec stack entry");
		} else {
			// varselector access?
			if (xs.argc) { // write?
				*var = xs.variables_argp[1];

			} else // No, read
				s->r_acc = *var;
		}
		s->_executionStack.pop_back();
	}
}

/** This struct is used to buffer the list of send calls in send_selector() */
struct CallsStruct {
	reg_t addr_func;
	reg_t varp_objp;
	union {
		reg_t func;
		ObjVarRef var;
	} address;
	StackPtr argp;
	int argc;
	Selector selector;
	StackPtr sp; /**< Stack pointer */
	int type; /**< Same as ExecStack.type */
};

ExecStack *send_selector(EngineState *s, reg_t send_obj, reg_t work_obj, StackPtr sp, int framesize, StackPtr argp) {
// send_obj and work_obj are equal for anything but 'super'
// Returns a pointer to the TOS exec_stack element
	assert(s);

	reg_t funcp;
	int selector;
	int argc;
	int origin = s->_executionStack.size()-1; // Origin: Used for debugging
	int print_send_action = 0;
	// We return a pointer to the new active ExecStack

	// The selector calls we catch are stored below:
	Common::Stack<CallsStruct> sendCalls;

	while (framesize > 0) {
		selector = validate_arithmetic(*argp++);
		argc = validate_arithmetic(*argp);

		if (argc > 0x800) { // More arguments than the stack could possibly accomodate for
			error("send_selector(): More than 0x800 arguments to function call");
		}

		// Check if a breakpoint is set on this method
		if (s->_activeBreakpointTypes & BREAK_SELECTOR) {
			char method_name[256];

			sprintf(method_name, "%s::%s", s->_segMan->getObjectName(send_obj), g_sci->getKernel()->getSelectorName(selector).c_str());

			Common::List<Breakpoint>::const_iterator bp;
			for (bp = s->_breakpoints.begin(); bp != s->_breakpoints.end(); ++bp) {
				int cmplen = bp->name.size();
				if (bp->name.lastChar() != ':')
					cmplen = 256;

				if (bp->type == BREAK_SELECTOR && !strncmp(bp->name.c_str(), method_name, cmplen)) {
					Console *con = g_sci->getSciDebugger();
					con->DebugPrintf("Break on %s (in [%04x:%04x])\n", method_name, PRINT_REG(send_obj));
					print_send_action = 1;
					breakpointWasHit = true;
					g_debugState.debugging = true;
					break;
				}
			}
		}

#ifdef VM_DEBUG_SEND
		printf("Send to %04x:%04x, selector %04x (%s):", PRINT_REG(send_obj), selector, g_sci->getKernel()->getSelectorName(selector).c_str());
#endif // VM_DEBUG_SEND

		ObjVarRef varp;
		switch (lookup_selector(s->_segMan, send_obj, selector, &varp, &funcp)) {
		case kSelectorNone:
			error("Send to invalid selector 0x%x of object at %04x:%04x", 0xffff & selector, PRINT_REG(send_obj));
			break;

		case kSelectorVariable:

#ifdef VM_DEBUG_SEND
			if (argc)
				printf("Varselector: Write %04x:%04x\n", PRINT_REG(argp[1]));
			else
				printf("Varselector: Read\n");
#endif // VM_DEBUG_SEND

			// argc == 0: read selector
			// argc == 1: write selector
			// argc > 1: write selector?
			if (print_send_action && argc ==  0) {	// read selector
				printf("[read selector]\n");
				print_send_action = 0;
			}

			if (print_send_action && argc > 0) {
				reg_t oldReg = *varp.getPointer(s->_segMan);
				reg_t newReg = argp[1];
				printf("[write to selector: change %04x:%04x to %04x:%04x]\n", PRINT_REG(oldReg), PRINT_REG(newReg));
				print_send_action = 0;
			}

			if (argc > 1)
				warning("send_selector(): more than 1 parameter (%d) while modifying a variable selector", argc);

			{
				CallsStruct call;
				call.address.var = varp; // register the call
				call.argp = argp;
				call.argc = argc;
				call.selector = selector;
				call.type = EXEC_STACK_TYPE_VARSELECTOR; // Register as a varselector
				sendCalls.push(call);
			}

			break;

		case kSelectorMethod:

#ifdef VM_DEBUG_SEND
			printf("Funcselector(");
			for (int i = 0; i < argc; i++) {
				printf("%04x:%04x", PRINT_REG(argp[i+1]));
				if (i + 1 < argc)
					printf(", ");
			}
			printf(") at %04x:%04x\n", PRINT_REG(funcp));
#endif // VM_DEBUG_SEND
			if (print_send_action) {
				printf("[invoke selector]\n");
				print_send_action = 0;
			}

			{
				CallsStruct call;
				call.address.func = funcp; // register call
				call.argp = argp;
				call.argc = argc;
				call.selector = selector;
				call.type = EXEC_STACK_TYPE_CALL;
				call.sp = sp;
				sp = CALL_SP_CARRY; // Destroy sp, as it will be carried over
				sendCalls.push(call);
			}

			break;
		} // switch (lookup_selector())

		framesize -= (2 + argc);
		argp += argc + 1;
	}

	// Iterate over all registered calls in the reverse order. This way, the first call is
	// placed on the TOS; as soon as it returns, it will cause the second call to be executed.
	while (!sendCalls.empty()) {
		CallsStruct call = sendCalls.pop();
		if (call.type == EXEC_STACK_TYPE_VARSELECTOR) // Write/read variable?
			add_exec_stack_varselector(s->_executionStack, work_obj, call.argc, call.argp,
			                                    call.selector, call.address.var, origin);
		else
			add_exec_stack_entry(s->_executionStack, call.address.func, call.sp, work_obj,
			                         call.argc, call.argp,
			                         call.selector, send_obj, origin, SCI_XS_CALLEE_LOCALS);
	}

	_exec_varselectors(s);

	if (s->_executionStack.empty())
		return NULL;
	return &(s->_executionStack.back());
}

static ExecStack *add_exec_stack_varselector(Common::List<ExecStack> &execStack, reg_t objp, int argc, StackPtr argp, Selector selector, const ObjVarRef& address, int origin) {
	ExecStack *xstack = add_exec_stack_entry(execStack, NULL_REG, 0, objp, argc, argp, selector, objp, origin, SCI_XS_CALLEE_LOCALS);
	// Store selector address in sp

	xstack->addr.varp = address;
	xstack->type = EXEC_STACK_TYPE_VARSELECTOR;

	return xstack;
}

static ExecStack *add_exec_stack_entry(Common::List<ExecStack> &execStack, reg_t pc, StackPtr sp, reg_t objp, int argc,
								   StackPtr argp, Selector selector, reg_t sendp, int origin, SegmentId _localsSegment) {
	// Returns new TOS element for the execution stack
	// _localsSegment may be -1 if derived from the called object

	//printf("Exec stack: [%d/%d], origin %d, at %p\n", s->execution_stack_pos, s->_executionStack.size(), origin, s->execution_stack);

	ExecStack xstack;

	xstack.objp = objp;
	if (_localsSegment != SCI_XS_CALLEE_LOCALS)
		xstack.local_segment = _localsSegment;
	else
		xstack.local_segment = pc.segment;

	xstack.sendp = sendp;
	xstack.addr.pc = pc;
	xstack.fp = xstack.sp = sp;
	xstack.argc = argc;

	xstack.variables_argp = argp; // Parameters

	*argp = make_reg(0, argc);  // SCI code relies on the zeroeth argument to equal argc

	// Additional debug information
	xstack.selector = selector;
	xstack.origin = origin;

	xstack.type = EXEC_STACK_TYPE_CALL; // Normal call

	execStack.push_back(xstack);
	return &(execStack.back());
}

#ifdef DISABLE_VALIDATIONS
#  define kernel_matches_signature(a, b, c, d) 1
#endif

static reg_t pointer_add(EngineState *s, reg_t base, int offset) {
	SegmentObj *mobj = s->_segMan->getSegmentObj(base.segment);

	if (!mobj) {
		error("[VM] Error: Attempt to add %d to invalid pointer %04x:%04x", offset, PRINT_REG(base));
		return NULL_REG;
	}

	switch (mobj->getType()) {

	case SEG_TYPE_LOCALS:
	case SEG_TYPE_SCRIPT:
	case SEG_TYPE_STACK:
	case SEG_TYPE_DYNMEM:
		base.offset += offset;
		return base;
		break;

	default:
		// FIXME: Changed this to warning, because iceman does this during dancing with girl.
		// Investigate why that is so and either fix the underlying issue or implement a more
		// specialized workaround!
		warning("[VM] Error: Attempt to add %d to pointer %04x:%04x, type %d: Pointer arithmetics of this type unsupported", offset, PRINT_REG(base), mobj->getType());
		return NULL_REG;

	}
}

static void callKernelFunc(EngineState *s, int kernelFuncNum, int argc) {

	if (kernelFuncNum >= (int)g_sci->getKernel()->_kernelFuncs.size())
		error("Invalid kernel function 0x%x requested", kernelFuncNum);

	const KernelFuncWithSignature &kernelFunc = g_sci->getKernel()->_kernelFuncs[kernelFuncNum];

	if (kernelFunc.signature
			&& !kernel_matches_signature(s->_segMan, kernelFunc.signature, argc, scriptState.xs->sp + 1)) {
		error("[VM] Invalid arguments to kernel call %x", kernelFuncNum);
	}

	reg_t *argv = scriptState.xs->sp + 1;

	if (!kernelFunc.isDummy) {
		// Add stack frame to indicate we're executing a callk.
		// This is useful in debugger backtraces if this
		// kernel function calls a script itself.
		ExecStack *xstack;
		xstack = add_exec_stack_entry(s->_executionStack, NULL_REG, NULL, NULL_REG, argc, argv - 1, 0, NULL_REG,
				  s->_executionStack.size()-1, SCI_XS_CALLEE_LOCALS);
		xstack->selector = kernelFuncNum;
		xstack->type = EXEC_STACK_TYPE_KERNEL;

		//warning("callk %s", kernelFunc.orig_name.c_str());

		// TODO: SCI2/SCI2.1+ equivalent, once saving/loading works in SCI2/SCI2.1+
		if (g_loadFromLauncher >= 0 && kernelFuncNum == 0x8) {
			// A game is being loaded from the launcher, and kDisplay is called, all initialization has taken
			// place (i.e. menus have been constructed etc). Therefore, inject a kRestoreGame call
			// here, instead of the requested function.
			int saveSlot = g_loadFromLauncher;
			g_loadFromLauncher = -1;	// invalidate slot, so that we don't load again

			if (saveSlot < 0)
				error("Requested to load invalid save slot");	// should never happen, really

			reg_t restoreArgv[2] = { NULL_REG, make_reg(0, saveSlot) };	// special call (argv[0] is NULL)
			kRestoreGame(s, 2, restoreArgv);
		} else {
			// Call kernel function
			s->r_acc = kernelFunc.fun(s, argc, argv);
		}

		// Remove callk stack frame again
		s->_executionStack.pop_back();
	} else {
		Common::String warningMsg = "Dummy function " + kernelFunc.orig_name +
									Common::String::printf("[0x%x]", kernelFuncNum) +
									" invoked - ignoring. Params: " +
									Common::String::printf("%d", argc) + " (";

		for (int i = 0; i < argc; i++) {
			warningMsg +=  Common::String::printf("%04x:%04x", PRINT_REG(argv[i]));
			warningMsg += (i == argc - 1 ? ")" : ", ");
		}

		warning("%s", warningMsg.c_str());
	}
}

static void gc_countdown(EngineState *s) {
	if (s->gc_countdown-- <= 0) {
		s->gc_countdown = script_gc_interval;
		run_gc(s);
	}
}

static const byte _fake_return_buffer[2] = {op_ret << 1, op_ret << 1};


int readPMachineInstruction(const byte *src, byte &extOpcode, int16 opparams[4]) {
	uint offset = 0;
	extOpcode = src[offset++]; // Get "extended" opcode (lower bit has special meaning)
	const byte opcode = extOpcode >> 1;	// get the actual opcode

	memset(opparams, 0, sizeof(opparams));

	for (int i = 0; g_opcode_formats[opcode][i]; ++i) {
		//printf("Opcode: 0x%x, Opnumber: 0x%x, temp: %d\n", opcode, opcode, temp);
		assert(i < 4);
		switch (g_opcode_formats[opcode][i]) {

		case Script_Byte:
			opparams[i] = src[offset++];
			break;
		case Script_SByte:
			opparams[i] = (int8)src[offset++];
			break;

		case Script_Word:
			opparams[i] = READ_LE_UINT16(src + offset);
			offset += 2;
			break;
		case Script_SWord:
			opparams[i] = (int16)READ_LE_UINT16(src + offset);
			offset += 2;
			break;

		case Script_Variable:
		case Script_Property:

		case Script_Local:
		case Script_Temp:
		case Script_Global:
		case Script_Param:

		case Script_Offset:
			if (extOpcode & 1) {
				opparams[i] = src[offset++];
			} else {
				opparams[i] = READ_LE_UINT16(src + offset);
				offset += 2;
			}
			break;

		case Script_SVariable:
		case Script_SRelative:
			if (extOpcode & 1) {
				opparams[i] = (int8)src[offset++];
			} else {
				opparams[i] = (int16)READ_LE_UINT16(src + offset);
				offset += 2;
			}
			break;

		case Script_None:
		case Script_End:
			break;

		case Script_Invalid:
		default:
			error("opcode %02x: Invalid", extOpcode);
		}
	}

	return offset;
}

void run_vm(EngineState *s, bool restoring) {
	assert(s);

#ifndef DISABLE_VALIDATIONS
	unsigned int code_buf_size = 0 ; // (Avoid spurious warning)
#endif
	int temp;
	int16 aux_acc; // Auxiliary 16 bit accumulator
	reg_t r_temp; // Temporary register
	StackPtr s_temp; // Temporary stack pointer
	int16 opparams[4]; // opcode parameters

	scriptState.restAdjust = s->restAdjust;
	// &rest adjusts the parameter count by this value
	// Current execution data:
	scriptState.xs = &(s->_executionStack.back());
	ExecStack *xs_new = NULL;
	Object *obj = s->_segMan->getObject(scriptState.xs->objp);
	Script *local_script = s->_segMan->getScriptIfLoaded(scriptState.xs->local_segment);
	int old_execution_stack_base = s->execution_stack_base;
	// Used to detect the stack bottom, for "physical" returns
	const byte *code_buf = NULL; // (Avoid spurious warning)

	if (!local_script) {
		error("run_vm(): program counter gone astray (local_script pointer is null)");
	}

	if (!restoring)
		s->execution_stack_base = s->_executionStack.size()-1;

#ifndef DISABLE_VALIDATIONS
	// Initialize maximum variable count
	if (s->script_000->_localsBlock)
		scriptState.variables_max[VAR_GLOBAL] = s->script_000->_localsBlock->_locals.size();
	else
		scriptState.variables_max[VAR_GLOBAL] = 0;
#endif

	scriptState.variables_seg[VAR_GLOBAL] = s->script_000->_localsSegment;
	scriptState.variables_seg[VAR_TEMP] = scriptState.variables_seg[VAR_PARAM] = s->_segMan->findSegmentByType(SEG_TYPE_STACK);
	scriptState.variables_base[VAR_TEMP] = scriptState.variables_base[VAR_PARAM] = s->stack_base;

	// SCI code reads the zeroth argument to determine argc
	if (s->script_000->_localsBlock)
		scriptState.variables_base[VAR_GLOBAL] = scriptState.variables[VAR_GLOBAL] = s->script_000->_localsBlock->_locals.begin();
	else
		scriptState.variables_base[VAR_GLOBAL] = scriptState.variables[VAR_GLOBAL] = NULL;

	s->_executionStackPosChanged = true; // Force initialization

	while (1) {
		int var_type; // See description below
		int var_number;

		g_debugState.old_pc_offset = scriptState.xs->addr.pc.offset;
		g_debugState.old_sp = scriptState.xs->sp;

		if (s->_executionStackPosChanged) {
			Script *scr;
			scriptState.xs = &(s->_executionStack.back());
			s->_executionStackPosChanged = false;

			scr = s->_segMan->getScriptIfLoaded(scriptState.xs->addr.pc.segment);
			if (!scr) {
				// No script? Implicit return via fake instruction buffer
				warning("Running on non-existant script in segment %x", scriptState.xs->addr.pc.segment);
				code_buf = _fake_return_buffer;
#ifndef DISABLE_VALIDATIONS
				code_buf_size = 2;
#endif
				scriptState.xs->addr.pc.offset = 1;

				scr = NULL;
				obj = NULL;
			} else {
				obj = s->_segMan->getObject(scriptState.xs->objp);
				code_buf = scr->_buf;
#ifndef DISABLE_VALIDATIONS
				code_buf_size = scr->_bufSize;
#endif
				local_script = s->_segMan->getScriptIfLoaded(scriptState.xs->local_segment);
				if (!local_script) {
					warning("Could not find local script from segment %x", scriptState.xs->local_segment);
					local_script = NULL;
					scriptState.variables_base[VAR_LOCAL] = scriptState.variables[VAR_LOCAL] = NULL;
#ifndef DISABLE_VALIDATIONS
					scriptState.variables_max[VAR_LOCAL] = 0;
#endif
				} else {

					scriptState.variables_seg[VAR_LOCAL] = local_script->_localsSegment;
					if (local_script->_localsBlock)
						scriptState.variables_base[VAR_LOCAL] = scriptState.variables[VAR_LOCAL] = local_script->_localsBlock->_locals.begin();
					else
						scriptState.variables_base[VAR_LOCAL] = scriptState.variables[VAR_LOCAL] = NULL;
#ifndef DISABLE_VALIDATIONS
					if (local_script->_localsBlock)
						scriptState.variables_max[VAR_LOCAL] = local_script->_localsBlock->_locals.size();
					else
						scriptState.variables_max[VAR_LOCAL] = 0;
					scriptState.variables_max[VAR_TEMP] = scriptState.xs->sp - scriptState.xs->fp;
					scriptState.variables_max[VAR_PARAM] = scriptState.xs->argc + 1;
#endif
				}
				scriptState.variables[VAR_TEMP] = scriptState.xs->fp;
				scriptState.variables[VAR_PARAM] = scriptState.xs->variables_argp;
			}

		}

		if (script_abort_flag || g_engine->shouldQuit())
			return; // Emergency

		// Debug if this has been requested:
		// TODO: re-implement sci_debug_flags
		if (g_debugState.debugging /* sci_debug_flags*/) {
			script_debug(s, breakpointWasHit);
			breakpointWasHit = false;
		}
		Console *con = g_sci->getSciDebugger();
		if (con->isAttached()) {
			con->onFrame();
		}

#ifndef DISABLE_VALIDATIONS
		if (scriptState.xs->sp < scriptState.xs->fp)
			error("run_vm(): stack underflow");

		scriptState.variables_max[VAR_TEMP] = scriptState.xs->sp - scriptState.xs->fp;

		if (scriptState.xs->addr.pc.offset >= code_buf_size)
			error("run_vm(): program counter gone astray");
#endif

		// Get opcode
		byte extOpcode;
		scriptState.xs->addr.pc.offset += readPMachineInstruction(code_buf + scriptState.xs->addr.pc.offset, extOpcode, opparams);
		const byte opcode = extOpcode >> 1;

		switch (opcode) {

		case op_bnot: // 0x00 (00)
			s->r_acc = ACC_ARITHMETIC_L(0xffff ^ /*acc*/);
			break;

		case op_add: // 0x01 (01)
			r_temp = POP32();
			if (r_temp.segment || s->r_acc.segment) {
				reg_t r_ptr = NULL_REG;
				int offset;
				// Pointer arithmetics!
				if (s->r_acc.segment) {
					if (r_temp.segment) {
						error("Attempt to add two pointers, stack=%04x:%04x and acc=%04x:%04x",
						          PRINT_REG(r_temp), PRINT_REG(s->r_acc));
						offset = 0;
					} else {
						r_ptr = s->r_acc;
						offset = r_temp.offset;
					}
				} else {
					r_ptr = r_temp;
					offset = s->r_acc.offset;
				}

				s->r_acc = pointer_add(s, r_ptr, offset);

			} else
				s->r_acc = make_reg(0, r_temp.offset + s->r_acc.offset);
			break;

		case op_sub: // 0x02 (02)
			r_temp = POP32();
			if (r_temp.segment != s->r_acc.segment) {
				reg_t r_ptr = NULL_REG;
				int offset;
				// Pointer arithmetics!
				if (s->r_acc.segment) {
					if (r_temp.segment) {
						error("Attempt to subtract two pointers, stack=%04x:%04x and acc=%04x:%04x",
						          PRINT_REG(r_temp), PRINT_REG(s->r_acc));
						offset = 0;
					} else {
						r_ptr = s->r_acc;
						offset = r_temp.offset;
					}
				} else {
					r_ptr = r_temp;
					offset = s->r_acc.offset;
				}

				s->r_acc = pointer_add(s, r_ptr, -offset);

			} else {
				// We can subtract numbers, or pointers with the same segment,
				// an operation which will yield a number like in C
				s->r_acc = make_reg(0, r_temp.offset - s->r_acc.offset);
			}
			break;

		case op_mul: // 0x03 (03)
			s->r_acc = ACC_ARITHMETIC_L(((int16)POP()) * (int16)/*acc*/);
			break;

		case op_div: // 0x04 (04)
			ACC_AUX_LOAD();
			aux_acc = aux_acc != 0 ? ((int16)POP()) / aux_acc : 0;
			ACC_AUX_STORE();
			break;

		case op_mod: // 0x05 (05)
			ACC_AUX_LOAD();
			aux_acc = aux_acc != 0 ? ((int16)POP()) % aux_acc : 0;
			ACC_AUX_STORE();
			break;

		case op_shr: // 0x06 (06)
			s->r_acc = ACC_ARITHMETIC_L(((uint16)POP()) >> /*acc*/);
			break;

		case op_shl: // 0x07 (07)
			s->r_acc = ACC_ARITHMETIC_L(((uint16)POP()) << /*acc*/);
			break;

		case op_xor: // 0x08 (08)
			s->r_acc = ACC_ARITHMETIC_L(POP() ^ /*acc*/);
			break;

		case op_and: // 0x09 (09)
			s->r_acc = ACC_ARITHMETIC_L(POP() & /*acc*/);
			break;

		case op_or: // 0x0a (10)
			s->r_acc = ACC_ARITHMETIC_L(POP() | /*acc*/);
			break;

		case op_neg: // 0x0b (11)
			s->r_acc = ACC_ARITHMETIC_L(-/*acc*/);
			break;

		case op_not: // 0x0c (12)
			s->r_acc = make_reg(0, !(s->r_acc.offset || s->r_acc.segment));
			// Must allow pointers to be negated, as this is used for checking whether objects exist
			break;

		case op_eq_: // 0x0d (13)
			s->r_prev = s->r_acc;
			r_temp = POP32();
			s->r_acc = make_reg(0, r_temp == s->r_acc);
			// Explicitly allow pointers to be compared
			break;

		case op_ne_: // 0x0e (14)
			s->r_prev = s->r_acc;
			r_temp = POP32();
			s->r_acc = make_reg(0, r_temp != s->r_acc);
			// Explicitly allow pointers to be compared
			break;

		case op_gt_: // 0x0f (15)
			s->r_prev = s->r_acc;
			r_temp = POP32();
			if (r_temp.segment && s->r_acc.segment) {
				// Signed pointer comparison. We do unsigned comparison instead, as that is probably what was intended.
				if (r_temp.segment != s->r_acc.segment)
					warning("[VM] Comparing pointers in different segments (%04x:%04x vs. %04x:%04x)", PRINT_REG(r_temp), PRINT_REG(s->r_acc));
				s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset > s->r_acc.offset);
			} else
				s->r_acc = ACC_ARITHMETIC_L(signed_validate_arithmetic(r_temp) > (int16)/*acc*/);
			break;

		case op_ge_: // 0x10 (16)
			s->r_prev = s->r_acc;
			r_temp = POP32();
			if (r_temp.segment && s->r_acc.segment) {
				if (r_temp.segment != s->r_acc.segment)
					warning("[VM] Comparing pointers in different segments (%04x:%04x vs. %04x:%04x)", PRINT_REG(r_temp), PRINT_REG(s->r_acc));
				s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset >= s->r_acc.offset);
			} else
				s->r_acc = ACC_ARITHMETIC_L(signed_validate_arithmetic(r_temp) >= (int16)/*acc*/);
			break;

		case op_lt_: // 0x11 (17)
			s->r_prev = s->r_acc;
			r_temp = POP32();
			if (r_temp.segment && s->r_acc.segment) {
				if (r_temp.segment != s->r_acc.segment)
					warning("[VM] Comparing pointers in different segments (%04x:%04x vs. %04x:%04x)", PRINT_REG(r_temp), PRINT_REG(s->r_acc));
				s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset < s->r_acc.offset);
			} else
				s->r_acc = ACC_ARITHMETIC_L(signed_validate_arithmetic(r_temp) < (int16)/*acc*/);
			break;

		case op_le_: // 0x12 (18)
			s->r_prev = s->r_acc;
			r_temp = POP32();
			if (r_temp.segment && s->r_acc.segment) {
				if (r_temp.segment != s->r_acc.segment)
					warning("[VM] Comparing pointers in different segments (%04x:%04x vs. %04x:%04x)", PRINT_REG(r_temp), PRINT_REG(s->r_acc));
				s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset <= s->r_acc.offset);
			} else
				s->r_acc = ACC_ARITHMETIC_L(signed_validate_arithmetic(r_temp) <= (int16)/*acc*/);
			break;

		case op_ugt_: // 0x13 (19)
			s->r_prev = s->r_acc;
			r_temp = POP32();

			// SCI0/SCI1 scripts use this to check whether a
			// parameter is a pointer or a far text
			// reference. It is used e.g. by the standard library
			// Print function to distinguish two ways of calling it:
			//
			// (Print "foo") // Pointer to a string
			// (Print 420 5) // Reference to the fifth message in text resource 420

			// It works because in those games, the maximum resource number is 999, 
			// so any parameter value above that threshold must be a pointer. 
			if (r_temp.segment && (s->r_acc == make_reg(0, 1000)))
				s->r_acc = make_reg(0, 1);
			else if (r_temp.segment && s->r_acc.segment)
				s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset > s->r_acc.offset);
			else
				s->r_acc = ACC_ARITHMETIC_L(validate_arithmetic(r_temp) > /*acc*/);
			break;

		case op_uge_: // 0x14 (20)
			s->r_prev = s->r_acc;
			r_temp = POP32();

			// See above
			if (r_temp.segment && (s->r_acc == make_reg(0, 1000)))
				s->r_acc = make_reg(0, 1);
			else if (r_temp.segment && s->r_acc.segment)
				s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset >= s->r_acc.offset);
			else
				s->r_acc = ACC_ARITHMETIC_L(validate_arithmetic(r_temp) >= /*acc*/);
			break;

		case op_ult_: // 0x15 (21)
			s->r_prev = s->r_acc;
			r_temp = POP32();

			// See above
			if (r_temp.segment && (s->r_acc == make_reg(0, 1000)))
				s->r_acc = NULL_REG;
			else if (r_temp.segment && s->r_acc.segment)
				s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset < s->r_acc.offset);
			else
				s->r_acc = ACC_ARITHMETIC_L(validate_arithmetic(r_temp) < /*acc*/);
			break;

		case op_ule_: // 0x16 (22)
			s->r_prev = s->r_acc;
			r_temp = POP32();

			// See above
			if (r_temp.segment && (s->r_acc == make_reg(0, 1000)))
				s->r_acc = NULL_REG;
			else if (r_temp.segment && s->r_acc.segment)
				s->r_acc = make_reg(0, (r_temp.segment == s->r_acc.segment) && r_temp.offset <= s->r_acc.offset);
			else
				s->r_acc = ACC_ARITHMETIC_L(validate_arithmetic(r_temp) <= /*acc*/);
			break;

		case op_bt: // 0x17 (23)
			if (s->r_acc.offset || s->r_acc.segment)
				scriptState.xs->addr.pc.offset += opparams[0];
			break;

		case op_bnt: // 0x18 (24)
			if (!(s->r_acc.offset || s->r_acc.segment))
				scriptState.xs->addr.pc.offset += opparams[0];
			break;

		case op_jmp: // 0x19 (25)
			scriptState.xs->addr.pc.offset += opparams[0];
			break;

		case op_ldi: // 0x1a (26)
			s->r_acc = make_reg(0, opparams[0]);
			break;

		case op_push: // 0x1b (27)
			PUSH32(s->r_acc);
			break;

		case op_pushi: // 0x1c (28)
			PUSH(opparams[0]);
			break;

		case op_toss: // 0x1d (29)
			scriptState.xs->sp--;
			break;

		case op_dup: // 0x1e (30)
			r_temp = scriptState.xs->sp[-1];
			PUSH32(r_temp);
			break;

		case op_link: // 0x1f (31)
			for (int i = 0; i < opparams[0]; i++)
				scriptState.xs->sp[i] = NULL_REG;
			scriptState.xs->sp += opparams[0];
			break;

		case op_call: { // 0x20 (32)
			int argc = (opparams[1] >> 1) // Given as offset, but we need count
			           + 1 + scriptState.restAdjust;
			StackPtr call_base = scriptState.xs->sp - argc;
			scriptState.xs->sp[1].offset += scriptState.restAdjust;

			xs_new = add_exec_stack_entry(s->_executionStack, make_reg(scriptState.xs->addr.pc.segment,
											scriptState.xs->addr.pc.offset + opparams[0]),
											scriptState.xs->sp, scriptState.xs->objp,
											(validate_arithmetic(*call_base)) + scriptState.restAdjust,
											call_base, NULL_SELECTOR, scriptState.xs->objp,
											s->_executionStack.size()-1, scriptState.xs->local_segment);
			scriptState.restAdjust = 0; // Used up the &rest adjustment
			scriptState.xs->sp = call_base;

			s->_executionStackPosChanged = true;
			break;
		}

		case op_callk: { // 0x21 (33)
			gc_countdown(s);

			scriptState.xs->sp -= (opparams[1] >> 1) + 1;

			bool oldScriptHeader = (getSciVersion() == SCI_VERSION_0_EARLY);
			if (!oldScriptHeader) {
				scriptState.xs->sp -= scriptState.restAdjust;
				s->restAdjust = 0; // We just used up the scriptState.restAdjust, remember?
			}

			int argc = validate_arithmetic(scriptState.xs->sp[0]);

			if (!oldScriptHeader)
				argc += scriptState.restAdjust;

			callKernelFunc(s, opparams[0], argc);

			if (!oldScriptHeader)
				scriptState.restAdjust = s->restAdjust;

			// Calculate xs again: The kernel function might
			// have spawned a new VM

			xs_new = &(s->_executionStack.back());
			s->_executionStackPosChanged = true;
			break;
		}

		case op_callb: // 0x22 (34)
			temp = ((opparams[1] >> 1) + scriptState.restAdjust + 1);
			s_temp = scriptState.xs->sp;
			scriptState.xs->sp -= temp;

			scriptState.xs->sp[0].offset += scriptState.restAdjust;
			xs_new = execute_method(s, 0, opparams[0], s_temp, scriptState.xs->objp,
									scriptState.xs->sp[0].offset, scriptState.xs->sp);
			scriptState.restAdjust = 0; // Used up the &rest adjustment
			if (xs_new)    // in case of error, keep old stack
				s->_executionStackPosChanged = true;
			break;

		case op_calle: // 0x23 (35)
			temp = ((opparams[2] >> 1) + scriptState.restAdjust + 1);
			s_temp = scriptState.xs->sp;
			scriptState.xs->sp -= temp;

			scriptState.xs->sp[0].offset += scriptState.restAdjust;
			xs_new = execute_method(s, opparams[0], opparams[1], s_temp, scriptState.xs->objp,
									scriptState.xs->sp[0].offset, scriptState.xs->sp);
			scriptState.restAdjust = 0; // Used up the &rest adjustment

			if (xs_new)  // in case of error, keep old stack
				s->_executionStackPosChanged = true;
			break;

		case op_ret: // 0x24 (36)
			do {
				StackPtr old_sp2 = scriptState.xs->sp;
				StackPtr old_fp = scriptState.xs->fp;
				ExecStack *old_xs = &(s->_executionStack.back());

				if ((int)s->_executionStack.size()-1 == s->execution_stack_base) { // Have we reached the base?
					s->execution_stack_base = old_execution_stack_base; // Restore stack base

					s->_executionStack.pop_back();

					s->_executionStackPosChanged = true;
					s->restAdjust = scriptState.restAdjust; // Update &rest
					return; // "Hard" return
				}

				if (old_xs->type == EXEC_STACK_TYPE_VARSELECTOR) {
					// varselector access?
					reg_t *var = old_xs->getVarPointer(s->_segMan);
					if (old_xs->argc) // write?
						*var = old_xs->variables_argp[1];
					else // No, read
						s->r_acc = *var;
				}

				// Not reached the base, so let's do a soft return
				s->_executionStack.pop_back();
				s->_executionStackPosChanged = true;
				scriptState.xs = &(s->_executionStack.back());

				if (scriptState.xs->sp == CALL_SP_CARRY // Used in sends to 'carry' the stack pointer
				        || scriptState.xs->type != EXEC_STACK_TYPE_CALL) {
					scriptState.xs->sp = old_sp2;
					scriptState.xs->fp = old_fp;
				}

			} while (scriptState.xs->type == EXEC_STACK_TYPE_VARSELECTOR);
			// Iterate over all varselector accesses
			s->_executionStackPosChanged = true;
			xs_new = scriptState.xs;

			break;

		case op_send: // 0x25 (37)
			s_temp = scriptState.xs->sp;
			scriptState.xs->sp -= ((opparams[0] >> 1) + scriptState.restAdjust); // Adjust stack

			scriptState.xs->sp[1].offset += scriptState.restAdjust;
			xs_new = send_selector(s, s->r_acc, s->r_acc, s_temp,
									(int)(opparams[0] >> 1) + (uint16)scriptState.restAdjust, scriptState.xs->sp);

			if (xs_new && xs_new != scriptState.xs)
				s->_executionStackPosChanged = true;

			scriptState.restAdjust = 0;

			break;

		case 0x26: // (38)
		case 0x27: // (39)
			error("Dummy opcode 0x%x called", opcode);	// should never happen
			break;

		case op_class: // 0x28 (40)
			s->r_acc = s->_segMan->getClassAddress((unsigned)opparams[0], SCRIPT_GET_LOCK,
											scriptState.xs->addr.pc);
			break;

		case 0x29: // (41)
			error("Dummy opcode 0x%x called", opcode);	// should never happen
			break;

		case op_self: // 0x2a (42)
			s_temp = scriptState.xs->sp;
			scriptState.xs->sp -= ((opparams[0] >> 1) + scriptState.restAdjust); // Adjust stack

			scriptState.xs->sp[1].offset += scriptState.restAdjust;
			xs_new = send_selector(s, scriptState.xs->objp, scriptState.xs->objp,
									s_temp, (int)(opparams[0] >> 1) + (uint16)scriptState.restAdjust,
									scriptState.xs->sp);

			if (xs_new && xs_new != scriptState.xs)
				s->_executionStackPosChanged = true;

			scriptState.restAdjust = 0;
			break;

		case op_super: // 0x2b (43)
			r_temp = s->_segMan->getClassAddress(opparams[0], SCRIPT_GET_LOAD, scriptState.xs->addr.pc);

			if (!r_temp.segment)
				error("[VM]: Invalid superclass in object");
			else {
				s_temp = scriptState.xs->sp;
				scriptState.xs->sp -= ((opparams[1] >> 1) + scriptState.restAdjust); // Adjust stack

				scriptState.xs->sp[1].offset += scriptState.restAdjust;
				xs_new = send_selector(s, r_temp, scriptState.xs->objp, s_temp,
										(int)(opparams[1] >> 1) + (uint16)scriptState.restAdjust,
										scriptState.xs->sp);

				if (xs_new && xs_new != scriptState.xs)
					s->_executionStackPosChanged = true;

				scriptState.restAdjust = 0;
			}

			break;

		case op_rest: // 0x2c (44)
			temp = (uint16) opparams[0]; // First argument
			scriptState.restAdjust = MAX<int16>(scriptState.xs->argc - temp + 1, 0); // +1 because temp counts the paramcount while argc doesn't

			for (; temp <= scriptState.xs->argc; temp++)
				PUSH32(scriptState.xs->variables_argp[temp]);

			break;

		case op_lea: // 0x2d (45)
			temp = (uint16) opparams[0] >> 1;
			var_number = temp & 0x03; // Get variable type

			// Get variable block offset
			r_temp.segment = scriptState.variables_seg[var_number];
			r_temp.offset = scriptState.variables[var_number] - scriptState.variables_base[var_number];

			if (temp & 0x08)  // Add accumulator offset if requested
				r_temp.offset += signed_validate_arithmetic(s->r_acc);

			r_temp.offset += opparams[1];  // Add index
			r_temp.offset *= 2; // variables are 16 bit
			// That's the immediate address now
			s->r_acc = r_temp;
			break;


		case op_selfID: // 0x2e (46)
			s->r_acc = scriptState.xs->objp;
			break;

		case 0x2f: // (47)
			error("Dummy opcode 0x%x called", opcode);	// should never happen
			break;

		case op_pprev: // 0x30 (48)
			PUSH32(s->r_prev);
			break;

		case op_pToa: // 0x31 (49)
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1));
			break;

		case op_aTop: // 0x32 (50)
			OBJ_PROPERTY(obj, (opparams[0] >> 1)) = s->r_acc;
			break;

		case op_pTos: // 0x33 (51)
			PUSH32(OBJ_PROPERTY(obj, opparams[0] >> 1));
			break;

		case op_sTop: // 0x34 (52)
			OBJ_PROPERTY(obj, (opparams[0] >> 1)) = POP32();
			break;

		case op_ipToa: // 0x35 (53)
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1));
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1)) = ACC_ARITHMETIC_L(1 + /*acc*/);
			break;

		case op_dpToa: { // 0x36 (54)
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1));
#if 0
			// Speed throttling is possible here as well
			// although this opens other issues like mud wrestling in lsl5 uses another local variable for delays
			Object *var_container = obj;
			if (!(obj->getInfoSelector().offset & SCRIPT_INFO_CLASS))
				var_container = s->_segMan->getObject(obj->getSuperClassSelector());
			uint16 varSelector = var_container->getVarSelector(opparams[0] >> 1);
//			printf("%X\n", varSelector);
//			printf("%s\n", g_sci->getKernel()->getSelectorName(varSelector).c_str());
			if ((varSelector == 0x84) || (varSelector == 0x92))) {
				// selectors cycles, cycleCnt from lsl5 hardcoded
				uint32 curTime = g_system->getMillis();
				if (s->_lastAnimateTime + 30 > curTime)
					break;
				s->_lastAnimateTime = curTime;
			}
#endif
			s->r_acc = OBJ_PROPERTY(obj, (opparams[0] >> 1)) = ACC_ARITHMETIC_L(-1 + /*acc*/);
			break;
		}

		case op_ipTos: // 0x37 (55)
			validate_arithmetic(OBJ_PROPERTY(obj, (opparams[0] >> 1)));
			temp = ++OBJ_PROPERTY(obj, (opparams[0] >> 1)).offset;
			PUSH(temp);
			break;

		case op_dpTos: // 0x38 (56)
			validate_arithmetic(OBJ_PROPERTY(obj, (opparams[0] >> 1)));
			temp = --OBJ_PROPERTY(obj, (opparams[0] >> 1)).offset;
			PUSH(temp);
			break;

		case op_lofsa: // 0x39 (57)
			s->r_acc.segment = scriptState.xs->addr.pc.segment;

			switch (g_sci->_features->detectLofsType()) {
			case SCI_VERSION_1_1:
				s->r_acc.offset = opparams[0] + local_script->_scriptSize;
				break;
			case SCI_VERSION_1_MIDDLE:
				s->r_acc.offset = opparams[0];
				break;
			default:
				s->r_acc.offset = scriptState.xs->addr.pc.offset + opparams[0];
			}

#ifndef DISABLE_VALIDATIONS
			if (s->r_acc.offset >= code_buf_size) {
				error("VM: lofsa operation overflowed: %04x:%04x beyond end"
				          " of script (at %04x)\n", PRINT_REG(s->r_acc), code_buf_size);
			}
#endif
			break;

		case op_lofss: // 0x3a (58)
			r_temp.segment = scriptState.xs->addr.pc.segment;

			switch (g_sci->_features->detectLofsType()) {
			case SCI_VERSION_1_1:
				r_temp.offset = opparams[0] + local_script->_scriptSize;
				break;
			case SCI_VERSION_1_MIDDLE:
				r_temp.offset = opparams[0];
				break;
			default:
				r_temp.offset = scriptState.xs->addr.pc.offset + opparams[0];
			}

#ifndef DISABLE_VALIDATIONS
			if (r_temp.offset >= code_buf_size) {
				error("VM: lofss operation overflowed: %04x:%04x beyond end"
				          " of script (at %04x)", PRINT_REG(r_temp), code_buf_size);
			}
#endif
			PUSH32(r_temp);
			break;

		case op_push0: // 0x3b (59)
			PUSH(0);
			break;

		case op_push1: // 0x3c (60)
			PUSH(1);
			break;

		case op_push2: // 0x3d (61)
			PUSH(2);
			break;

		case op_pushSelf: // 0x3e (62)
			if (!(extOpcode & 1)) {
				PUSH32(scriptState.xs->objp);
			} else {
				// Debug opcode op_file, skip null-terminated string (file name)
				while (code_buf[scriptState.xs->addr.pc.offset++]) ;
			}
			break;

		case op_line: // 0x3f (63)
			// Debug opcode (line number)
			break;

		case op_lag: // 0x40 (64)
		case op_lal: // 0x41 (65)
		case op_lat: // 0x42 (66)
		case op_lap: // 0x43 (67)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			s->r_acc = READ_VAR(var_type, var_number, s->r_acc);
			break;

		case op_lsg: // 0x44 (68)
		case op_lsl: // 0x45 (69)
		case op_lst: // 0x46 (70)
		case op_lsp: // 0x47 (71)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			PUSH32(READ_VAR(var_type, var_number, s->r_acc));
			break;

		case op_lagi: // 0x48 (72)
		case op_lali: // 0x49 (73)
		case op_lati: // 0x4a (74)
		case op_lapi: // 0x4b (75)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			s->r_acc = READ_VAR(var_type, var_number, s->r_acc);
			break;

		case op_lsgi: // 0x4c (76)
		case op_lsli: // 0x4d (77)
		case op_lsti: // 0x4e (78)
		case op_lspi: // 0x4f (79)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			PUSH32(READ_VAR(var_type, var_number, s->r_acc));
			break;

		case op_sag: // 0x50 (80)
		case op_sal: // 0x51 (81)
		case op_sat: // 0x52 (82)
		case op_sap: // 0x53 (83)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case op_ssg: // 0x54 (84)
		case op_ssl: // 0x55 (85)
		case op_sst: // 0x56 (86)
		case op_ssp: // 0x57 (87)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			WRITE_VAR(var_type, var_number, POP32());
			break;

		case op_sagi: // 0x58 (88)
		case op_sali: // 0x59 (89)
		case op_sati: // 0x5a (90)
		case op_sapi: // 0x5b (91)
			// Special semantics because it wouldn't really make a whole lot
			// of sense otherwise, with acc being used for two things
			// simultaneously...
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			s->r_acc = POP32();
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case op_ssgi: // 0x5c (92)
		case op_ssli: // 0x5d (93)
		case op_ssti: // 0x5e (94)
		case op_sspi: // 0x5f (95)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			WRITE_VAR(var_type, var_number, POP32());
			break;

		case op_plusag: // 0x60 (96)
		case op_plusal: // 0x61 (97)
		case op_plusat: // 0x62 (98)
		case op_plusap: // 0x63 (99)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			r_temp = READ_VAR(var_type, var_number, s->r_acc);
			if (r_temp.segment) {
				// Pointer arithmetics!
				s->r_acc = pointer_add(s, r_temp, 1);
			} else
				s->r_acc = make_reg(0, r_temp.offset + 1);
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case op_plussg: // 0x64 (100)
		case op_plussl: // 0x65 (101)
		case op_plusst: // 0x66 (102)
		case op_plussp: // 0x67 (103)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			r_temp = READ_VAR(var_type, var_number, s->r_acc);
			if (r_temp.segment) {
				// Pointer arithmetics!
				r_temp = pointer_add(s, r_temp, 1);
			} else
				r_temp = make_reg(0, r_temp.offset + 1);
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case op_plusagi: // 0x68 (104)
		case op_plusali: // 0x69 (105)
		case op_plusati: // 0x6a (106)
		case op_plusapi: // 0x6b (107)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			r_temp = READ_VAR(var_type, var_number, s->r_acc);
			if (r_temp.segment) {
				// Pointer arithmetics!
				s->r_acc = pointer_add(s, r_temp, 1);
			} else
				s->r_acc = make_reg(0, r_temp.offset + 1);
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case op_plussgi: // 0x6c (108)
		case op_plussli: // 0x6d (109)
		case op_plussti: // 0x6e (110)
		case op_plusspi: // 0x6f (111)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			r_temp = READ_VAR(var_type, var_number, s->r_acc);
			if (r_temp.segment) {
				// Pointer arithmetics!
				r_temp = pointer_add(s, r_temp, 1);
			} else
				r_temp = make_reg(0, r_temp.offset + 1);
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case op_minusag: // 0x70 (112)
		case op_minusal: // 0x71 (113)
		case op_minusat: // 0x72 (114)
		case op_minusap: // 0x73 (115)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			r_temp = READ_VAR(var_type, var_number, s->r_acc);
			if (r_temp.segment) {
				// Pointer arithmetics!
				s->r_acc = pointer_add(s, r_temp, -1);
			} else
				s->r_acc = make_reg(0, r_temp.offset - 1);
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case op_minussg: // 0x74 (116)
		case op_minussl: // 0x75 (117)
		case op_minusst: // 0x76 (118)
		case op_minussp: // 0x77 (119)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0];
			r_temp = READ_VAR(var_type, var_number, s->r_acc);
			if (r_temp.segment) {
				// Pointer arithmetics!
				r_temp = pointer_add(s, r_temp, -1);
			} else
				r_temp = make_reg(0, r_temp.offset - 1);
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		case op_minusagi: // 0x78 (120)
		case op_minusali: // 0x79 (121)
		case op_minusati: // 0x7a (122)
		case op_minusapi: // 0x7b (123)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			r_temp = READ_VAR(var_type, var_number, s->r_acc);
			if (r_temp.segment) {
				// Pointer arithmetics!
				s->r_acc = pointer_add(s, r_temp, -1);
			} else
				s->r_acc = make_reg(0, r_temp.offset - 1);
			WRITE_VAR(var_type, var_number, s->r_acc);
			break;

		case op_minussgi: // 0x7c (124)
		case op_minussli: // 0x7d (125)
		case op_minussti: // 0x7e (126)
		case op_minusspi: // 0x7f (127)
			var_type = opcode & 0x3; // Gets the variable type: g, l, t or p
			var_number = opparams[0] + signed_validate_arithmetic(s->r_acc);
			r_temp = READ_VAR(var_type, var_number, s->r_acc);
			if (r_temp.segment) {
				// Pointer arithmetics!
				r_temp = pointer_add(s, r_temp, -1);
			} else
				r_temp = make_reg(0, r_temp.offset - 1);
			PUSH32(r_temp);
			WRITE_VAR(var_type, var_number, r_temp);
			break;

		default:
			error("run_vm(): illegal opcode %x", opcode);

		} // switch (opcode)

		if (s->_executionStackPosChanged) // Force initialization
			scriptState.xs = xs_new;

//#ifndef DISABLE_VALIDATIONS
		if (scriptState.xs != &(s->_executionStack.back())) {
			warning("xs is stale (%p vs %p); last command was %02x",
					(void *)scriptState.xs, (void *)&(s->_executionStack.back()),
					opcode);
		}
//#endif
		++script_step_counter;
	}
}

static void _init_stack_base_with_selector(EngineState *s, Selector selector) {
	s->stack_base[0] = make_reg(0, (uint16)selector);
	s->stack_base[1] = NULL_REG;
}

static EngineState *_game_run(EngineState *&s) {
	EngineState *successor = NULL;
	int game_is_finished = 0;

	if (Common::isDebugChannelEnabled(kDebugLevelOnStartup))
		g_sci->getSciDebugger()->attach();

	do {
		s->_executionStackPosChanged = false;
		run_vm(s, successor ? true : false);
		if (s->restarting_flags & SCI_GAME_IS_RESTARTING_NOW) { // Restart was requested?
			successor = NULL;
			s->_executionStack.clear();
			s->_executionStackPosChanged = false;

			game_exit(s);
			script_init_engine(s);
			game_init(s);
#ifdef USE_OLD_MUSIC_FUNCTIONS
			s->_sound.sfx_reset_player();
#endif
			_init_stack_base_with_selector(s, g_sci->getKernel()->_selectorCache.play);

			send_selector(s, s->_gameObj, s->_gameObj, s->stack_base, 2, s->stack_base);

			script_abort_flag = 0;
			s->restarting_flags = SCI_GAME_WAS_RESTARTED | SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE;

		} else {
			successor = s->successor;
			if (successor) {
				game_exit(s);
				delete s;
				s = successor;

				if (script_abort_flag == 2) {
					debugC(2, kDebugLevelVM, "Restarting with replay()");
					s->_executionStack.clear(); // Restart with replay

					_init_stack_base_with_selector(s, g_sci->getKernel()->_selectorCache.replay);

					send_selector(s, s->_gameObj, s->_gameObj, s->stack_base, 2, s->stack_base);
				}

				script_abort_flag = 0;

			} else
				game_is_finished = 1;
		}
	} while (!game_is_finished);

	return s;
}

int game_run(EngineState **_s) {
	EngineState *s = *_s;

	debugC(2, kDebugLevelVM, "Calling %s::play()", s->_gameId.c_str());
	_init_stack_base_with_selector(s, g_sci->getKernel()->_selectorCache.play); // Call the play selector

	// Now: Register the first element on the execution stack-
	if (!send_selector(s, s->_gameObj, s->_gameObj, s->stack_base, 2, s->stack_base)) {
		Console *con = g_sci->getSciDebugger();
		con->printObject(s->_gameObj);
		warning("Failed to run the game! Aborting...");
		return 1;
	}
	// and ENGAGE!
	_game_run(*_s);

	debugC(2, kDebugLevelVM, "Game::play() finished.");

	return 0;
}

void quit_vm() {
	script_abort_flag = 1; // Terminate VM
	g_debugState.seeking = kDebugSeekNothing;
	g_debugState.runningStep = 0;
}

void shrink_execution_stack(EngineState *s, uint size) {
	assert(s->_executionStack.size() >= size);
	Common::List<ExecStack>::iterator iter;
	iter = s->_executionStack.begin();
	for (uint i = 0; i < size; ++i)
		++iter;
	s->_executionStack.erase(iter, s->_executionStack.end());
}

reg_t* ObjVarRef::getPointer(SegManager *segMan) const {
	Object *o = segMan->getObject(obj);
	if (!o) return 0;
	return &(o->_variables[varindex]);
}

reg_t* ExecStack::getVarPointer(SegManager *segMan) const {
	assert(type == EXEC_STACK_TYPE_VARSELECTOR);
	return addr.varp.getPointer(segMan);
}

} // End of namespace Sci
