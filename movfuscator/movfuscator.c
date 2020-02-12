/*
This software is licensed under a BSD Style License

Copyright (c) 2015 Battelle Memorial Institute.  All Rights Reserved.
http://www.battelle.org/

Redistribution and use of this software and associated documentation
("Software"), with or without modification, are permitted provided that the
following conditions are met:

1.   Redistributions of source code must retain copyright statements and
     notices. Redistributions must also contain a copy of this document.

2.   Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

3.   The names "M/o/Vfuscator" and "Battelle" must not be used to endorse or
     promote products derived from this Software without prior written
     permission of Battelle Memorial Institute. For written permission,
     please contact solutions@battelle.org

4.   Products derived from this Software may not be called "M/o/Vfuscator" or
     "Battelle", nor may "M/o/Vfuscator" or "Battelle" appear in their names
     without prior written permission of Battelle Memorial Institute.
     Battelle is a registered trademark of Battelle Memorial Institute.

5.   Due credit should be given to the Battelle Memorial Institute.

THIS SOFTWARE IS PROVIDED BY BATTELLE MEMORIAL INSTITUTE "AS IS" AND ANY
EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL BATTELLE MEMORIAL INSTITUTE OR ITS
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The author of this software is Christopher P. Domas, an employee of the 
Battelle Memorial Institute.
*/

/*
       ___     ___            ___    ___     ___     ___     ___          ___     ___      
      /\  \   /\  \    ___   /\__\  /\  \   /\__\   /\__\   /\  \        /\  \   /\  \    .
     |::\  \ /::\  \  /\  \ /:/ _/_ \:\  \ /:/ _/_ /:/  /  /::\  \  ___ /::\  \ /::\  \   .
     |:::\  \:/\:\  \ \:\  \:/ /\__\ \:\  \:/ /\  \:/  /  /:/\:\  \/\__\:/\:\  \:/\:\__\  .
   __|:|\:\  \  \:\  \ \:\  \ /:/  /  \:\  \ /::\  \  /  _:/ /::\  \/  //  \:\  \ /:/  /   
  /::::|_\:\__\/ \:\__\ \:\__\:/  / \  \:\__\:/\:\__\/  /\__\:/\:\__\_//__/ \:\__\:/__/___ 
  \:\~~\  \/__/\ /:/  / |:|  |/  /\  \ /:/  // /:/  /\ /:/  //  \/__/ \\  \ /:/  /::::/  / 
   \:\  \  \:\  /:/  / \|:|  |__/\:\  /:/  // /:/  /  /:/  //__/:/\:\  \\  /:/  //~~/~~~~  
    \:\  \  \:\/:/  /\__|:|__|  \ \:\/:/  //_/:/  /:\/:/  /:\  \/__\:\  \\/:/  /:\~~\     .
     \:\__\  \::/  /\::::/__/:\__\ \::/  /  /:/  / \::/  / \:\__\   \:\__\:/  / \:\__\    .
      \/__/   \/__/  ~~~~    \/__/  \/__/   \/__/   \/__/   \/__/    \/__/ __/   \/__/    2
                                                                                           

 M/o/Vfuscator2

 github.com/xoreaxeaxeax/movfuscator :: the single instruction C compiler
 chris domas           @xoreaxeaxeax

*/

/*
 * DESIGN
 *
 * - movfuscator2 is a crudely hacked together abomination, built from scavenged
 *   pieces of movfuscator1, the alu pocs, and whatever else i could find lying
 *   around. i'm working on limited sleep, unlimited coffee, and no knowledge of
 *   compiler design. take everything with a grain of salt, this isn't meant to
 *   be serious.
 *
 * - 1 byte char, 2 b short, 4 b int/long/long long/float, 8 b double/long double
 * - eax, ecx, edx, ebx will be used for shuffling memory
 *   - ebx is only ever used as a 4 byte register; this allows it to be easily
 *     swapped out for another register in optional post-compilation obfuscation
 *     steps.
 * - soft-registers R0-RN, F0-FN, D0-DN available to compiler
 * - 'movdecl': 
 *   - R0/F0/D0 hold return value
 *   - R0,F0,D0 destroyed
 *   - other soft-registers preserved, hard-registers destroyed
 *   - parameters on stack, right to left
 *   - caller cleanup
 */

/*
 * TASK LIST
 *
 * high priority:
 *
 * - done!
 *
 * medium priority:
 *
 * - get rid of plt
 * - finish movfuscation of external calls:
 *   - remove single jmp used for external calls
 *   - idea i've been playing with:
 *     - set sigaction as the exception handler for a segfault.  trigger a
 *       segfault to register new handlers for other functions
 *       - i just can't get the stack to work out for this, no matter what i try
 *     - would be so easy in ring 0...
 *   - current implementation: sigaction setup to trampoline to a common
 *     dispatch area - dispatch has a single jmp, which we self modify
 *   - here's how it maybe 'can' be done...
 *     - use sigaction to set signal X's handler as Y in libc
 *       - use sa_onstack flag
 *       - Y is in the middle of a function, s.t. just something like 'leave; ret'
 *         is executed
 *     - mov desired branch address Z onto alternate stack
 *     - trigger signal X
 *     - select Y s.t. removes enough from stack (~0x80 bytes) that it will
 *       return to Z
 *       - to allow passing unlimited parameters, Y needs to remove ~0x300 bytes
 *         from stack
 *
 * low priority:
 *
 * - 64 bit long long
 * - 64 bit compiler
 * - just switched from hard-regs to soft-regs for the allocator:
 *   - clean up vestigial instructions
 *   - rework rules to take advantage of new regs
 *   - re-examine old assumptions about temps, re-use, etc
 *   - add set of soft-regs for char, short
 *   - optimize register count, 8 vs 2 is a 20% bloat in softfloat
 * - fix kludginess in float64 calls - float64 is implemented as a struct; lcc
 *   expects the struct address as a first hidden parameter when returning
 *   structs; should fix this when switch to bits64 library, and/or set
 *   wants_dag to 1 and fix up calling conventions
 * - setup wants_callb to 1 so can call extern fns returning structs
 * - tweak calling convention, adjust clobber list
 * - better way to resolve internal vs. external functions?  namely, make it so
 *   that execution doesn't hang when someone tries to use a function before
 *   declaring it.
 * - could probably get a 2x speed boost with more robust rules to take
 *   advantage of x86 addressing modes, wouldn't require any actual code changes
 * - div by 0 exception (pretty easy, just a standard halt/nohalt thing) - note
 *     that halt (side effects in general) should only occur if execution is on
 * - better comment output - include parameters to each function
 * - import exit
 * - if you delete the 1 at the end, the prime demo won't build
 * 		addrj: reg                     "*%0" 1
 * 		should just need to rework some of your rules
 * - implement blkfetch, blkstore, rewrite asgn+b, arg+b
 * - recompile libc stuff to better support the M/o/Vfuscator - float return
 *   values in gprs, 32 bit bit long long, etc
 * - clean up code a bit by using stringf function
 * - standardize use of emit_kid vs getting register name directly
 * - load and store jmp regs could be made much more efficient for high numbers
 *   of soft regs (block xfers etc)
 *
 * very low priority:
 *
 * - tap into the register allocator to see who is in use on each jump?
 * - since all the rules use simple regs for kids, much of the emit_kid
 *   complexity (and redundancy) could be removed
 * - ida won't disassemble the "mov cs, ax" instruction, which prevents it from
 *   creating the amusing straight line cfg (compile with --no-mov-loop to fix)
 *   find another way to fault.
 * - switch to simpler addressing to allow better adaptation to risc
 *   architectures
 * - see if you can reduce the size of the shift tables, they take up most of
 *   the space
 * - mov replacements, like xor, push/pop, etc
 * - simd instructions
 * - second pass obfuscator - register renaming, instruction shuffling, etc
 * - fast push/pop tables for 16/etc byte adjustments
 * - compiler flags for smaller or larger lookup tables, dynamically generated
 *   tables
 * - finish indirect calls to external functions
 *   - there's likely just no way to get this to work the way i want it:
 *     - internal vs. external calls have two different calling mechanisms (have
 *     to - different calling conventions etc) - so need to be able to resolve
 *     whether a call is internal or external at runtime ... which is
 *     ... challenging with only mov's. 
 */

/*
 * DEBUG NOTES
 * - turn on compiler debug messages with -Wf-d
 * - enable source debugging with -g
 * - turn on id flags with -Wf--mov-id
 */

#include "c.h"
#include <stdarg.h>
#include <signal.h>

/* M/o/Vfuscation options */
static int quiet=           0;
#define QUIET_FLAG          "--q"

static int mov_id=          0;
#define MOV_ID_FLAG         "--mov-id"
#define NO_MOV_ID_FLAG      "--no-mov-id"

static int mov_flow=        1;
#define MOV_FLOW_FLAG       "--mov-flow"
#define NO_MOV_FLOW_FLAG    "--no-mov-flow"

static int mov_extern=      1;
#define MOV_EXTERN_FLAG     "--mov-extern"
#define NO_MOV_EXTERN_FLAG  "--no-mov-extern"

static int mov_loop=        1;
#define MOV_LOOP_FLAG       "--mov-loop"
#define NO_MOV_LOOP_FLAG    "--no-mov-loop"

static int crt0=            0;
#define CRT0_FLAG           "--crt0"

static int crtf=            0;
#define CRTF_FLAG           "--crtf"

static int crtd=            0;
#define CRTD_FLAG           "--crtd"

#define CRT_FLAG            "--crt"

#define MOV_OFFSET      0x80000000
#define STACK_SIZE      0x200000
#define DISCARD_SIZE    0x200000   /* constrains max size struct/array */
#define DEBUG_ID        0xcc000000

#define I_LABEL         "LCI"      /* label used by internal (movfuscated) code */
#define E_LABEL         "LCE"      /* label used by external (non-movfuscated code) */
#define S_LABEL         "LCS"      /* label used by symbol */

#define REQ             "REQ"      /* indicate to post-processors that the register targets are fixed */

#define MARKER(s)       " # <" s ">"


/* M/o/Vfuscation helpers */
#define STR(x) #x
#define CNST(x) "$" STR(x)
#define ABS(x) ((x)>0?(x):-(x))

/* macros needed during build */
#define NODEPTR_TYPE Node
#define OP_LABEL(p) ((p)->op)
#define LEFT_CHILD(p) ((p)->kids[0])
#define RIGHT_CHILD(p) ((p)->kids[1])
#define STATE_LABEL(p) ((p)->x.state)

/* register allocator can use psuedo-registers */
/* hard-registers available for internal use */
/* can tweak number of soft-regs for optimal performance */
/* a minimum of 3 registers are necessary to accomodate lcc register spillage */
#define SOFT_I_REGS 4
#define SOFT_F_REGS 3
#define SOFT_D_REGS 3
/* 0-7 immediately accessible */
enum { R0=0, R1, R2, R3, R4, R5, R6, R7 };
enum { F0=0, F1, F2, F3, F4, F5, F6, F7 };
enum { D0=SOFT_F_REGS, D1, D2, D3, D4, D5, D6, D7 };

/* optimization */
#define STACK_EX_THRESH (128*4)

/* frontend regs */
static Symbol reg_char[32], reg_short[32], reg_int[32], reg_float[32], reg_double[32];
static Symbol reg_char_wc, reg_short_wc, reg_int_wc, reg_float_wc, reg_double_wc;

/* backend data */
typedef enum { MOV_NONE, MOV_CODE, MOV_BSS, MOV_DATA, MOV_PLT } segment_t;
static Symbol last_global;
static segment_t current_segment=MOV_NONE;

/* backend functions */
static void address(Symbol, Symbol, long);
static void blkfetch(int, int, int, int);
static void blkloop(int, int, int, int, int, int[]);
static void blkstore(int, int, int, int);
static void defaddress(Symbol);
static void defconst(int, int, Value);
static void defstring(int, char *);
static void defsymbol(Symbol);
static void doarg(Node);
static void emit2(Node);
static void emit2_nt(Node, int);
static void export(Symbol);
static void clobber(Node);
static void function(Symbol, Symbol [], Symbol [], int);
static void global(Symbol);
static void import(Symbol);
static void local(Symbol);
static void progbeg(int, char **);
static void progend(void);
static void segment(int);
static void space(int);
static void target(Node);
static int ckstack(Node, int);

/* m/o/vfuscator functions */
static void emit_header(FILE*, int);
static void debug_id(void);
static void data_setup(void);
static void execution_on(char*);
static void execution_off(char*);
static void store_target(char*, char*);
static void store_data(char*, char*, char*);
static void store_jmp_regs(char*);
static void load_data(char*, char*, char*);
static void load_jmp_regs(char*);
static void select_data(char*, char*);
static void push(char*);
static void pushm8(char*);
static void popr(char*);
static void popm(char*);
static void popm8(char*);
static void alu_add8(char*, char*, char*, char*, int);
static void alu_add8_fast(char*, char*, char*, char*, int);
static void alu_add16_fast(char*, char*, char*, char*, int);
static void alu_add32(char*, char*, char*, char*);
static void alu_add(char*, char*, char*);
static void alu_sub16_fast(char*, char*, char*, char*, int);
static void alu_sub32(char*, char*, char*, char*);
static void alu_inv8(char*, int);
static void alu_sub(char*, char*, char*);
static void alu_band8(char*, char*, char*, int);
static void alu_band32(char*, char*, char*);
static void alu_band(char*, char*, char*);
static void alu_bor8(char*, char*, char*, int);
static void alu_bor32(char*, char*, char*);
static void alu_bor(char*, char*, char*);
static void alu_bxor8(char*, char*, char*, int);
static void alu_bxor32(char*, char*, char*);
static void alu_bxor(char*, char*, char*);
static void alu_bcom(char*, char*);
static void alu_neg(char*, char*);
static void alu_lshu(char*, char*, char*);
static void alu_lshu32(char*, char*, char*);
static void alu_rshu(char*, char*, char*);
static void alu_rshu32(char*, char*, char*);
static void alu_rshi(char*, char*, char*);
static void alu_rshi32(char*, char*, char*);
static void alu_clamp32(char*, char*);
static void alu_mul(char*, char*, char*);
static void alu_mul8(char*, int, char*, int, char*, int, char*);
static void alu_mul32(char*, char*, char*, char*);
static void alu_add8n(char*, int, char*, int, char*, int, char*, int, ...);
static void alu_idiv(char*, char*, char*);
static void alu_udiv(char*, char*, char*);
static void alu_divrem(char*, char*);
static void alu_div_setb32(char*, int);
static void alu_div_shl1_8_c(char*, int, char*);
static void alu_div_shl1_32_c(char*, char*);
static void alu_bit(char*, char*, int);
static void alu_div_gte32(char*, char*, char*, char*);
static void alu_imod(char*, char*, char*);
static void alu_umod(char*, char*, char*);
static void alu_cmp(char*, char*);
static void alu_eq(char*, char*, char*);
static void alu_bool(char*, char*, char*, char*);
static void alu_not(char*, char*);
static void jmp_eqi(char*, char*, char*);
static void jmp_equ(char*, char*, char*);
static void jmp_gei(char*, char*, char*);
static void jmp_geu(char*, char*, char*);
static void jmp_gti(char*, char*, char*);
static void jmp_gtu(char*, char*, char*);
static void jmp_lei(char*, char*, char*);
static void jmp_leu(char*, char*, char*);
static void jmp_lti(char*, char*, char*);
static void jmp_ltu(char*, char*, char*);
static void jmp_nei(char*, char*, char*);
static void jmp_neu(char*, char*, char*);
static void jmp_jumpv(char*);
static void jmp_extern(char*);
static void emit_start(void);
static void call_lib(char*);
static void pop_args(int);
static void fpu_cmp32(char*, char*, Node, int);
static void fpu_cmp64(char*, char*, Node, int);
static void cc_branch(char*, char*);

/* code generator modifications */
static int mov_getrule(Node, int);
static Node mov_reuse(Node, int);
static unsigned mov_emitter(Node, int);
static void emit_kid(Node, int, int);
static void emit_sym(Node, int, int);
static void dump_tree(FILE*, Node);
static char* op_name(int);

/* externals */
extern char* stabprefix;
extern void stabblock(int, int, Symbol*);
extern void stabend(Coordinate*, Symbol, Coordinate**, Symbol*, Symbol*);
extern void stabfend(Symbol, int);
extern void stabinit(char*, int, char*[]);
extern void stabline(Coordinate*);
extern void stabsym(Symbol);
extern void stabtype(Symbol);

/* generated by terms */
static void _kids(NODEPTR_TYPE, int, NODEPTR_TYPE[]);
static void _label(NODEPTR_TYPE);
static int _rule(void*, int);
static short* _nts[];
static char* _string[];
static char* _templates[];
static char _isinstruction[];
static char* _ntname[];

static void emit_header(FILE* f, int comment)
{
	char c=comment?'#':' ';
	fprint(f,"%c       ___     ___            ___    ___     _",c);
	fprint(f,"__     ___     ___          ___     ___      \n");
	fprint(f,"%c      /\\  \\   /\\  \\    ___   /\\__\\  /\\  \\   /\\",c);
	fprint(f,"__\\   /\\__\\   /\\  \\        /\\  \\   /\\  \\    .\n");
	fprint(f,"%c     |::\\  \\ /::\\  \\  /\\  \\ /:/ _/_ \\:\\  \\ /:/",c);
	fprint(f," _/_ /:/  /  /::\\  \\  ___ /::\\  \\ /::\\  \\   .\n");
	fprint(f,"%c     |:::\\  \\:/\\:\\  \\ \\:\\  \\:/ /\\__\\ \\:\\  \\:/ ",c);
	fprint(f,"/\\  \\:/  /  /:/\\:\\  \\/\\__\\:/\\:\\  \\:/\\:\\__\\  .\n");
	fprint(f,"%c   __|:|\\:\\  \\  \\:\\  \\ \\:\\  \\ /:/  /  \\:\\  \\ /",c);
	fprint(f,"::\\  \\  /  _:/ /::\\  \\/  //  \\:\\  \\ /:/  /   \n");
	fprint(f,"%c  /::::|_\\:\\__\\/ \\:\\__\\ \\:\\__\\:/  / \\  \\:\\__\\:",c);
	fprint(f,"/\\:\\__\\/  /\\__\\:/\\:\\__\\_//__/ \\:\\__\\:/__/___ \n");
	fprint(f,"%c  \\:\\~~\\  \\/__/\\ /:/  / |:|  |/  /\\  \\ /:/  //",c);
	fprint(f," /:/  /\\ /:/  //  \\/__/ \\\\  \\ /:/  /::::/  / \n");
	fprint(f,"%c   \\:\\  \\  \\:\\  /:/  / \\|:|  |__/\\:\\  /:/  // ",c);
	fprint(f,"/:/  /  /:/  //__/:/\\:\\  \\\\  /:/  //~~/~~~~  \n");
	fprint(f,"%c    \\:\\  \\  \\:\\/:/  /\\__|:|__|  \\ \\:\\/:/  //_/",c);
	fprint(f,":/  /:\\/:/  /:\\  \\/__\\:\\  \\\\/:/  /:\\~~\\     .\n");
	fprint(f,"%c     \\:\\__\\  \\::/  /\\::::/__/:\\__\\ \\::/  /  /:",c);
	fprint(f,"/  / \\::/  / \\:\\__\\   \\:\\__\\:/  / \\:\\__\\    .\n");
	fprint(f,"%c      \\/__/   \\/__/  ~~~~    \\/__/  \\/__/   \\/",c);
	fprint(f,"__/   \\/__/   \\/__/    \\/__/ __/   \\/__/    2\n");
	fprint(f,"%c                                              ",c);
	fprint(f,"                                             \n");

	fprint(f,"%c\n",c);
	fprint(f,"%c M/o/Vfuscator2\n",c);
	fprint(f,"%c\n",c);
	fprint(f,"%c github.com/xoreaxeaxeax/movfuscator\n",c);
	fprint(f,"%c chris domas           @xoreaxeaxeax\n",c);
	fprint(f,"%c\n",c);
	fprint(f,"\n");
}

static void debug_id(void) 
{
	static int id=DEBUG_ID;
	id++; 
	if (mov_id) {
		print("movl $0x%x, (id)\n", id); 
	}
}

/* this is borderline abusive, but seemed marginally better than a ridiculous
 * unnecessary library of function pointers to achieve the expression part */

#define BUILD_1D_TABLE(name, e_size, elements, expr) \
	{\
		int X; \
		print(".align 16\n"); \
		print(".globl %s\n", (name)); \
		print("%s: .%s ", (name), (e_size)); \
		for (X=0; X<elements; X++) print("0x%x,", (expr)); \
		print("END\n"); \
		print("\n"); \
	}

#define BUILD_2D_TABLE(name, e_size, height, width, expr) \
	{ \
		int X, Y; \
		print(".align 16\n"); \
		print(".globl %s\n", (name)); \
		print("%s: .long ", (name)); \
		for (X=0; X<(height); X++) { \
			print("%s_%d,", (name), X); \
		} \
		print("END\n"); \
		\
		for (X=0; X<height; X++) { \
			print(".globl %s_%d\n", (name), X); \
			print("%s_%d: .%s ", (name), X, e_size); \
			for (Y=0; Y<width; Y++) print("0x%x,",(expr)); \
			print("END\n"); \
		} \
		print("\n"); \
	}

#define BUILD_1D_INDEX_TABLE(name, e_size, height, width, expr) \
	{ \
		int X, Y; \
		print(".align 16\n"); \
		print(".globl %s\n", (name)); \
		print("%s: .long ", (name)); \
		for (X=0; X<(height); X++) { \
			print("%s_%d,", (name), X); \
		} \
		print("END\n"); \
		\
		for (Y=0; Y<width; Y++) { \
			print("%s_%d: .%s ", (name), Y, e_size); \
			print("0x%x\n",(expr)); \
		} \
		print("\n"); \
	}

static void data_setup(void)
{
	/* these tables get large. if you need a smaller data footprint, either
	 * bloat the code (many of the tables exist to make the code smaller), or
	 * generate the tables dynamically */

	int i;

	(*IR->segment)(MOV_DATA);

	print("############## begin data tables ##############\n");
	print("\n");

	print(".equ END, 0\n");
	print("\n");

	print(".globl push\n");
	print(".globl pop\n");
	print(".equ push, pushpop-stack-4\n");
	print(".equ pop,  pushpop-stack+4\n");

	/* soft-registers */
	print(".align 16\n");
	for (i=0; i<SOFT_I_REGS; i++) {
		print(".globl R%d\n", i);
		print("R%d: .long 0\n", i);
	}

	print(".align 16\n");
	for (i=0; i<SOFT_F_REGS; i++) {
		print(".globl F%d\n", i);
		print("F%d: .long 0\n", i);
	}

	print(".align 16\n");
	for (i=0; i<SOFT_D_REGS; i++) {
		print(".globl D%d\n", i);
		print("D%d: .long 0, 0\n", i);
	}

	/* or */
	print(".align 16\n");
	print(".globl or\n");
	print("or:   .long or_0, or_1\n");
	print("or_0: .long 0, 1\n");
	print("or_1: .long 1, 1\n");
	print("\n");

	/* and */
	print(".align 16\n");
	print(".globl and\n");
	print("and:   .long and_0, and_1\n");
	print("and_0: .long 0, 0\n");
	print("and_1: .long 0, 1\n");
	print("\n");

	/* xor */
	print(".align 16\n");
	print(".globl xor\n");
	print("xor:   .long xor_0, xor_1\n");
	print("xor_0: .long 0, 1\n");
	print("xor_1: .long 1, 0\n");
	print("\n");

	/* xnor */
	print(".align 16\n");
	print(".globl xnor\n");
	print("xnor:   .long xnor_0, xnor_1\n");
	print("xnor_0: .long 1, 0\n");
	print("xnor_1: .long 0, 1\n");
	print("\n");

	/* true/false */
	BUILD_1D_TABLE("alu_true",   "byte", 512, X?1:0);
	BUILD_1D_TABLE("alu_false",  "byte", 512, X?0:1);

	/* bit selection */
	BUILD_1D_TABLE("alu_b0",     "long", 256, (X&0x01)>>0);
	BUILD_1D_TABLE("alu_b1",     "long", 256, (X&0x02)>>1);
	BUILD_1D_TABLE("alu_b2",     "long", 256, (X&0x04)>>2);
	BUILD_1D_TABLE("alu_b3",     "long", 256, (X&0x08)>>3);
	BUILD_1D_TABLE("alu_b4",     "long", 256, (X&0x10)>>4);
	BUILD_1D_TABLE("alu_b5",     "long", 256, (X&0x20)>>5);
	BUILD_1D_TABLE("alu_b6",     "long", 256, (X&0x40)>>6);
	BUILD_1D_TABLE("alu_b7",     "long", 256, (X&0x80)>>7);

	/* bit set/clear */
	BUILD_2D_TABLE("alu_b_s",    "byte",   8, 256, Y|(1<<X));
	BUILD_2D_TABLE("alu_b_c",    "byte",   8, 256, Y&~(1<<X));

	/* equality */
	BUILD_2D_TABLE("alu_eq",     "byte", 256, 256, Y==X?1:0);

	/* add */
	BUILD_1D_TABLE("alu_add8l",  "byte", 512, X&0xff);
	BUILD_1D_TABLE("alu_add8h",  "byte", 512, (X&0xff00)>>8);
	/* (this table is easily removed if size is a concern) */
	BUILD_1D_INDEX_TABLE("alu_add16", "long", 65536*2, 65536*2, Y);

	/* sub */
	BUILD_1D_TABLE("alu_inv8",   "byte", 256, (~X)&0xff);
	/* (this table is easily removed if size is a concern) */
	BUILD_1D_TABLE("alu_inv16",  "word", 65536, (~X)&0xffff);

	/* band */
	BUILD_2D_TABLE("alu_band8",  "byte", 256, 256, (X&Y)&0xff);

	/* bor */
	BUILD_2D_TABLE("alu_bor8",   "byte", 256, 256, (X|Y)&0xff);

	/* bxor */
	BUILD_2D_TABLE("alu_bxor8",  "byte", 256, 256, (X^Y)&0xff);

	/* although shifting a 32 bit var > 31 bits is generally undefined in C,
	 * when execution is off, the shift amount may be read from .discard - so we
	 * need execution to not fault with large shift values - clamp32
	 * and the larger shift tables accomplish this */

	/* lshu */
	BUILD_2D_TABLE("alu_lshu8",  "long", 33, 256, X>31?0:((unsigned int)Y)<<X);

	/* rshu */
	BUILD_2D_TABLE("alu_rshu8",  "long", 33, 256, X>31?0:((unsigned int)Y)<<24>>X);

	/* rshi */
	BUILD_2D_TABLE("alu_rshi8s", "long", 33, 256, X>31?0xffffffff:(((int)Y)&0x80)<<24>>X);

	/* clamp32 */
	BUILD_1D_TABLE("alu_clamp32", "long", 512, X>32?32:X);

	/* mul */
	BUILD_1D_TABLE("alu_mul_sum8l", "byte", 256*3,  X&0xff);
	BUILD_1D_TABLE("alu_mul_sum8h", "byte", 256*3,  (X&0xff00)>>8);
	BUILD_1D_TABLE("alu_mul_shl2",  "long", 256*16,  X*4);
	BUILD_1D_TABLE("alu_mul_sums",  "long", 256*16, X);
	BUILD_2D_TABLE("alu_mul_mul8l", "byte", 256, 256, (X*Y)&0xff);
	BUILD_2D_TABLE("alu_mul_mul8h", "byte", 256, 256, ((X*Y)&0xff00)>>8);

	/* div */
	BUILD_1D_TABLE("alu_div_shl1_8_c_d", "long", 512, X);
	BUILD_1D_TABLE("alu_div_shl1_8_d",   "long", 256, X*2);
	BUILD_1D_TABLE("alu_div_shl2_8_d",   "long", 256, X*4);
	BUILD_1D_TABLE("alu_div_shl3_8_d",   "long", 256, X*8);

	/* sign extend */
	BUILD_1D_TABLE("alu_sex8", "long", 256, (int)(signed char)X);

	/* cmp */
	print(".align 16\n");
	print(".globl alu_cmp_of\n");
	print("alu_cmp_of:     .long alu_cmp_of_0,  alu_cmp_of_1\n");
	print("alu_cmp_of_0:   .long alu_cmp_of_00, alu_cmp_of_01\n");
	print("alu_cmp_of_1:   .long alu_cmp_of_10, alu_cmp_of_11\n");
	print("alu_cmp_of_00:  .long alu_cmp_of_000, alu_cmp_of_001\n");
	print("alu_cmp_of_01:  .long alu_cmp_of_010, alu_cmp_of_011\n");
	print("alu_cmp_of_10:  .long alu_cmp_of_100, alu_cmp_of_101\n");
	print("alu_cmp_of_11:  .long alu_cmp_of_110, alu_cmp_of_111\n");
	print("alu_cmp_of_000: .long 0\n"); /* - - -> - */
	print("alu_cmp_of_001: .long 0\n"); /* - - -> + */
	print("alu_cmp_of_010: .long 0\n"); /* - + -> - */
	print("alu_cmp_of_011: .long 1\n"); /* - + -> + */
	print("alu_cmp_of_100: .long 1\n"); /* + - -> - */
	print("alu_cmp_of_101: .long 0\n"); /* + - -> + */
	print("alu_cmp_of_110: .long 0\n"); /* + + -> - */
	print("alu_cmp_of_111: .long 0\n"); /* + + -> + */
	print("\n");

	/* bool */
	print(".align 16\n");
	print(".globl b0, b1, b2, b3\n");
	print("b0: .long 0\n");
	print("b1: .long 0\n");
	print("b2: .long 0\n");
	print("b3: .long 0\n");
	print("\n");

	/* alu scratch */
	print(".align 16\n");
	print(".globl alu_x, alu_y, alu_s, alu_c\n");
	print("alu_x: .long 0\n");
	print("alu_y: .long 0\n");
	print("alu_s: .long 0\n");
	/* the extensions of alu_c allows an important optimization when the carry
	 * is in the high word.  the extra word after alu_c allows moving the a high
	 * word result into the low word of the carry */
	print(".long 0\n");
	print("alu_c: .long 0, 0\n");
	print("\n");

	/* alu shift scratch */
	/* padded for discarded overflows */
	print(".align 16\n");
	print(".globl alu_s0, alu_s1, alu_s2, alu_s3\n");
	print(".globl alu_ss, alu_sc, alu_sx\n");
	print(".long 0\nalu_s0: .long 0\n.long 0\n"); 
	print(".long 0\nalu_s1: .long 0\n.long 0\n"); 
	print(".long 0\nalu_s2: .long 0\n.long 0\n"); 
	print(".long 0\nalu_s3: .long 0\n.long 0\n"); 
	print(".long 0\nalu_ss: .long 0\n.long 0\n"); 
	print(".long 0\nalu_sc: .long 0\n.long 0\n"); 
	print(".long 0\nalu_sx: .long 0\n.long 0\n"); 
	print("\n");

	/* alu mul/div scratch */
	print(".align 16\n");
	print(".globl alu_z0, alu_z1, alu_z2, alu_z3\n");
	print("alu_z0: .long 0\n");
	print("alu_z1: .long 0\n");
	print("alu_z2: .long 0\n");
	print("alu_z3: .long 0\n");
	print("\n");

	/* alu div/rem scratch */
	print(".align 16\n");

	print(".globl alu_n, alu_d, alu_q, alu_r, alu_t\n");
	print(".globl alu_ns, alu_ds, alu_qs, alu_rs\n");
	print("alu_n: .long 0\n");
	print("alu_d: .long 0\n");
	print("alu_q: .long 0\n");
	print("alu_r: .long 0\n");
	print("alu_t: .long 0\n");
	print("alu_ns: .long 0\n");
	print("alu_ds: .long 0\n");
	print("alu_qs: .long 0\n");
	print("alu_rs: .long 0\n");

	print(".globl alu_sn, alu_sd, alu_sq, alu_sr\n");
	print("alu_sn: .long 0\n");
	print("alu_sd: .long 0\n");
	print("alu_sq: .long 0\n");
	print("alu_sr: .long 0\n");

	print(".globl alu_sel_r, alu_sel_d, alu_sel_q, alu_sel_n\n");
	print("alu_sel_r: .long alu_sr, alu_r\n");
	print("alu_sel_d: .long alu_sd, alu_d\n");
	print("alu_sel_q: .long alu_sq, alu_q\n");
	print("alu_sel_n: .long alu_sn, alu_n\n");

	print(".globl alu_psel_r, alu_psel_d, alu_psel_q, alu_psel_n\n");
	print("alu_psel_r: .long 0\n");
	print("alu_psel_d: .long 0\n");
	print("alu_psel_q: .long 0\n");
	print("alu_psel_n: .long 0\n");
	print("\n");

	/* flags */
	print(".align 16\n");
	print(".globl zf, sf, of, cf\n");
	print("zf: .long 0\n");
	print("sf: .long 0\n");
	print("of: .long 0\n");
	print("cf: .long 0\n");
	print("\n");

	/* branches */
	print(".align 16\n");
	print(".globl branch_temp\n");
	print("branch_temp: .long 0\n");
	print("\n");

	/* stack scratch */
	print(".align 16\n");
	print(".globl stack_temp\n");
	print("stack_temp: .long 0, 0\n");
	print("\n");

	/* push/pop table */
	/* this will be about as large as the stack itself - can be done with an
	 * add/sub instead if space is a priority */
	/* push/pop guards prevent push/pop macros from moving off the table when
	 * execution is off */
	print("pop_guard: .long stack+0x%x\n", 0);
	print("pushpop: .long ");
	for (i=0; i<=STACK_SIZE; i+=4) {
		print("stack+0x%x%c", i, i==STACK_SIZE?'\n':',');
	}
	print("push_guard: .long stack+0x%x\n", STACK_SIZE);
	print("\n");

	/* stack pointers */
	print(".align 16\n");
	print(".globl sp, fp\n");
	print("sp: .long stack+0x%x\n", STACK_SIZE);
	print("fp: .long stack+0x%x\n", STACK_SIZE);
	print("\n");
	
	/* command line arguments */
	print(".align 16\n");
	print(".globl sesp\n");
	print("sesp: .long 0\n");
	print("\n");

	/* execution on or off */
	print(".align 16\n");
	print(".globl sel_on, on, toggle_execution\n");
	print("sel_on: .long discard, on\n"); 
	print("on: .long 0\n");
	print("toggle_execution: .long 1\n");
	print("\n");
	
	/* branch target */
	print(".align 16\n");
	print(".globl sel_target, target\n");
	print("sel_target: .long discard, target\n"); 
	print("target: .long 0\n");
	print("\n");

	/* data selector */
	print(".align 16\n");
	print(".globl sel_data, data_p\n");
	print("sel_data: .long discard\n");
	print("data_p: .long 0\n");
	print("\n");

	/* stack */
	/* stack would ideally be in bss, but pushpop table needs to be in the same
	 * segment as the stack, for the lookup to work */
	//(*IR->segment)(MOV_BSS);
	print(".align 16\n");
	print(".globl stack\n");
	print("stack:\n");
	print(".fill 0x%x, 1, 0\n", STACK_SIZE);
	print("\n");

	/* scratch target for id */
	print(".align 16\n");
	print(".globl id\n");
	print("id: .long 0\n");
	print("\n");

	/* backup of registers on jump */
	print(".align 16\n");
	for (i=0; i<SOFT_I_REGS; i++) {
		print(".globl jmp_r%d\n", i);
		print("jmp_r%d: .long 0\n", i);
	}
	for (i=0; i<SOFT_F_REGS; i++) {
		print(".globl jmp_f%d\n", i);
		print("jmp_f%d: .long 0\n", i);
	}
	for (i=0; i<SOFT_D_REGS; i++) {
		print(".globl jmp_d%d\n", i);
		print("jmp_d%d: .long 0, 0\n", i);
	}
	print("\n");

	/* copy of return value from external functions */
	print(".align 16\n");
	print(".globl ext_ret_val\n");
	print("ext_ret_val: .long 0\n");
	print("\n");

	/* external function address */
	print(".globl external\n");
	print("external: .long 0\n");
	print("\n");

	/* seg fault */
	print(".globl fault\n");
	print("fault: .long no_fault, 0\n");
	print("no_fault: .long 0\n");
	print("\n");

	if (mov_extern) {
		/* note: structure may vary based on kernel, clib */
		print(".globl sa_dispatch\n");
		print("sa_dispatch: .long dispatch\n");        /* sa_handler */
		print("             .fill 0x20, 4, 0\n");      /* sa_mask */
		print("             .long %d\n", SA_NODEFER);  /* sa_flags */
		print("             .long 0\n");               /* sa_restorer */
		print("\n");
	}

	if (mov_loop) {
		/* note: structure may vary based on kernel, clib */
		print(".globl sa_loop\n");
		print("sa_loop: .long master_loop\n");     /* sa_handler */
		print("         .fill 0x20, 4, 0\n");      /* sa_mask */
		print("         .long %d\n", SA_NODEFER);  /* sa_flags */
		print("         .long 0\n");               /* sa_restorer */
		print("\n");
	}

	/* discarded data */
	(*IR->segment)(MOV_BSS);
	print(".align 16\n");
	print(".globl discard\n");
	print("discard: .fill 0x%x, 1, 0\n", DISCARD_SIZE);
}

static void execution_on(char* b)
{
	print("# execute on %s\n", b);
	debug_id();
	print("movl %s, %%eax\n", b);
	print("movl sel_on(,%%eax,4), %%eax\n");
	print("movl $1, (%%eax)\n");
	print("# end execute on\n");
}

static void execution_off(char* b)
{
	print("# execute off %s\n", b);
	debug_id();
	print("movl %s, %%eax\n", b);
	print("movl sel_on(,%%eax,4), %%eax\n");
	print("movl $0, (%%eax)\n");
	print("# end execute off\n");
}

static void store_target(char* a, char* b)
{
	print("# store target %s %s\n", a, b);
	debug_id();
	print("movl %s, %%eax\n", b);
	print("movl sel_target(,%%eax,4), %%eax\n");
	print("movl %s, %%edx\n", a);
	print("movl %%edx, (%%eax)\n");
	print("# end store target\n");
}

/* store_data c, r, m */
static void store_data(char* a, char* v, char* b)
{
	print("# store data %s %s %s\n", a, v, b);
	debug_id();
	print("movl $%s, (data_p)\n", a);
	print("movl %s, %%eax\n", b);
	print("movl sel_data(,%%eax,4), %%eax\n");
	print("movl %s, %%edx\n", v);
	print("movl %%edx, (%%eax)\n");
	print("# end store data\n");
}

static void store_jmp_regs(char* b)
{
	int i;
	print("# store jmp regs %s\n", b);
	debug_id();
	print("movl %s, %%ecx\n", b);
	/* this is a major bottleneck */
	/* slight optimization with block copies */
	/* requires contiguous register sets! */
	print("movl $jmp_r0, (data_p)\n");
	print("movl sel_data(,%%ecx,4), %%eax\n");
	for (i=0; i<SOFT_I_REGS; i++) {
		print("movl (R%d), %%edx\n", i);
		print("movl %%edx, %d(%%eax)\n", i*4);
	}
	print("movl $jmp_f0, (data_p)\n");
	print("movl sel_data(,%%ecx,4), %%eax\n");
	for (i=0; i<SOFT_F_REGS; i++) {
		print("movl (F%d), %%edx\n", i);
		print("movl %%edx, %d(%%eax)\n", i*4);
	}
	print("movl $jmp_d0, (data_p)\n");
	print("movl sel_data(,%%ecx,4), %%eax\n");
	for (i=0; i<SOFT_D_REGS; i++) {
		print("movl (D%d), %%edx\n", i);
		print("movl %%edx, %d(%%eax)\n", i*8);
		print("movl (D%d+4), %%edx\n", i);
		print("movl %%edx, %d(%%eax)\n", i*8+4);
	}
	print("# end store jmp regs\n");
}

/* load_data c, rm, m */
static void load_data(char* a, char* v, char* b)
{
	print("# load data %s %s %s\n", a, v, b);
	debug_id();
	print("movl $%s, (data_p)\n", a);
	print("movl %s, %%eax\n", v);
	print("movl %%eax, (discard)\n");
	print("movl %s, %%eax\n", b);
	print("movl sel_data(,%%eax,4), %%eax\n");
	print("movl (%%eax), %%eax\n");
	print("movl %%eax, %s\n", v);
	print("# end load data\n");
}

static void load_jmp_regs(char* b)
{
	int i;
	print("# load jmp regs %s\n", b);
	debug_id();
	print("movl %s, %%ecx\n", b);
	for (i=0; i<SOFT_I_REGS; i++) {
		print("movl $R%d, (data_p)\n", i);
		print("movl sel_data(,%%ecx,4), %%eax\n");
		print("movl (jmp_r%d), %%edx\n", i);
		print("movl %%edx, (%%eax)\n");
	}
	for (i=0; i<SOFT_F_REGS; i++) {
		print("movl $F%d, (data_p)\n", i);
		print("movl sel_data(,%%ecx,4), %%eax\n");
		print("movl (jmp_f%d), %%edx\n", i);
		print("movl %%edx, (%%eax)\n");
	}
	for (i=0; i<SOFT_D_REGS; i++) {
		print("movl $D%d, (data_p)\n", i);
		print("movl sel_data(,%%ecx,4), %%eax\n");
		print("movl (jmp_d%d), %%edx\n", i);
		print("movl %%edx, (%%eax)\n");
		print("movl (jmp_d%d+4), %%edx\n", i);
		print("movl %%edx, 4(%%eax)\n");
	}
	print("# end load jmp regs\n");
}

/* select_data reg, reg (register safe) */
static void select_data(char* data, char* b)
{
	print("# select data %s %s\n", data, b);
	debug_id();
	print("movl %s, (data_p)\n", data);
	print("movl sel_data(,%s,4), %s\n", b, data);
	print("# end select data\n");
}

/* PUSH */
static void push(char* s)
{
	print("# push %s\n", s);
	debug_id();
	print("movl %s, %%eax\n", s);
	print("movl %%eax, (stack_temp)\n");

	print("movl $sp, %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (sp), %%edx\n");
	print("movl push(%%edx), %%edx\n");
	print("movl %%edx, (%%eax)\n");

	print("movl (sp), %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (stack_temp), %%edx\n");
	print("movl %%edx, (%%eax)\n");
	print("# end push\n");
}

static void pushm8(char* s)
{
	print("# push %s\n", s);
	debug_id();
	print("movl (%s), %%eax\n", s);
	print("movl %%eax, (stack_temp)\n");
	print("movl (%s+4), %%eax\n", s);
	print("movl %%eax, (stack_temp+4)\n");

	print("movl $sp, %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (sp), %%edx\n");
	print("movl push(%%edx), %%edx\n");
	print("movl push(%%edx), %%edx\n");
	print("movl %%edx, (%%eax)\n");

	print("movl (sp), %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (stack_temp), %%edx\n");
	print("movl %%edx, (%%eax)\n");
	print("movl (stack_temp+4), %%edx\n");
	print("movl %%edx, 4(%%eax)\n");
	print("# end push\n");
}

/* POP */
static void popr(char* s)
{
	print("# pop %s\n", s);
	debug_id();
	print("movl (sp), %%eax\n");
	print("movl (%%eax), %%edx\n");
	print("movl %%edx, (stack_temp)\n");

	print("movl $sp, %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (sp), %%edx\n");
	print("movl pop(%%edx), %%edx\n");
	print("movl %%edx, (%%eax)\n");

	print("movl (stack_temp), %%edx\n");
	print("movl %%edx, %s\n", s);
	print("# end pop\n");
}

static void popm(char* s)
{
	print("# pop %s\n", s);
	debug_id();
	print("movl (sp), %%eax\n");
	print("movl (%%eax), %%edx\n");
	print("movl %%edx, (stack_temp)\n");

	print("movl $sp, %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (sp), %%edx\n");
	print("movl pop(%%edx), %%edx\n");
	print("movl %%edx, (%%eax)\n");

	print("movl $%s, %%eax\n", s);
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (stack_temp), %%edx\n");
	print("movl %%edx, (%%eax)\n");
	print("# end pop\n");
}

static void popm8(char* s)
{
	print("# pop8 %s\n", s);
	debug_id();
	print("movl (sp), %%eax\n");
	print("movl (%%eax), %%edx\n");
	print("movl %%edx, (stack_temp)\n");
	print("movl 4(%%eax), %%edx\n");
	print("movl %%edx, (stack_temp+4)\n");

	print("movl $sp, %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (sp), %%edx\n");
	print("movl pop(%%edx), %%edx\n");
	print("movl pop(%%edx), %%edx\n");
	print("movl %%edx, (%%eax)\n");

	print("movl $%s, %%eax\n", s);
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (stack_temp), %%edx\n");
	print("movl %%edx, (%%eax)\n");
	print("movl (stack_temp+4), %%edx\n");
	print("movl %%edx, 4(%%eax)\n");
	print("# end pop8\n");
}

/* ADD */

/* add reg, reg */
static void alu_add(char* s, char* x, char* y)
{
	print("# alu_add\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_add32("alu_s", "alu_x", "alu_y", "alu_c");
	print("movl (alu_s), %s\n", s);
	print("# end alu_add\n");
}

static void alu_add8(char* s, char* x, char* y, char* c, int off)
{
	/* requires dword carry initialized */
	print("# alu_add8\n");
	debug_id();
	print("movl $0, %%eax\n");
	print("movl $0, %%ebx\n");
	print("movl $0, %%ecx\n");
	print("movl $0, %%edx\n");
	print("movb (%s+%d), %%al\n", x, off);
	print("movb (%s+%d), %%cl\n", y, off);
	print("movl (%s), %%ebx\n", c);
	print("movb alu_add8l(%%eax,%%ecx), %%dl\n");
	print("movb alu_add8h(%%eax,%%ecx), %%dh\n");
	print("movb alu_add8l(%%edx,%%ebx), %%al\n");
	print("movb %%al, (%s+%d)\n", s, off);
	print("movb alu_add8h(%%edx,%%ebx), %%al\n");
	print("movb %%al, (%s)\n", c);
	print("# end alu_add8\n");
}

static void alu_add8_fast(char* s, char* x, char* y, char* c, int off)
{
	/* requires eax, ebx, ecx, edx=0 */
	/* requires dword carry initialized */
	print("# alu_add8_fast\n");
	debug_id();
	print("movb (%s+%d), %%al\n", x, off);
	print("movb (%s+%d), %%cl\n", y, off);
	print("movl (%s), %%ebx\n", c);
	print("movb alu_add8l(%%eax,%%ecx), %%dl\n");
	print("movb alu_add8h(%%eax,%%ecx), %%dh\n");
	print("movb alu_add8l(%%edx,%%ebx), %%al\n");
	print("movb %%al, (%s+%d)\n", s, off);
	print("movb alu_add8h(%%edx,%%ebx), %%al\n");
	print("movb %%al, (%s)\n", c);
	print("# end alu_add8_fast\n");
}

static void alu_add16_fast(char* s, char* x, char* y, char* c, int off)
{
	/* requires eax, ecx=0 */
	print("# alu_add16_fast\n");
	debug_id();
	print("movw (%s+%d), %%ax\n", x, off);
	print("movw (%s+%d), %%cx\n", y, off);
	
	print("movl alu_add16(,%%eax,4), %%edx\n");
	print("movl (%%edx,%%ecx,4), %%edx\n");

	/* quirky hack for optimization - this is one of the most commonly called
	 * functions; x86 doesn't allow us to access high 16 bits of register; save
	 * a bunch of register shuffling by loading the high 16 bits of carry here */
	print("movw (%s+2), %%cx\n", c);

	print("movl alu_add16(,%%edx,4), %%edx\n");
	print("movl (%%edx,%%ecx,4), %%edx\n");

	print("movw %%dx, (%s+%d)\n", s, off);
	print("movl %%edx, (%s)\n", c);
	print("# end alu_add16_fast\n");
}

static void alu_add32(char* s, char* x, char* y, char* c)
{
	print("# alu_add32\n");
	debug_id();
	/* use the 8 bit adders if program size is an issue */
	/*
	print("movl $0, %%eax\n");
	print("movl $0, %%ebx\n");
	print("movl $0, %%ecx\n");
	print("movl $0, %%edx\n");
	print("movl $0, (%s)\n", c);
	alu_add8_fast(s, x, y, c, 0);
	alu_add8_fast(s, x, y, c, 1);
	alu_add8_fast(s, x, y, c, 2);
	alu_add8_fast(s, x, y, c, 3);
	*/

	print("movl $0, %%eax\n");
	print("movl $0, %%ecx\n");
	print("movl $0, (%s)\n", c);
	alu_add16_fast(s, x, y, c, 0);
	alu_add16_fast(s, x, y, c, 2);

	print("# end alu_add32\n");
}

/* SUB */

/* sub reg, reg */
static void alu_sub(char* s, char* x, char* y)
{
	print("# alu_sub\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_sub32("alu_s", "alu_x", "alu_y", "alu_c");
	print("movl (alu_s), %s\n", s);
	print("# end alu_sub\n");
}

static void alu_inv8(char* b, int off)
{
	print("# alu_inv8\n");
	debug_id();
	print("movl $0, %%eax\n");
	print("movb (%s+%d), %%al\n", b, off);
	print("movb alu_inv8(%%eax), %%al\n");
	print("movb %%al, (%s+%d)\n", b, off);
	print("# end alu_inv8\n");
}

static void alu_sub16_fast(char* s, char* x, char* y, char* c, int off)
{
	/* requires eax, ecx=0 */

	/* carry accesses are a quirky hack for optimization - this is one of the
	 * most commonly called functions; x86 doesn't allow us to access high 16
	 * bits of register; save a bunch of register shuffling by loading the high
	 * 16 bits of carry here */

	print("# alu_sub16_fast\n");
	debug_id();
	print("movw (%s+%d), %%ax\n", x, off);
	print("movw (%s+%d), %%cx\n", y, off);

	print("movw alu_inv16(,%%ecx,2), %%cx\n");
	print("movl alu_add16(,%%eax,4), %%edx\n");
	print("movl (%%edx,%%ecx,4), %%edx\n");
	print("movl alu_add16(,%%edx,4), %%edx\n");
	print("movl (%s), %%ecx\n", c);
	print("movl (%%edx,%%ecx,4), %%edx\n");

	print("movw %%dx, (%s+%d)\n", s, off);
	print("movl %%edx, (%s-2)\n", c);

	print("# end alu_sub16_fast\n");
}

/* warning: destroys y */
static void alu_sub32(char* s, char* x, char* y, char* c)
{
	print("# alu_sub32\n");
	debug_id();
	/*
	alu_inv8(y, 0);
	alu_inv8(y, 1);
	alu_inv8(y, 2);
	alu_inv8(y, 3);
	print("movl $0, %%eax\n");
	print("movl $0, %%ebx\n");
	print("movl $0, %%ecx\n");
	print("movl $0, %%edx\n");
	print("movl $1, (%s)\n", c);
	alu_add8_fast(s, x, y, c, 0);
	alu_add8_fast(s, x, y, c, 1);
	alu_add8_fast(s, x, y, c, 2);
	alu_add8_fast(s, x, y, c, 3);
	*/

	print("movl $0, %%eax\n");
	print("movl $0, %%ecx\n");
	print("movl $0x1, (%s)\n", c); 
	alu_sub16_fast(s, x, y, c, 0);
	alu_sub16_fast(s, x, y, c, 2);

	print("# end alu_sub32\n");
}

/* BAND */

/* band reg, reg */
static void alu_band(char* s, char* x, char* y)
{
	print("# alu_band\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_band32("alu_s", "alu_x", "alu_y");
	print("movl (alu_s), %s\n", s);
	print("# end alu_band\n");
}

static void alu_band32(char* s, char* x, char* y)
{
	print("# alu_band32\n");
	debug_id();
	alu_band8(s, x, y, 0);
	alu_band8(s, x, y, 1);
	alu_band8(s, x, y, 2);
	alu_band8(s, x, y, 3);
	print("# end alu_band32\n");
}

static void alu_band8(char* s, char* x, char* y, int off)
{
	print("# alu_band8\n");
	debug_id();
	print("movl $0, %%eax\n");
	print("movl $0, %%edx\n");
	print("movb (%s+%d), %%al\n", x, off);
	print("movb (%s+%d), %%dl\n", y, off);
	print("movl alu_band8(,%%eax,4), %%eax\n");
	print("movb (%%eax,%%edx), %%al\n");
	print("movb %%al, (%s+%d)\n", s, off);
	print("# end alu_band8\n");
}

/* BOR */

/* bor reg, reg */
static void alu_bor(char* s, char* x, char* y)
{
	print("# alu_bor\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_bor32("alu_s", "alu_x", "alu_y");
	print("movl (alu_s), %s\n", s);
	print("# end alu_bor\n");
}

static void alu_bor32(char* s, char* x, char* y)
{
	print("# alu_bor32\n");
	debug_id();
	alu_bor8(s, x, y, 0);
	alu_bor8(s, x, y, 1);
	alu_bor8(s, x, y, 2);
	alu_bor8(s, x, y, 3);
	print("# end alu_bor32\n");
}

static void alu_bor8(char* s, char* x, char* y, int off)
{
	print("# alu_bor8\n");
	debug_id();
	print("movl $0, %%eax\n");
	print("movl $0, %%edx\n");
	print("movb (%s+%d), %%al\n", x, off);
	print("movb (%s+%d), %%dl\n", y, off);
	print("movl alu_bor8(,%%eax,4), %%eax\n");
	print("movb (%%eax,%%edx), %%al\n");
	print("movb %%al, (%s+%d)\n", s, off);
	print("# end alu_bor8\n");
}

/* BXOR */

/* bxor reg, reg */
static void alu_bxor(char* s, char* x, char* y)
{
	print("# alu_bxor\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_bxor32("alu_s", "alu_x", "alu_y");
	print("movl (alu_s), %s\n", s);
	print("# end alu_bxor\n");
}

static void alu_bxor32(char* s, char* x, char* y)
{
	print("# alu_bxor32\n");
	debug_id();
	alu_bxor8(s, x, y, 0);
	alu_bxor8(s, x, y, 1);
	alu_bxor8(s, x, y, 2);
	alu_bxor8(s, x, y, 3);
	print("# end alu_bxor32\n");
}

static void alu_bxor8(char* s, char* x, char* y, int off)
{
	print("# alu_bxor8\n");
	debug_id();
	print("movl $0, %%eax\n");
	print("movl $0, %%edx\n");
	print("movb (%s+%d), %%al\n", x, off);
	print("movb (%s+%d), %%dl\n", y, off);
	print("movl alu_bxor8(,%%eax,4), %%eax\n");
	print("movb (%%eax,%%edx), %%al\n");
	print("movb %%al, (%s+%d)\n", s, off);
	print("# end alu_bxor8\n");
}

/* BCOM */

/* bcom reg */
static void alu_bcom(char* s, char* x)
{
	print("# alu_bcom\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	alu_inv8("alu_x", 0);
	alu_inv8("alu_x", 1);
	alu_inv8("alu_x", 2);
	alu_inv8("alu_x", 3);
	print("movl (alu_x), %s\n", s);
	print("# end alu_bcom\n");
}

/* NEG */

/* neg reg */
static void alu_neg(char* s, char* x)
{
	print("# alu_neg\n");
	debug_id();
	print("movl %s, (alu_y)\n", x);
	print("movl $0, (alu_x)\n");
	alu_sub32("alu_s", "alu_x", "alu_y", "alu_c");
	print("movl (alu_s), %s\n", s);
	print("# end alu_neg\n");
}

/* LSHU */

/* lshu reg, reg */
static void alu_lshu(char* s, char* x, char* y)
{
	print("# alu_lshu\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_clamp32("(alu_y)", "(alu_y)");
	alu_lshu32("alu_s", "alu_x", "alu_y");
	print("movl (alu_s), %s\n", s);
	print("# end alu_lshu\n");
}

static void alu_lshu32(char* s, char* x, char* y)
{
	print("# alu_lshu32\n");
	debug_id();

	print("movl $0, %%eax\n");

	print("movl $0, (alu_s0)\n");
	print("movl $0, (alu_s1)\n");
	print("movl $0, (alu_s2)\n");
	print("movl $0, (alu_s3)\n");

	print("movl (%s), %%edx\n", y);
	print("movl alu_lshu8(,%%edx,4), %%edx\n");

	print("movb (%s+0), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s0+0)\n");

	print("movb (%s+1), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s1+1)\n");

	print("movb (%s+2), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s2+2)\n");

	print("movb (%s+3), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s3+3)\n");

	alu_bor32(s, "alu_s0", "alu_s1");
	alu_bor32(s, s, "alu_s2");
	alu_bor32(s, s, "alu_s3");

	print("# end alu_lshu32\n");
}

/* RSHU */

/* rshu reg, reg */
static void alu_rshu(char* s, char* x, char* y)
{
	print("# alu_rshu\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_clamp32("(alu_y)", "(alu_y)");
	alu_rshu32("alu_s", "alu_x", "alu_y");
	print("movl (alu_s), %s\n", s);
	print("# end alu_rshu\n");
}

static void alu_rshu32(char* s, char* x, char* y)
{
	print("# alu_rshu32\n");
	debug_id();

	print("movl $0, %%eax\n");

	print("movl $0, (alu_s0)\n");
	print("movl $0, (alu_s1)\n");
	print("movl $0, (alu_s2)\n");
	print("movl $0, (alu_s3)\n");

	print("movl (%s), %%edx\n", y);
	print("movl alu_rshu8(,%%edx,4), %%edx\n");

	print("movb (%s+0), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s0-3)\n");

	print("movb (%s+1), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s1-2)\n");

	print("movb (%s+2), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s2-1)\n");

	print("movb (%s+3), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s3-0)\n");

	alu_bor32(s, "alu_s0", "alu_s1");
	alu_bor32(s, s, "alu_s2");
	alu_bor32(s, s, "alu_s3");

	print("# end alu_rshu32\n");
}

/* RSHI */

/* rshi reg, reg */
static void alu_rshi(char* s, char* x, char* y)
{
	print("# alu_rshi\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_clamp32("(alu_y)", "(alu_y)");
	alu_rshi32("alu_s", "alu_x", "alu_y");
	print("movl (alu_s), %s\n", s);
	print("# end alu_rshi\n");
}

static void alu_rshi32(char* s, char* x, char* y)
{
	print("# alu_rshi32\n");
	debug_id();

	print("movl $0, %%eax\n");

	print("movl $0, (alu_s0)\n");
	print("movl $0, (alu_s1)\n");
	print("movl $0, (alu_s2)\n");
	print("movl $0, (alu_s3)\n");

	print("movl (%s), %%edx\n", y);
	print("movl alu_rshu8(,%%edx,4), %%edx\n");

	print("movb (%s+0), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s0-3)\n");

	print("movb (%s+1), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s1-2)\n");

	print("movb (%s+2), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s2-1)\n");

	print("movb (%s+3), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_s3-0)\n");

	/* sign */
	print("movl (%s), %%edx\n", y);
	print("movl alu_rshi8s(,%%edx,4), %%edx\n");

	print("movb (%s+3), %%al\n", x);
	print("movl (%%edx,%%eax,4), %%ecx\n");
	print("movl %%ecx, (alu_ss)\n");

	alu_bor32(s, "alu_s0", "alu_s1");
	alu_bor32(s, s, "alu_s2");
	alu_bor32(s, s, "alu_s3");
	alu_bor32(s, s, "alu_ss");

	print("# end alu_rshi32\n");
}

static void alu_clamp32(char* s, char* x)
{
	print("# alu_clamp32\n");
	debug_id();

	/* clamp to 9 bits */
	print("movl %s, %%eax\n", x);
	print("movl %%eax, (alu_sx)\n");
	print("movl $0, (alu_sc)\n");
	alu_bor8("alu_sc", "alu_sc", "alu_sx+1", 0);
	alu_bor8("alu_sc", "alu_sc", "alu_sx+2", 0);
	alu_bor8("alu_sc", "alu_sc", "alu_sx+3", 0);
	print("movl (alu_sc), %%eax\n");
	print("movb alu_true(%%eax), %%al\n");
	print("movl $0, (alu_sc)\n");
	print("movb %%al, (alu_sc+1)\n");
	print("movb (alu_sx+0), %%al\n");
	print("movb %%al, (alu_sc+0)\n");
	print("movl (alu_sc), %%eax\n");

	/* clamp to 5 bits */
	print("movl alu_clamp32(,%%eax,4), %%eax\n");

	print("movl %%eax, %s\n", s);

	print("# end alu_clamp32\n");
}

/* MUL */

/* mul reg, reg */
static void alu_mul(char* s, char* x, char* y)
{
	print("# alu_mul\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	alu_mul32("alu_s", "alu_x", "alu_y", "alu_c");
	print("movl (alu_s), %s\n", s);
	print("# end alu_mul\n");
}

static void alu_mul8(char* s, int s_off, char* x, int x_off, char* y, int y_off, char* c)
{
	print("# alu_mul8\n");
	debug_id();
	print("movl $0, %%eax\n");
	print("movl $0, %%ebx\n");
	print("movl $0, %%ecx\n");
	print("movl $0, %%edx\n");

	//TODO: redo this to be more like the sums array where you just use shifts
	//and grab both the sum and carry at once... probably should do the same
	//with add
	print("movb (%s+%d), %%al\n", x, x_off);
	print("movb (%s+%d), %%dl\n", y, y_off);
	print("movl alu_mul_mul8l(,%%eax,4), %%ebx\n");
	print("movb (%%ebx,%%edx), %%cl\n");
	print("movl alu_mul_mul8h(,%%eax,4), %%ebx\n");
	print("movb (%%ebx,%%edx), %%al\n");
	print("movl $0, %%ebx\n");
	print("movb (%s), %%dl\n", c);
	print("movb alu_mul_sum8l(%%ecx,%%edx), %%dl\n");
	print("movb %%dl, (%s+%d)\n", s, s_off);
	print("movb (%s), %%dl\n", c); /* dup necessary to stick to dword ebx */
	print("movb alu_mul_sum8h(%%ecx,%%edx), %%dl\n");
	print("movb alu_mul_sum8l(%%edx,%%eax), %%dl\n");
	print("movb %%dl, (%s)\n", c);
	print("# end alu_mul8\n");
}

static void alu_mul32(char* s, char* x, char* y, char* c)
{
	print("# alu_mul32\n");
	debug_id();

	print("movl $0, (alu_z0)\n");
	print("movl $0, (alu_z1)\n");
	print("movl $0, (alu_z2)\n");
	print("movl $0, (alu_z3)\n");

	print("movl $0, (%s)\n", c);
	alu_mul8("alu_z0", 0, x, 0, y, 0, c);
	alu_mul8("alu_z0", 1, x, 1, y, 0, c);
	alu_mul8("alu_z0", 2, x, 2, y, 0, c);
	alu_mul8("alu_z0", 3, x, 3, y, 0, c);

	print("movl $0, (%s)\n", c);
	alu_mul8("alu_z1", 1, x, 0, y, 1, c);
	alu_mul8("alu_z1", 2, x, 1, y, 1, c);
	alu_mul8("alu_z1", 3, x, 2, y, 1, c);

	print("movl $0, (%s)\n", c);
	alu_mul8("alu_z2", 2, x, 0, y, 2, c);
	alu_mul8("alu_z2", 3, x, 1, y, 2, c);

	print("movl $0, (%s)\n", c);
	alu_mul8("alu_z3", 3, x, 0, y, 3, c);

	print("movl $0, (%s)\n", c);
	alu_add8n(s, 0, c, 2, "alu_z0", 0, "alu_c", 0);
	alu_add8n(s, 1, c, 3, "alu_z0", 1, "alu_z1", 1, "alu_c", 0);
	alu_add8n(s, 2, c, 4, "alu_z0", 2, "alu_z1", 2, "alu_z2", 2, "alu_c", 0);
	alu_add8n(s, 3, c, 5, "alu_z0", 3, "alu_z1", 3, "alu_z2", 3, "alu_z3", 3, "alu_c", 0);

	print("# end alu_mul32\n");
}

static void alu_add8n(char* s, int s_off, char* c, int n, char* x, int x_off, char* y, int y_off, ...)
{
	va_list args;
	int i;

	va_start(args, y_off);

	print("# alu_add8n\n");
	debug_id();

	print("movl $0, %%ebx\n");
	print("movl $0, %%edx\n");
	print("movl $0, %%eax\n");

	print("movb (%s+%d), %%al\n", x, x_off);
	print("movb (%s+%d), %%dl\n", y, y_off);
	print("movl alu_mul_shl2(,%%eax,4), %%eax\n");
	print("movl alu_mul_shl2(,%%edx,4), %%edx\n");
	print("movl alu_mul_sums(%%eax,%%edx), %%edx\n");

	for (i=2; i<n; i++) {
		char* p=va_arg(args,char*);
		int o=va_arg(args,int);
		print("movl $0, %%eax\n");
		print("movb (%s+%d), %%al\n", p, o);
		print("movl alu_mul_shl2(,%%edx,4), %%edx\n");
		print("movl alu_mul_shl2(,%%eax,4), %%eax\n");
		print("movl alu_mul_sums(%%eax,%%edx), %%edx\n");
	}

	print("movb %%dl, (%s+%d)\n", s, s_off);
	print("movb %%dh, (%s)\n", c);

	print("# end alu_add8n\n");

	va_end(args);
}

/* DIV */

/* div reg, reg */
static void alu_udiv(char* s, char* n, char* d)
{
	print("# alu_udiv\n");
	debug_id();
	alu_divrem(n, d);
	print("movl (alu_q), %s\n", s);
	print("# end alu_udiv\n");
}

static void alu_idiv(char* s, char* n, char* d)
{
	print("# alu_idiv\n");
	debug_id();

	print("movl %s, (alu_n)\n", n);
	print("movl %s, (alu_d)\n", d);

	print("movl $0, %%eax\n");
	print("movb (alu_n+3), %%al\n");
	print("movl alu_b7(,%%eax,4), %%eax\n");
	print("movl %%eax, (alu_ns)\n");

	print("movl $0, %%eax\n");
	print("movb (alu_d+3), %%al\n");
	print("movl alu_b7(,%%eax,4), %%eax\n");
	print("movl %%eax, (alu_ds)\n");

	alu_bxor8("alu_qs", "alu_ns", "alu_ds", 0);

	print("movl (alu_n), %%ecx\n");
	alu_neg("%ecx", "%ecx");
	print("movl $alu_n, %%eax\n");
	print("movl (alu_ns), %%edx\n");
	select_data("%eax", "%edx");
	print("movl %%ecx, (%%eax)\n");

	print("movl (alu_d), %%ecx\n");
	alu_neg("%ecx", "%ecx");
	print("movl $alu_d, %%eax\n");
	print("movl (alu_ds), %%edx\n");
	select_data("%eax", "%edx");
	print("movl %%ecx, (%%eax)\n");

	print("movl (alu_n), %%eax\n");
	print("movl (alu_d), %%edx\n");
	alu_divrem("%eax", "%edx");

	print("movl (alu_q), %%ecx\n");
	alu_neg("%ecx", "%ecx");
	print("movl $alu_q, %%eax\n");
	print("movl (alu_qs), %%edx\n");
	select_data("%eax", "%edx");
	print("movl %%ecx, (%%eax)\n");

	print("movl (alu_q), %s\n", s);
	print("# end alu_idiv\n");
}

static void alu_divrem(char* n, char* d)
{
	int bit;

	print("# alu_divrem\n");
	debug_id();

	print("movl %s, (alu_n)\n", n);
	print("movl %s, (alu_d)\n", d);

	/* if D==0 then error(DivisionByZeroException) end */

	/* Q := 0 */
	/* R := 0 */
	print("movl $0, (alu_q)\n");
	print("movl $0, (alu_r)\n");

	/* expanding the loop like this makes the individual division run faster,
	 * but the overall program run slower.  may want to think about passing off
	 * the loop to the program */

	/* for i=n-1...0 do */
	for (bit=31; bit>=0; bit--) {
		/* R := R << 1 */
		/* R(0) := N(i) */

		alu_bit("alu_c", "alu_n", bit);
		alu_div_shl1_32_c("alu_r", "alu_c");

		/* if R>=D then */

		alu_div_gte32("alu_t", "alu_r", "alu_d", "alu_c");

		print("movl (alu_t), %%eax\n");
		print("movl alu_sel_r(,%%eax,4), %%edx\n");
		print("movl %%edx, (alu_psel_r)\n");
		print("movl alu_sel_q(,%%eax,4), %%edx\n");
		print("movl %%edx, (alu_psel_q)\n");

		/* R := R-D */
		
		print("movl (alu_psel_r), %%eax\n");
		print("movl (%%eax), %%eax\n");
		print("movl %%eax, (alu_sr)\n");
		print("movl (alu_sr), %%eax\n");
		print("movl %%eax, (alu_x)\n");
		print("movl (alu_d), %%eax\n");
		print("movl %%eax, (alu_y)\n");
		alu_sub32("alu_sr", "alu_x", "alu_y", "alu_c");
		print("movl (alu_psel_r), %%eax\n");
		print("movl (alu_sr), %%edx\n");
		print("movl %%edx, (%%eax)\n");

		/* Q(i) := 1 */

		print("movl (alu_psel_q), %%eax\n");
		print("movl (%%eax), %%eax\n");
		print("movl %%eax, (alu_sq)\n");
		alu_div_setb32("alu_sq", bit);
		print("movl (alu_psel_q), %%eax\n");
		print("movl (alu_sq), %%edx\n");
		print("movl %%edx, (%%eax)\n");
	}

	print("# end alu_divrem\n");
}

static void alu_div_setb32(char* s, int n)
{
	print("# alu_div_setb32\n");
	debug_id();
	print("movl $0, %%eax\n");
	if (n<8) {
		print("movb (%s+0), %%al\n",s);
		print("movb alu_b_s_%d(%%eax), %%al\n", n-0);
		print("movb %%al, (%s+0)\n",s);
	}
	else if (n<16) {
		print("movb (%s+1), %%al\n",s);
		print("movb alu_b_s_%d(%%eax), %%al\n", n-8);
		print("movb %%al, (%s+1)\n",s);
	}
	else if (n<24) {
		print("movb (%s+2), %%al\n",s);
		print("movb alu_b_s_%d(%%eax), %%al\n", n-16);
		print("movb %%al, (%s+2)\n",s);
	}
	else if (n<32) {
		print("movb (%s+3), %%al\n",s);
		print("movb alu_b_s_%d(%%eax), %%al\n", n-24);
		print("movb %%al, (%s+3)\n",s);
	}
	print("# end alu_div_setb32\n");
}

static void alu_div_shl1_8_c(char* s, int s_off, char* c)
{
	print("# alu_div_shl1_8_c\n");
	debug_id();
	print("movl $0, %%eax\n");
	print("movl $0, %%edx\n");
	print("movb (%s+%d), %%al\n", s, s_off); 
	print("movb (%s), %%dl\n", c); 
	print("movl alu_div_shl3_8_d(,%%eax,4), %%eax\n");
	print("movl alu_div_shl1_8_c_d(%%eax,%%edx,4), %%eax\n");
	print("movb %%al, (%s+%d)\n", s, s_off);
	print("movb %%ah, (%s)\n", c);
	print("# end alu_div_shl1_8_c\n");
}

static void alu_div_shl1_32_c(char* s, char* c)
{
	print("# alu_div_shl1_32_c\n");
	debug_id();
	alu_div_shl1_8_c(s, 0, c);
	alu_div_shl1_8_c(s, 1, c);
	alu_div_shl1_8_c(s, 2, c);
	alu_div_shl1_8_c(s, 3, c);
	print("# end alu_div_shl1_32_c\n");
}

static void alu_bit(char* s, char* x, int n)
{
	print("# alu_bit\n");
	debug_id();
	print("movl $0, %%edx\n");
	if (n<8) {
		print("movb (%s+0), %%dl\n", x);
		print("movl alu_b%d(,%%edx,4), %%eax\n", n-0);
	}
	else if (n<16) {
		print("movb (%s+1), %%dl\n", x);
		print("movl alu_b%d(,%%edx,4), %%eax\n", n-8);
	}
	else if (n<24) {
		print("movb (%s+2), %%dl\n", x);
		print("movl alu_b%d(,%%edx,4), %%eax\n", n-16);
	}
	else if (n<32) {
		print("movb (%s+3), %%dl\n", x);
		print("movl alu_b%d(,%%edx,4), %%eax\n", n-24);
	}
	print("movl %%eax, (%s)\n", s);
	print("# end alu_bit\n");
}

static void alu_div_gte32(char* s, char* x, char* y, char* c)
{
	print("# alu_div_gte32\n");
	debug_id();
	print("movl $0, (%s)\n", c);
	print("movl (%s), %%eax\n", x);
	print("movl %%eax, (alu_x)\n");
	print("movl (%s), %%eax\n", y);
	print("movl %%eax, (alu_y)\n");
	alu_sub32(s,"alu_x","alu_y",c);
	print("movl $0, %%eax\n");
	print("movb (%s), %%al\n", c);
	print("movb alu_true(%%eax), %%al\n");
	print("movl %%eax, (%s)\n", s);
	print("# end alu_div_gte32\n");
}

/* MOD */

/* mod reg, reg */
static void alu_umod(char* s, char* n, char* d)
{
	print("# alu_umod\n");
	debug_id();
	alu_divrem(n, d);
	print("movl (alu_r), %s\n", s);
	print("# end alu_umod\n");
}

/* mod reg, reg */
static void alu_imod(char* s, char* n, char* d)
{
	print("# alu_imod\n");
	debug_id();

	print("movl %s, (alu_n)\n", n);
	print("movl %s, (alu_d)\n", d);

	print("movl $0, %%eax\n");
	print("movb (alu_n+3), %%al\n");
	print("movl alu_b7(,%%eax,4), %%eax\n");
	print("movl %%eax, (alu_ns)\n");

	print("movl $0, %%eax\n");
	print("movb (alu_d+3), %%al\n");
	print("movl alu_b7(,%%eax,4), %%eax\n");
	print("movl %%eax, (alu_ds)\n");

	print("movl (alu_ns), %%eax\n");
	print("movl %%eax, (alu_rs)\n");

	print("movl (alu_n), %%ecx\n");
	alu_neg("%ecx", "%ecx");
	print("movl $alu_n, %%eax\n");
	print("movl (alu_ns), %%edx\n");
	select_data("%eax", "%edx");
	print("movl %%ecx, (%%eax)\n");

	print("movl (alu_d), %%ecx\n");
	alu_neg("%ecx", "%ecx");
	print("movl $alu_d, %%eax\n");
	print("movl (alu_ds), %%edx\n");
	select_data("%eax", "%edx");
	print("movl %%ecx, (%%eax)\n");

	print("movl (alu_n), %%eax\n");
	print("movl (alu_d), %%edx\n");
	alu_divrem("%eax", "%edx");

	print("movl (alu_r), %%ecx\n");
	alu_neg("%ecx", "%ecx");
	print("movl $alu_r, %%eax\n");
	print("movl (alu_rs), %%edx\n");
	select_data("%eax", "%edx");
	print("movl %%ecx, (%%eax)\n");

	print("movl (alu_r), %s\n", s);
	print("# end alu_imod\n");
}

/* CMP */

/* cmp reg, reg */
static void alu_cmp(char* x, char* y)
{
	print("# alu_cmp\n");
	debug_id();

	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	print("movl %s, (alu_t)\n", y);
	alu_sub32("alu_s", "alu_x", "alu_y", "alu_c");

	print("movl (alu_t), %%eax\n"); /* sub32 destroys y */
	print("movl %%eax, (alu_y)\n"); 

	print("movl $0, %%eax\n");

	print("movb (alu_c), %%al\n");
	print("movb alu_false(%%eax), %%al\n"); /* we actually performed an add, not subtract */
	print("movb %%al, (cf)\n");

	print("movb (alu_s+3), %%al\n");
	print("movl alu_b7(,%%eax,4), %%eax\n");
	print("movb %%al, (sf)\n");

	print("movl $0, %%eax\n");
	print("movl $0, %%edx\n");
	print("movb (alu_s+0), %%dl\n");
	print("movb alu_true(%%edx,%%eax), %%al\n");
	print("movb (alu_s+1), %%dl\n");
	print("movb alu_true(%%edx,%%eax), %%al\n");
	print("movb (alu_s+2), %%dl\n");
	print("movb alu_true(%%edx,%%eax), %%al\n");
	print("movb (alu_s+3), %%dl\n");
	print("movb alu_true(%%edx,%%eax), %%al\n");
	print("movb alu_false(%%eax), %%al\n");
	print("movb %%al, (zf)\n");

	print("movl $alu_cmp_of, %%edx\n");
	print("movb (alu_x+3), %%al\n");
	print("movl alu_b7(,%%eax,4), %%eax\n");
	print("movl (%%edx,%%eax,4), %%edx\n");
	print("movb (alu_y+3), %%al\n");
	print("movl alu_b7(,%%eax,4), %%eax\n");
	print("movl (%%edx,%%eax,4), %%edx\n");
	print("movb (alu_s+3), %%al\n");
	print("movl alu_b7(,%%eax,4), %%eax\n");
	print("movl (%%edx,%%eax,4), %%edx\n");
	print("movl (%%edx), %%edx\n");
	print("movb %%dl, (of)\n");

	print("# end alu_cmp\n");
}

/* eq reg, reg */
static void alu_eq(char* s, char* x, char* y)
{
	print("# alu_eq\n");
	debug_id();
	print("movl %s, (alu_x)\n", x);
	print("movl %s, (alu_y)\n", y);
	print("movl $0, %%eax\n");
	print("movl $0, %%ecx\n");
	print("movl $0, %%edx\n");
	print("movb (alu_x+0), %%al\n");
	print("movl alu_eq(,%%eax,4), %%ecx\n");
	print("movb (alu_y+0), %%dl\n");
	print("movb (%%ecx,%%edx), %%dl\n");
	print("movl %%edx, (b0)\n");
	print("movb (alu_x+1), %%al\n");
	print("movl alu_eq(,%%eax,4), %%ecx\n");
	print("movb (alu_y+1), %%dl\n");
	print("movb (%%ecx,%%edx), %%dl\n");
	print("movl %%edx, (b1)\n");
	print("movb (alu_x+2), %%al\n");
	print("movl alu_eq(,%%eax,4), %%ecx\n");
	print("movb (alu_y+2), %%dl\n");
	print("movb (%%ecx,%%edx), %%dl\n");
	print("movl %%edx, (b2)\n");
	print("movb (alu_x+3), %%al\n");
	print("movl alu_eq(,%%eax,4), %%ecx\n");
	print("movb (alu_y+3), %%dl\n");
	print("movb (%%ecx,%%edx), %%dl\n");
	print("movl %%edx, (b3)\n");
	alu_bool("and", "b0", "b0", "b1");
	alu_bool("and", "b0", "b0", "b2");
	alu_bool("and", "b0", "b0", "b3");
	print("movl (b0), %%eax\n");
	print("movl %%eax, %s\n", s);
	print("# end alu_eq\n");
}

static void alu_bool(char* op, char* s, char* x, char* y)
{
	print("# alu_bool\n");
	debug_id();
	print("movl (%s), %%eax\n", x);
	print("movl (%s), %%edx\n", y);
	print("movl %s(,%%eax,4), %%eax\n", op);
	print("movl (%%eax,%%edx,4), %%eax\n");
	print("movl %%eax, (%s)\n", s);
	print("# end alu_bool\n");
}

static void alu_not(char* s, char* x)
{
	print("# alu_not\n");
	debug_id();
	print("movl (%s), %%eax\n", x);
	print("movl alu_false(,%%eax,4), %%eax\n");
	print("movl %%eax, (%s)\n", s);
	print("# end alu_not\n");
}

static void jmp_eqi(char* a, char* x, char* y)
{
	/* zf=1 */
	print("# jmp_eqi\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_bool("and", "b0", "zf", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_eqi\n");
}

static void jmp_equ(char* a, char* x, char* y)
{
	/* zf=1 */
	print("# jmp_equ\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_bool("and", "b0", "zf", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_equ\n");
}

static void jmp_gei(char* a, char* x, char* y)
{
	/* sf=of */
	print("# jmp_gei\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_bool("xnor", "b0", "sf", "of");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_gei\n");
}

static void jmp_geu(char* a, char* x, char* y)
{
	/* cf=0 */
	print("# jmp_geu\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_not("b0", "cf");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_geu\n");
}

static void jmp_gti(char* a, char* x, char* y)
{
	/* zf=0 && sf=of */
	print("# jmp_gti\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_not("b0", "zf");
	alu_bool("xnor", "b1", "sf", "of");
	alu_bool("and", "b0", "b0", "b1");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_gti\n");
}

static void jmp_gtu(char* a, char* x, char* y)
{
	/* cf=0 && zf=0 */
	print("# jmp_gtu\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_not("b0", "cf");
	alu_not("b1", "zf");
	alu_bool("and", "b0", "b0", "b1");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_gtu\n");
}

static void jmp_lei(char* a, char* x, char* y)
{
	/* zf=1 || sf!=of */
	print("# jmp_lei\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_bool("xor", "b0", "sf", "of");
	alu_bool("or", "b0", "b0", "zf");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_lei\n");
}

static void jmp_leu(char* a, char* x, char* y)
{
	/* cf=1 || zf=1 */
	print("# jmp_leu\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_bool("or", "b0", "cf", "zf");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_leu\n");
}

static void jmp_lti(char* a, char* x, char* y)
{
	/* sf!=of */
	print("# jmp_lti\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_bool("xor", "b0", "sf", "of");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_lti\n");
}

static void jmp_ltu(char* a, char* x, char* y)
{
	/* cf=1 */
	print("# jmp_ltu\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_bool("and", "b0", "cf", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_ltu\n");
}

static void jmp_nei(char* a, char* x, char* y)
{
	/* zf=0 */
	print("# jmp_nei\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_not("b0", "zf");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_nei\n");
}

static void jmp_neu(char* a, char* x, char* y)
{
	/* zf=0 */
	print("# jmp_neu\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	alu_cmp(x, y);
	alu_not("b0", "zf");
	alu_bool("and", "b0", "b0", "on");
	store_target("(branch_temp)", "(b0)");
	store_jmp_regs("(b0)");
	execution_off("(b0)");
	print("# end jmp_neu\n");
}

static void jmp_jumpv(char* a)
{
	print("# jmp_jumpv\n");
	debug_id();
	print("movl %s, (branch_temp)\n", a);
	store_target("(branch_temp)", "(on)");
	store_jmp_regs("(on)");
	execution_off("(on)");
	print("# end jmp_jumpv\n");
}

static void jmp_extern(char* f)
{
	if (mov_extern) {
		print("movl (sp), %%esp %s\n", MARKER(REQ));
		print("movl $%s, (external)\n", f);
		print("movl (on), %%eax\n");
		print("movl fault(,%%eax,4), %%eax\n");
		print("movl (%%eax), %%eax\n");
	}
	else {
		print("movl (sp), %%esp %s\n", MARKER(REQ));
		print("cmpl $1, (on)\n");
		print("je %s\n", f);
	}
}

static void pop_args(int bytes)
{
	print("# pop args (%d)\n", bytes);
	print("movl (sp), %%eax\n");
	if ((bytes%4)==0 && bytes<STACK_EX_THRESH) {
		int i;
		for (i=0; i<bytes; i+=4) {
			print("movl pop(%%eax), %%eax\n");
		}
	}
	else {
		print("movl $%d, %%edx\n", bytes);
		alu_add("%eax", "%eax", "%edx");
	}
	print("movl %%eax, (stack_temp)\n");
	print("movl $sp, %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (stack_temp), %%edx\n");
	print("movl %%edx, (%%eax)\n");
	print("# end pop args\n");
}

static void fpu_cmp32(char* f, char* b, Node p, int nt)
{
	print("movl "); emit_kid(p,1,nt); print(", %%eax\n");
	push("%eax");
	print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
	push("%eax");
	call_lib(f);
	pop_args(2*4);
	//TODO: clean this up, it's ridiculous after edi->R0 switch
	print("movl (R0), %%edx\n");
	print("movl $0, %%eax\n");
	print("movb %%dl, %%al\n");
	print("movb alu_true(%%eax), %%al\n");
	print("movl %%eax, %s\n", b);
}

static void fpu_cmp64(char* f, char* b, Node p, int nt)
{
	pushm8(p->kids[1]->syms[RX]->x.name);
	pushm8(p->kids[0]->syms[RX]->x.name);
	call_lib(f);
	pop_args(2*8);
	//TODO: clean this up, it's ridiculous after edi->R0 switch
	print("movl (R0), %%edx\n");
	print("movl $0, %%eax\n");
	print("movb %%dl, %%al\n");
	print("movb alu_true(%%eax), %%al\n");
	print("movl %%eax, %s\n", b);
}

static void cc_branch(char* t, char* b)
{
	if (mov_flow) {
		print("movl $%s-0x%x, (branch_temp)\n", t, MOV_OFFSET);
		alu_bool("and", "b0", b, "on");
		store_target("(branch_temp)", "(b0)");
		store_jmp_regs("(b0)");
		execution_off("(b0)");
	}
	else {
		print("cmpl $1, (%s)\n", b);
		print("je %s\n", t);
	}
}

static void call_lib(char* f)
{
	int l=genlabel(1);
	print("\n# emit CALL (lib '%s')\n\n", f);
	debug_id();

	/* push return */
	print("# push return\n");
	if (!mov_flow) {
		print("movl $.%s%d, %%eax\n", I_LABEL, l);
		push("%eax");
	}
	else {
		print("movl $.%s%d-0x%x, %%eax\n", I_LABEL, l, MOV_OFFSET);
		push("%eax");
	}
	print("# end push return\n\n");

	/* generate call */
	if (!mov_flow) {
		print("jmp %s\n", f);
	}
	else {
		print("movl $%s-0x%x, %%eax\n", f, MOV_OFFSET);
		jmp_jumpv("%eax");
	}

	/* generate return target */
	print(".%s%d:\n", I_LABEL, l);
	if (mov_flow) {
		print("movl (target), %%eax\n");
		print("movl $.%s%d-0x%x, %%edx\n", I_LABEL, l, MOV_OFFSET); 
		alu_eq("(b0)", "%eax", "%edx");
		load_jmp_regs("(b0)");
		execution_on("(b0)");
	}

	print("\n# end call\n\n");
}

static int mov_getrule(Node p, int nt)
{
	int rulenum;
	assert(p);
	rulenum=(*IR->x._rule)(p->x.state, nt);
	if (!rulenum) {
		fprint(stderr, "(%x->op=%s at %w is corrupt.)\n", p, op_name(p->op), &src);
		assert(0);
	}
	return rulenum;
}

static Node mov_reuse(Node p, int nt)
{
	struct _state {
		short cost[1];
	};
	Symbol r=p->syms[RX];

	if (generic(p->op) == INDIR && p->kids[0]->op == VREG+P && 
			r->u.t.cse && p->x.mayrecalc && 
			((struct _state*)r->u.t.cse->x.state)->cost[nt] == 0) {
		return r->u.t.cse;
	}
	else {
		return p;
	}
}

/* the need for a better emitter is frustrating; emit2 simply does not have
 * enough control over the emit process, because it is not passed the
 * non-terminal number (in the way that emit is) - so it is difficult to resolve
 * detailed information about the rest of the tree in emit2.  all we need is for
 * emit2 to get the same information as emit (one extra parameter), which
 * requires either patching lcc (and probably breaking the other backends) or
 * overriding the default emitter, which is what we do.  */

static unsigned mov_emitter(Node p, int nt)
{
	int rulenum;
	short *nts;
	char *fmt;
	Node kids[10];

	if (!quiet) {
		fprint(stderr, "emit/mov>");
		dump_tree(stderr,p);
		fprint(stderr, "\n");
	}

	p=mov_reuse(p, nt);
	rulenum=mov_getrule(p, nt);
	nts=IR->x._nts[rulenum];
	fmt=IR->x._templates[rulenum];
	assert(fmt);

	if (IR->x._isinstruction[rulenum] && !p->x.emitted) {
		print("# emit/mov>");
		dump_tree(stdout,p);
		print("\n");
	}

	if (IR->x._isinstruction[rulenum] && p->x.emitted) {
		print("%s", p->syms[RX]->x.name);
	}
	else if (*fmt=='#') {
		/* (*IR->x.emit2)(p); */
		emit2_nt(p, nt);
	}
	else {
		if (*fmt=='?') {
			fmt++;
			assert(p->kids[0]);
			if (p->syms[RX] == p->x.kids[0]->syms[RX]) {
				while (*fmt++ != '\n')
					;
			}
		}
		for ((*IR->x._kids)(p, rulenum, kids); *fmt; fmt++) {
			if (*fmt!='%') {
				(void)putchar(*fmt);
			}
			else if (*++fmt=='F') {
				print("%d", framesize);
			}
			else if (*fmt>='0' && *fmt<='9') {
				mov_emitter(kids[*fmt-'0'], nts[*fmt-'0']);
			}
			else if (*fmt>='a' && *fmt<'a'+NELEMS(p->syms)) {
				fputs(p->syms[*fmt-'a']->x.name, stdout);
			}
			else {
				(void)putchar(*fmt);
			}
		}
	}

	return 0;
}

static void emit_kid(Node p, int kid, int nt)
{
	int rulenum=mov_getrule(p, nt);
	short *nts=(IR->x._nts)[rulenum];
	mov_emitter(p->kids[kid], nts[kid]);
}

static void emit_sym(Node p, int sym, int nt)
{
	fputs(p->syms[sym]->x.name, stdout);
}

static void print_block_header(int op)
{
	print("\n# emit ");
	print(op_name(op));
	print("\n\n");
}

static void print_block_footer(int op)
{
	print("\n# end emit ");
	print(op_name(op));
	print("\n\n");
}

static char* op_name(int op) {
	static char* opnames[]={
		"",
		"cnst",
		"arg",
		"asgn",
		"indir",
		"cvc",
		"cvd",
		"cvf",
		"cvi",
		"cvp",
		"cvs",
		"cvu",
		"neg",
		"call",
		"load",
		"ret",
		"addrg",
		"addrf",
		"addrl",
		"add",
		"sub",
		"lsh",
		"mod",
		"rsh",
		"band",
		"bcom",
		"bor",
		"bxor",
		"div",
		"mul",
		"eq",
		"ge",
		"gt",
		"le",
		"lt",
		"ne",
		"jump",
		"label",
		"and",
		"not",
		"or",
		"cond",
		"right",
		"field"
	},
	*suffixes[] = {
		"0",
		"f",
		"d",
		"c",
		"s",
		"i",
		"u",
		"p",
		"v",
		"b",
		"10",
		"11",
		"12",
		"13",
		"14",
		"15"
	};

	if (generic(op)>=AND && generic(op)<=FIELD && opsize(op)==0) {
		return opnames[opindex(op)];
	}
	return 
		stringf("%s%s%s",
			opindex(op)>0 && opindex(op)<NELEMS(opnames)?
				opnames[opindex(op)]:stringd(opindex(op)),
				suffixes[optype(op)],opsize(op)>0?stringd(opsize(op)):"");
}

static void dump_tree(FILE* f, Node p)
{
	if (p->op==VREG+P && p->syms[0]) {
		fprint(f, "vregp(%s)", p->syms[0]->name);
		return;
	}
	else if (generic(p->op)==LOAD) {
		fprint(f, "load("); dump_tree(f,p->kids[0]); fprint(f, ")");
		return;
	}
	fprint(f, "%s(", op_name(p->op));
	switch (generic(p->op)) {
		case CNST:
		case LABEL:
		case ADDRG: case ADDRF: case ADDRL:
			if (p->syms[0]) {
				fprint(f, "%s", p->syms[0]->name);
			}
			break;
		case RET:
			if (p->kids[0]) {
				dump_tree(f,p->kids[0]);
			}
			break;
		case CVF: case CVI: case CVP: case CVU:
		case JUMP: 
		case ARG:
		case BCOM:
		case NEG:
		case INDIR:
			dump_tree(f,p->kids[0]);
			break;
		case CALL:
			if (optype(p->op)!=B) {
				dump_tree(f,p->kids[0]);
				break;
			}
			/* else fall thru */
		case EQ: case NE: case GT: case GE: case LE: case LT:
		case ASGN:
		case BOR: case BAND: case BXOR: case RSH: case LSH:
		case ADD: case SUB:  case DIV: case MUL: case MOD:
			dump_tree(f,p->kids[0]); fprint(f, ","); dump_tree(f,p->kids[1]);
			break;
		default:
			assert(0);
	}
	fprint(f, ")");
}

static void emit_start(void)
{
	int l;

	(*IR->segment)(MOV_CODE);

	/* environment setup */
	if (crt0) {

		print("\n");
		print("#\n");
		print("# environment setup\n");
		print("#\n");
		print("\n");

		/* important note on non-mov instructions */
		/*
		 * the environment setup here occurs outside of the main execution loop.
		 * it simply sets up the environment, and performs none of the program's
		 * actions.  in this sense, it is not actually part of the program; for
		 * example, when the os sets up a stack or page tables for a process, we
		 * don't typically consider that action part of the program itself.  as
		 * such, it seems to be acceptable to have non-mov instructions in this
		 * setup - since it is not part of the 'program', it doesn't violate the
		 * spirit of the project.  these non-movs arise from our need to
		 * communicate with the kernel for environment setup.  if we were in
		 * ring 0, we wouldn't need help from the kernel, and could do this with
		 * only movs - but i don't think anyone wants a ring-0-only compiler. 
		 */
		if (mov_extern || mov_loop) {
			printf("\n");
			printf("# (see compiler notes on non-mov instructions\n");
			printf("\n");
		}

		print(".globl _start\n");
		print("_start:\n");

		print("movl %%esp, (sesp)\n");

		if (mov_extern) {
			print("movl (sp), %%esp\n");
			print("movl push(%%esp), %%esp\n");
			print("movl push(%%esp), %%esp\n");
			print("movl push(%%esp), %%esp\n");
			print("movl push(%%esp), %%esp %s\n", MARKER(REQ));

			print(".extern sigaction\n");
			print("movl $%d, (%%esp)\n", SIGSEGV);
			print("movl $sa_dispatch, 4(%%esp)\n");
			print("movl $0, 8(%%esp)\n");
			print("call sigaction\n");
			print("\n");
		}

		if (mov_loop) {
			print("movl (sp), %%esp\n");
			print("movl push(%%esp), %%esp\n");
			print("movl push(%%esp), %%esp\n");
			print("movl push(%%esp), %%esp %s\n", MARKER(REQ));

			print(".extern sigaction\n");
			print("movl $%d, (%%esp)\n", SIGILL);
			print("movl $sa_loop, 4(%%esp)\n");
			print("movl $0, 8(%%esp)\n");
			print("call sigaction\n");
			print("\n");
		}

		print("\n");
		print("#\n");
		print("# end environment setup\n");
		print("#\n");
		print("\n");

		/* launch main */

		print(".globl master_loop\n");
		print("master_loop:\n");

		print("# _start0\n");

		print(".globl _start0\n");
		print("_start0:\n");

		if (mov_loop) {
			print("movl (sp), %%esp %s\n", MARKER(REQ));
		}

		execution_on("(toggle_execution)");
		print("movl $0, (toggle_execution)\n");

		//TODO: make a function for calling, this is largely duplicated from emit2
		l=genlabel(1);

		/* push argv */
		print("# push argv\n");
		print("movl (sesp), %%eax\n");
		print("movl $4, %%edx\n");
		alu_add("%eax", "%eax", "%edx");
		push("%eax");

		/* push argc */
		print("# push argc\n");
		print("movl (sesp), %%eax\n");
		print("movl (%%eax), %%eax\n");
		push("%eax");

		/* push return, generate call */

		if (!mov_flow) {
			print("# push return\n");
			print("movl $.%s%d, %%eax\n", I_LABEL, l);
			push("%eax");
			print("# end push return\n\n");
			print("jmp main\n");
		}
		else {
			print("# push return\n");
			print("movl $.%s%d-0x%x, %%eax\n", I_LABEL, l, MOV_OFFSET);
			push("%eax");
			print("# end push return\n\n");
			print("movl $main-0x%x, %%eax\n", MOV_OFFSET);
			jmp_jumpv("%eax");
		}

		/* generate return target */

		print(".%s%d:\n", I_LABEL, l);

		if (mov_flow) {
			print("movl (target), %%eax\n");
			print("movl $.%s%d-0x%x, %%edx\n", I_LABEL, l, MOV_OFFSET);
			alu_eq("(b0)", "%eax", "%edx");
			load_jmp_regs("(b0)");
			execution_on("(b0)");
		}

		print("# exit\n");
		push("$0");
		if (!mov_flow) {
			print("movl (sp), %%esp %s\n", MARKER(REQ));
			print("jmp exit\n");
		}
		else {
			jmp_extern("exit");
		}
		print("# end exit\n");

		print("# end _start0\n");
	}
}

static void progbeg(int argc, char *argv[]) 
{
	int i;

	parseflags(argc, argv);
	for (i=0; i<argc; i++) {
		if (strcmp(argv[i], QUIET_FLAG)==0) {
			quiet=1;
		}
		if (strcmp(argv[i], MOV_ID_FLAG)==0) {
			mov_id=1;
		}
		if (strcmp(argv[i], NO_MOV_ID_FLAG)==0) {
			mov_id=0;
		}
		if (strcmp(argv[i], MOV_FLOW_FLAG)==0) {
			mov_flow=1;
		}
		if (strcmp(argv[i], NO_MOV_FLOW_FLAG)==0) {
			mov_flow=0;
		}
		if (strcmp(argv[i], MOV_EXTERN_FLAG)==0) {
			mov_extern=1;
		}
		if (strcmp(argv[i], NO_MOV_EXTERN_FLAG)==0) {
			mov_extern=0;
		}
		if (strcmp(argv[i], MOV_LOOP_FLAG)==0) {
			mov_loop=1;
		}
		if (strcmp(argv[i], NO_MOV_LOOP_FLAG)==0) {
			mov_loop=0;
		}
		if (strcmp(argv[i], CRT0_FLAG)==0) {
			crt0=1;
		}
		if (strcmp(argv[i], CRTF_FLAG)==0) {
			crtf=1;
		}
		if (strcmp(argv[i], CRTD_FLAG)==0) {
			crtd=1;
		}
		if (strcmp(argv[i], CRT_FLAG)==0) {
			crt0=1;
			crtf=1;
			crtd=1;
		}
	}

	if (!quiet) {
		emit_header(stderr,0);
	}

	emit_header(stdout,1);

	if (!quiet) {
		fprint(stderr, "M/o/Vfuscation started...\n\n");
	}

	/* override the default emitter in src/gen.c */
	extern unsigned (*emitter)(Node, int);
	emitter=mov_emitter;

	/* create the registers available to the compiler */
	for (i=0; i<SOFT_I_REGS; i++) {
		reg_int[i]=mkreg(stringf("(R%d)",i), i, 1, IREG);
	}
	for (i=0; i<SOFT_F_REGS; i++) {
		reg_float[i]=mkreg(stringf("(F%d)",i), i, 1, FREG);
	}
	for (i=0; i<SOFT_D_REGS; i++) {
		/* no parens allows us to access high/low separately */
		/* (sort of) quirk of at&t - 4(X) is okay if X is a register, not if
		 * it's a constant ... complicates the syntax when we don't know which
		 * X will be */
		/* but no parens just looks weird to me ... at&t :( */
		reg_double[i+SOFT_F_REGS]=mkreg(stringf("D%d",i), i+SOFT_F_REGS, 1, FREG);
	}

	//TODO: fix this since R0 switch
	/* allow R0 to be used as a byte and short registers - we'll handle it in
	 * the translation */
	reg_char[R0]=mkreg("(R0)", R0, 1, IREG);
	reg_short[R0]=mkreg("(R0)", R0, 1, IREG);

	reg_char_wc=mkwildcard(reg_char);
	reg_short_wc=mkwildcard(reg_short);
	reg_int_wc=mkwildcard(reg_int);
	reg_float_wc=mkwildcard(reg_float);
	reg_double_wc=mkwildcard(reg_double);

	/* everyone can hold temps */
	tmask[IREG]=0xff;
	/*
	tmask[IREG]=0;
	for (i=0; i<SOFT_I_REGS; i++) {
		tmask[IREG]|=(1<<i);
	}
	*/

	/* no one can hold variables */
	vmask[IREG]=0;

	/* everyone can hold temps */
	tmask[FREG]=0xff;

	/* no one can hold variables */
	vmask[FREG]=0;

	current_segment=0;

	stabprefix=".LL";

	emit_start();

	(*IR->segment)(MOV_CODE);
}

static Symbol rmap(int opk)
{
	switch (optype(opk)) {
		case B:
	   	case P:
			return reg_int_wc;
		case I:
	   	case U:
			if (opsize(opk)==1) {
				return reg_char_wc;
			}
			else if (opsize(opk)==2) {
				return reg_short_wc;
			}
			else if (opsize(opk)==4) {
				return reg_int_wc;
			}
			else {
				assert(0);
			}
		case F:
			if (opsize(opk)==4) {
				return reg_float_wc;
			}
			else if (opsize(opk)==8) {
				return reg_double_wc;
			}
			else {
				assert(0);
			}
		default:
			return 0;
	}
}

static void globalend(void)
{
	if (last_global && last_global->type->size>0)
		print(".size %s,%d\n", last_global->x.name, last_global->type->size);
	last_global=NULL;
}

static void progend(void)
{
	globalend();
	(*IR->segment)(MOV_CODE);

	if (crtf) {
		if (mov_loop) {
			print("movl (sp), %%esp %s\n", MARKER(REQ));
			print("movw %%ax, %%cs\n");
		}
		else {
			print("jmp master_loop\n");
		}

		if (mov_extern) {
			(*IR->segment)(MOV_PLT);
			print(".globl dispatch\n");
			print("dispatch:\n");
			print("movl (sp), %%esp %s\n", MARKER(REQ));
			print("jmp *external\n");
			(*IR->segment)(MOV_CODE);
		}
	}

	if (crtd) {
		data_setup();
	}

	extern int errcnt;
	if (!quiet) {
		if (errcnt==0) {
			fprint(stderr, "\nM/o/Vfuscation complete.\n\n");
		}
		else {
			fprint(stderr, "\nM/o/Vfuscation failed.\n\n");
		}
	}
}

static void target(Node p) 
{
	assert(p);
	switch (specific(p->op)) {
		case CALL+I:
	   	case CALL+U:
	   	case CALL+P:
	   	case CALL+V:
			setreg(p, reg_int[R0]);
			break;
	   	case CALL+F:
			if (opsize(p->op)==4) {
				setreg(p, reg_float[F0]);
			}
			else if (opsize(p->op)==8) {
				setreg(p, reg_double[D0]);
			}
			else {
				assert(0);
			}
			break;
		case RET+I:
	   	case RET+U:
	   	case RET+P:
			rtarget(p, 0, reg_int[R0]);
			break;
	   	case RET+F:
			if (opsize(p->op)==4) {
                rtarget(p, 0, reg_float[F0]);
			}
			else if (opsize(p->op)==8) {
                rtarget(p, 0, reg_double[D0]);
			}
			else {
				assert(0);
			}
			break;
	}
}

static void clobber(Node p)
{
	static int nstack=0;
	assert(p);
	nstack=ckstack(p, nstack);
	switch (specific(p->op)) {
		case CALL+F:
			if (opsize(p->op)==4) {
				spill(1<<D0, FREG, p);
				spill(1<<R0, IREG, p);
			}
			else if (opsize(p->op)==8) {
				spill(1<<F0, FREG, p);
				spill(1<<R0, IREG, p);
			}
			else {
				assert(0);
			}
			break;
		case CALL+I: case CALL+U: case CALL+P: case CALL+V:
			spill(1<<F0 | 1<<D0, FREG, p);
			break;
	}
}

static int ckstack(Node p, int n) 
{
	#define isfp(p) (optype((p)->op)==F)

	int i;

	for (i=0; i<NELEMS(p->x.kids) && p->x.kids[i]; i++) {
		if (isfp(p->x.kids[i])) {
			n--;
		}
	}

	if (isfp(p) && p->count>0) {
		n++;
	}

	if (n>8) {
		error("expression too complicated\n");
	}

	assert(n>=0);

	return n;
}

static void emit2(Node p)
{
	/* the normal emit2 should not be called - expect everything to go through
	 * the new emitter, and the emit2_nt ancillary function */
	assert(0);
}

static void emit2_nt(Node p, int nt)
{
	int op=specific(p->op);

	print_block_header(op);
	debug_id();

	if (op==ADD+I || op==ADD+U || op==ADD+P) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_add("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==SUB+I || op==SUB+U || op==SUB+P) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_sub("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==BAND+I || op==BAND+U) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_band("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==BOR+I || op==BOR+U) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_bor("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==BXOR+I || op==BXOR+U) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_bxor("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==BCOM+I || op==BCOM+U) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n"); 
		alu_bcom("%eax", "%eax");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==NEG+I) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		alu_neg("%eax", "%eax");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==LSH+U) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_lshu("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==RSH+U) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_rshu("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==LSH+I) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_lshu("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==RSH+I) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_rshi("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==MUL+I || op==MUL+U) {
		/* low half of a multiply is the same for signed and unsigned */
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_mul("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==DIV+I) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_idiv("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==DIV+U) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_udiv("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==MOD+I) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_imod("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}
	else if (op==MOD+U) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		alu_umod("%eax", "%eax", "%edx");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}

	/* jumps */
	else if (op==JUMP+V) {
		if (mov_flow) {
			//TODO: cleaner checks
			if (p->kids[0]->syms[0]) {
				/* direct jump */
				print("# (direct jump)\n");
				print("movl $%s-0x%x, %%eax\n", p->kids[0]->syms[0]->x.name, MOV_OFFSET);
			}
			else {
				/* indirect jump */
				print("# (indirect jump)\n");
				print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
				//TODO: target should be statically obfuscated when loaded into
				//the register, not here
				print("movl $0x%x, %%edx\n", MOV_OFFSET);
				alu_sub("%eax", "%eax", "%edx");
			}
			jmp_jumpv("%eax");
		}
		else {
			if (p->kids[0]->syms[0]) {
				/* direct jump */
				print("jmp %s\n", p->kids[0]->syms[0]->x.name);
			}
			else {
				/* indirect jump */
				print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
				print("jmp *%%eax\n");
			}
		}
	}
	else if (op==EQ+I) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_eqi("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("je "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==GE+I) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_gei("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jge "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==GT+I) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_gti("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jg "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==LE+I) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_lei("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jle "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==LT+I) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_lti("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jl "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==NE+I) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_nei("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jne "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==EQ+U) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_equ("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("je "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==GE+U) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_geu("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jae "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==GT+U) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_gtu("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("ja "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==LE+U) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_leu("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jbe "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==LT+U) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_ltu("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jb "); emit_sym(p,0,nt); print("\n");
		}
	}
	else if (op==NE+U) {
		if (mov_flow) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
			print("movl $%s-0x%x, %%ecx\n", p->syms[0]->x.name, MOV_OFFSET);
			jmp_neu("%ecx", "%eax", "%edx");
		}
		else {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("cmpl "); emit_kid(p,1,nt); print(", %%eax\n");
			print("jne "); emit_sym(p,0,nt); print("\n");
		}
	}

	else if (op==ARG+I || op==ARG+U || op==ARG+P) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		push("%eax");
	}

	else if (op==CALL+I || op==CALL+U || op==CALL+P || op==CALL+V || op==CALL+F) {
		Symbol s;
		int ext=0;
		int l=genlabel(1);
		int direct;

		//TODO: find a better way to resolve this
		direct=(p->kids[0]->syms[0]!=NULL);

		if (direct) {
			print("# call '%s'\n" , p->kids[0]->syms[0]->x.name);
			print("# (direct call)\n");
		}
		else {
			//print("# call '%s'\n" , p->kids[0]->syms[RX]);
			print("# call '"); emit_kid(p,0,nt); print("'\n");
			print("# (indirect call)\n");
		}
		debug_id();

		if (direct) {
			s=lookup(p->kids[0]->syms[0]->x.name, identifiers);
			ext=!s||s->sclass==EXTERN;

			print("# %s is %s\n", p->kids[0]->syms[0]->x.name, ext?"external":"internal");
		}
		else {
			// for now, assume internal
			//TODO: resolve internal/external on indirect calls
			ext=0;
		}

		/* make a basic effort to hide control flow.  instead of loading the
		 * actual return address, load an obfuscated address and correct it
		 * dynamically.  this prevents ida from breaking apart our one giant
		 * node.  internal call/rets do not need real addresses, so this is a
		 * non-issue with internal calls */
		/* an understandable (albeit frustrating) limitation exists - gnu as
		 * only allows addition/subtraction here */

		/* push return */

		print("# push return\n");
		if (!mov_flow) {
			print("movl $.%s%d, %%eax\n", ext?E_LABEL:I_LABEL, l);
			push("%eax");
		}
		else if (ext) {
			print("movl $.%s%d-0x%x, %%eax\n", ext?E_LABEL:I_LABEL, l, MOV_OFFSET);
			alu_add("%eax", "%eax", CNST(MOV_OFFSET));
			push("%eax");
		}
		else {
			print("movl $.%s%d-0x%x, %%eax\n", ext?E_LABEL:I_LABEL, l, MOV_OFFSET);
			push("%eax");
		}
		print("# end push return\n\n");

		/* generate call */

		if (ext) {
			print("# (external call)\n");
			if (!mov_flow) {
				print("movl (sp), %%esp %s\n", MARKER(REQ));
				print("jmp "); emit_kid(p,0,nt); print("\n");
			}
			else {
				assert(p->kids[0]->syms[0] && "indirect external calls not yet supported");
				jmp_extern(p->kids[0]->syms[0]->x.name);
			}
		}
		else if (!mov_flow) {
			if (direct) {
				print("jmp "); emit_kid(p,0,nt); print("\n");
			}
			else {
				print("jmp *"); emit_kid(p,0,nt); print("\n");
			}
		}
		else {
			if (direct) {
				print("movl $"); emit_kid(p,0,nt); print("-0x%x, %%eax\n", MOV_OFFSET);
				jmp_jumpv("%eax");
			}
			else {
				print("movl "); emit_kid(p,0,nt); print(", %%eax\n", MOV_OFFSET);
				//TODO: target should be statically obfuscated when loaded into
				//the register, not here
				print("movl $0x%x, %%edx\n", MOV_OFFSET);
				alu_sub("%eax", "%eax", "%edx");
				jmp_jumpv("%eax");
			}
		}

		/* generate return target */

		print(".%s%d:\n", ext?E_LABEL:I_LABEL, l);

		if (ext) {
			print("# fix ret conv\n");
			if (op==CALL+F) {
				if (opsize(p->op)==4) {
					print("fstp (F0)\n");
				}
				else if (opsize(p->op)==8) {
					print("fstpl (D0)\n");
				}
				else {
					assert(0);
				}
			}
			else {
				print("movl %%eax, (R0) %s\n", MARKER(REQ));
			}
			popr("%eax");
			print("# end fix ret conv\n");
		}
		else if (mov_flow) {
			print("movl (target), %%eax\n");
			print("movl $.%s%d-0x%x, %%edx\n", ext?E_LABEL:I_LABEL, l, MOV_OFFSET); 
			alu_eq("(b0)", "%eax", "%edx");
			load_jmp_regs("(b0)");
			execution_on("(b0)");
		}

		/* remove args */
		pop_args(p->syms[0]->u.value);
	}

	else if (op==LABEL+V) {
		emit_sym(p,0,nt);
		print(":\n");

		if (mov_flow) {
			print("movl (target), %%eax\n");
			print("movl $%s-0x%x, %%edx\n", p->syms[0]->x.name, MOV_OFFSET);
			alu_eq("(b0)", "%eax", "%edx");
			load_jmp_regs("(b0)");
			execution_on("(b0)");
		}
	}

	else if (op==ADDRL+P || op==ADDRF+P || op==ADDRG+P) {
		if (op==ADDRL+P || op==ADDRF+P) {
			int bytes=p->syms[0]->x.offset;
			print("# (offset %d)\n", bytes);
			print("movl (fp), %%eax\n");
			if ((bytes%4)==0 && ABS(bytes)<STACK_EX_THRESH) {
				int i;
				if (bytes<0) {
					for (i=0; i<ABS(bytes); i+=4) {
						print("movl push(%%eax), %%eax\n");
					}
				}
				else {
					for (i=0; i<bytes; i+=4) {
						print("movl pop(%%eax), %%eax\n");
					}
				}
			}
			else {
				print("movl $%d, %%edx\n", bytes);
				alu_add("%eax", "%eax", "%edx");
			}
		}
		else {
			print("movl $"); emit_sym(p,0,nt); print(", %%eax\n");
		}
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}

	else if (op==ASGN+B) {
		int s=p->syms[0]->u.c.v.i;
		int o=0;

		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl (on), %%ecx\n");
		select_data("%eax", "%ecx");

		print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
		print("movl (on), %%ecx\n");
		select_data("%edx", "%ecx");

		while (s>3) {
			print("movl %d(%%edx), %%ecx\n", o);
			print("movl %%ecx, %d(%%eax)\n", o);
			s-=4;
			o+=4;
		}

		while (s>0) {
			print("movb %d(%%edx), %%cl\n", o);
			print("movb %%cl, %d(%%eax)\n", o);
			s-=1;
			o+=1;
		}
	}

	else if (op==ARG+B) {
		int s=p->syms[0]->u.c.v.i;
		int o=0;

		print("movl (sp), %%eax\n");
		print("movl $%d, %%edx\n", roundup(p->syms[0]->u.c.v.i, 4));
		alu_sub("%eax", "%eax", "%edx");

		print("movl %%eax, (stack_temp)\n");
		print("movl $sp, %%eax\n");
		print("movl (on), %%edx\n");
		select_data("%eax", "%edx");
		print("movl (stack_temp), %%edx\n");
		print("movl %%edx, (%%eax)\n");

		print("movl (sp), %%eax\n");
		print("movl (on), %%ecx\n");
		select_data("%eax", "%ecx");

		print("movl "); emit_kid(p,0,nt); print(", %%edx\n");
		print("movl (on), %%ecx\n");
		select_data("%edx", "%ecx");

		while (s>3) {
			print("movl %d(%%edx), %%ecx\n", o);
			print("movl %%ecx, %d(%%eax)\n", o);
			s-=4;
			o+=4;
		}

		while (s>0) {
			print("movb %d(%%edx), %%cl\n", o);
			print("movb %%cl, %d(%%eax)\n", o);
			s-=1;
			o+=1;
		}
	}

	else if (op==ASGN+I || op==ASGN+U || op==ASGN+P || op==ASGN+F ) {

		if (generic(p->kids[0]->op)!=VREG) {
			
			/* need to distinguish between the spill assign (which assigns to
			 * a offset(base)) and the other assigns (which assign to a simple
			 * (base)).  i can't figure out how to do that correctly.
			 * p->syms[0] exists for one and not the other - that's what i'm
			 * going with for now */

			//TODO: figure out the right way to do this
			//TODO: this may not even be a problem now that the rules are
			//reworked
			
			if (generic(p->kids[0]->op)!=ADDRL) {
				print("# (!ADDRL)\n");
				print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
				print("movl (on), %%edx\n");
				select_data("%eax", "%edx");
				if (opsize(p->op)==1) {
					print("movb "); emit_kid(p,1,nt); print(", %%dl\n");
					print("movb %%dl, (%%eax)\n");
				}
				else if (opsize(p->op)==2) {
					print("movw "); emit_kid(p,1,nt); print(", %%dx\n");
					print("movw %%dx, (%%eax)\n");
				}
				else if (opsize(p->op)==4) {
					print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
					print("movl %%edx, (%%eax)\n");
				}
				else if (opsize(p->op)==8) {
					print("movl (%s), %%edx\n", p->kids[1]->syms[RX]->x.name);
					print("movl %%edx, (%%eax)\n");
					print("movl (%s+4), %%edx\n", p->kids[1]->syms[RX]->x.name);
					print("movl %%edx, 4(%%eax)\n");
				}
				else {
					assert(0);
				}
			}
			else {
				print("# (ADDRL)\n");

				if (p->kids[0]->syms[RX]->x.regnode) {
					print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
					print("movl (fp), %%edx\n");
					alu_add("%eax", "%eax", "%edx");
				}
				else {
					int bytes=p->kids[0]->syms[0]->x.offset;
					print("# (offset %d)\n", p->kids[0]->syms[0]->x.offset);
					print("movl (fp), %%eax\n");
					if ((bytes%4)==0 && ABS(bytes)<STACK_EX_THRESH) {
						int i;
						if (bytes<0) {
							for (i=0; i<ABS(bytes); i+=4) {
								print("movl push(%%eax), %%eax\n");
							}
						}
						else {
							for (i=0; i<bytes; i+=4) {
								print("movl pop(%%eax), %%eax\n");
							}
						}
					}
					else {
						print("movl $%d, %%edx\n", bytes);
						alu_add("%eax", "%eax", "%edx");
					}
				}
				print("movl (on), %%edx\n");
				select_data("%eax", "%edx");

				if (opsize(p->op)==1) {
					print("movb "); emit_kid(p,1,nt); print(", %%dl\n");
					print("movb %%dl, (%%eax)\n");
				}
				else if (opsize(p->op)==2) {
					print("movw "); emit_kid(p,1,nt); print(", %%dx\n");
					print("movw %%dx, (%%eax)\n");
				}
				else if (opsize(p->op)==4) {
					print("movl "); emit_kid(p,1,nt); print(", %%edx\n");
					print("movl %%edx, (%%eax)\n");
				}
				else if (opsize(p->op)==8) {
					print("movl (%s), %%edx\n", p->kids[1]->syms[RX]->x.name);
					print("movl %%edx, (%%eax)\n");
					print("movl (%s+4), %%edx\n", p->kids[1]->syms[RX]->x.name);
					print("movl %%edx, 4(%%eax)\n");
				}
				else {
					assert(0);
				}
			}
		}
		else {
			print("\n# (emit vreg asgn)\n\n");
		}
	}

	else if (op==RET+I || op==RET+U || op==RET+V || op==RET+P || op==RET+F) {
		/* handled by function() */
	}

	else if (op==INDIR+I || op==INDIR+P || op==INDIR+U) {
		/* necessary to intercept pointer dereferences while execution is off */

		if (opsize(p->op)==1) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl (on), %%edx\n");
			select_data("%eax", "%edx");
			print("movl $0, %%edx\n");
			print("movb (%%eax), %%dl\n");
			print("movl %%edx, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==2) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl (on), %%edx\n");
			select_data("%eax", "%edx");
			print("movl $0, %%edx\n");
			print("movw (%%eax), %%dx\n");
			print("movl %%edx, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl (on), %%edx\n");
			select_data("%eax", "%edx");
			print("movl (%%eax), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else {
			assert(0);
		}
	}

	else if (op==CVU+I || op==CVU+U) {
		if (opsize(p->op)==4 && opsize(p->x.kids[0]->op)==1) {
			/* zero extend */
			print("# (zero extend)\n\n");
			print("movl $0, %%edx\n");
			print("movb "); emit_kid(p,0,nt); print(", %%dl\n");
			print("movl %%edx, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==4 && opsize(p->x.kids[0]->op)==2) {
			/* zero extend */
			print("# (zero extend)\n\n");
			print("movl $0, %%edx\n");
			print("movw "); emit_kid(p,0,nt); print(", %%dx\n");
			print("movl %%edx, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==1 && opsize(p->x.kids[0]->op)==4) {
			/* truncate */
			print("# (truncate)\n\n");
			print("movl "); emit_kid(p,0,nt); print(", %%edx\n");
			print("movb %%dl, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==2 && opsize(p->x.kids[0]->op)==4) {
			/* truncate */
			print("# (truncate)\n\n");
			print("movl "); emit_kid(p,0,nt); print(", %%edx\n");
			print("movw %%dx, "); emit_sym(p,2,nt); print("\n");
		}
		else {
			print("# (generic)\n\n");
			print("movl "); emit_kid(p,0,nt); print(", %%edx\n");
			print("movl %%edx, "); emit_sym(p,2,nt); print("\n");
		}
	}
	else if (op==CVI+I || op==CVI+U) {
		if (opsize(p->op)==4 && opsize(p->x.kids[0]->op)==1) {
			/* sign extend */
			print("# (sign extend)\n\n");
			print("movl $0, %%edx\n");
			print("movb "); emit_kid(p,0,nt); print(", %%dl\n");
			print("movl alu_sex8(,%%edx,4), %%edx\n");
			print("movl %%edx, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==4 && opsize(p->x.kids[0]->op)==2) {
			/* sign extend */
			print("# (sign extend)\n\n");
			print("movl $0, %%edx\n");
			print("movw "); emit_kid(p,0,nt); print(", %%dx\n");
			print("movl $0, %%eax\n");
			print("movb %%dh, %%al\n");
			print("movl alu_sex8(,%%eax,4), %%eax\n");
			print("movw %%dx, %%ax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==1 && opsize(p->x.kids[0]->op)==4) {
			/* truncate */
			print("# (truncate)\n\n");
			print("movl "); emit_kid(p,0,nt); print(", %%edx\n");
			print("movb %%dl, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==2 && opsize(p->x.kids[0]->op)==4) {
			/* truncate */
			print("# (truncate)\n\n");
			print("movl "); emit_kid(p,0,nt); print(", %%edx\n");
			print("movw %%dx, "); emit_sym(p,2,nt); print("\n");
		}
		else {
			print("# (generic)\n\n");
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
	}
	else if (generic(op)==LOAD) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
	}

	/* floating point support with a m/o/vfuscated soft float library */
	else if (op==INDIR+F) {
		if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl (on), %%edx\n");
			select_data("%eax", "%edx");
			print("movl (%%eax), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==8) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			print("movl (on), %%edx\n");
			select_data("%eax", "%edx");
			print("movl (%%eax), %%edx\n");
			print("movl %%edx, (%s)\n", p->syms[RX]->x.name);
			print("movl 4(%%eax), %%edx\n");
			print("movl %%edx, (%s+4)\n", p->syms[RX]->x.name);
		}
		else {
			assert(0);
		}
	}

	/*
	else if (op==ASGN+F) {
		print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
		print("movl (on), %%edx\n");
		select_data("%eax", "%edx");
		print("movl "); emit_kid(p,1,nt); print(", (%%eax)\n");
	}
	*/

	else if (op==ARG+F) {
		if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
		}
		else if (opsize(p->op)==8) {
			pushm8(p->kids[0]->syms[RX]->x.name);
		}
		else {
			assert(0);
		}
	}

	else if (op==NEG+F) {
		if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
			print("movl $0, %%eax\n");
			push("%eax");
			call_lib("float32_sub");
			pop_args(2*4);
			/* R0 - softfloat uses a typedef float */
			print("movl (R0), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==8) {
			pushm8(p->kids[0]->syms[RX]->x.name);
			print("movl $0, %%eax\n");
			push("%eax");
			print("movl $0, %%eax\n");
			push("%eax");
			push("$D0");
			call_lib("float64_sub");
			pop_args(5*4);
			print("movl (D0), %%eax\n");
			print("movl %%eax, (%s)\n", p->syms[RX]->x.name);
			print("movl (D0+4), %%eax\n");
			print("movl %%eax, (%s+4)\n", p->syms[RX]->x.name);
		}
		else {
			assert(0);
		}
	}

	else if (op==ADD+F) {
		if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,1,nt); print(", %%eax\n");
			push("%eax");
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
			call_lib("float32_add");
			pop_args(2*4);
			/* R0 - softfloat uses a typedef float */
			print("movl (R0), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==8) {
			pushm8(p->kids[1]->syms[RX]->x.name);
			pushm8(p->kids[0]->syms[RX]->x.name);
			push("$D0");
			call_lib("float64_add");
			pop_args(5*4);
			print("movl (D0), %%eax\n");
			print("movl %%eax, (%s)\n", p->syms[RX]->x.name);
			print("movl (D0+4), %%eax\n");
			print("movl %%eax, (%s+4)\n", p->syms[RX]->x.name);
		}
		else {
			assert(0);
		}
	}
	else if (op==SUB+F) {
		if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,1,nt); print(", %%eax\n");
			push("%eax");
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
			call_lib("float32_sub");
			pop_args(2*4);
			/* R0 - softfloat uses a typedef float */
			print("movl (R0), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==8) {
			pushm8(p->kids[1]->syms[RX]->x.name);
			pushm8(p->kids[0]->syms[RX]->x.name);
			push("$D0");
			call_lib("float64_sub");
			pop_args(5*4);
			print("movl (D0), %%eax\n");
			print("movl %%eax, (%s)\n", p->syms[RX]->x.name);
			print("movl (D0+4), %%eax\n");
			print("movl %%eax, (%s+4)\n", p->syms[RX]->x.name);
		}
		else {
			assert(0);
		}
	}
	else if (op==DIV+F) {
		if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,1,nt); print(", %%eax\n");
			push("%eax");
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
			call_lib("float32_div");
			pop_args(2*4);
			/* R0 - softfloat uses a typedef float */
			print("movl (R0), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==8) {
			pushm8(p->kids[1]->syms[RX]->x.name);
			pushm8(p->kids[0]->syms[RX]->x.name);
			push("$D0");
			call_lib("float64_div");
			pop_args(5*4);
			print("movl (D0), %%eax\n");
			print("movl %%eax, (%s)\n", p->syms[RX]->x.name);
			print("movl (D0+4), %%eax\n");
			print("movl %%eax, (%s+4)\n", p->syms[RX]->x.name);
		}
		else {
			assert(0);
		}
	}
	else if (op==MUL+F) {
		if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,1,nt); print(", %%eax\n");
			push("%eax");
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
			call_lib("float32_mul");
			pop_args(2*4);
			/* R0 - softfloat uses a typedef float */
			print("movl (R0), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==8) {
			pushm8(p->kids[1]->syms[RX]->x.name);
			pushm8(p->kids[0]->syms[RX]->x.name);
			push("$D0");
			call_lib("float64_mul");
			pop_args(5*4);
			print("movl (D0), %%eax\n");
			print("movl %%eax, (%s)\n", p->syms[RX]->x.name);
			print("movl (D0+4), %%eax\n");
			print("movl %%eax, (%s+4)\n", p->syms[RX]->x.name);
		}
		else {
			assert(0);
		}
	}

	else if (op==CVF+F) {
		if (opsize(p->op)==4) {
			if (opsize(p->x.kids[0]->op)==4) {
				print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
				print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
			}
			else if (opsize(p->x.kids[0]->op)==8) {
				pushm8(p->kids[0]->syms[RX]->x.name);
				call_lib("float64_to_float32");
				pop_args(8);
				/* R0 - softfloat uses a typedef float */
				print("movl (R0), %%eax\n");
				print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
			}
			else {
				assert(0);
			}
		}
		else if (opsize(p->op)==8) {
			if (opsize(p->x.kids[0]->op)==4) {
				push(p->kids[0]->syms[RX]->x.name);
				push("$D0");
				call_lib("float32_to_float64");
				pop_args(8);
				print("movl (D0), %%eax\n");
				print("movl %%eax, (%s)\n", p->syms[RX]->x.name);
				print("movl (D0+4), %%eax\n");
				print("movl %%eax, (%s+4)\n", p->syms[RX]->x.name);
			}
			else if (opsize(p->x.kids[0]->op)==8) {
				print("movl (%s), %%eax\n", p->kids[0]->syms[RX]->x.name);
				print("movl %%eax, (%s)\n", p->syms[RX]->x.name);
				print("movl (%s+4), %%eax\n", p->kids[0]->syms[RX]->x.name);
				print("movl %%eax, (%s+4)\n", p->syms[RX]->x.name);
			}
			else {
				assert(0);
			}
		}
		else {
			assert(0);
		}
	}
	else if (op==CVF+I) {
		if (opsize(p->x.kids[0]->op)==4) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
			call_lib("float32_to_int32");
			pop_args(4);
			print("movl (R0), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->x.kids[0]->op)==8) {
			pushm8(p->kids[0]->syms[RX]->x.name);
			call_lib("float64_to_int32");
			pop_args(8);
			print("movl (R0), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else {
			assert(0);
		}
	}
	else if (op==CVI+F) {
		if (opsize(p->op)==4) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
			call_lib("int32_to_float32");
			pop_args(4);
			/* R0 - softfloat uses a typedef float */
			print("movl (R0), %%eax\n");
			print("movl %%eax, "); emit_sym(p,2,nt); print("\n");
		}
		else if (opsize(p->op)==8) {
			print("movl "); emit_kid(p,0,nt); print(", %%eax\n");
			push("%eax");
			push("$D0");
			call_lib("int32_to_float64");
			pop_args(8);
			print("movl (D0), %%eax\n");
			print("movl %%eax, (%s)\n", p->syms[RX]->x.name);
			print("movl (D0+4), %%eax\n");
			print("movl %%eax, (%s+4)\n", p->syms[RX]->x.name);
		}
		else {
			assert(0);
		}
	}

	else if (op==EQ+F) {
		if (opsize(p->op)==4) {
			fpu_cmp32("float32_eq", "(b0)", p, nt);
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else if (opsize(p->op)==8) {
			fpu_cmp64("float64_eq", "(b0)", p, nt);
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else {
			assert(0);
		}
	}
	else if (op==GE+F) {
		if (opsize(p->op)==4) {
			fpu_cmp32("float32_lt", "(b0)", p, nt);
			alu_not("b0", "b0");
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else if (opsize(p->op)==8) {
			fpu_cmp64("float64_lt", "(b0)", p, nt);
			alu_not("b0", "b0");
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else {
			assert(0);
		}
	}
	else if (op==GT+F) {
		if (opsize(p->op)==4) {
			fpu_cmp32("float32_le", "(b0)", p, nt);
			alu_not("b0", "b0");
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else if (opsize(p->op)==8) {
			fpu_cmp64("float64_le", "(b0)", p, nt);
			alu_not("b0", "b0");
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else {
			assert(0);
		}
	}
	else if (op==LE+F) {
		if (opsize(p->op)==4) {
			fpu_cmp32("float32_le", "(b0)", p, nt);
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else if (opsize(p->op)==8) {
			fpu_cmp64("float64_le", "(b0)", p, nt);
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else {
			assert(0);
		}
	}
	else if (op==LT+F) {
		if (opsize(p->op)==4) {
			fpu_cmp32("float32_lt", "(b0)", p, nt);
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else if (opsize(p->op)==8) {
			fpu_cmp64("float64_lt", "(b0)", p, nt);
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else {
			assert(0);
		}
	}
	else if (op==NE+F) {
		if (opsize(p->op)==4) {
			fpu_cmp32("float32_eq", "(b0)", p, nt);
			alu_not("b0", "b0");
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else if (opsize(p->op)==8) {
			fpu_cmp64("float64_eq", "(b0)", p, nt);
			alu_not("b0", "b0");
			cc_branch(p->syms[0]->x.name, "b0");
		}
		else {
			assert(0);
		}
	}

	else {
		print("# (blank emit %d)\n", p->op);
	}

	print_block_footer(op);
}

static void function(Symbol f, Symbol caller[], Symbol callee[], int n)
{
	int i, l;

	globalend();

	// gas listens to you unless you put 0x90 - then it picks its own nop
	// padding... i hate that assembler 
	// print(".align 16, 0x90\n"); 
	print(".type %s,@function\n", f->x.name);
	print("%s: %s\n", f->x.name, MARKER(I_LABEL));

	if (mov_flow) {
		print("# label\n");
		print("movl (target), %%eax\n");
		print("movl $%s-0x%x, %%edx\n", f->x.name, MOV_OFFSET);
		alu_eq("(b0)", "%eax", "%edx");
		load_jmp_regs("(b0)");
		execution_on("(b0)");
		print("# end label\n");
	}

	print("# prologue\n");
	push("(fp)");
	
	/* push all but R0/F0/D0 */
	for (i=1; i<SOFT_I_REGS; i++) {
		push(stringf("(R%d)", i));
	}
	for (i=1; i<SOFT_F_REGS; i++) {
		push(stringf("(F%d)", i));
	}
	for (i=1; i<SOFT_D_REGS; i++) {
		pushm8(stringf("D%d", i));
	}

	print("# mov %%esp, %%ebp\n");
	print("movl $fp, %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (sp), %%edx\n");
	print("movl %%edx, (%%eax)\n");
	print("# end mov %%esp, %%ebp\n");

	usedmask[0]=usedmask[1]=0;
	freemask[0]=freemask[1]=~0U;
	offset=
		4*(SOFT_I_REGS-1)+ /* r1 - rN */
		4*(SOFT_F_REGS-1)+ /* f1 - fN */
		8*(SOFT_D_REGS-1)+ /* d1 - dN */
		4+ /* fp */
		4; /* ret */
	for (i=0; callee[i]; i++) {
		Symbol p=callee[i];
		Symbol q=caller[i];
		assert(q);
		offset=roundup(offset, q->type->align);
		p->x.offset=q->x.offset=offset;
		p->x.name=q->x.name=stringf("%d", p->x.offset);
		p->sclass=q->sclass=AUTO;
		offset += roundup(q->type->size, 4);
	}
	assert(caller[i]==0);
	offset=maxoffset=0;

	gencode(caller, callee);

	framesize=roundup(maxoffset, 4);
	if (framesize>0) {
		print("# frame\n");
		print("movl (sp), %%eax\n");
		if ((framesize%4)==0 && framesize<STACK_EX_THRESH) {
			int i;
			for (i=0; i<framesize; i+=4) {
				print("movl push(%%eax), %%eax\n");
			}
		}
		else {
			print("movl $%d, %%edx\n", -framesize);
			alu_add("%eax", "%eax", "%edx");
		}
		print("movl %%eax, (stack_temp)\n");
		print("movl $sp, %%eax\n");
		print("movl (on), %%edx\n");
		select_data("%eax", "%edx");
		print("movl (stack_temp), %%edx\n");
		print("movl %%edx, (%%eax)\n");
		print("#end frame\n");
	}

	emitcode();

	print("# epilogue\n");
	print("# movl %%ebp, %%esp\n");
	print("movl $sp, %%eax\n");
	print("movl (on), %%edx\n");
	select_data("%eax", "%edx");
	print("movl (fp), %%edx\n");
	print("movl %%edx, (%%eax)\n");
	print("# end movl %%ebp, %%esp\n");

	for (i=SOFT_D_REGS-1; i>0; i--) {
		popm8(stringf("D%d", i));
	}
	for (i=SOFT_F_REGS-1; i>0; i--) {
		popm(stringf("F%d", i));
	}
	for (i=SOFT_I_REGS-1; i>0; i--) {
		popm(stringf("R%d", i));
	}

	popm("fp");

	print("# ret\n");
	popr("%eax");
	if (!mov_flow) {
		print("jmp *%%eax\n");
	}
	else {
		jmp_jumpv("%eax");
	}
	print("# end ret\n");

	l=genlabel(1);
	print(".Lf%d:\n", l);
	print(".size %s,.Lf%d-%s\n", f->x.name, l, f->x.name);

	print("\n");
}

static void defsymbol(Symbol p) 
{
	if (p->scope>=LOCAL && p->sclass==STATIC) {
		p->x.name=stringf("%s.%d", p->name, genlabel(1));
	}
	else if (p->generated) {
		p->x.name=stringf(".%s%s", p->scope==LABELS?I_LABEL:S_LABEL, p->name);
	}
	else if (p->scope==GLOBAL || p->sclass==EXTERN) {
		p->x.name=stringf("%s", p->name);
	}
	else {
		p->x.name=p->name;
	}
}

static void doarg(Node p)
{
	assert(p && p->syms[0]);
	mkactual(4, p->syms[0]->u.c.v.i);
}

static void segment(int n) 
{
	if (n==current_segment) {
		return;
	}

	/* the newline _before_ the segment name is necessary to prevent an apparent
	 * bug in the gnu assembler - in a specific configuration, without the
	 * newline, it is not properly switching between segments (a .data segment
	 * is being combined with the surrounding .text sections).  the issue
	 * disappears when adding a newline before emitting the .bss directive
	 * (sic). meticulously reproduced. i have a whole stack of these. should
	 * probably submit a bug report at some point. */

	current_segment=n;
	if (current_segment==MOV_CODE) {
		print("\n.text\n");
	}
	else if (current_segment==MOV_BSS) {
		print("\n.bss\n");
	}
	else if (current_segment==MOV_DATA) {
		print("\n.data\n");
	}
	else if (current_segment==MOV_PLT) {
		print("\n.section .plt\n");
	}
}

static void blkfetch(int k, int off, int reg, int tmp)
{
}

static void blkstore(int k, int off, int reg, int tmp)
{
}

static void blkloop(
		int dreg,
		int doff,
		int sreg,
		int soff,
		int size,
		int tmps[]
		)
{
}

static void local(Symbol p)
{
	if (isfloat(p->type)) {
		p->sclass=AUTO;
	}
	if (askregvar(p, (*IR->x.rmap)(ttob(p->type)))==0) {
		assert(p->sclass==AUTO);
		offset=roundup(
				offset+p->type->size,
				p->type->align<4 ? 4 : p->type->align
				);
		p->x.offset=-offset;
		p->x.name=stringd(-offset);
	}
}

static void defconst(int suffix, int size, Value v)
{
	(*IR->segment)(MOV_DATA);
	if (suffix==I && size==1) {
		print(".byte %d\n", (int)v.u);
	}
	else if (suffix==I && size==2) {
		print(".word %d\n", (int)v.i);
	}
	else if (suffix==I && size==4) {
		print(".long %d\n", (int)v.i);
	}
	else if (suffix==U && size==1) {
		print(".byte %d\n", (int)((char)v.u));
	}
	else if (suffix==U && size==2) {
		print(".word %d\n", (int)((short)v.u));
	}
	else if (suffix==U && size==4) {
		print(".long %d\n", (int)v.u);
	}
	else if (suffix==P && size==4) {
		print(".long %d\n", (int)(long long int)v.p);
	}
	else if (suffix==F && size==4) {
		float f=v.d;
		print(".long %d\n", (int)(*(unsigned *)&f));
	}
	else if (suffix == F && size == 8) {
		double d=v.d;
		unsigned* p=(unsigned*)&d;
		print(".long %d, %d\n", p[swap], p[!swap]);
	}
	else {
		assert(0);
	}
}

static void address(Symbol q, Symbol p, long n)
{
	if (p->scope==GLOBAL || p->sclass==STATIC || p->sclass==EXTERN) {
		q->x.name=stringf("%s%s%D", p->x.name, n>=0?"+":"", n);
	}
	else {
		assert(n<=INT_MAX && n>=INT_MIN);
		q->x.offset=p->x.offset+n;
		q->x.name=stringd(q->x.offset);
	}
}

static void defaddress(Symbol p)
{
	(*IR->segment)(MOV_DATA);
	print(".long %s\n", p->x.name);
}

static void defstring(int n, char* str)
{
	char* s;
	(*IR->segment)(MOV_DATA);
	for (s=str; s<str+n; s++) {
		print(".byte 0x%x\n", (*s)&0xff);
	}
}

static void import(Symbol p) 
{
	print("# import '%s'\n", p->x.name);
	print(".extern '%s'\n", p->x.name);
}

static void export(Symbol p)
{
	globalend();
	print("# export '%s'\n", p->x.name);
	print(".globl %s\n", p->x.name);
}

static void global(Symbol p)
{
	globalend();

	// gas listens to you unless you put 0x90-then it picks its own nop padding
	// print(".align %d, 0x90\n", p->type->align>4 ? 4 : p->type->align);

	if (!p->generated) {
		print(".type %s,@%s\n", p->x.name, isfunc(p->type) ? "function" : "object");
		if (p->type->size>0) {
			print(".size %s,%d\n", p->x.name, p->type->size);
		}
		else {
			last_global=p;
		}
	}

	//TODO: u.seg is not set correctly ...
	if (p->u.seg==MOV_BSS) {
		if (p->sclass==STATIC) {
			print(".lcomm %s,%d\n", p->x.name, p->type->size);
		}
		else {
			print(".comm %s,%d\n", p->x.name, p->type->size);
		}
	}
	else {
		(*IR->segment)(MOV_DATA);
		print("%s: %s\n", p->x.name, MARKER(S_LABEL));
	}
}

static void space(int n)
{
	if (current_segment!=MOV_BSS) {
		print(".space %d\n", n);
	}
}

Interface x86movIR={
	/* metrics-size, align, outofline (can appear in dag) */
	1, 1, 0,  /* char */
	2, 2, 0,  /* short */
	4, 4, 0,  /* int */
	4, 4, 0,  /* long */
	4, 4, 0,  /* long long */
	4, 4, 1,  /* float */
	8, 8, 1,  /* double */
	8, 8, 1,  /* long double */
	4, 4, 0,  /* T * */
	0, 1, 0,  /* struct */
	1,        /* little_endian */
	0,        /* mulops_calls */
	0,        /* wants_callb */
	1,        /* wants_argb */
	0,        /* left_to_right */
	0,        /* wants_dag */
	0,        /* unsigned_char */

	/* m/o/vfuscator */
	address,
	blockbeg,
	blockend,
	defaddress,
	defconst,
	defstring,
	defsymbol,
	emit,
	export,
	function,
	gen,
	global,
	import,
	local,
	progbeg,
	progend,
	segment,
	space,

	/* externs */
	stabblock,
	stabend,
	stabfend,
	stabinit,
	stabline,
	stabsym,
	stabtype,
	
	/* Xinterface */
	{
		1,             /* max_unaligned_load */
		rmap,
		blkfetch, 
		blkstore, 
		blkloop,
		
		/* built by terms */
	    _label,
	    _rule,
	    _nts,
	    _kids,
	    _string,
	    _templates,
	    _isinstruction,
	    _ntname,

	    emit2,
	    doarg,
	    target,
	    clobber,
	}
};
