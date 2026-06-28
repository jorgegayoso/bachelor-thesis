#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;

struct operand           { u8 _opaque[40]; };
struct fetch_cache       { u8 _opaque[24]; };
struct read_cache        { unsigned long pos, end; u8 data[1024]; };
struct x86_exception     { u8 _opaque[24]; };
struct x86_emulate_ops   { u8 _opaque[8]; };
typedef u64 gpa_t;

#define NR_VCPU_REGS 16

enum x86emul_mode { X86EMUL_MODE_PROT64 = 4 };

struct fastop;
typedef void (*fastop_t)(struct fastop *);

struct x86_emulate_ctxt {
	void *vcpu;
	const struct x86_emulate_ops *ops;

	/* Register state before/after emulation. */
	unsigned long eflags;
	unsigned long eip; /* eip before instruction emulation */
	/* Emulated execution mode, represented by an X86EMUL_MODE value. */
	enum x86emul_mode mode;

	/* interruptibility state, as a result of execution of STI or MOV SS */
	int interruptibility;

	bool perm_ok; /* do not check permissions if true */
	bool ud;	/* inject an #UD if host doesn't support insn */
	bool tf;	/* TF value before instruction (after for syscall/sysret) */

	bool have_exception;
	struct x86_exception exception;

	/* GPA available */
	bool gpa_available;
	gpa_t gpa_val;

	/*
	 * decode cache
	 */

	/* current opcode length in bytes */
	u8 opcode_len;
	u8 b;
	u8 intercept;
	u8 op_bytes;
	u8 ad_bytes;
	union {
		int (*execute)(struct x86_emulate_ctxt *ctxt);
		fastop_t fop;
	};
	int (*check_perm)(struct x86_emulate_ctxt *ctxt);
	/*
	 * The following six fields are cleared together,
	 * the rest are initialized unconditionally in x86_decode_insn
	 * or elsewhere
	 */
	bool rip_relative;
	u8 rex_prefix;
	u8 lock_prefix;
	u8 rep_prefix;
	/* bitmaps of registers in _regs[] that can be read */
	u32 regs_valid;
	/* bitmaps of registers in _regs[] that have been written */
	u32 regs_dirty;
	/* modrm */
	u8 modrm;
	u8 modrm_mod;
	u8 modrm_reg;
	u8 modrm_rm;
	u8 modrm_seg;
	u8 seg_override;
	u64 d;
	unsigned long _eip;

	/* Here begins the usercopy section. */
	struct operand src;
	struct operand src2;
	struct operand dst;
	struct operand memop;
	unsigned long _regs[NR_VCPU_REGS];
	struct operand *memopp;
	struct fetch_cache fetch;
	struct read_cache io_read;
	struct read_cache mem_read;
};

/* IOO bug function. copied from
   arch/x86/kvm/emulate.c */
void init_decode_cache(struct x86_emulate_ctxt *ctxt)
{
	/* buggy line, B1 */
	memset(&ctxt->rip_relative, 0,
	       (void *)&ctxt->modrm - (void *)&ctxt->rip_relative);

	ctxt->io_read.pos = 0;
	ctxt->io_read.end = 0;
	ctxt->mem_read.end = 0;
}
