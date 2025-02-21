/* Output routines for GCC for ARM.
   Copyright (C) 1991, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001,
   2002, 2003, 2004, 2005, 2006  Free Software Foundation, Inc.
   Contributed by Pieter `Tiggr' Schoenmakers (rcpieter@win.tue.nl)
   and Martin Simmons (@harleqn.co.uk).
   More major hacks by Richard Earnshaw (rearnsha@arm.com).

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 2, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* ALQAAHIRA LOCAL begin 6902792 Q register clobbers in inline asm */
#include <stdlib.h>
#include <ctype.h>
/* ALQAAHIRA LOCAL end 6902792 Q register clobbers in inline asm */
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "rtl.h"
#include "tree.h"
#include "obstack.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "insn-config.h"
#include "conditions.h"
#include "output.h"
#include "insn-attr.h"
#include "flags.h"
#include "reload.h"
#include "function.h"
#include "expr.h"
#include "optabs.h"
#include "toplev.h"
#include "recog.h"
#include "ggc.h"
#include "except.h"
#include "c-pragma.h"
#include "integrate.h"
#include "tm_p.h"
#include "target.h"
#include "target-def.h"
#include "debug.h"
#include "langhooks.h"
/* APPLE LOCAL ARM needed for SUBSUBTARGET_OVERRIDE_OPTIONS */
#include "../../libcpp/internal.h"
/* APPLE LOCAL ARM needed for set_param_value */
#include "params.h"

/* Forward definitions of types.  */
typedef struct minipool_node    Mnode;
typedef struct minipool_fixup   Mfix;

const struct attribute_spec arm_attribute_table[];

/* Forward function declarations.  */
static arm_stack_offsets *arm_get_frame_offsets (void);
static void arm_add_gc_roots (void);
static int arm_gen_constant (enum rtx_code, enum machine_mode, rtx,
			     HOST_WIDE_INT, rtx, rtx, int, int);
static unsigned bit_count (unsigned long);
static int arm_address_register_rtx_p (rtx, int);
static int arm_legitimate_index_p (enum machine_mode, rtx, RTX_CODE, int);
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
static int thumb2_legitimate_index_p (enum machine_mode, rtx, int);
static int thumb1_base_register_rtx_p (rtx, enum machine_mode, int);
inline static int thumb1_index_register_rtx_p (rtx, int);
static int thumb_far_jump_used_p (void);
static bool thumb_force_lr_save (void);
static unsigned long thumb1_compute_save_reg_mask (void);
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
static int const_ok_for_op (HOST_WIDE_INT, enum rtx_code);
static rtx emit_sfm (int, int);
static int arm_size_return_regs (void);
#ifndef AOF_ASSEMBLER
static bool arm_assemble_integer (rtx, unsigned int, int);
#endif
static const char *fp_const_from_val (REAL_VALUE_TYPE *);
static arm_cc get_arm_condition_code (rtx);
static HOST_WIDE_INT int_log2 (HOST_WIDE_INT);
static rtx is_jump_table (rtx);
static const char *output_multi_immediate (rtx *, const char *, const char *,
					   int, HOST_WIDE_INT);
static const char *shift_op (rtx, HOST_WIDE_INT *);
static struct machine_function *arm_init_machine_status (void);
/* APPLE LOCAL begin compact switch tables */
static int handle_thumb_unexpanded_prologue (FILE *, bool);
static int handle_thumb_unexpanded_epilogue (bool);
static int handle_thumb_exit (FILE *, int, bool);
static int handle_thumb_pushpop (FILE *, unsigned long, int, int *, unsigned long, bool);
/* APPLE LOCAL end compact switch tables */
static rtx is_jump_table (rtx);
static HOST_WIDE_INT get_jump_table_size (rtx);
static Mnode *move_minipool_fix_forward_ref (Mnode *, Mnode *, HOST_WIDE_INT);
static Mnode *add_minipool_forward_ref (Mfix *);
static Mnode *move_minipool_fix_backward_ref (Mnode *, Mnode *, HOST_WIDE_INT);
static Mnode *add_minipool_backward_ref (Mfix *);
static void assign_minipool_offsets (Mfix *);
static void arm_print_value (FILE *, rtx);
static void dump_minipool (rtx);
static int arm_barrier_cost (rtx);
static Mfix *create_fix_barrier (Mfix *, HOST_WIDE_INT);
static void push_minipool_barrier (rtx, HOST_WIDE_INT);
static void push_minipool_fix (rtx, HOST_WIDE_INT, rtx *, enum machine_mode,
			       rtx);
static void arm_reorg (void);
static bool note_invalid_constants (rtx, HOST_WIDE_INT, int);
static int current_file_function_operand (rtx);
static unsigned long arm_compute_save_reg0_reg12_mask (void);
static unsigned long arm_compute_save_reg_mask (void);
static unsigned long arm_isr_value (tree);
static unsigned long arm_compute_func_type (void);
static tree arm_handle_fndecl_attribute (tree *, tree, tree, int, bool *);
static tree arm_handle_isr_attribute (tree *, tree, tree, int, bool *);
#if TARGET_DLLIMPORT_DECL_ATTRIBUTES
static tree arm_handle_notshared_attribute (tree *, tree, tree, int, bool *);
#endif
static void arm_output_function_epilogue (FILE *, HOST_WIDE_INT);
static void arm_output_function_prologue (FILE *, HOST_WIDE_INT);
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
static void thumb1_output_function_prologue (FILE *, HOST_WIDE_INT);
static int arm_comp_type_attributes (tree, tree);
static void arm_set_default_type_attributes (tree);
static int arm_adjust_cost (rtx, rtx, rtx, int);
static int count_insns_for_constant (HOST_WIDE_INT, int);
static int arm_get_strip_length (int);
static bool arm_function_ok_for_sibcall (tree, tree);
static void arm_internal_label (FILE *, const char *, unsigned long);
static void arm_output_mi_thunk (FILE *, tree, HOST_WIDE_INT, HOST_WIDE_INT,
				 tree);
static int arm_rtx_costs_1 (rtx, enum rtx_code, enum rtx_code);
static bool arm_size_rtx_costs (rtx, int, int, int *);
static bool arm_slowmul_rtx_costs (rtx, int, int, int *);
static bool arm_fastmul_rtx_costs (rtx, int, int, int *);
static bool arm_xscale_rtx_costs (rtx, int, int, int *);
static bool arm_9e_rtx_costs (rtx, int, int, int *);
static int arm_address_cost (rtx);
static bool arm_memory_load_p (rtx);
static bool arm_cirrus_insn_p (rtx);
static void cirrus_reorg (rtx);
static void arm_init_builtins (void);
static rtx arm_expand_builtin (tree, rtx, rtx, enum machine_mode, int);
static void arm_init_iwmmxt_builtins (void);
static rtx safe_vector_operand (rtx, enum machine_mode);
static rtx arm_expand_binop_builtin (enum insn_code, tree, rtx);
static rtx arm_expand_unop_builtin (enum insn_code, tree, rtx, int);
static rtx arm_expand_builtin (tree, rtx, rtx, enum machine_mode, int);
static void emit_constant_insn (rtx cond, rtx pattern);
static rtx emit_set_insn (rtx, rtx);
static int arm_arg_partial_bytes (CUMULATIVE_ARGS *, enum machine_mode,
				  tree, bool);

#ifdef OBJECT_FORMAT_ELF
static void arm_elf_asm_constructor (rtx, int);
#endif
/* APPLE LOCAL begin ARM darwin section_info */
#if TARGET_MACHO
static void arm_darwin_encode_section_info (tree, rtx, int);
#elif !defined(ARM_PE)
static void arm_encode_section_info (tree, rtx, int);
#endif
/* APPLE LOCAL end ARM darwin section_info */

static void arm_file_end (void);
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
static void arm_file_start (void);

/* APPLE LOCAL begin ARM asm file hooks */
#if TARGET_MACHO
static void arm_darwin_file_start (void);
static void arm_darwin_file_end (void);
#endif
/* APPLE LOCAL end ARM asm file hooks */

#ifdef AOF_ASSEMBLER
static void aof_globalize_label (FILE *, const char *);
static void aof_dump_imports (FILE *);
static void aof_dump_pic_table (FILE *);
static void aof_file_start (void);
static void aof_file_end (void);
static void aof_asm_init_sections (void);
#endif
static void arm_setup_incoming_varargs (CUMULATIVE_ARGS *, enum machine_mode,
					tree, int *, int);
static bool arm_pass_by_reference (CUMULATIVE_ARGS *,
				   enum machine_mode, tree, bool);
static bool arm_promote_prototypes (tree);
static bool arm_default_short_enums (void);
static bool arm_align_anon_bitfield (void);
static bool arm_return_in_msb (tree);
static bool arm_must_pass_in_stack (enum machine_mode, tree);
#ifdef TARGET_UNWIND_INFO
static void arm_unwind_emit (FILE *, rtx);
static bool arm_output_ttype (rtx);
#endif
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
static void arm_dwarf_handle_frame_unspec (const char *, rtx, int);
/* ALQAAHIRA LOCAL v7 support. Merge from Codesourcery */
static rtx arm_dwarf_register_span(rtx);

static tree arm_cxx_guard_type (void);
static bool arm_cxx_guard_mask_bit (void);
static tree arm_get_cookie_size (tree);
static bool arm_cookie_has_size (void);
static bool arm_cxx_cdtor_returns_this (void);
static bool arm_cxx_key_method_may_be_inline (void);
static void arm_cxx_determine_class_data_visibility (tree);
static bool arm_cxx_class_data_always_comdat (void);
static bool arm_cxx_use_aeabi_atexit (void);
static void arm_init_libfuncs (void);
static bool arm_handle_option (size_t, const char *, int);
static unsigned HOST_WIDE_INT arm_shift_truncation_mask (enum machine_mode);
static bool arm_cannot_copy_insn_p (rtx);
static bool arm_tls_symbol_p (rtx x);
/* APPLE LOCAL begin ARM -mdynamic-no-pic support */
static int symbol_mentioned_with_filter (rtx, int);
static bool arm_cannot_force_const_mem (rtx x);
/* APPLE LOCAL end ARM -mdynamic-no-pic support */
/* APPLE LOCAL ARM reliable backtraces */
static rtx arm_builtin_setjmp_frame_value (void);
/* APPLE LOCAL begin ARM darwin local binding */
#if TARGET_MACHO
static bool arm_binds_local_p (tree);
#endif
/* APPLE LOCAL end ARM darwin local binding */
/* APPLE LOCAL ARM 6008578 */
static HOST_WIDE_INT get_label_pad (rtx, HOST_WIDE_INT);

/* ALQAAHIRA LOCAL 6902792 Q register clobbers in inline asm */
static tree arm_md_asm_clobbers (tree, tree, tree);

/* Initialize the GCC target structure.  */
#if TARGET_DLLIMPORT_DECL_ATTRIBUTES
#undef  TARGET_MERGE_DECL_ATTRIBUTES
#define TARGET_MERGE_DECL_ATTRIBUTES merge_dllimport_decl_attributes
#endif

#undef  TARGET_ATTRIBUTE_TABLE
#define TARGET_ATTRIBUTE_TABLE arm_attribute_table

#undef TARGET_ASM_FILE_END
#define TARGET_ASM_FILE_END arm_file_end

/* APPLE LOCAL begin ARM asm file hooks */
#if TARGET_MACHO
#undef  TARGET_ASM_FILE_START
#define TARGET_ASM_FILE_START arm_darwin_file_start
#undef  TARGET_ASM_FILE_END
#define TARGET_ASM_FILE_END arm_darwin_file_end
#endif
/* APPLE LOCAL end ARM asm file hooks */

#ifdef AOF_ASSEMBLER
#undef  TARGET_ASM_BYTE_OP
#define TARGET_ASM_BYTE_OP "\tDCB\t"
#undef  TARGET_ASM_ALIGNED_HI_OP
#define TARGET_ASM_ALIGNED_HI_OP "\tDCW\t"
#undef  TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP "\tDCD\t"
#undef TARGET_ASM_GLOBALIZE_LABEL
#define TARGET_ASM_GLOBALIZE_LABEL aof_globalize_label
#undef TARGET_ASM_FILE_START
#define TARGET_ASM_FILE_START aof_file_start
#undef TARGET_ASM_FILE_END
#define TARGET_ASM_FILE_END aof_file_end
#else
#undef  TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP NULL
#undef  TARGET_ASM_INTEGER
#define TARGET_ASM_INTEGER arm_assemble_integer
#endif

/* APPLE LOCAL begin ARM MACH assembler */
#ifdef OBJECT_FORMAT_MACHO
#undef TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP "\t.long\t"
#endif
/* APPLE LOCAL end ARM MACH assembler */

#undef  TARGET_ASM_FUNCTION_PROLOGUE
#define TARGET_ASM_FUNCTION_PROLOGUE arm_output_function_prologue

#undef  TARGET_ASM_FUNCTION_EPILOGUE
#define TARGET_ASM_FUNCTION_EPILOGUE arm_output_function_epilogue

#undef  TARGET_DEFAULT_TARGET_FLAGS
/* ALQAAHIRA LOCAL 6216388 Don't schedule prologue by default */
#define TARGET_DEFAULT_TARGET_FLAGS (TARGET_DEFAULT)
#undef  TARGET_HANDLE_OPTION
#define TARGET_HANDLE_OPTION arm_handle_option

#undef  TARGET_COMP_TYPE_ATTRIBUTES
#define TARGET_COMP_TYPE_ATTRIBUTES arm_comp_type_attributes

#undef  TARGET_SET_DEFAULT_TYPE_ATTRIBUTES
#define TARGET_SET_DEFAULT_TYPE_ATTRIBUTES arm_set_default_type_attributes

#undef  TARGET_SCHED_ADJUST_COST
#define TARGET_SCHED_ADJUST_COST arm_adjust_cost

#undef TARGET_ENCODE_SECTION_INFO
#ifdef ARM_PE
#define TARGET_ENCODE_SECTION_INFO  arm_pe_encode_section_info
/* APPLE LOCAL begin ARM darwin section_info */
#elif  TARGET_MACHO
#define TARGET_ENCODE_SECTION_INFO  arm_darwin_encode_section_info
/* APPLE LOCAL end ARM darwin section_info */
#else
#define TARGET_ENCODE_SECTION_INFO  arm_encode_section_info
#endif

#undef  TARGET_STRIP_NAME_ENCODING
#define TARGET_STRIP_NAME_ENCODING arm_strip_name_encoding

#undef  TARGET_ASM_INTERNAL_LABEL
#define TARGET_ASM_INTERNAL_LABEL arm_internal_label

#undef  TARGET_FUNCTION_OK_FOR_SIBCALL
#define TARGET_FUNCTION_OK_FOR_SIBCALL arm_function_ok_for_sibcall

#undef  TARGET_ASM_OUTPUT_MI_THUNK
#define TARGET_ASM_OUTPUT_MI_THUNK arm_output_mi_thunk
#undef  TARGET_ASM_CAN_OUTPUT_MI_THUNK
#define TARGET_ASM_CAN_OUTPUT_MI_THUNK default_can_output_mi_thunk_no_vcall

/* This will be overridden in arm_override_options.  */
#undef  TARGET_RTX_COSTS
#define TARGET_RTX_COSTS arm_slowmul_rtx_costs
#undef  TARGET_ADDRESS_COST
#define TARGET_ADDRESS_COST arm_address_cost

#undef TARGET_SHIFT_TRUNCATION_MASK
#define TARGET_SHIFT_TRUNCATION_MASK arm_shift_truncation_mask
#undef TARGET_VECTOR_MODE_SUPPORTED_P
#define TARGET_VECTOR_MODE_SUPPORTED_P arm_vector_mode_supported_p

#undef  TARGET_MACHINE_DEPENDENT_REORG
#define TARGET_MACHINE_DEPENDENT_REORG arm_reorg

#undef  TARGET_INIT_BUILTINS
#define TARGET_INIT_BUILTINS  arm_init_builtins
#undef  TARGET_EXPAND_BUILTIN
#define TARGET_EXPAND_BUILTIN arm_expand_builtin

#undef TARGET_INIT_LIBFUNCS
#define TARGET_INIT_LIBFUNCS arm_init_libfuncs

#undef TARGET_PROMOTE_FUNCTION_ARGS
#define TARGET_PROMOTE_FUNCTION_ARGS hook_bool_tree_true
#undef TARGET_PROMOTE_FUNCTION_RETURN
#define TARGET_PROMOTE_FUNCTION_RETURN hook_bool_tree_true
#undef TARGET_PROMOTE_PROTOTYPES
#define TARGET_PROMOTE_PROTOTYPES arm_promote_prototypes
#undef TARGET_PASS_BY_REFERENCE
#define TARGET_PASS_BY_REFERENCE arm_pass_by_reference
#undef TARGET_ARG_PARTIAL_BYTES
#define TARGET_ARG_PARTIAL_BYTES arm_arg_partial_bytes

#undef  TARGET_SETUP_INCOMING_VARARGS
#define TARGET_SETUP_INCOMING_VARARGS arm_setup_incoming_varargs

#undef TARGET_DEFAULT_SHORT_ENUMS
#define TARGET_DEFAULT_SHORT_ENUMS arm_default_short_enums

#undef TARGET_ALIGN_ANON_BITFIELD
#define TARGET_ALIGN_ANON_BITFIELD arm_align_anon_bitfield

#undef TARGET_NARROW_VOLATILE_BITFIELD
#define TARGET_NARROW_VOLATILE_BITFIELD hook_bool_void_false

#undef TARGET_CXX_GUARD_TYPE
#define TARGET_CXX_GUARD_TYPE arm_cxx_guard_type

#undef TARGET_CXX_GUARD_MASK_BIT
#define TARGET_CXX_GUARD_MASK_BIT arm_cxx_guard_mask_bit

#undef TARGET_CXX_GET_COOKIE_SIZE
#define TARGET_CXX_GET_COOKIE_SIZE arm_get_cookie_size

#undef TARGET_CXX_COOKIE_HAS_SIZE
#define TARGET_CXX_COOKIE_HAS_SIZE arm_cookie_has_size

#undef TARGET_CXX_CDTOR_RETURNS_THIS
#define TARGET_CXX_CDTOR_RETURNS_THIS arm_cxx_cdtor_returns_this

#undef TARGET_CXX_KEY_METHOD_MAY_BE_INLINE
#define TARGET_CXX_KEY_METHOD_MAY_BE_INLINE arm_cxx_key_method_may_be_inline

#undef TARGET_CXX_USE_AEABI_ATEXIT
#define TARGET_CXX_USE_AEABI_ATEXIT arm_cxx_use_aeabi_atexit

#undef TARGET_CXX_DETERMINE_CLASS_DATA_VISIBILITY
#define TARGET_CXX_DETERMINE_CLASS_DATA_VISIBILITY \
  arm_cxx_determine_class_data_visibility

#undef TARGET_CXX_CLASS_DATA_ALWAYS_COMDAT
#define TARGET_CXX_CLASS_DATA_ALWAYS_COMDAT arm_cxx_class_data_always_comdat

#undef TARGET_RETURN_IN_MSB
#define TARGET_RETURN_IN_MSB arm_return_in_msb

#undef TARGET_MUST_PASS_IN_STACK
#define TARGET_MUST_PASS_IN_STACK arm_must_pass_in_stack

#ifdef TARGET_UNWIND_INFO
#undef TARGET_UNWIND_EMIT
#define TARGET_UNWIND_EMIT arm_unwind_emit

/* EABI unwinding tables use a different format for the typeinfo tables.  */
#undef TARGET_ASM_TTYPE
#define TARGET_ASM_TTYPE arm_output_ttype

#undef TARGET_ARM_EABI_UNWINDER
#define TARGET_ARM_EABI_UNWINDER true
#endif /* TARGET_UNWIND_INFO */

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
#undef TARGET_DWARF_HANDLE_FRAME_UNSPEC
#define TARGET_DWARF_HANDLE_FRAME_UNSPEC arm_dwarf_handle_frame_unspec
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
#undef TARGET_DWARF_REGISTER_SPAN
#define TARGET_DWARF_REGISTER_SPAN arm_dwarf_register_span
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

#undef  TARGET_CANNOT_COPY_INSN_P
#define TARGET_CANNOT_COPY_INSN_P arm_cannot_copy_insn_p

#ifdef HAVE_AS_TLS
#undef TARGET_HAVE_TLS
#define TARGET_HAVE_TLS true
#endif

#undef TARGET_CANNOT_FORCE_CONST_MEM
/* APPLE LOCAL ARM -mdynamic-no-pic support */
#define TARGET_CANNOT_FORCE_CONST_MEM arm_cannot_force_const_mem

/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
#undef TARGET_MAX_ANCHOR_OFFSET
#define TARGET_MAX_ANCHOR_OFFSET 4095

/* The minimum is set such that the total size of the block
   for a particular anchor is -4088 + 1 + 4095 bytes, which is
   divisible by eight, ensuring natural spacing of anchors.  */
#undef TARGET_MIN_ANCHOR_OFFSET
#define TARGET_MIN_ANCHOR_OFFSET -4088

/* ALQAAHIRA LOCAL v7 support. Merge from Codesourcery */
/* APPLE LOCAL begin ARM darwin local binding */
#if TARGET_MACHO
#undef TARGET_BINDS_LOCAL_P
#define TARGET_BINDS_LOCAL_P arm_binds_local_p
#endif
/* APPLE LOCAL end ARM darwin local binding */

/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
#undef TARGET_MANGLE_VECTOR_TYPE
#define TARGET_MANGLE_VECTOR_TYPE arm_mangle_vector_type

/* ALQAAHIRA LOCAL v7 support. Merge from Codesourcery */
/* APPLE LOCAL begin ARM reliable backtraces */
#undef TARGET_BUILTIN_SETJMP_FRAME_VALUE
#define TARGET_BUILTIN_SETJMP_FRAME_VALUE arm_builtin_setjmp_frame_value
/* APPLE LOCAL end ARM reliable backtraces */

/* ALQAAHIRA LOCAL begin 6902792 Q register clobbers in inline asm */
#undef TARGET_MD_ASM_CLOBBERS
#define TARGET_MD_ASM_CLOBBERS arm_md_asm_clobbers
/* ALQAAHIRA LOCAL end 6902792 Q register clobbers in inline asm */

struct gcc_target targetm = TARGET_INITIALIZER;

/* Obstack for minipool constant handling.  */
static struct obstack minipool_obstack;
static char *         minipool_startobj;

/* The maximum number of insns skipped which
   will be conditionalised if possible.  */
static int max_insns_skipped = 5;

extern FILE * asm_out_file;

/* True if we are currently building a constant table.  */
int making_const_table;

/* Define the information needed to generate branch insns.  This is
   stored from the compare operation.  */
rtx arm_compare_op0, arm_compare_op1;

/* The processor for which instructions should be scheduled.  */
enum processor_type arm_tune = arm_none;

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* The default processor used if not overriden by commandline.  */
static enum processor_type arm_default_cpu = arm_none;

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* Which floating point model to use.  */
enum arm_fp_model arm_fp_model;

/* Which floating point hardware is available.  */
enum fputype arm_fpu_arch;

/* Which floating point hardware to schedule for.  */
enum fputype arm_fpu_tune;

/* Whether to use floating point hardware.  */
enum float_abi_type arm_float_abi;

/* Which ABI to use.  */
enum arm_abi_type arm_abi;

/* Which thread pointer model to use.  */
enum arm_tp_type target_thread_pointer = TP_AUTO;

/* Used to parse -mstructure_size_boundary command line option.  */
int    arm_structure_size_boundary = DEFAULT_STRUCTURE_SIZE_BOUNDARY;

/* Used for Thumb call_via trampolines.  */
rtx thumb_call_via_label[14];
static int thumb_call_reg_needed;

/* ALQAAHIRA LOCAL 5571707 Allow R9 as caller-saved register */
static int darwin_reserve_r9_on_v6 = 0;

/* APPLE LOCAL begin ARM compact switch tables */
/* Keeps track of which *_switch* functions we've used, so we
   can emit the right stubs. */
static GTY(()) rtx switch8_libfunc;
static GTY(()) rtx switchu8_libfunc;
static GTY(()) rtx switch16_libfunc;
static GTY(()) rtx switch32_libfunc;
/* APPLE LOCAL end ARM compact switch tables */

/* Bit values used to identify processor capabilities.  */
#define FL_CO_PROC    (1 << 0)        /* Has external co-processor bus */
#define FL_ARCH3M     (1 << 1)        /* Extended multiply */
#define FL_MODE26     (1 << 2)        /* 26-bit mode support */
#define FL_MODE32     (1 << 3)        /* 32-bit mode support */
#define FL_ARCH4      (1 << 4)        /* Architecture rel 4 */
#define FL_ARCH5      (1 << 5)        /* Architecture rel 5 */
#define FL_THUMB      (1 << 6)        /* Thumb aware */
#define FL_LDSCHED    (1 << 7)	      /* Load scheduling necessary */
#define FL_STRONG     (1 << 8)	      /* StrongARM */
#define FL_ARCH5E     (1 << 9)        /* DSP extensions to v5 */
#define FL_XSCALE     (1 << 10)	      /* XScale */
#define FL_CIRRUS     (1 << 11)	      /* Cirrus/DSP.  */
#define FL_ARCH6      (1 << 12)       /* Architecture rel 6.  Adds
					 media instructions.  */
#define FL_VFPV2      (1 << 13)       /* Vector Floating Point V2.  */
#define FL_WBUF	      (1 << 14)	      /* Schedule for write buffer ops.
					 Note: ARM6 & 7 derivatives only.  */
#define FL_ARCH6K     (1 << 15)       /* Architecture rel 6 K extensions.  */
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
#define FL_THUMB2     (1 << 16)	      /* Thumb-2.  */
#define FL_NOTM	      (1 << 17)	      /* Instructions not present in the 'M'
					 profile.  */
#define FL_DIV	      (1 << 18)	      /* Hardware divde.  */
#define FL_VFPV3      (1 << 19)       /* Vector Floating Point V3.  */
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* ALQAAHIRA LOCAL v7 support. Merge from Codesourcery */
#define FL_NEON       (1 << 20)       /* Neon instructions.  */

#define FL_IWMMXT     (1 << 29)	      /* XScale v2 or "Intel Wireless MMX technology".  */

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
#define FL_FOR_ARCH2	FL_NOTM
#define FL_FOR_ARCH3	(FL_FOR_ARCH2 | FL_MODE32)
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
#define FL_FOR_ARCH3M	(FL_FOR_ARCH3 | FL_ARCH3M)
#define FL_FOR_ARCH4	(FL_FOR_ARCH3M | FL_ARCH4)
#define FL_FOR_ARCH4T	(FL_FOR_ARCH4 | FL_THUMB)
#define FL_FOR_ARCH5	(FL_FOR_ARCH4 | FL_ARCH5)
#define FL_FOR_ARCH5T	(FL_FOR_ARCH5 | FL_THUMB)
#define FL_FOR_ARCH5E	(FL_FOR_ARCH5 | FL_ARCH5E)
#define FL_FOR_ARCH5TE	(FL_FOR_ARCH5E | FL_THUMB)
#define FL_FOR_ARCH5TEJ	FL_FOR_ARCH5TE
#define FL_FOR_ARCH6	(FL_FOR_ARCH5TE | FL_ARCH6)
#define FL_FOR_ARCH6J	FL_FOR_ARCH6
#define FL_FOR_ARCH6K	(FL_FOR_ARCH6 | FL_ARCH6K)
#define FL_FOR_ARCH6Z	FL_FOR_ARCH6
#define FL_FOR_ARCH6ZK	FL_FOR_ARCH6K
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
#define FL_FOR_ARCH6T2	(FL_FOR_ARCH6 | FL_THUMB2)
#define FL_FOR_ARCH7	(FL_FOR_ARCH6T2 &~ FL_NOTM)
/* ALQAAHIRA LOCAL 6093388 -mfpu=neon default for v7a */
#define FL_FOR_ARCH7A	(FL_FOR_ARCH7 | FL_NOTM | FL_NEON)
#define FL_FOR_ARCH7R	(FL_FOR_ARCH7A | FL_DIV)
#define FL_FOR_ARCH7M	(FL_FOR_ARCH7 | FL_DIV)
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

/* The bits in this mask specify which
   instructions we are allowed to generate.  */
static unsigned long insn_flags = 0;

/* The bits in this mask specify which instruction scheduling options should
   be used.  */
static unsigned long tune_flags = 0;

/* The following are used in the arm.md file as equivalents to bits
   in the above two flag variables.  */

/* Nonzero if this chip supports the ARM Architecture 3M extensions.  */
int arm_arch3m = 0;

/* Nonzero if this chip supports the ARM Architecture 4 extensions.  */
int arm_arch4 = 0;

/* Nonzero if this chip supports the ARM Architecture 4t extensions.  */
int arm_arch4t = 0;

/* Nonzero if this chip supports the ARM Architecture 5 extensions.  */
int arm_arch5 = 0;

/* Nonzero if this chip supports the ARM Architecture 5E extensions.  */
int arm_arch5e = 0;

/* Nonzero if this chip supports the ARM Architecture 6 extensions.  */
int arm_arch6 = 0;

/* Nonzero if this chip supports the ARM 6K extensions.  */
int arm_arch6k = 0;

/* ALQAAHIRA LOCAL begin 6093388 -mfpu=neon default for v7a */
/* Nonzero if this chip supports the ARM 7A extensions.  */
int arm_arch7a = 0;
/* ALQAAHIRA LOCAL end 6093388 -mfpu=neon default for v7a */

/* Nonzero if instructions not present in the 'M' profile can be used.  */
int arm_arch_notm = 0;

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Nonzero if this chip can benefit from load scheduling.  */
int arm_ld_sched = 0;

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* Nonzero if this chip is a StrongARM.  */
int arm_tune_strongarm = 0;

/* Nonzero if this chip is a Cirrus variant.  */
int arm_arch_cirrus = 0;

/* Nonzero if this chip supports Intel Wireless MMX technology.  */
int arm_arch_iwmmxt = 0;

/* Nonzero if this chip is an XScale.  */
int arm_arch_xscale = 0;

/* Nonzero if tuning for XScale  */
int arm_tune_xscale = 0;

/* Nonzero if we want to tune for stores that access the write-buffer.
   This typically means an ARM6 or ARM7 with MMU or MPU.  */
int arm_tune_wbuf = 0;

/* Nonzero if generating Thumb instructions.  */
int thumb_code = 0;

/* Nonzero if we should define __THUMB_INTERWORK__ in the
   preprocessor.
   XXX This is a bit of a hack, it's intended to help work around
   problems in GLD which doesn't understand that armv5t code is
   interworking clean.  */
int arm_cpp_interwork = 0;

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Nonzero if chip supports Thumb 2.  */
int arm_arch_thumb2;

/* Nonzero if chip supports integer division instruction.  */
int arm_arch_hwdiv;

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* In case of a PRE_INC, POST_INC, PRE_DEC, POST_DEC memory reference, we
   must report the mode of the memory reference from PRINT_OPERAND to
   PRINT_OPERAND_ADDRESS.  */
enum machine_mode output_memory_reference_mode;

/* The register number to be used for the PIC offset register.  */
unsigned arm_pic_register = INVALID_REGNUM;

/* Set to 1 when a return insn is output, this means that the epilogue
   is not needed.  */
int return_used_this_function;

/* Set to 1 after arm_reorg has started.  Reset to start at the start of
   the next function.  */
static int after_arm_reorg = 0;

/* The maximum number of insns to be used when loading a constant.  */
static int arm_constant_limit = 3;

/* For an explanation of these variables, see final_prescan_insn below.  */
int arm_ccfsm_state;
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* arm_current_cc is also used for Thumb-2 cond_exec blocks.  */
enum arm_cond_code arm_current_cc;
rtx arm_target_insn;
int arm_target_label;
/* The number of conditionally executed insns, including the current insn.  */
int arm_condexec_count = 0;
/* A bitmask specifying the patterns for the IT block.
   Zero means do not output an IT block before this insn. */
int arm_condexec_mask = 0;
/* The number of bits used in arm_condexec_mask.  */
int arm_condexec_masklen = 0;
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

/* The condition codes of the ARM, and the inverse function.  */
static const char * const arm_condition_codes[] =
{
  "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
  "hi", "ls", "ge", "lt", "gt", "le", "al", "nv"
};

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
#define ARM_LSL_NAME (TARGET_UNIFIED_ASM ? "lsl" : "asl")
#define streq(string1, string2) (strcmp (string1, string2) == 0)

#define THUMB2_WORK_REGS (0xff & ~(  (1 << THUMB_HARD_FRAME_POINTER_REGNUM) \
				   | (1 << SP_REGNUM) | (1 << PC_REGNUM) \
				   | (1 << PIC_OFFSET_TABLE_REGNUM)))
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

/* Initialization code.  */

struct processors
{
  const char *const name;
  enum processor_type core;
  const char *arch;
  const unsigned long flags;
  bool (* rtx_costs) (rtx, int, int, int *);
};

/* Not all of these give usefully different compilation alternatives,
   but there is no simple way of generalizing them.  */
static const struct processors all_cores[] =
{
  /* ARM Cores */
#define ARM_CORE(NAME, IDENT, ARCH, FLAGS, COSTS) \
  {NAME, arm_none, #ARCH, FLAGS | FL_FOR_ARCH##ARCH, arm_##COSTS##_rtx_costs},
#include "arm-cores.def"
#undef ARM_CORE
  {NULL, arm_none, NULL, 0, NULL}
};

static const struct processors all_architectures[] =
{
  /* ARM Architectures */
  /* We don't specify rtx_costs here as it will be figured out
     from the core.  */

  {"armv2",   arm2,       "2",   FL_CO_PROC | FL_MODE26 | FL_FOR_ARCH2, NULL},
  {"armv2a",  arm2,       "2",   FL_CO_PROC | FL_MODE26 | FL_FOR_ARCH2, NULL},
  {"armv3",   arm6,       "3",   FL_CO_PROC | FL_MODE26 | FL_FOR_ARCH3, NULL},
  {"armv3m",  arm7m,      "3M",  FL_CO_PROC | FL_MODE26 | FL_FOR_ARCH3M, NULL},
  {"armv4",   arm7tdmi,   "4",   FL_CO_PROC | FL_MODE26 | FL_FOR_ARCH4, NULL},
  /* Strictly, FL_MODE26 is a permitted option for v4t, but there are no
     implementations that support it, so we will leave it out for now.  */
/* APPLE LOCAL begin ARM custom architectures */
#if TARGET_MACHO
  {"armv4t",  arm7tdmi,   "4T",  FL_CO_PROC |             FL_FOR_ARCH4, NULL},
#else
  {"armv4t",  arm7tdmi,   "4T",  FL_CO_PROC |             FL_FOR_ARCH4T, NULL},
#endif
/* APPLE LOCAL end ARM custom architectures */
  {"armv5",   arm10tdmi,  "5",   FL_CO_PROC |             FL_FOR_ARCH5, NULL},
  {"armv5t",  arm10tdmi,  "5T",  FL_CO_PROC |             FL_FOR_ARCH5T, NULL},
  {"armv5e",  arm1026ejs, "5E",  FL_CO_PROC |             FL_FOR_ARCH5E, NULL},
  {"armv5te", arm1026ejs, "5TE", FL_CO_PROC |             FL_FOR_ARCH5TE, NULL},
/* APPLE LOCAL begin ARM custom architectures */
#if TARGET_MACHO
  {"armv5tej",arm926ejs,  "5TEJ",FL_CO_PROC |             FL_FOR_ARCH5TEJ, NULL},
  {"xscale",  xscale,     "5TE", FL_CO_PROC | FL_XSCALE | FL_FOR_ARCH5TE, NULL},
  {"armv6",   arm1136jfs, "6",   FL_CO_PROC |             FL_FOR_ARCH6, NULL},
  {"armv6j",  arm1136js,  "6J",  FL_CO_PROC |             FL_FOR_ARCH6J, NULL},
  {"armv6k",  arm1136jfs, "6K",  FL_CO_PROC |             FL_FOR_ARCH6K, NULL},
#else
  {"armv6",   arm1136js,  "6",   FL_CO_PROC |             FL_FOR_ARCH6, NULL},
  {"armv6j",  arm1136js,  "6J",  FL_CO_PROC |             FL_FOR_ARCH6J, NULL},
  {"armv6k",  mpcore,	  "6K",  FL_CO_PROC |             FL_FOR_ARCH6K, NULL},
#endif
/* APPLE LOCAL end ARM custom architectures */
  {"armv6z",  arm1176jzs, "6Z",  FL_CO_PROC |             FL_FOR_ARCH6Z, NULL},
  {"armv6zk", arm1176jzs, "6ZK", FL_CO_PROC |             FL_FOR_ARCH6ZK, NULL},
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  {"armv6t2", arm1156t2s, "6T2", FL_CO_PROC |             FL_FOR_ARCH6T2, NULL},
  {"armv7",   cortexa8,	  "7",	 FL_CO_PROC |		  FL_FOR_ARCH7, NULL},
  {"armv7a",  cortexa8,	  "7A",	 FL_CO_PROC |		  FL_FOR_ARCH7A, NULL},
/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  {"armv7r",  cortexr4,	  "7R",	 FL_CO_PROC |		  FL_FOR_ARCH7R, NULL},
  {"armv7m",  cortexm3,	  "7M",	 FL_CO_PROC |		  FL_FOR_ARCH7M, NULL},
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  {"armv7-a", cortexa8,	  "7A",	 FL_CO_PROC |		  FL_FOR_ARCH7A, NULL},
  {"armv7-r", cortexr4,	  "7R",	 FL_CO_PROC |		  FL_FOR_ARCH7R, NULL},
  {"armv7-m", cortexm3,	  "7M",	 FL_CO_PROC |		  FL_FOR_ARCH7M, NULL},
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  {"ep9312",  ep9312,     "4T",  FL_LDSCHED | FL_CIRRUS | FL_FOR_ARCH4, NULL},
  {"iwmmxt",  iwmmxt,     "5TE", FL_LDSCHED | FL_STRONG | FL_FOR_ARCH5TE | FL_XSCALE | FL_IWMMXT , NULL},
  {NULL, arm_none, NULL, 0 , NULL}
};

struct arm_cpu_select
{
  const char *              string;
  const char *              name;
  const struct processors * processors;
};

/* This is a magic structure.  The 'string' field is magically filled in
   with a pointer to the value specified by the user on the command line
   assuming that the user has specified such a value.  */

static struct arm_cpu_select arm_select[] =
{
  /* string	  name            processors  */
  { NULL,	"-mcpu=",	all_cores  },
  { NULL,	"-march=",	all_architectures },
  { NULL,	"-mtune=",	all_cores }
};

/* Defines representing the indexes into the above table.  */
#define ARM_OPT_SET_CPU 0
#define ARM_OPT_SET_ARCH 1
#define ARM_OPT_SET_TUNE 2

/* The name of the preprocessor macro to define for this architecture.  */

/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
#define ARM_ARCH_NAME_SIZE 25
char arm_arch_name[ARM_ARCH_NAME_SIZE] = "__ARM_ARCH_0UNK__";
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

struct fpu_desc
{
  const char * name;
  enum fputype fpu;
};


/* Available values for -mfpu=.  */

static const struct fpu_desc all_fpus[] =
{
  {"fpa",	FPUTYPE_FPA},
  {"fpe2",	FPUTYPE_FPA_EMU2},
  {"fpe3",	FPUTYPE_FPA_EMU2},
  {"maverick",	FPUTYPE_MAVERICK},
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  {"vfp",	FPUTYPE_VFP},
  {"vfp3",	FPUTYPE_VFP3},
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* ALQAAHIRA LOCAL v7 support. Merge from Codesourcery */
  {"neon",	FPUTYPE_NEON}
};


/* Floating point models used by the different hardware.
   See fputype in arm.h.  */

static const enum fputype fp_model_for_fpu[] =
{
  /* No FP hardware.  */
  ARM_FP_MODEL_UNKNOWN,		/* FPUTYPE_NONE  */
  ARM_FP_MODEL_FPA,		/* FPUTYPE_FPA  */
  ARM_FP_MODEL_FPA,		/* FPUTYPE_FPA_EMU2  */
  ARM_FP_MODEL_FPA,		/* FPUTYPE_FPA_EMU3  */
  ARM_FP_MODEL_MAVERICK,	/* FPUTYPE_MAVERICK  */
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  ARM_FP_MODEL_VFP,		/* FPUTYPE_VFP  */
/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  ARM_FP_MODEL_VFP,		/* FPUTYPE_VFP3  */
  ARM_FP_MODEL_VFP		/* FPUTYPE_NEON  */
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
};


struct float_abi
{
  const char * name;
  enum float_abi_type abi_type;
};


/* Available values for -mfloat-abi=.  */

static const struct float_abi all_float_abis[] =
{
  {"soft",	ARM_FLOAT_ABI_SOFT},
  {"softfp",	ARM_FLOAT_ABI_SOFTFP},
  {"hard",	ARM_FLOAT_ABI_HARD}
};


struct abi_name
{
  const char *name;
  enum arm_abi_type abi_type;
};


/* Available values for -mabi=.  */

static const struct abi_name arm_all_abis[] =
{
  {"apcs-gnu",    ARM_ABI_APCS},
  {"atpcs",   ARM_ABI_ATPCS},
  {"aapcs",   ARM_ABI_AAPCS},
  {"iwmmxt",  ARM_ABI_IWMMXT},
  {"aapcs-linux",   ARM_ABI_AAPCS_LINUX}
};

/* Supported TLS relocations.  */

enum tls_reloc {
  TLS_GD32,
  TLS_LDM32,
  TLS_LDO32,
  TLS_IE32,
  TLS_LE32
};

/* Emit an insn that's a simple single-set.  Both the operands must be known
   to be valid.  */
inline static rtx
emit_set_insn (rtx x, rtx y)
{
  return emit_insn (gen_rtx_SET (VOIDmode, x, y));
}

/* Return the number of bits set in VALUE.  */
static unsigned
bit_count (unsigned long value)
{
  unsigned long count = 0;

  while (value)
    {
      count++;
      value &= value - 1;  /* Clear the least-significant set bit.  */
    }

  return count;
}

/* APPLE LOCAL begin ARM custom frame layout */
/* Generate a bitmask that has bits end:start (inclusive) set.  */
static unsigned long
inclusive_bitmask (int start, int end)
{
  return (((unsigned long) 1 << start) - 1)
	 ^ (((unsigned long) 1 << (end + 1)) - 1);
}
/* APPLE LOCAL end ARM custom frame layout */

/* APPLE LOCAL begin ARM compact switch tables */
/* These are library functions, but calls to them are not 
   represented as calls in the RTL because they do not have 
   normal function-call semantics.   We generate the
   Mach-O stuff lazily in this case. */

void register_switch8_libfunc (void)
{
#if TARGET_MACHO
  if (switch8_libfunc == NULL)
    switch8_libfunc = gen_rtx_SYMBOL_REF (Pmode, 
		ggc_alloc_string ("__switch8", sizeof ("__switch8")));
  if (flag_pic || MACHO_DYNAMIC_NO_PIC_P)
    machopic_validate_stub_or_non_lazy_ptr
	(machopic_indirection_name (switch8_libfunc, true));
#endif
}

void register_switchu8_libfunc (void)
{
#if TARGET_MACHO
  if (switchu8_libfunc == NULL)
    switchu8_libfunc = gen_rtx_SYMBOL_REF (Pmode, 
		ggc_alloc_string ("__switchu8", sizeof ("__switchu8")));
  if (flag_pic || MACHO_DYNAMIC_NO_PIC_P)
    machopic_validate_stub_or_non_lazy_ptr
	(machopic_indirection_name (switchu8_libfunc, true));
#endif
}

void register_switch16_libfunc (void)
{
#if TARGET_MACHO
  if (switch16_libfunc == NULL)
    switch16_libfunc = gen_rtx_SYMBOL_REF (Pmode, 
		ggc_alloc_string ("__switch16", sizeof ("__switch16")));
  if (flag_pic || MACHO_DYNAMIC_NO_PIC_P)
    machopic_validate_stub_or_non_lazy_ptr
	(machopic_indirection_name (switch16_libfunc, true));
#endif
}

void register_switch32_libfunc (void)
{
#if TARGET_MACHO
  if (switch32_libfunc == NULL)
    switch32_libfunc = gen_rtx_SYMBOL_REF (Pmode, 
		ggc_alloc_string ("__switch32", sizeof ("__switch32")));
  if (flag_pic || MACHO_DYNAMIC_NO_PIC_P)
    machopic_validate_stub_or_non_lazy_ptr
	(machopic_indirection_name (switch32_libfunc, true));
#endif
}
/* APPLE LOCAL end ARM compact switch tables */

/* Set up library functions unique to ARM.  */

static void
arm_init_libfuncs (void)
{
  /* APPLE LOCAL begin ARM 4702983 Thumb VFP math */
  if (TARGET_MACHO && TARGET_THUMB && !TARGET_SOFT_FLOAT
      && (flag_pic || MACHO_DYNAMIC_NO_PIC_P))
    {
      /* Double-precision floating-point arithmetic.  */
      set_optab_libfunc (add_optab, DFmode, "__adddf3vfp");
      set_optab_libfunc (sdiv_optab, DFmode, "__divdf3vfp");
      set_optab_libfunc (smul_optab, DFmode, "__muldf3vfp");
      set_optab_libfunc (neg_optab, DFmode, NULL);
      set_optab_libfunc (sub_optab, DFmode, "__subdf3vfp");

      /* Double-precision comparisons. */
      set_optab_libfunc (eq_optab, DFmode, "__eqdf2vfp");
      set_optab_libfunc (ne_optab, DFmode, "__nedf2vfp");
      set_optab_libfunc (lt_optab, DFmode, "__ltdf2vfp");
      set_optab_libfunc (le_optab, DFmode, "__ledf2vfp");
      set_optab_libfunc (ge_optab, DFmode, "__gedf2vfp");
      set_optab_libfunc (gt_optab, DFmode, "__gtdf2vfp");
      set_optab_libfunc (unord_optab, DFmode, "__unorddf2vfp");

      /* Single-precision floating-point arithmetic.  */
      set_optab_libfunc (add_optab, SFmode, "__addsf3vfp");
      set_optab_libfunc (sdiv_optab, SFmode, "__divsf3vfp");
      set_optab_libfunc (smul_optab, SFmode, "__mulsf3vfp");
      set_optab_libfunc (neg_optab, SFmode, NULL);
      set_optab_libfunc (sub_optab, SFmode, "__subsf3vfp");

      /* Single-precision comparisons.  */
      set_optab_libfunc (eq_optab, SFmode, "__eqsf2vfp");
      set_optab_libfunc (ne_optab, SFmode, "__nesf2vfp");
      set_optab_libfunc (lt_optab, SFmode, "__ltsf2vfp");
      set_optab_libfunc (le_optab, SFmode, "__lesf2vfp");
      set_optab_libfunc (ge_optab, SFmode, "__gesf2vfp");
      set_optab_libfunc (gt_optab, SFmode, "__gtsf2vfp");
      set_optab_libfunc (unord_optab, SFmode, "__unordsf2vfp");

      /* Floating-point to integer conversions.  */
      /* DImode conversions are done via library routines even
	 when generating VFP instructions, so use the same ones. */
      set_conv_libfunc (sfix_optab, SImode, DFmode, "__fixdfsivfp");
      set_conv_libfunc (ufix_optab, SImode, DFmode, "__fixunsdfsivfp");
      set_conv_libfunc (sfix_optab, SImode, SFmode, "__fixsfsivfp");
      set_conv_libfunc (ufix_optab, SImode, SFmode, "__fixunssfsivfp");

      /* Conversions between floating types. */
      set_conv_libfunc (trunc_optab, SFmode, DFmode, "__truncdfsf2vfp");
      set_conv_libfunc (sext_optab, DFmode, SFmode, "__extendsfdf2vfp");

      /* Integer to floating-point conversions.  */
      /* DImode conversions are done via library routines even
	 when generating VFP instructions, so use the same ones. */
      set_conv_libfunc (sfloat_optab, DFmode, SImode, "__floatsidfvfp");
      set_conv_libfunc (ufloat_optab, DFmode, SImode, "__floatunssidfvfp");
      set_conv_libfunc (sfloat_optab, SFmode, SImode, "__floatsisfvfp");
      set_conv_libfunc (ufloat_optab, SFmode, SImode, "__floatunssisfvfp");
      return;
    }
  /* APPLE LOCAL end ARM 4702983 Thumb VFP math */

  /* There are no special library functions unless we are using the
     ARM BPABI.  */
  if (!TARGET_BPABI)
    return;

  /* The functions below are described in Section 4 of the "Run-Time
     ABI for the ARM architecture", Version 1.0.  */

  /* Double-precision floating-point arithmetic.  Table 2.  */
  set_optab_libfunc (add_optab, DFmode, "__aeabi_dadd");
  set_optab_libfunc (sdiv_optab, DFmode, "__aeabi_ddiv");
  set_optab_libfunc (smul_optab, DFmode, "__aeabi_dmul");
  set_optab_libfunc (neg_optab, DFmode, "__aeabi_dneg");
  set_optab_libfunc (sub_optab, DFmode, "__aeabi_dsub");

  /* Double-precision comparisons.  Table 3.  */
  set_optab_libfunc (eq_optab, DFmode, "__aeabi_dcmpeq");
  set_optab_libfunc (ne_optab, DFmode, NULL);
  set_optab_libfunc (lt_optab, DFmode, "__aeabi_dcmplt");
  set_optab_libfunc (le_optab, DFmode, "__aeabi_dcmple");
  set_optab_libfunc (ge_optab, DFmode, "__aeabi_dcmpge");
  set_optab_libfunc (gt_optab, DFmode, "__aeabi_dcmpgt");
  set_optab_libfunc (unord_optab, DFmode, "__aeabi_dcmpun");

  /* Single-precision floating-point arithmetic.  Table 4.  */
  set_optab_libfunc (add_optab, SFmode, "__aeabi_fadd");
  set_optab_libfunc (sdiv_optab, SFmode, "__aeabi_fdiv");
  set_optab_libfunc (smul_optab, SFmode, "__aeabi_fmul");
  set_optab_libfunc (neg_optab, SFmode, "__aeabi_fneg");
  set_optab_libfunc (sub_optab, SFmode, "__aeabi_fsub");

  /* Single-precision comparisons.  Table 5.  */
  set_optab_libfunc (eq_optab, SFmode, "__aeabi_fcmpeq");
  set_optab_libfunc (ne_optab, SFmode, NULL);
  set_optab_libfunc (lt_optab, SFmode, "__aeabi_fcmplt");
  set_optab_libfunc (le_optab, SFmode, "__aeabi_fcmple");
  set_optab_libfunc (ge_optab, SFmode, "__aeabi_fcmpge");
  set_optab_libfunc (gt_optab, SFmode, "__aeabi_fcmpgt");
  set_optab_libfunc (unord_optab, SFmode, "__aeabi_fcmpun");

  /* Floating-point to integer conversions.  Table 6.  */
  set_conv_libfunc (sfix_optab, SImode, DFmode, "__aeabi_d2iz");
  set_conv_libfunc (ufix_optab, SImode, DFmode, "__aeabi_d2uiz");
  set_conv_libfunc (sfix_optab, DImode, DFmode, "__aeabi_d2lz");
  set_conv_libfunc (ufix_optab, DImode, DFmode, "__aeabi_d2ulz");
  set_conv_libfunc (sfix_optab, SImode, SFmode, "__aeabi_f2iz");
  set_conv_libfunc (ufix_optab, SImode, SFmode, "__aeabi_f2uiz");
  set_conv_libfunc (sfix_optab, DImode, SFmode, "__aeabi_f2lz");
  set_conv_libfunc (ufix_optab, DImode, SFmode, "__aeabi_f2ulz");

  /* Conversions between floating types.  Table 7.  */
  set_conv_libfunc (trunc_optab, SFmode, DFmode, "__aeabi_d2f");
  set_conv_libfunc (sext_optab, DFmode, SFmode, "__aeabi_f2d");

  /* Integer to floating-point conversions.  Table 8.  */
  set_conv_libfunc (sfloat_optab, DFmode, SImode, "__aeabi_i2d");
  set_conv_libfunc (ufloat_optab, DFmode, SImode, "__aeabi_ui2d");
  set_conv_libfunc (sfloat_optab, DFmode, DImode, "__aeabi_l2d");
  set_conv_libfunc (ufloat_optab, DFmode, DImode, "__aeabi_ul2d");
  set_conv_libfunc (sfloat_optab, SFmode, SImode, "__aeabi_i2f");
  set_conv_libfunc (ufloat_optab, SFmode, SImode, "__aeabi_ui2f");
  set_conv_libfunc (sfloat_optab, SFmode, DImode, "__aeabi_l2f");
  set_conv_libfunc (ufloat_optab, SFmode, DImode, "__aeabi_ul2f");

  /* Long long.  Table 9.  */
  set_optab_libfunc (smul_optab, DImode, "__aeabi_lmul");
  set_optab_libfunc (sdivmod_optab, DImode, "__aeabi_ldivmod");
  set_optab_libfunc (udivmod_optab, DImode, "__aeabi_uldivmod");
  set_optab_libfunc (ashl_optab, DImode, "__aeabi_llsl");
  set_optab_libfunc (lshr_optab, DImode, "__aeabi_llsr");
  set_optab_libfunc (ashr_optab, DImode, "__aeabi_lasr");
  set_optab_libfunc (cmp_optab, DImode, "__aeabi_lcmp");
  set_optab_libfunc (ucmp_optab, DImode, "__aeabi_ulcmp");

  /* Integer (32/32->32) division.  \S 4.3.1.  */
  set_optab_libfunc (sdivmod_optab, SImode, "__aeabi_idivmod");
  set_optab_libfunc (udivmod_optab, SImode, "__aeabi_uidivmod");

  /* The divmod functions are designed so that they can be used for
     plain division, even though they return both the quotient and the
     remainder.  The quotient is returned in the usual location (i.e.,
     r0 for SImode, {r0, r1} for DImode), just as would be expected
     for an ordinary division routine.  Because the AAPCS calling
     conventions specify that all of { r0, r1, r2, r3 } are
     callee-saved registers, there is no need to tell the compiler
     explicitly that those registers are clobbered by these
     routines.  */
  set_optab_libfunc (sdiv_optab, DImode, "__aeabi_ldivmod");
  set_optab_libfunc (udiv_optab, DImode, "__aeabi_uldivmod");

  /* For SImode division the ABI provides div-without-mod routines,
     which are faster.  */
  set_optab_libfunc (sdiv_optab, SImode, "__aeabi_idiv");
  set_optab_libfunc (udiv_optab, SImode, "__aeabi_uidiv");

  /* We don't have mod libcalls.  Fortunately gcc knows how to use the
     divmod libcalls instead.  */
  set_optab_libfunc (smod_optab, DImode, NULL);
  set_optab_libfunc (umod_optab, DImode, NULL);
  set_optab_libfunc (smod_optab, SImode, NULL);
  set_optab_libfunc (umod_optab, SImode, NULL);
}

/* Implement TARGET_HANDLE_OPTION.  */

static bool
arm_handle_option (size_t code, const char *arg, int value ATTRIBUTE_UNUSED)
{
  switch (code)
    {
    case OPT_march_:
      arm_select[1].string = arg;
      return true;

    case OPT_mcpu_:
      arm_select[0].string = arg;
      return true;

    case OPT_mhard_float:
      target_float_abi_name = "hard";
      return true;

    case OPT_msoft_float:
      target_float_abi_name = "soft";
      return true;

    case OPT_mtune_:
      arm_select[2].string = arg;
      return true;

    default:
      return true;
    }
}

/* Fix up any incompatible options that the user has specified.
   This has now turned into a maze.  */
void
arm_override_options (void)
{
  unsigned i;
/* ALQAAHIRA LOCAL v7 support. Merge from Codesourcery */
  int len;
  enum processor_type target_arch_cpu = arm_none;

  /* Set up the flags based on the cpu/architecture selected by the user.  */
  for (i = ARRAY_SIZE (arm_select); i--;)
    {
      struct arm_cpu_select * ptr = arm_select + i;

      if (ptr->string != NULL && ptr->string[0] != '\0')
        {
	  const struct processors * sel;

          for (sel = ptr->processors; sel->name != NULL; sel++)
            if (streq (ptr->string, sel->name))
              {
		/* Set the architecture define.  */
		if (i != ARM_OPT_SET_TUNE)
/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
		  {
		    len = snprintf (arm_arch_name, ARM_ARCH_NAME_SIZE,
				    "__ARM_ARCH_%s__", sel->arch);
		    gcc_assert (len < ARM_ARCH_NAME_SIZE);
		  }
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

		/* Determine the processor core for which we should
		   tune code-generation.  */
		if (/* -mcpu= is a sensible default.  */
		    i == ARM_OPT_SET_CPU
		    /* -mtune= overrides -mcpu= and -march=.  */
		    || i == ARM_OPT_SET_TUNE)
		  arm_tune = (enum processor_type) (sel - ptr->processors);

		/* Remember the CPU associated with this architecture.
		   If no other option is used to set the CPU type,
		   we'll use this to guess the most suitable tuning
		   options.  */
		if (i == ARM_OPT_SET_ARCH)
		  target_arch_cpu = sel->core;

		if (i != ARM_OPT_SET_TUNE)
		  {
/* APPLE LOCAL begin ARM darwin driver */
#if !TARGET_MACHO
		    /* If we have been given an architecture and a processor
		       make sure that they are compatible.  We only generate
		       a warning though, and we prefer the CPU over the
		       architecture.  */
		    if (insn_flags != 0 && (insn_flags ^ sel->flags))
		      warning (0, "switch -mcpu=%s conflicts with -march= switch",
			       ptr->string);
#else
		    /* More likely the -march was inherited from -arch which
		       had to be given to the darwin driver to get the correct
		       compiler.  So, make it relatively painless to specify
		       -mcpu=... by not warning that it supercedes -march.   */
#endif
/* APPLE LOCAL end ARM darwin driver */

		    insn_flags = sel->flags;
		  }

                break;
              }

          if (sel->name == NULL)
            error ("bad value (%s) for %s switch", ptr->string, ptr->name);
        }
    }

  /* Guess the tuning options from the architecture if necessary.  */
  if (arm_tune == arm_none)
    arm_tune = target_arch_cpu;

  /* If the user did not specify a processor, choose one for them.  */
  if (insn_flags == 0)
    {
      const struct processors * sel;
      unsigned int        sought;
      enum processor_type cpu;

      cpu = TARGET_CPU_DEFAULT;
      if (cpu == arm_none)
	{
#ifdef SUBTARGET_CPU_DEFAULT
	  /* Use the subtarget default CPU if none was specified by
	     configure.  */
	  cpu = SUBTARGET_CPU_DEFAULT;
#endif
	  /* Default to ARM6.  */
	  if (cpu == arm_none)
	    cpu = arm6;
	}
      sel = &all_cores[cpu];

      insn_flags = sel->flags;

      /* Now check to see if the user has specified some command line
	 switch that require certain abilities from the cpu.  */
      sought = 0;

      if (TARGET_INTERWORK || TARGET_THUMB)
	{
	  sought |= (FL_THUMB | FL_MODE32);

	  /* There are no ARM processors that support both APCS-26 and
	     interworking.  Therefore we force FL_MODE26 to be removed
	     from insn_flags here (if it was set), so that the search
	     below will always be able to find a compatible processor.  */
	  insn_flags &= ~FL_MODE26;
	}

      if (sought != 0 && ((sought & insn_flags) != sought))
	{
	  /* Try to locate a CPU type that supports all of the abilities
	     of the default CPU, plus the extra abilities requested by
	     the user.  */
	  for (sel = all_cores; sel->name != NULL; sel++)
	    if ((sel->flags & sought) == (sought | insn_flags))
	      break;

	  if (sel->name == NULL)
	    {
	      unsigned current_bit_count = 0;
	      const struct processors * best_fit = NULL;

	      /* Ideally we would like to issue an error message here
		 saying that it was not possible to find a CPU compatible
		 with the default CPU, but which also supports the command
		 line options specified by the programmer, and so they
		 ought to use the -mcpu=<name> command line option to
		 override the default CPU type.

		 If we cannot find a cpu that has both the
		 characteristics of the default cpu and the given
		 command line options we scan the array again looking
		 for a best match.  */
	      for (sel = all_cores; sel->name != NULL; sel++)
		if ((sel->flags & sought) == sought)
		  {
		    unsigned count;

		    count = bit_count (sel->flags & insn_flags);

		    if (count >= current_bit_count)
		      {
			best_fit = sel;
			current_bit_count = count;
		      }
		  }

	      gcc_assert (best_fit);
	      sel = best_fit;
	    }

	  insn_flags = sel->flags;
	}
/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */

      len = snprintf (arm_arch_name, ARM_ARCH_NAME_SIZE,
		      "__ARM_ARCH_%s__", sel->arch);
      gcc_assert (len < ARM_ARCH_NAME_SIZE);

      arm_default_cpu = (enum processor_type) (sel - all_cores);
      if (arm_tune == arm_none)
	arm_tune = arm_default_cpu;
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
    }

  /* The processor for which we should tune should now have been
     chosen.  */
  gcc_assert (arm_tune != arm_none);

  tune_flags = all_cores[(int)arm_tune].flags;
  if (optimize_size)
    targetm.rtx_costs = arm_size_rtx_costs;
  else
    targetm.rtx_costs = all_cores[(int)arm_tune].rtx_costs;

  /* Make sure that the processor choice does not conflict with any of the
     other command line choices.  */
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (TARGET_ARM && !(insn_flags & FL_NOTM))
    error ("target CPU does not support ARM mode");
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

  if (TARGET_INTERWORK && !(insn_flags & FL_THUMB))
    {
      /* APPLE LOCAL begin ARM interworking */
      /* Don't emit warning for MACHO -- see radar://4825264 */
      if (! TARGET_MACHO)
	warning (0, "target CPU does not support interworking" );

      interwork_option = 0;
      /* APPLE LOCAL end ARM interworking */
    }

  /* ALQAAHIRA LOCAL begin 6150882 use thumb2 by default for v7 */
  /* If we're compiling for v7, we should default to using thumb2
     codegen. */
  if ((insn_flags & FL_FOR_ARCH7A) == FL_FOR_ARCH7A 
      && thumb_option < 0)
    thumb_option = 1;
  /* ALQAAHIRA LOCAL end 6150882 use thumb2 by default for v7 */

  if (TARGET_THUMB && !(insn_flags & FL_THUMB))
    {
      warning (0, "target CPU does not support THUMB instructions");
      /* ALQAAHIRA LOCAL 6150882 use thumb2 by default for v7 */
      thumb_option = 0;
    }

  if (TARGET_APCS_FRAME && TARGET_THUMB)
    {
      /* warning (0, "ignoring -mapcs-frame because -mthumb was used"); */
      target_flags &= ~MASK_APCS_FRAME;
    }

  /* Callee super interworking implies thumb interworking.  Adding
     this to the flags here simplifies the logic elsewhere.  */
  if (TARGET_THUMB && TARGET_CALLEE_INTERWORKING)
    /* APPLE LOCAL ARM interworking */
    interwork_option = 1;

  /* TARGET_BACKTRACE calls leaf_function_p, which causes a crash if done
     from here where no function is being compiled currently.  */
  if ((TARGET_TPCS_FRAME || TARGET_TPCS_LEAF_FRAME) && TARGET_ARM)
    warning (0, "enabling backtrace support is only meaningful when compiling for the Thumb");

  if (TARGET_ARM && TARGET_CALLEE_INTERWORKING)
    warning (0, "enabling callee interworking support is only meaningful when compiling for the Thumb");

  if (TARGET_ARM && TARGET_CALLER_INTERWORKING)
    warning (0, "enabling caller interworking support is only meaningful when compiling for the Thumb");

  if (TARGET_APCS_STACK && !TARGET_APCS_FRAME)
    {
      warning (0, "-mapcs-stack-check incompatible with -mno-apcs-frame");
      target_flags |= MASK_APCS_FRAME;
    }

  if (TARGET_POKE_FUNCTION_NAME)
    target_flags |= MASK_APCS_FRAME;

  if (TARGET_APCS_REENT && flag_pic)
    error ("-fpic and -mapcs-reent are incompatible");

  if (TARGET_APCS_REENT)
    warning (0, "APCS reentrant code not supported.  Ignored");

  /* If this target is normally configured to use APCS frames, warn if they
     are turned off and debugging is turned on.  */
  if (TARGET_ARM
      && write_symbols != NO_DEBUG
      && !TARGET_APCS_FRAME
      && (TARGET_DEFAULT & MASK_APCS_FRAME))
    warning (0, "-g with -mno-apcs-frame may not give sensible debugging");

  /* If stack checking is disabled, we can use r10 as the PIC register,
     which keeps r9 available.  */
  /* APPLE LOCAL ARM pic support */
  if (flag_pic && TARGET_SINGLE_PIC_BASE && !TARGET_MACHO)
    arm_pic_register = TARGET_APCS_STACK ? 9 : 10;

  if (TARGET_APCS_FLOAT)
    warning (0, "passing floating point arguments in fp regs not yet supported");

  /* Initialize boolean versions of the flags, for use in the arm.md file.  */
  arm_arch3m = (insn_flags & FL_ARCH3M) != 0;
  arm_arch4 = (insn_flags & FL_ARCH4) != 0;
  arm_arch4t = arm_arch4 & ((insn_flags & FL_THUMB) != 0);
  arm_arch5 = (insn_flags & FL_ARCH5) != 0;
  arm_arch5e = (insn_flags & FL_ARCH5E) != 0;
  arm_arch6 = (insn_flags & FL_ARCH6) != 0;
  arm_arch6k = (insn_flags & FL_ARCH6K) != 0;
  arm_arch7a = (insn_flags & FL_FOR_ARCH7A) == FL_FOR_ARCH7A;
  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  arm_arch_notm = (insn_flags & FL_NOTM) != 0;
  arm_arch_thumb2 = (insn_flags & FL_THUMB2) != 0;
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  arm_arch_xscale = (insn_flags & FL_XSCALE) != 0;
  arm_arch_cirrus = (insn_flags & FL_CIRRUS) != 0;

  arm_ld_sched = (tune_flags & FL_LDSCHED) != 0;
  arm_tune_strongarm = (tune_flags & FL_STRONG) != 0;
  thumb_code = (TARGET_ARM == 0);
  arm_tune_wbuf = (tune_flags & FL_WBUF) != 0;
  arm_tune_xscale = (tune_flags & FL_XSCALE) != 0;
  arm_arch_iwmmxt = (insn_flags & FL_IWMMXT) != 0;
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  arm_arch_hwdiv = (insn_flags & FL_DIV) != 0;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  /* If we are not using the default (ARM mode) section anchor offset
     ranges, then set the correct ranges now.  */
  if (TARGET_THUMB1)
    {
      /* Thumb-1 LDR instructions cannot have negative offsets.
         Permissible positive offset ranges are 5-bit (for byte loads),
         6-bit (for halfword loads), or 7-bit (for word loads).
         Empirical results suggest a 7-bit anchor range gives the best
         overall code size.  */
      targetm.min_anchor_offset = 0;
      targetm.max_anchor_offset = 127;
    }
  else if (TARGET_THUMB2)
    {
      /* The minimum is set such that the total size of the block
         for a particular anchor is 248 + 1 + 4095 bytes, which is
         divisible by eight, ensuring natural spacing of anchors.  */
      targetm.min_anchor_offset = -248;
      targetm.max_anchor_offset = 4095;
    }

  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  /* APPLE LOCAL begin ARM interworking */
  /* Choose a default interworking setting if not specified on the
     command line.  */
  if (interwork_option == -1)
    interwork_option = arm_arch5 ? 1 : 0;

  /* XXX However, we must pass the right pre-processor defines to CPP
     or GLD can get confused.  This is a hack.  */
  if (TARGET_INTERWORK)
    arm_cpp_interwork = 1;
  /* APPLE LOCAL end ARM interworking */

  if (target_abi_name)
    {
      for (i = 0; i < ARRAY_SIZE (arm_all_abis); i++)
	{
	  if (streq (arm_all_abis[i].name, target_abi_name))
	    {
	      arm_abi = arm_all_abis[i].abi_type;
	      break;
	    }
	}
      if (i == ARRAY_SIZE (arm_all_abis))
	error ("invalid ABI option: -mabi=%s", target_abi_name);
    }
  else
    arm_abi = ARM_DEFAULT_ABI;

  if (TARGET_IWMMXT && !ARM_DOUBLEWORD_ALIGN)
    error ("iwmmxt requires an AAPCS compatible ABI for proper operation");

  if (TARGET_IWMMXT_ABI && !TARGET_IWMMXT)
    error ("iwmmxt abi requires an iwmmxt capable cpu");

  arm_fp_model = ARM_FP_MODEL_UNKNOWN;
  if (target_fpu_name == NULL && target_fpe_name != NULL)
    {
      if (streq (target_fpe_name, "2"))
	target_fpu_name = "fpe2";
      else if (streq (target_fpe_name, "3"))
	target_fpu_name = "fpe3";
      else
	error ("invalid floating point emulation option: -mfpe=%s",
	       target_fpe_name);
    }
  if (target_fpu_name != NULL)
    {
      /* The user specified a FPU.  */
      for (i = 0; i < ARRAY_SIZE (all_fpus); i++)
	{
	  if (streq (all_fpus[i].name, target_fpu_name))
	    {
	      arm_fpu_arch = all_fpus[i].fpu;
	      arm_fpu_tune = arm_fpu_arch;
	      arm_fp_model = fp_model_for_fpu[arm_fpu_arch];
	      break;
	    }
	}
      if (arm_fp_model == ARM_FP_MODEL_UNKNOWN)
	error ("invalid floating point option: -mfpu=%s", target_fpu_name);
    }
  else
    {
#ifdef FPUTYPE_DEFAULT
      /* Use the default if it is specified for this platform.  */
      arm_fpu_arch = FPUTYPE_DEFAULT;
      arm_fpu_tune = FPUTYPE_DEFAULT;
#else
      /* Pick one based on CPU type.  */
      /* ??? Some targets assume FPA is the default.
      if ((insn_flags & FL_VFP) != 0)
	arm_fpu_arch = FPUTYPE_VFP;
      else
      */
      if (arm_arch_cirrus)
	arm_fpu_arch = FPUTYPE_MAVERICK;
      else
	arm_fpu_arch = FPUTYPE_FPA_EMU2;
#endif
      if (tune_flags & FL_CO_PROC && arm_fpu_arch == FPUTYPE_FPA_EMU2)
	arm_fpu_tune = FPUTYPE_FPA;
      else
	arm_fpu_tune = arm_fpu_arch;
      arm_fp_model = fp_model_for_fpu[arm_fpu_arch];
      gcc_assert (arm_fp_model != ARM_FP_MODEL_UNKNOWN);
    }

  if (target_float_abi_name != NULL)
    {
      /* The user specified a FP ABI.  */
      for (i = 0; i < ARRAY_SIZE (all_float_abis); i++)
	{
	  if (streq (all_float_abis[i].name, target_float_abi_name))
	    {
	      arm_float_abi = all_float_abis[i].abi_type;
	      break;
	    }
	}
      if (i == ARRAY_SIZE (all_float_abis))
	error ("invalid floating point abi: -mfloat-abi=%s",
	       target_float_abi_name);
    }
  else
    arm_float_abi = TARGET_DEFAULT_FLOAT_ABI;

  if (arm_float_abi == ARM_FLOAT_ABI_HARD && TARGET_VFP)
    sorry ("-mfloat-abi=hard and VFP");

  /* FPA and iWMMXt are incompatible because the insn encodings overlap.
     VFP and iWMMXt can theoretically coexist, but it's unlikely such silicon
     will ever exist.  GCC makes no attempt to support this combination.  */
  if (TARGET_IWMMXT && !TARGET_SOFT_FLOAT)
    sorry ("iWMMXt and hardware floating point");

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* ??? iWMMXt insn patterns need auditing for Thumb-2.  */
  if (TARGET_THUMB2 && TARGET_IWMMXT)
    sorry ("Thumb-2 iWMMXt");

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  /* If soft-float is specified then don't use FPU.  */
  if (TARGET_SOFT_FLOAT)
    arm_fpu_arch = FPUTYPE_NONE;

  /* For arm2/3 there is no need to do any scheduling if there is only
     a floating point emulator, or we are doing software floating-point.  */
  if ((TARGET_SOFT_FLOAT
       || arm_fpu_tune == FPUTYPE_FPA_EMU2
       || arm_fpu_tune == FPUTYPE_FPA_EMU3)
      && (tune_flags & FL_MODE32) == 0)
    flag_schedule_insns = flag_schedule_insns_after_reload = 0;

  if (target_thread_switch)
    {
      if (strcmp (target_thread_switch, "soft") == 0)
	target_thread_pointer = TP_SOFT;
      else if (strcmp (target_thread_switch, "auto") == 0)
	target_thread_pointer = TP_AUTO;
      else if (strcmp (target_thread_switch, "cp15") == 0)
	target_thread_pointer = TP_CP15;
      else
	error ("invalid thread pointer option: -mtp=%s", target_thread_switch);
    }

  /* Use the cp15 method if it is available.  */
  if (target_thread_pointer == TP_AUTO)
    {
      if (arm_arch6k && !TARGET_THUMB)
	target_thread_pointer = TP_CP15;
      else
	target_thread_pointer = TP_SOFT;
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (TARGET_HARD_TP && TARGET_THUMB1)
    error ("can not use -mtp=cp15 with 16-bit Thumb");
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

  /* Override the default structure alignment for AAPCS ABI.  */
  if (TARGET_AAPCS_BASED)
    arm_structure_size_boundary = 8;

  if (structure_size_string != NULL)
    {
      int size = strtol (structure_size_string, NULL, 0);

      if (size == 8 || size == 32
	  || (ARM_DOUBLEWORD_ALIGN && size == 64))
	arm_structure_size_boundary = size;
      else
	warning (0, "structure size boundary can only be set to %s",
		 ARM_DOUBLEWORD_ALIGN ? "8, 32 or 64": "8 or 32");
    }

  if (arm_pic_register_string != NULL)
    {
      int pic_register = decode_reg_name (arm_pic_register_string);

      if (!flag_pic)
	warning (0, "-mpic-register= is useless without -fpic");

      /* Prevent the user from choosing an obviously stupid PIC register.  */
      else if (pic_register < 0 || call_used_regs[pic_register]
	       || pic_register == HARD_FRAME_POINTER_REGNUM
	       || pic_register == STACK_POINTER_REGNUM
	       || pic_register >= PC_REGNUM)
	error ("unable to use '%s' for PIC register", arm_pic_register_string);
      else
	arm_pic_register = pic_register;
    }

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  /* ??? We might want scheduling for thumb2.  */
  if (TARGET_THUMB && flag_schedule_insns)
    {
      /* Don't warn since it's on by default in -O2.  */
      flag_schedule_insns = 0;
    }

  if (optimize_size)
    {
      arm_constant_limit = 1;

      /* If optimizing for size, bump the number of instructions that we
         are prepared to conditionally execute (even on a StrongARM).  */
      max_insns_skipped = 6;
    }
  else
    {
      /* For processors with load scheduling, it never costs more than
         2 cycles to load a constant, and the load scheduler may well
	 reduce that to 1.  */
      if (arm_ld_sched)
        arm_constant_limit = 1;

      /* On XScale the longer latency of a load makes it more difficult
         to achieve a good schedule, so it's faster to synthesize
	 constants that can be done in two insns.  */
      if (arm_tune_xscale)
        arm_constant_limit = 2;

      /* StrongARM has early execution of branches, so a sequence
         that is worth skipping is shorter.  */
      if (arm_tune_strongarm)
        max_insns_skipped = 3;
    }

/* APPLE LOCAL begin ARM darwin options */
#ifdef SUBTARGET_OVERRIDE_OPTIONS
  SUBTARGET_OVERRIDE_OPTIONS;
#endif

#ifdef SUBSUBTARGET_OVERRIDE_OPTIONS
  SUBSUBTARGET_OVERRIDE_OPTIONS;
#endif
/* APPLE LOCAL end ARM darwin options */

  /* Register global variables with the garbage collector.  */
  arm_add_gc_roots ();
}

static void
arm_add_gc_roots (void)
{
  gcc_obstack_init(&minipool_obstack);
  minipool_startobj = (char *) obstack_alloc (&minipool_obstack, 0);
}

/* A table of known ARM exception types.
   For use with the interrupt function attribute.  */

typedef struct
{
  const char *const arg;
  const unsigned long return_value;
}
isr_attribute_arg;

static const isr_attribute_arg isr_attribute_args [] =
{
  { "IRQ",   ARM_FT_ISR },
  { "irq",   ARM_FT_ISR },
  { "FIQ",   ARM_FT_FIQ },
  { "fiq",   ARM_FT_FIQ },
  { "ABORT", ARM_FT_ISR },
  { "abort", ARM_FT_ISR },
  { "ABORT", ARM_FT_ISR },
  { "abort", ARM_FT_ISR },
  { "UNDEF", ARM_FT_EXCEPTION },
  { "undef", ARM_FT_EXCEPTION },
  { "SWI",   ARM_FT_EXCEPTION },
  { "swi",   ARM_FT_EXCEPTION },
  { NULL,    ARM_FT_NORMAL }
};

/* Returns the (interrupt) function type of the current
   function, or ARM_FT_UNKNOWN if the type cannot be determined.  */

static unsigned long
arm_isr_value (tree argument)
{
  const isr_attribute_arg * ptr;
  const char *              arg;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (!arm_arch_notm)
    return ARM_FT_NORMAL | ARM_FT_STACKALIGN;

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  /* No argument - default to IRQ.  */
  if (argument == NULL_TREE)
    return ARM_FT_ISR;

  /* Get the value of the argument.  */
  if (TREE_VALUE (argument) == NULL_TREE
      || TREE_CODE (TREE_VALUE (argument)) != STRING_CST)
    return ARM_FT_UNKNOWN;

  arg = TREE_STRING_POINTER (TREE_VALUE (argument));

  /* Check it against the list of known arguments.  */
  for (ptr = isr_attribute_args; ptr->arg != NULL; ptr++)
    if (streq (arg, ptr->arg))
      return ptr->return_value;

  /* An unrecognized interrupt type.  */
  return ARM_FT_UNKNOWN;
}

/* Computes the type of the current function.  */

static unsigned long
arm_compute_func_type (void)
{
  unsigned long type = ARM_FT_UNKNOWN;
  tree a;
  tree attr;

  gcc_assert (TREE_CODE (current_function_decl) == FUNCTION_DECL);

  /* Decide if the current function is volatile.  Such functions
     never return, and many memory cycles can be saved by not storing
     register values that will never be needed again.  This optimization
     was added to speed up context switching in a kernel application.  */
  if (optimize > 0
      && (TREE_NOTHROW (current_function_decl)
          || !(flag_unwind_tables
               || (flag_exceptions && !USING_SJLJ_EXCEPTIONS)))
      && TREE_THIS_VOLATILE (current_function_decl))
    type |= ARM_FT_VOLATILE;

  if (cfun->static_chain_decl != NULL)
    type |= ARM_FT_NESTED;

  attr = DECL_ATTRIBUTES (current_function_decl);

  a = lookup_attribute ("naked", attr);
  if (a != NULL_TREE)
    type |= ARM_FT_NAKED;

  a = lookup_attribute ("isr", attr);
  if (a == NULL_TREE)
    a = lookup_attribute ("interrupt", attr);

  if (a == NULL_TREE)
    /* APPLE LOCAL ARM interworking */
    type |= (TARGET_INTERWORK && !arm_arch5) ? ARM_FT_INTERWORKED : ARM_FT_NORMAL;
  else
    type |= arm_isr_value (TREE_VALUE (a));

  return type;
}

/* Returns the type of the current function.  */

unsigned long
arm_current_func_type (void)
{
  if (ARM_FUNC_TYPE (cfun->machine->func_type) == ARM_FT_UNKNOWN)
    cfun->machine->func_type = arm_compute_func_type ();

  return cfun->machine->func_type;
}

/* APPLE LOCAL begin ARM indirect sibcalls */
/* Look for an indirect sibling call that uses a callee-saved reg.
   We'll need to copy this reg to IP and change the call, since
   the callee-saved reg will be clobbered by the restore of the old
   value.   (This negates the code size advantage of the sibcall, but
   not the gain in stack size at runtime.) */

static int
indirect_sibreturn_reg (rtx sibling, bool *is_value)
{
  if (GET_CODE (sibling) == CALL_INSN
      && GET_CODE (PATTERN (sibling)) == PARALLEL
      && GET_CODE (XVECEXP (PATTERN (sibling), 0, 0)) == CALL
      && GET_CODE (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0)) == MEM
      && GET_CODE (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0), 0)) == REG)
    {
      *is_value = 0;
      return REGNO (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0), 0));
    }
  if (GET_CODE (sibling) == CALL_INSN
      && GET_CODE (PATTERN (sibling)) == PARALLEL
      && GET_CODE (XVECEXP (PATTERN (sibling), 0, 0)) == SET
      && GET_CODE (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1)) == CALL
      && GET_CODE (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0)) == MEM
      && GET_CODE (XEXP (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0), 0)) == REG)
    {
      *is_value = 1;
      return REGNO (XEXP (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0), 0));
    }
  return -1;
}

/* Look for an indirect sibling call that uses a memory location, at
   reg + or - constant; this will be a stack location, but registers
   other than SP and FP are possible with large stack frames.
   We'll need to load this location into IP and change the call, since
   a memory location is not valid in the instruction.  (The usual approach
   of forcing reload to copy the value into a register through predicates
   and constraints will not work here, as the load would come out after
   the restore of FP and SP, too late.)
   Return value = signed offset from register *reg (usually SP or FP).
   Null if this case doesn't apply.
   We do not check for offsets too big to fit in a load, nor offsets in a
   register; it is believed that these cases cannot occur. */

static rtx
indirect_sibreturn_mem (rtx sibling, rtx* reg, bool *is_value)
{
  rtx mem = NULL_RTX;
  if (GET_CODE (sibling) == CALL_INSN
      && GET_CODE (PATTERN (sibling)) == PARALLEL
      && GET_CODE (XVECEXP (PATTERN (sibling), 0, 0)) == CALL
      && GET_CODE (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0)) == MEM
      && GET_CODE (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0), 0)) == MEM)
    {
      *is_value = 0;
      mem = XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0), 0);
    }
  else if (GET_CODE (sibling) == CALL_INSN
      && GET_CODE (PATTERN (sibling)) == PARALLEL
      && GET_CODE (XVECEXP (PATTERN (sibling), 0, 0)) == SET
      && GET_CODE (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1)) == CALL
      && GET_CODE (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0)) == MEM
      && GET_CODE (XEXP (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0), 0)) == MEM)
    {
      *is_value = 1;
      mem = XEXP (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0), 0);
    }
 if (mem 
     && GET_CODE (XEXP (mem, 0)) == PLUS
     && GET_CODE (XEXP (XEXP (mem, 0), 0)) == REG
     && GET_CODE (XEXP (XEXP (mem, 0), 1)) == CONST_INT)
    {
      *reg = XEXP (XEXP (mem, 0), 0);
      return XEXP (XEXP (mem, 0), 1);
    }
  else if (mem && GET_CODE (XEXP (mem, 0)) == REG)
    {
      *reg = XEXP (mem, 0);
      return const0_rtx;
    }
  return NULL_RTX;
}
/* APPLE LOCAL end ARM indirect sibcalls */

/* Return 1 if it is possible to return using a single instruction.
   If SIBLING is non-null, this is a test for a return before a sibling
   call.  SIBLING is the call insn, so we can examine its register usage.  */

int
use_return_insn (int iscond, rtx sibling)
{
  int regno;
  unsigned int func_type;
  unsigned long saved_int_regs;
  unsigned HOST_WIDE_INT stack_adjust;
  arm_stack_offsets *offsets;

  /* Never use a return instruction before reload has run.  */
  if (!reload_completed)
    return 0;

  func_type = arm_current_func_type ();

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* Naked, volatile and stack alignment functions need special
     consideration.  */
  if (func_type & (ARM_FT_VOLATILE | ARM_FT_NAKED | ARM_FT_STACKALIGN))
    return 0;
  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  /* So do interrupt functions that use the frame pointer and Thumb
     interrupt functions.  */
  if (IS_INTERRUPT (func_type) && (frame_pointer_needed || TARGET_THUMB))
    return 0;
  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

  offsets = arm_get_frame_offsets ();
  stack_adjust = offsets->outgoing_args - offsets->saved_regs;

  /* As do variadic functions.  */
  if (current_function_pretend_args_size
      || cfun->machine->uses_anonymous_args
      /* Or if the function calls __builtin_eh_return () */
      || current_function_calls_eh_return
      /* Or if the function calls alloca */
      || current_function_calls_alloca
      /* APPLE LOCAL begin ARM custom frame layout */
      /* Or if there is a stack adjustment.  */
      || !(stack_adjust == 0))
      /* APPLE LOCAL end ARM custom frame layout */
    return 0;

  saved_int_regs = arm_compute_save_reg_mask ();

  /* Unfortunately, the insn

       ldmib sp, {..., sp, ...}

     triggers a bug on most SA-110 based devices, such that the stack
     pointer won't be correctly restored if the instruction takes a
     page fault.  We work around this problem by popping r3 along with
     the other registers, since that is never slower than executing
     another instruction.

     We test for !arm_arch5 here, because code for any architecture
     less than this could potentially be run on one of the buggy
     chips.  */
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (stack_adjust == 4 && !arm_arch5 && TARGET_ARM)
    {
      /* Validate that r3 is a call-clobbered register (always true in
	 the default abi) ...  */
      if (!call_used_regs[3])
	return 0;

      /* ... that it isn't being used for a return value ... */
      if (arm_size_return_regs () >= (4 * UNITS_PER_WORD))
	return 0;

      /* ... or for a tail-call argument ...  */
      if (sibling)
	{
	  gcc_assert (GET_CODE (sibling) == CALL_INSN);

	  if (find_regno_fusage (sibling, USE, 3))
	    return 0;

	  /* APPLE LOCAL begin ARM indirect sibcalls */
	  /* ... or to hold the target address for an indirect sibcall. */
	  {
	    bool ignored;
	    int regno = indirect_sibreturn_reg (sibling, &ignored);
	    if (regno == 3)
	      return 0;
	  }
	  /* APPLE LOCAL end ARM indirect sibcalls */
	}

      /* ... and that there are no call-saved registers in r0-r2
	 (always true in the default ABI).  */
      if (saved_int_regs & 0x7)
	return 0;
    }

  /* Can't be done if interworking with Thumb, and any registers have been
     stacked.  */
  /* APPLE LOCAL ARM interworking */
  if (TARGET_INTERWORK && !arm_arch5 && saved_int_regs != 0)
    return 0;

  /* On StrongARM, conditional returns are expensive if they aren't
     taken and multiple registers have been stacked.  */
  if (iscond && arm_tune_strongarm)
    {
      /* Conditional return when just the LR is stored is a simple
	 conditional-load instruction, that's not expensive.  */
      if (saved_int_regs != 0 && saved_int_regs != (1 << LR_REGNUM))
	return 0;

      if (flag_pic 
	  && arm_pic_register != INVALID_REGNUM
	  && regs_ever_live[PIC_OFFSET_TABLE_REGNUM])
	return 0;
    }

  /* If there are saved registers but the LR isn't saved, then we need
     two instructions for the return.  */
  if (saved_int_regs && !(saved_int_regs & (1 << LR_REGNUM)))
    return 0;

  /* APPLE LOCAL begin ARM indirect sibcalls */
  /* If we have an indirect sibcall that uses a saved reg, we'll need
     to copy that value into IP before restoring. */
  if (sibling)
    {
      bool ignored;
      int regno = indirect_sibreturn_reg (sibling, &ignored);
      if (regno > 3 && regno != 12)
	return 0;
      if (regno == -1)
	return 0;
    }
  /* APPLE LOCAL end ARM indirect sibcalls */

  /* Can't be done if any of the FPA regs are pushed,
     since this also requires an insn.  */
  if (TARGET_HARD_FLOAT && TARGET_FPA)
    for (regno = FIRST_FPA_REGNUM; regno <= LAST_FPA_REGNUM; regno++)
      if (regs_ever_live[regno] && !call_used_regs[regno])
	return 0;

  /* Likewise VFP regs.  */
  if (TARGET_HARD_FLOAT && TARGET_VFP)
    for (regno = FIRST_VFP_REGNUM; regno <= LAST_VFP_REGNUM; regno++)
      if (regs_ever_live[regno] && !call_used_regs[regno])
	return 0;

  if (TARGET_REALLY_IWMMXT)
    for (regno = FIRST_IWMMXT_REGNUM; regno <= LAST_IWMMXT_REGNUM; regno++)
      if (regs_ever_live[regno] && ! call_used_regs [regno])
	return 0;

  /* APPLE LOCAL begin ARM custom frame layout */
  /* If anything above the frame pointer was saved, they were saved
     below r0, which means we have to pop them in a separate
     instruction.  */
  if (saved_int_regs & (1 << LR_REGNUM))
    for (regno = ARM_HARD_FRAME_POINTER_REGNUM + 1; regno <= 11; regno++)
      if (saved_int_regs & (1 << regno))
	return 0;
  /* APPLE LOCAL end ARM custom frame layout */

  return 1;
}

/* Return TRUE if int I is a valid immediate ARM constant.  */

int
const_ok_for_arm (HOST_WIDE_INT i)
{
  int lowbit;

  /* For machines with >32 bit HOST_WIDE_INT, the bits above bit 31 must
     be all zero, or all one.  */
  if ((i & ~(unsigned HOST_WIDE_INT) 0xffffffff) != 0
      && ((i & ~(unsigned HOST_WIDE_INT) 0xffffffff)
	  != ((~(unsigned HOST_WIDE_INT) 0)
	      & ~(unsigned HOST_WIDE_INT) 0xffffffff)))
    return FALSE;

  i &= (unsigned HOST_WIDE_INT) 0xffffffff;

  /* Fast return for 0 and small values.  We must do this for zero, since
     the code below can't handle that one case.  */
  if ((i & ~(unsigned HOST_WIDE_INT) 0xff) == 0)
    return TRUE;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* Get the number of trailing zeros.  */
  lowbit = ffs((int) i) - 1;
  
  /* Only even shifts are allowed in ARM mode so round down to the
     nearest even number.  */
  if (TARGET_ARM)
    lowbit &= ~1;

  if ((i & ~(((unsigned HOST_WIDE_INT) 0xff) << lowbit)) == 0)
    return TRUE;

  if (TARGET_ARM)
    {
      /* Allow rotated constants in ARM mode.  */
      if (lowbit <= 4
	   && ((i & ~0xc000003f) == 0
	       || (i & ~0xf000000f) == 0
	       || (i & ~0xfc000003) == 0))
	return TRUE;
    }
  else
    {
      HOST_WIDE_INT v;

      /* Allow repeated pattern.  */
      v = i & 0xff;
      v |= v << 16;
      if (i == v || i == (v | (v << 8)))
	return TRUE;
    }

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  return FALSE;
}

/* Return true if I is a valid constant for the operation CODE.  */
static int
const_ok_for_op (HOST_WIDE_INT i, enum rtx_code code)
{
  if (const_ok_for_arm (i))
    return 1;

  switch (code)
    {
    case PLUS:
      return const_ok_for_arm (ARM_SIGN_EXTEND (-i));

    case MINUS:		/* Should only occur with (MINUS I reg) => rsb */
    case XOR:
    case IOR:
      return 0;

    case AND:
      return const_ok_for_arm (ARM_SIGN_EXTEND (~i));

    default:
      gcc_unreachable ();
    }
}

/* Emit a sequence of insns to handle a large constant.
   CODE is the code of the operation required, it can be any of SET, PLUS,
   IOR, AND, XOR, MINUS;
   MODE is the mode in which the operation is being performed;
   VAL is the integer to operate on;
   SOURCE is the other operand (a register, or a null-pointer for SET);
   SUBTARGETS means it is safe to create scratch registers if that will
   either produce a simpler sequence, or we will want to cse the values.
   Return value is the number of insns emitted.  */

/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
/* ??? Tweak this for thumb2.  */
int
arm_split_constant (enum rtx_code code, enum machine_mode mode, rtx insn,
		    HOST_WIDE_INT val, rtx target, rtx source, int subtargets)
{
  rtx cond;

  if (insn && GET_CODE (PATTERN (insn)) == COND_EXEC)
    cond = COND_EXEC_TEST (PATTERN (insn));
  else
    cond = NULL_RTX;

  if (subtargets || code == SET
      || (GET_CODE (target) == REG && GET_CODE (source) == REG
	  && REGNO (target) != REGNO (source)))
    {
      /* After arm_reorg has been called, we can't fix up expensive
	 constants by pushing them into memory so we must synthesize
	 them in-line, regardless of the cost.  This is only likely to
	 be more costly on chips that have load delay slots and we are
	 compiling without running the scheduler (so no splitting
	 occurred before the final instruction emission).

	 Ref: gcc -O1 -mcpu=strongarm gcc.c-torture/compile/980506-2.c
      */
      if (!after_arm_reorg
	  && !cond
	  && (arm_gen_constant (code, mode, NULL_RTX, val, target, source,
				1, 0)
	      > arm_constant_limit + (code != SET)))
	{
	  if (code == SET)
	    {
	      /* Currently SET is the only monadic value for CODE, all
		 the rest are diadic.  */
	      emit_set_insn (target, GEN_INT (val));
	      return 1;
	    }
	  else
	    {
	      rtx temp = subtargets ? gen_reg_rtx (mode) : target;

	      emit_set_insn (temp, GEN_INT (val));
	      /* For MINUS, the value is subtracted from, since we never
		 have subtraction of a constant.  */
	      if (code == MINUS)
		emit_set_insn (target, gen_rtx_MINUS (mode, temp, source));
	      else
		emit_set_insn (target,
			       gen_rtx_fmt_ee (code, mode, source, temp));
	      return 2;
	    }
	}
    }

  return arm_gen_constant (code, mode, cond, val, target, source, subtargets,
			   1);
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Return the number of ARM instructions required to synthesize the given
   constant.  */
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
static int
count_insns_for_constant (HOST_WIDE_INT remainder, int i)
{
  HOST_WIDE_INT temp1;
  int num_insns = 0;
  do
    {
      int end;

      if (i <= 0)
	i += 32;
      if (remainder & (3 << (i - 2)))
	{
	  end = i - 8;
	  if (end < 0)
	    end += 32;
	  temp1 = remainder & ((0x0ff << end)
				    | ((i < end) ? (0xff >> (32 - end)) : 0));
	  remainder &= ~temp1;
	  num_insns++;
	  i -= 6;
	}
      i -= 2;
    } while (remainder);
  return num_insns;
}

/* Emit an instruction with the indicated PATTERN.  If COND is
   non-NULL, conditionalize the execution of the instruction on COND
   being true.  */

static void
emit_constant_insn (rtx cond, rtx pattern)
{
  if (cond)
    pattern = gen_rtx_COND_EXEC (VOIDmode, copy_rtx (cond), pattern);
  emit_insn (pattern);
}

/* As above, but extra parameter GENERATE which, if clear, suppresses
   RTL generation.  */
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
/* ??? This needs more work for thumb2.  */

static int
arm_gen_constant (enum rtx_code code, enum machine_mode mode, rtx cond,
		  HOST_WIDE_INT val, rtx target, rtx source, int subtargets,
		  int generate)
{
  int can_invert = 0;
  int can_negate = 0;
  int can_negate_initial = 0;
  int can_shift = 0;
  int i;
  int num_bits_set = 0;
  int set_sign_bit_copies = 0;
  int clear_sign_bit_copies = 0;
  int clear_zero_bit_copies = 0;
  int set_zero_bit_copies = 0;
  int insns = 0;
  unsigned HOST_WIDE_INT temp1, temp2;
  unsigned HOST_WIDE_INT remainder = val & 0xffffffff;

  /* Find out which operations are safe for a given CODE.  Also do a quick
     check for degenerate cases; these can occur when DImode operations
     are split.  */
  switch (code)
    {
    case SET:
      can_invert = 1;
      can_shift = 1;
      can_negate = 1;
      break;

    case PLUS:
      can_negate = 1;
      can_negate_initial = 1;
      break;

    case IOR:
      if (remainder == 0xffffffff)
	{
	  if (generate)
	    emit_constant_insn (cond,
				gen_rtx_SET (VOIDmode, target,
					     GEN_INT (ARM_SIGN_EXTEND (val))));
	  return 1;
	}
      if (remainder == 0)
	{
	  if (reload_completed && rtx_equal_p (target, source))
	    return 0;
	  if (generate)
	    emit_constant_insn (cond,
				gen_rtx_SET (VOIDmode, target, source));
	  return 1;
	}
      break;

    case AND:
      if (remainder == 0)
	{
	  if (generate)
	    emit_constant_insn (cond,
				gen_rtx_SET (VOIDmode, target, const0_rtx));
	  return 1;
	}
      if (remainder == 0xffffffff)
	{
	  if (reload_completed && rtx_equal_p (target, source))
	    return 0;
	  if (generate)
	    emit_constant_insn (cond,
				gen_rtx_SET (VOIDmode, target, source));
	  return 1;
	}
      can_invert = 1;
      break;

    case XOR:
      if (remainder == 0)
	{
	  if (reload_completed && rtx_equal_p (target, source))
	    return 0;
	  if (generate)
	    emit_constant_insn (cond,
				gen_rtx_SET (VOIDmode, target, source));
	  return 1;
	}

      /* We don't know how to handle other cases yet.  */
      gcc_assert (remainder == 0xffffffff);

      if (generate)
	emit_constant_insn (cond,
			    gen_rtx_SET (VOIDmode, target,
					 gen_rtx_NOT (mode, source)));
      return 1;

    case MINUS:
      /* We treat MINUS as (val - source), since (source - val) is always
	 passed as (source + (-val)).  */
      if (remainder == 0)
	{
	  if (generate)
	    emit_constant_insn (cond,
				gen_rtx_SET (VOIDmode, target,
					     gen_rtx_NEG (mode, source)));
	  return 1;
	}
      if (const_ok_for_arm (val))
	{
	  if (generate)
	    emit_constant_insn (cond,
				gen_rtx_SET (VOIDmode, target,
					     gen_rtx_MINUS (mode, GEN_INT (val),
							    source)));
	  return 1;
	}
      can_negate = 1;

      break;

    default:
      gcc_unreachable ();
    }

  /* If we can do it in one insn get out quickly.  */
  if (const_ok_for_arm (val)
      || (can_negate_initial && const_ok_for_arm (-val))
      || (can_invert && const_ok_for_arm (~val)))
    {
      if (generate)
	emit_constant_insn (cond,
			    gen_rtx_SET (VOIDmode, target,
					 (source
					  ? gen_rtx_fmt_ee (code, mode, source,
							    GEN_INT (val))
					  : GEN_INT (val))));
      return 1;
    }

  /* Calculate a few attributes that may be useful for specific
     optimizations.  */
  for (i = 31; i >= 0; i--)
    {
      if ((remainder & (1 << i)) == 0)
	clear_sign_bit_copies++;
      else
	break;
    }

  for (i = 31; i >= 0; i--)
    {
      if ((remainder & (1 << i)) != 0)
	set_sign_bit_copies++;
      else
	break;
    }

  for (i = 0; i <= 31; i++)
    {
      if ((remainder & (1 << i)) == 0)
	clear_zero_bit_copies++;
      else
	break;
    }

  for (i = 0; i <= 31; i++)
    {
      if ((remainder & (1 << i)) != 0)
	set_zero_bit_copies++;
      else
	break;
    }

  switch (code)
    {
    case SET:
      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      /* See if we can use movw.  */
      if (arm_arch_thumb2 && (remainder & 0xffff0000) == 0)
	{
	  if (generate)
	    emit_constant_insn (cond, gen_rtx_SET (VOIDmode, target,
						   GEN_INT (val)));
	  return 1;
	}

      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
      /* See if we can do this by sign_extending a constant that is known
	 to be negative.  This is a good, way of doing it, since the shift
	 may well merge into a subsequent insn.  */
      if (set_sign_bit_copies > 1)
	{
	  if (const_ok_for_arm
	      (temp1 = ARM_SIGN_EXTEND (remainder
					<< (set_sign_bit_copies - 1))))
	    {
	      if (generate)
		{
		  rtx new_src = subtargets ? gen_reg_rtx (mode) : target;
		  emit_constant_insn (cond,
				      gen_rtx_SET (VOIDmode, new_src,
						   GEN_INT (temp1)));
		  emit_constant_insn (cond,
				      gen_ashrsi3 (target, new_src,
						   GEN_INT (set_sign_bit_copies - 1)));
		}
	      return 2;
	    }
	  /* For an inverted constant, we will need to set the low bits,
	     these will be shifted out of harm's way.  */
	  temp1 |= (1 << (set_sign_bit_copies - 1)) - 1;
	  if (const_ok_for_arm (~temp1))
	    {
	      if (generate)
		{
		  rtx new_src = subtargets ? gen_reg_rtx (mode) : target;
		  emit_constant_insn (cond,
				      gen_rtx_SET (VOIDmode, new_src,
						   GEN_INT (temp1)));
		  emit_constant_insn (cond,
				      gen_ashrsi3 (target, new_src,
						   GEN_INT (set_sign_bit_copies - 1)));
		}
	      return 2;
	    }
	}

      /* See if we can calculate the value as the difference between two
	 valid immediates.  */
      if (clear_sign_bit_copies + clear_zero_bit_copies <= 16)
	{
	  int topshift = clear_sign_bit_copies & ~1;

	  temp1 = ARM_SIGN_EXTEND ((remainder + (0x00800000 >> topshift))
				   & (0xff000000 >> topshift));

	  /* If temp1 is zero, then that means the 9 most significant
	     bits of remainder were 1 and we've caused it to overflow.
	     When topshift is 0 we don't need to do anything since we
	     can borrow from 'bit 32'.  */
	  if (temp1 == 0 && topshift != 0)
	    temp1 = 0x80000000 >> (topshift - 1);

	  temp2 = ARM_SIGN_EXTEND (temp1 - remainder);

	  if (const_ok_for_arm (temp2))
	    {
	      if (generate)
		{
		  rtx new_src = subtargets ? gen_reg_rtx (mode) : target;
		  emit_constant_insn (cond,
				      gen_rtx_SET (VOIDmode, new_src,
						   GEN_INT (temp1)));
		  emit_constant_insn (cond,
				      gen_addsi3 (target, new_src,
						  GEN_INT (-temp2)));
		}

	      return 2;
	    }
	}

      /* See if we can generate this by setting the bottom (or the top)
	 16 bits, and then shifting these into the other half of the
	 word.  We only look for the simplest cases, to do more would cost
	 too much.  Be careful, however, not to generate this when the
	 alternative would take fewer insns.  */
      if (val & 0xffff0000)
	{
	  temp1 = remainder & 0xffff0000;
	  temp2 = remainder & 0x0000ffff;

	  /* Overlaps outside this range are best done using other methods.  */
	  for (i = 9; i < 24; i++)
	    {
	      if ((((temp2 | (temp2 << i)) & 0xffffffff) == remainder)
		  && !const_ok_for_arm (temp2))
		{
		  rtx new_src = (subtargets
				 ? (generate ? gen_reg_rtx (mode) : NULL_RTX)
				 : target);
		  insns = arm_gen_constant (code, mode, cond, temp2, new_src,
					    source, subtargets, generate);
		  source = new_src;
		  if (generate)
		    emit_constant_insn
		      (cond,
		       gen_rtx_SET
		       (VOIDmode, target,
			gen_rtx_IOR (mode,
				     gen_rtx_ASHIFT (mode, source,
						     GEN_INT (i)),
				     source)));
		  return insns + 1;
		}
	    }

	  /* Don't duplicate cases already considered.  */
	  for (i = 17; i < 24; i++)
	    {
	      if (((temp1 | (temp1 >> i)) == remainder)
		  && !const_ok_for_arm (temp1))
		{
		  rtx new_src = (subtargets
				 ? (generate ? gen_reg_rtx (mode) : NULL_RTX)
				 : target);
		  insns = arm_gen_constant (code, mode, cond, temp1, new_src,
					    source, subtargets, generate);
		  source = new_src;
		  if (generate)
		    emit_constant_insn
		      (cond,
		       gen_rtx_SET (VOIDmode, target,
				    gen_rtx_IOR
				    (mode,
				     gen_rtx_LSHIFTRT (mode, source,
						       GEN_INT (i)),
				     source)));
		  return insns + 1;
		}
	    }
	}
      break;

    case IOR:
    case XOR:
      /* If we have IOR or XOR, and the constant can be loaded in a
	 single instruction, and we can find a temporary to put it in,
	 then this can be done in two instructions instead of 3-4.  */
      if (subtargets
	  /* TARGET can't be NULL if SUBTARGETS is 0 */
	  || (reload_completed && !reg_mentioned_p (target, source)))
	{
	  if (const_ok_for_arm (ARM_SIGN_EXTEND (~val)))
	    {
	      if (generate)
		{
		  rtx sub = subtargets ? gen_reg_rtx (mode) : target;

		  emit_constant_insn (cond,
				      gen_rtx_SET (VOIDmode, sub,
						   GEN_INT (val)));
		  emit_constant_insn (cond,
				      gen_rtx_SET (VOIDmode, target,
						   gen_rtx_fmt_ee (code, mode,
								   source, sub)));
		}
	      return 2;
	    }
	}

      if (code == XOR)
	break;

      if (set_sign_bit_copies > 8
	  && (val & (-1 << (32 - set_sign_bit_copies))) == val)
	{
	  if (generate)
	    {
	      rtx sub = subtargets ? gen_reg_rtx (mode) : target;
	      rtx shift = GEN_INT (set_sign_bit_copies);

	      emit_constant_insn
		(cond,
		 gen_rtx_SET (VOIDmode, sub,
			      gen_rtx_NOT (mode,
					   gen_rtx_ASHIFT (mode,
							   source,
							   shift))));
	      emit_constant_insn
		(cond,
		 gen_rtx_SET (VOIDmode, target,
			      gen_rtx_NOT (mode,
					   gen_rtx_LSHIFTRT (mode, sub,
							     shift))));
	    }
	  return 2;
	}

      if (set_zero_bit_copies > 8
	  && (remainder & ((1 << set_zero_bit_copies) - 1)) == remainder)
	{
	  if (generate)
	    {
	      rtx sub = subtargets ? gen_reg_rtx (mode) : target;
	      rtx shift = GEN_INT (set_zero_bit_copies);

	      emit_constant_insn
		(cond,
		 gen_rtx_SET (VOIDmode, sub,
			      gen_rtx_NOT (mode,
					   gen_rtx_LSHIFTRT (mode,
							     source,
							     shift))));
	      emit_constant_insn
		(cond,
		 gen_rtx_SET (VOIDmode, target,
			      gen_rtx_NOT (mode,
					   gen_rtx_ASHIFT (mode, sub,
							   shift))));
	    }
	  return 2;
	}

      if (const_ok_for_arm (temp1 = ARM_SIGN_EXTEND (~val)))
	{
	  if (generate)
	    {
	      rtx sub = subtargets ? gen_reg_rtx (mode) : target;
	      emit_constant_insn (cond,
				  gen_rtx_SET (VOIDmode, sub,
					       gen_rtx_NOT (mode, source)));
	      source = sub;
	      if (subtargets)
		sub = gen_reg_rtx (mode);
	      emit_constant_insn (cond,
				  gen_rtx_SET (VOIDmode, sub,
					       gen_rtx_AND (mode, source,
							    GEN_INT (temp1))));
	      emit_constant_insn (cond,
				  gen_rtx_SET (VOIDmode, target,
					       gen_rtx_NOT (mode, sub)));
	    }
	  return 3;
	}
      break;

    case AND:
      /* See if two shifts will do 2 or more insn's worth of work.  */
      if (clear_sign_bit_copies >= 16 && clear_sign_bit_copies < 24)
	{
	  HOST_WIDE_INT shift_mask = ((0xffffffff
				       << (32 - clear_sign_bit_copies))
				      & 0xffffffff);

	  if ((remainder | shift_mask) != 0xffffffff)
	    {
	      if (generate)
		{
		  rtx new_src = subtargets ? gen_reg_rtx (mode) : target;
		  insns = arm_gen_constant (AND, mode, cond,
					    remainder | shift_mask,
					    new_src, source, subtargets, 1);
		  source = new_src;
		}
	      else
		{
		  rtx targ = subtargets ? NULL_RTX : target;
		  insns = arm_gen_constant (AND, mode, cond,
					    remainder | shift_mask,
					    targ, source, subtargets, 0);
		}
	    }

	  if (generate)
	    {
	      rtx new_src = subtargets ? gen_reg_rtx (mode) : target;
	      rtx shift = GEN_INT (clear_sign_bit_copies);

	      emit_insn (gen_ashlsi3 (new_src, source, shift));
	      emit_insn (gen_lshrsi3 (target, new_src, shift));
	    }

	  return insns + 2;
	}

      if (clear_zero_bit_copies >= 16 && clear_zero_bit_copies < 24)
	{
	  HOST_WIDE_INT shift_mask = (1 << clear_zero_bit_copies) - 1;

	  if ((remainder | shift_mask) != 0xffffffff)
	    {
	      if (generate)
		{
		  rtx new_src = subtargets ? gen_reg_rtx (mode) : target;

		  insns = arm_gen_constant (AND, mode, cond,
					    remainder | shift_mask,
					    new_src, source, subtargets, 1);
		  source = new_src;
		}
	      else
		{
		  rtx targ = subtargets ? NULL_RTX : target;

		  insns = arm_gen_constant (AND, mode, cond,
					    remainder | shift_mask,
					    targ, source, subtargets, 0);
		}
	    }

	  if (generate)
	    {
	      rtx new_src = subtargets ? gen_reg_rtx (mode) : target;
	      rtx shift = GEN_INT (clear_zero_bit_copies);

	      emit_insn (gen_lshrsi3 (new_src, source, shift));
	      emit_insn (gen_ashlsi3 (target, new_src, shift));
	    }

	  return insns + 2;
	}

      break;

    default:
      break;
    }

  for (i = 0; i < 32; i++)
    if (remainder & (1 << i))
      num_bits_set++;

  if (code == AND || (can_invert && num_bits_set > 16))
    remainder = (~remainder) & 0xffffffff;
  else if (code == PLUS && num_bits_set > 16)
    remainder = (-remainder) & 0xffffffff;
  else
    {
      can_invert = 0;
      can_negate = 0;
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* Now try and find a way of doing the job in either two or three
     instructions.
     We start by looking for the largest block of zeros that are aligned on
     a 2-bit boundary, we then fill up the temps, wrapping around to the
     top of the word when we drop off the bottom.
     In the worst case this code should produce no more than four insns.
     Thumb-2 constants are shifted, not rotated, so the MSB is always the
     best place to start.  */

  /* ??? Use thumb2 replicated constants when the high and low halfwords are
     the same.  */
  {
    int best_start = 0;
    if (!TARGET_THUMB2)
      {
	int best_consecutive_zeros = 0;

	for (i = 0; i < 32; i += 2)
	  {
	    int consecutive_zeros = 0;

	    if (!(remainder & (3 << i)))
	      {
		while ((i < 32) && !(remainder & (3 << i)))
		  {
		    consecutive_zeros += 2;
		    i += 2;
		  }
		if (consecutive_zeros > best_consecutive_zeros)
		  {
		    best_consecutive_zeros = consecutive_zeros;
		    best_start = i - consecutive_zeros;
		  }
		i -= 2;
	      }
	  }

	/* So long as it won't require any more insns to do so, it's
	   desirable to emit a small constant (in bits 0...9) in the last
	   insn.  This way there is more chance that it can be combined with
	   a later addressing insn to form a pre-indexed load or store
	   operation.  Consider:

		   *((volatile int *)0xe0000100) = 1;
		   *((volatile int *)0xe0000110) = 2;

	   We want this to wind up as:

		    mov rA, #0xe0000000
		    mov rB, #1
		    str rB, [rA, #0x100]
		    mov rB, #2
		    str rB, [rA, #0x110]

	   rather than having to synthesize both large constants from scratch.

	   Therefore, we calculate how many insns would be required to emit
	   the constant starting from `best_start', and also starting from
	   zero (i.e. with bit 31 first to be output).  If `best_start' doesn't
	   yield a shorter sequence, we may as well use zero.  */
	if (best_start != 0
	    && ((((unsigned HOST_WIDE_INT) 1) << best_start) < remainder)
	    && (count_insns_for_constant (remainder, 0) <=
		count_insns_for_constant (remainder, best_start)))
	  best_start = 0;
      }

    /* Now start emitting the insns.  */
    i = best_start;
    do
      {
	int end;

	if (i <= 0)
	  i += 32;
	if (remainder & (3 << (i - 2)))
	  {
	    end = i - 8;
	    if (end < 0)
	      end += 32;
	    temp1 = remainder & ((0x0ff << end)
				 | ((i < end) ? (0xff >> (32 - end)) : 0));
	    remainder &= ~temp1;

	    if (generate)
	      {
		rtx new_src, temp1_rtx;

		if (code == SET || code == MINUS)
		  {
		    new_src = (subtargets ? gen_reg_rtx (mode) : target);
		    if (can_invert && code != MINUS)
		      temp1 = ~temp1;
		  }
		else
		  {
		    if (remainder && subtargets)
		      new_src = gen_reg_rtx (mode);
		    else
		      new_src = target;
		    if (can_invert)
		      temp1 = ~temp1;
		    else if (can_negate)
		      temp1 = -temp1;
		  }

		temp1 = trunc_int_for_mode (temp1, mode);
		temp1_rtx = GEN_INT (temp1);

		if (code == SET)
		  ;
		else if (code == MINUS)
		  temp1_rtx = gen_rtx_MINUS (mode, temp1_rtx, source);
		else
		  temp1_rtx = gen_rtx_fmt_ee (code, mode, source, temp1_rtx);

		emit_constant_insn (cond,
				    gen_rtx_SET (VOIDmode, new_src,
						 temp1_rtx));
		source = new_src;
	      }

	    if (code == SET)
	      {
		can_invert = 0;
		code = PLUS;
	      }
	    else if (code == MINUS)
	      code = PLUS;

	    insns++;
	    if (TARGET_ARM)
	      i -= 6;
	    else
	      i -= 7;
	  }
	/* Arm allows rotates by a multiple of two. Thumb-2 allows arbitary
	   shifts.  */
	if (TARGET_ARM)
	  i -= 2;
	else
	  i--;
      }
    while (remainder);
  }
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

  return insns;
}

/* Canonicalize a comparison so that we are more likely to recognize it.
   This can be done for a few constant compares, where we can make the
   immediate value easier to load.  */

enum rtx_code
arm_canonicalize_comparison (enum rtx_code code, enum machine_mode mode,
			     rtx * op1)
{
  unsigned HOST_WIDE_INT i = INTVAL (*op1);
  unsigned HOST_WIDE_INT maxval;
  maxval = (((unsigned HOST_WIDE_INT) 1) << (GET_MODE_BITSIZE(mode) - 1)) - 1;

  switch (code)
    {
    case EQ:
    case NE:
      return code;

    case GT:
    case LE:
      if (i != maxval
	  && (const_ok_for_arm (i + 1) || const_ok_for_arm (-(i + 1))))
	{
	  *op1 = GEN_INT (i + 1);
	  return code == GT ? GE : LT;
	}
      break;

    case GE:
    case LT:
      if (i != ~maxval
	  && (const_ok_for_arm (i - 1) || const_ok_for_arm (-(i - 1))))
	{
	  *op1 = GEN_INT (i - 1);
	  return code == GE ? GT : LE;
	}
      break;

    case GTU:
    case LEU:
      if (i != ~((unsigned HOST_WIDE_INT) 0)
	  && (const_ok_for_arm (i + 1) || const_ok_for_arm (-(i + 1))))
	{
	  *op1 = GEN_INT (i + 1);
	  return code == GTU ? GEU : LTU;
	}
      break;

    case GEU:
    case LTU:
      if (i != 0
	  && (const_ok_for_arm (i - 1) || const_ok_for_arm (-(i - 1))))
	{
	  *op1 = GEN_INT (i - 1);
	  return code == GEU ? GTU : LEU;
	}
      break;

    default:
      gcc_unreachable ();
    }

  return code;
}


/* Define how to find the value returned by a function.  */

rtx
arm_function_value(tree type, tree func ATTRIBUTE_UNUSED)
{
  enum machine_mode mode;
  int unsignedp ATTRIBUTE_UNUSED;
  rtx r ATTRIBUTE_UNUSED;

  mode = TYPE_MODE (type);
  /* Promote integer types.  */
  if (INTEGRAL_TYPE_P (type))
    PROMOTE_FUNCTION_MODE (mode, unsignedp, type);

  /* Promotes small structs returned in a register to full-word size
     for big-endian AAPCS.  */
  if (arm_return_in_msb (type))
    {
      HOST_WIDE_INT size = int_size_in_bytes (type);
      if (size % UNITS_PER_WORD != 0)
	{
	  size += UNITS_PER_WORD - size % UNITS_PER_WORD;
	  mode = mode_for_size (size * BITS_PER_UNIT, MODE_INT, 0);
	}
    }

  return LIBCALL_VALUE(mode);
}

/* Determine the amount of memory needed to store the possible return
   registers of an untyped call.  */
int
arm_apply_result_size (void)
{
  int size = 16;

  if (TARGET_ARM)
    {
      if (TARGET_HARD_FLOAT_ABI)
	{
	  if (TARGET_FPA)
	    size += 12;
	  if (TARGET_MAVERICK)
	    size += 8;
	}
      if (TARGET_IWMMXT_ABI)
	size += 8;
    }

  return size;
}

/* Decide whether a type should be returned in memory (true)
   or in a register (false).  This is called by the macro
   RETURN_IN_MEMORY.  */
int
arm_return_in_memory (tree type)
{
  HOST_WIDE_INT size;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  size = int_size_in_bytes (type);

  /* Vector values should be returned using ARM registers, not memory (unless
     they're over 16 bytes, which will break since we only have four
     call-clobbered registers to play with).  */
  if (TREE_CODE (type) == VECTOR_TYPE)
    return (size < 0 || size > (4 * UNITS_PER_WORD));

  if (!AGGREGATE_TYPE_P (type) &&
      !(TARGET_AAPCS_BASED && TREE_CODE (type) == COMPLEX_TYPE))
    /* All simple types are returned in registers.
       For AAPCS, complex types are treated the same as aggregates.  */
    return 0;

  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  if (arm_abi != ARM_ABI_APCS)
    {
      /* ATPCS and later return aggregate types in memory only if they are
	 larger than a word (or are variable size).  */
      return (size < 0 || size > UNITS_PER_WORD);
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  /* Removed lines */
  /* For the arm-wince targets we choose to be compatible with Microsoft's
     ARM and Thumb compilers, which always return aggregates in memory.  */
#ifndef ARM_WINCE
  /* All structures/unions bigger than one word are returned in memory.
     Also catch the case where int_size_in_bytes returns -1.  In this case
     the aggregate is either huge or of variable size, and in either case
     we will want to return it via memory and not in a register.  */
  if (size < 0 || size > UNITS_PER_WORD)
    return 1;

  if (TREE_CODE (type) == RECORD_TYPE)
    {
      tree field;

      /* For a struct the APCS says that we only return in a register
	 if the type is 'integer like' and every addressable element
	 has an offset of zero.  For practical purposes this means
	 that the structure can have at most one non bit-field element
	 and that this element must be the first one in the structure.  */

      /* Find the first field, ignoring non FIELD_DECL things which will
	 have been created by C++.  */
      for (field = TYPE_FIELDS (type);
	   field && TREE_CODE (field) != FIELD_DECL;
	   field = TREE_CHAIN (field))
	continue;

      if (field == NULL)
	return 0; /* An empty structure.  Allowed by an extension to ANSI C.  */

      /* Check that the first field is valid for returning in a register.  */

      /* ... Floats are not allowed */
      if (FLOAT_TYPE_P (TREE_TYPE (field)))
	return 1;

      /* ... Aggregates that are not themselves valid for returning in
	 a register are not allowed.  */
      if (RETURN_IN_MEMORY (TREE_TYPE (field)))
	return 1;

      /* Now check the remaining fields, if any.  Only bitfields are allowed,
	 since they are not addressable.  */
      for (field = TREE_CHAIN (field);
	   field;
	   field = TREE_CHAIN (field))
	{
	  if (TREE_CODE (field) != FIELD_DECL)
	    continue;

	  if (!DECL_BIT_FIELD_TYPE (field))
	    return 1;
	}

      return 0;
    }

  if (TREE_CODE (type) == UNION_TYPE)
    {
      tree field;

      /* Unions can be returned in registers if every element is
	 integral, or can be returned in an integer register.  */
      for (field = TYPE_FIELDS (type);
	   field;
	   field = TREE_CHAIN (field))
	{
	  if (TREE_CODE (field) != FIELD_DECL)
	    continue;

	  if (FLOAT_TYPE_P (TREE_TYPE (field)))
	    return 1;

	  if (RETURN_IN_MEMORY (TREE_TYPE (field)))
	    return 1;
	}

      return 0;
    }
#endif /* not ARM_WINCE */

  /* Return all other types in memory.  */
  return 1;
}

/* Indicate whether or not words of a double are in big-endian order.  */

int
arm_float_words_big_endian (void)
{
  if (TARGET_MAVERICK)
    return 0;

  /* For FPA, float words are always big-endian.  For VFP, floats words
     follow the memory system mode.  */

  if (TARGET_FPA)
    {
      return 1;
    }

  if (TARGET_VFP)
    return (TARGET_BIG_END ? 1 : 0);

  return 1;
}

/* Initialize a variable CUM of type CUMULATIVE_ARGS
   for a call to a function whose data type is FNTYPE.
   For a library call, FNTYPE is NULL.  */
void
arm_init_cumulative_args (CUMULATIVE_ARGS *pcum, tree fntype,
			  rtx libname  ATTRIBUTE_UNUSED,
/* APPLE LOCAL 6738583 -mlong-calls PIC static functions */
			  tree fndecl)
{
  /* On the ARM, the offset starts at 0.  */
  pcum->nregs = 0;
  pcum->iwmmxt_nregs = 0;
  pcum->can_split = true;

  pcum->call_cookie = CALL_NORMAL;

  if (TARGET_LONG_CALLS)
    pcum->call_cookie = CALL_LONG;

  /* Check for long call/short call attributes.  The attributes
     override any command line option.  */
  if (fntype)
    {
      if (lookup_attribute ("short_call", TYPE_ATTRIBUTES (fntype)))
	pcum->call_cookie = CALL_SHORT;
      else if (lookup_attribute ("long_call", TYPE_ATTRIBUTES (fntype)))
	pcum->call_cookie = CALL_LONG;
      /* APPLE LOCAL begin 6738583 -mlong-calls PIC static functions */
      else if (fndecl && ! TREE_PUBLIC (fndecl))
	pcum->call_cookie = CALL_SHORT;
      /* APPLE LOCAL end 6738583 -mlong-calls PIC static functions */
    }

  /* Varargs vectors are treated the same as long long.
     named_count avoids having to change the way arm handles 'named' */
  pcum->named_count = 0;
  pcum->nargs = 0;

  if (TARGET_REALLY_IWMMXT && fntype)
    {
      tree fn_arg;

      for (fn_arg = TYPE_ARG_TYPES (fntype);
	   fn_arg;
	   fn_arg = TREE_CHAIN (fn_arg))
	pcum->named_count += 1;

      if (! pcum->named_count)
	pcum->named_count = INT_MAX;
    }
}


/* Return true if mode/type need doubleword alignment.  */
bool
arm_needs_doubleword_align (enum machine_mode mode, tree type)
{
  return (GET_MODE_ALIGNMENT (mode) > PARM_BOUNDARY
	  || (type && TYPE_ALIGN (type) > PARM_BOUNDARY));
}


/* Determine where to put an argument to a function.
   Value is zero to push the argument on the stack,
   or a hard register in which to store the argument.

   MODE is the argument's machine mode.
   TYPE is the data type of the argument (as a tree).
    This is null for libcalls where that information may
    not be available.
   CUM is a variable of type CUMULATIVE_ARGS which gives info about
    the preceding args and about the function being called.
   NAMED is nonzero if this argument is a named parameter
    (otherwise it is an extra parameter matching an ellipsis).  */

rtx
arm_function_arg (CUMULATIVE_ARGS *pcum, enum machine_mode mode,
		  tree type, int named)
{
  int nregs;

  /* Varargs vectors are treated the same as long long.
     named_count avoids having to change the way arm handles 'named' */
  if (TARGET_IWMMXT_ABI
      && arm_vector_mode_supported_p (mode)
      && pcum->named_count > pcum->nargs + 1)
    {
      if (pcum->iwmmxt_nregs <= 9)
	return gen_rtx_REG (mode, pcum->iwmmxt_nregs + FIRST_IWMMXT_REGNUM);
      else
	{
	  pcum->can_split = false;
	  return NULL_RTX;
	}
    }

  /* Put doubleword aligned quantities in even register pairs.  */
  if (pcum->nregs & 1
      && ARM_DOUBLEWORD_ALIGN
      && arm_needs_doubleword_align (mode, type))
    pcum->nregs++;

  if (mode == VOIDmode)
    /* Compute operand 2 of the call insn.  */
    return GEN_INT (pcum->call_cookie);

  /* Only allow splitting an arg between regs and memory if all preceding
     args were allocated to regs.  For args passed by reference we only count
     the reference pointer.  */
  if (pcum->can_split)
    nregs = 1;
  else
    nregs = ARM_NUM_REGS2 (mode, type);

  if (!named || pcum->nregs + nregs > NUM_ARG_REGS)
    return NULL_RTX;

  return gen_rtx_REG (mode, pcum->nregs);
}

static int
arm_arg_partial_bytes (CUMULATIVE_ARGS *pcum, enum machine_mode mode,
		       tree type, bool named ATTRIBUTE_UNUSED)
{
  int nregs = pcum->nregs;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  if (TARGET_IWMMXT_ABI && arm_vector_mode_supported_p (mode))
    return 0;

  if (NUM_ARG_REGS > nregs
      && (NUM_ARG_REGS < nregs + ARM_NUM_REGS2 (mode, type))
      && pcum->can_split)
    return (NUM_ARG_REGS - nregs) * UNITS_PER_WORD;

  return 0;
}

/* Variable sized types are passed by reference.  This is a GCC
   extension to the ARM ABI.  */

static bool
arm_pass_by_reference (CUMULATIVE_ARGS *cum ATTRIBUTE_UNUSED,
		       enum machine_mode mode ATTRIBUTE_UNUSED,
		       tree type, bool named ATTRIBUTE_UNUSED)
{
  return type && TREE_CODE (TYPE_SIZE (type)) != INTEGER_CST;
}

/* Encode the current state of the #pragma [no_]long_calls.  */
typedef enum
{
  OFF,		/* No #pragma [no_]long_calls is in effect.  */
  LONG,		/* #pragma long_calls is in effect.  */
  SHORT		/* #pragma no_long_calls is in effect.  */
} arm_pragma_enum;

static arm_pragma_enum arm_pragma_long_calls = OFF;

void
arm_pr_long_calls (struct cpp_reader * pfile ATTRIBUTE_UNUSED)
{
  arm_pragma_long_calls = LONG;
}

void
arm_pr_no_long_calls (struct cpp_reader * pfile ATTRIBUTE_UNUSED)
{
  arm_pragma_long_calls = SHORT;
}

void
arm_pr_long_calls_off (struct cpp_reader * pfile ATTRIBUTE_UNUSED)
{
  arm_pragma_long_calls = OFF;
}

/* Table of machine attributes.  */
const struct attribute_spec arm_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler } */
  /* Function calls made to this symbol must be done indirectly, because
     it may lie outside of the 26 bit addressing range of a normal function
     call.  */
  { "long_call",    0, 0, false, true,  true,  NULL },
  /* Whereas these functions are always known to reside within the 26 bit
     addressing range.  */
  { "short_call",   0, 0, false, true,  true,  NULL },
  /* Interrupt Service Routines have special prologue and epilogue requirements.  */
  { "isr",          0, 1, false, false, false, arm_handle_isr_attribute },
  { "interrupt",    0, 1, false, false, false, arm_handle_isr_attribute },
  { "naked",        0, 0, true,  false, false, arm_handle_fndecl_attribute },
#ifdef ARM_PE
  /* ARM/PE has three new attributes:
     interfacearm - ?
     dllexport - for exporting a function/variable that will live in a dll
     dllimport - for importing a function/variable from a dll

     Microsoft allows multiple declspecs in one __declspec, separating
     them with spaces.  We do NOT support this.  Instead, use __declspec
     multiple times.
  */
  { "dllimport",    0, 0, true,  false, false, NULL },
  { "dllexport",    0, 0, true,  false, false, NULL },
  { "interfacearm", 0, 0, true,  false, false, arm_handle_fndecl_attribute },
#elif TARGET_DLLIMPORT_DECL_ATTRIBUTES
  { "dllimport",    0, 0, false, false, false, handle_dll_attribute },
  { "dllexport",    0, 0, false, false, false, handle_dll_attribute },
  { "notshared",    0, 0, false, true, false, arm_handle_notshared_attribute },
#endif
/* APPLE LOCAL begin ARM darwin attributes */
#ifdef SUBTARGET_ATTRIBUTE_TABLE
  SUBTARGET_ATTRIBUTE_TABLE,
#endif
/* APPLE LOCAL end ARM darwin attributes */
  { NULL,           0, 0, false, false, false, NULL }
};

/* Handle an attribute requiring a FUNCTION_DECL;
   arguments as in struct attribute_spec.handler.  */
static tree
arm_handle_fndecl_attribute (tree *node, tree name, tree args ATTRIBUTE_UNUSED,
			     int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  if (TREE_CODE (*node) != FUNCTION_DECL)
    {
      warning (OPT_Wattributes, "%qs attribute only applies to functions",
	       IDENTIFIER_POINTER (name));
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* Handle an "interrupt" or "isr" attribute;
   arguments as in struct attribute_spec.handler.  */
static tree
arm_handle_isr_attribute (tree *node, tree name, tree args, int flags,
			  bool *no_add_attrs)
{
  if (DECL_P (*node))
    {
      if (TREE_CODE (*node) != FUNCTION_DECL)
	{
	  warning (OPT_Wattributes, "%qs attribute only applies to functions",
		   IDENTIFIER_POINTER (name));
	  *no_add_attrs = true;
	}
      /* FIXME: the argument if any is checked for type attributes;
	 should it be checked for decl ones?  */
    }
  else
    {
      if (TREE_CODE (*node) == FUNCTION_TYPE
	  || TREE_CODE (*node) == METHOD_TYPE)
	{
	  if (arm_isr_value (args) == ARM_FT_UNKNOWN)
	    {
	      warning (OPT_Wattributes, "%qs attribute ignored",
		       IDENTIFIER_POINTER (name));
	      *no_add_attrs = true;
	    }
	}
      else if (TREE_CODE (*node) == POINTER_TYPE
	       && (TREE_CODE (TREE_TYPE (*node)) == FUNCTION_TYPE
		   || TREE_CODE (TREE_TYPE (*node)) == METHOD_TYPE)
	       && arm_isr_value (args) != ARM_FT_UNKNOWN)
	{
	  *node = build_variant_type_copy (*node);
	  TREE_TYPE (*node) = build_type_attribute_variant
	    (TREE_TYPE (*node),
	     tree_cons (name, args, TYPE_ATTRIBUTES (TREE_TYPE (*node))));
	  *no_add_attrs = true;
	}
      else
	{
	  /* Possibly pass this attribute on from the type to a decl.  */
	  if (flags & ((int) ATTR_FLAG_DECL_NEXT
		       | (int) ATTR_FLAG_FUNCTION_NEXT
		       | (int) ATTR_FLAG_ARRAY_NEXT))
	    {
	      *no_add_attrs = true;
	      return tree_cons (name, args, NULL_TREE);
	    }
	  else
	    {
	      warning (OPT_Wattributes, "%qs attribute ignored",
		       IDENTIFIER_POINTER (name));
	    }
	}
    }

  return NULL_TREE;
}

#if TARGET_DLLIMPORT_DECL_ATTRIBUTES
/* Handle the "notshared" attribute.  This attribute is another way of
   requesting hidden visibility.  ARM's compiler supports
   "__declspec(notshared)"; we support the same thing via an
   attribute.  */

static tree
arm_handle_notshared_attribute (tree *node,
				tree name ATTRIBUTE_UNUSED,
				tree args ATTRIBUTE_UNUSED,
				int flags ATTRIBUTE_UNUSED,
				bool *no_add_attrs)
{
  tree decl = TYPE_NAME (*node);

  if (decl)
    {
      DECL_VISIBILITY (decl) = VISIBILITY_HIDDEN;
      DECL_VISIBILITY_SPECIFIED (decl) = 1;
      *no_add_attrs = false;
    }
  return NULL_TREE;
}
#endif

/* Return 0 if the attributes for two types are incompatible, 1 if they
   are compatible, and 2 if they are nearly compatible (which causes a
   warning to be generated).  */
static int
arm_comp_type_attributes (tree type1, tree type2)
{
  int l1, l2, s1, s2;

  /* Check for mismatch of non-default calling convention.  */
  if (TREE_CODE (type1) != FUNCTION_TYPE)
    return 1;

  /* Check for mismatched call attributes.  */
  l1 = lookup_attribute ("long_call", TYPE_ATTRIBUTES (type1)) != NULL;
  l2 = lookup_attribute ("long_call", TYPE_ATTRIBUTES (type2)) != NULL;
  s1 = lookup_attribute ("short_call", TYPE_ATTRIBUTES (type1)) != NULL;
  s2 = lookup_attribute ("short_call", TYPE_ATTRIBUTES (type2)) != NULL;

  /* Only bother to check if an attribute is defined.  */
  if (l1 | l2 | s1 | s2)
    {
      /* If one type has an attribute, the other must have the same attribute.  */
      if ((l1 != l2) || (s1 != s2))
	return 0;

      /* Disallow mixed attributes.  */
      if ((l1 & s2) || (l2 & s1))
	return 0;
    }

  /* Check for mismatched ISR attribute.  */
  l1 = lookup_attribute ("isr", TYPE_ATTRIBUTES (type1)) != NULL;
  if (! l1)
    l1 = lookup_attribute ("interrupt", TYPE_ATTRIBUTES (type1)) != NULL;
  l2 = lookup_attribute ("isr", TYPE_ATTRIBUTES (type2)) != NULL;
  if (! l2)
    l1 = lookup_attribute ("interrupt", TYPE_ATTRIBUTES (type2)) != NULL;
  if (l1 != l2)
    return 0;

  return 1;
}

/* APPLE LOCAL begin ARM longcall */
/*  Encode long_call or short_call attribute by prefixing
    symbol name in DECL with a special character FLAG.  */
void
arm_encode_call_attribute (tree decl, int flag)
{
#if TARGET_MACHO
  rtx sym_ref = XEXP (DECL_RTL (decl), 0);

  /* Do not allow weak functions with default visibility to be treated
     as short call.  */
  if (DECL_WEAK (decl)
      && DECL_VISIBILITY (decl) == VISIBILITY_DEFAULT
      && flag == SYMBOL_SHORT_CALL)
    return;

  SYMBOL_REF_FLAGS (sym_ref) |= flag;
#else
  const char * str = XSTR (XEXP (DECL_RTL (decl), 0), 0);
  int          len = strlen (str);
  char *       newstr;

  /* Do not allow weak functions to be treated as short call.  */
  if (DECL_WEAK (decl) && flag == SHORT_CALL_FLAG_CHAR)
    return;

  newstr = alloca (len + 2);
  newstr[0] = flag;
  strcpy (newstr + 1, str);

  newstr = (char *) ggc_alloc_string (newstr, len + 1);
  XSTR (XEXP (DECL_RTL (decl), 0), 0) = newstr;
#endif
}
/* APPLE LOCAL end ARM longcall */

/*  Assigns default attributes to newly defined type.  This is used to
    set short_call/long_call attributes for function types of
    functions defined inside corresponding #pragma scopes.  */
static void
arm_set_default_type_attributes (tree type)
{
  /* Add __attribute__ ((long_call)) to all functions, when
     inside #pragma long_calls or __attribute__ ((short_call)),
     when inside #pragma no_long_calls.  */
  if (TREE_CODE (type) == FUNCTION_TYPE || TREE_CODE (type) == METHOD_TYPE)
    {
      tree type_attr_list, attr_name;
      type_attr_list = TYPE_ATTRIBUTES (type);

      if (arm_pragma_long_calls == LONG)
 	attr_name = get_identifier ("long_call");
      else if (arm_pragma_long_calls == SHORT)
 	attr_name = get_identifier ("short_call");
      else
 	return;

      type_attr_list = tree_cons (attr_name, NULL_TREE, type_attr_list);
      TYPE_ATTRIBUTES (type) = type_attr_list;
    }
}

/* Return 1 if the operand is a SYMBOL_REF for a function known to be
   defined within the current compilation unit.  If this cannot be
   determined, then 0 is returned.  */
static int
current_file_function_operand (rtx sym_ref)
{
  /* This is a bit of a fib.  A function will have a short call flag
     applied to its name if it has the short call attribute, or it has
     already been defined within the current compilation unit.  */
/* APPLE LOCAL begin ARM longcall */
#if TARGET_MACHO
  if (SYMBOL_SHORT_CALL_ATTR_P (sym_ref))
#else
  if (ENCODED_SHORT_CALL_ATTR_P (XSTR (sym_ref, 0)))
#endif
    return 1;
/* APPLE LOCAL end ARM longcall */

  /* The current function is always defined within the current compilation
     unit.  If it s a weak definition however, then this may not be the real
     definition of the function, and so we have to say no.  */
  if (sym_ref == XEXP (DECL_RTL (current_function_decl), 0)
      && !DECL_WEAK (current_function_decl))
    return 1;

  /* We cannot make the determination - default to returning 0.  */
  return 0;
}

/* Return nonzero if a 32 bit "long_call" should be generated for
   this call.  We generate a long_call if the function:

        a.  has an __attribute__((long call))
     or b.  is within the scope of a #pragma long_calls
     or c.  the -mlong-calls command line switch has been specified
         .  and either:
                1. -ffunction-sections is in effect
	     or 2. the current function has __attribute__ ((section))
	     or 3. the target function has __attribute__ ((section))

   However we do not generate a long call if the function:

        d.  has an __attribute__ ((short_call))
     or e.  is inside the scope of a #pragma no_long_calls
     or f.  is defined within the current compilation unit.

   This function will be called by C fragments contained in the machine
   description file.  SYM_REF and CALL_COOKIE correspond to the matched
   rtl operands.  CALL_SYMBOL is used to distinguish between
   two different callers of the function.  It is set to 1 in the
   "call_symbol" and "call_symbol_value" patterns and to 0 in the "call"
   and "call_value" patterns.  This is because of the difference in the
   SYM_REFs passed by these patterns.  */
int
arm_is_longcall_p (rtx sym_ref, int call_cookie, int call_symbol)
{
  if (!call_symbol)
    {
      if (GET_CODE (sym_ref) != MEM)
	return 0;

      sym_ref = XEXP (sym_ref, 0);
    }

  if (GET_CODE (sym_ref) != SYMBOL_REF)
    return 0;

  if (call_cookie & CALL_SHORT)
    return 0;

  if (TARGET_LONG_CALLS)
    {
      if (flag_function_sections
	  || DECL_SECTION_NAME (current_function_decl))
	/* c.3 is handled by the definition of the
	   ARM_DECLARE_FUNCTION_SIZE macro.  */
	return 1;
    }

  if (current_file_function_operand (sym_ref))
    return 0;

  /* APPLE LOCAL begin ARM longcall */
#if TARGET_MACHO
  return (call_cookie & CALL_LONG)
    || SYMBOL_LONG_CALL_ATTR_P (sym_ref)
    || TARGET_LONG_CALLS;
#else
  return (call_cookie & CALL_LONG)
    || ENCODED_LONG_CALL_ATTR_P (XSTR (sym_ref, 0))
    || TARGET_LONG_CALLS;
#endif
  /* APPLE LOCAL end ARM longcall */
}

/* Return nonzero if it is ok to make a tail-call to DECL.  */
static bool
arm_function_ok_for_sibcall (tree decl, tree exp ATTRIBUTE_UNUSED)
{
  int call_type = TARGET_LONG_CALLS ? CALL_LONG : CALL_NORMAL;
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  unsigned long func_type;

  if (cfun->machine->sibcall_blocked)
    return false;

  /* APPLE LOCAL begin ARM indirect sibcalls */
  /* Never tailcall something for which we have no decl, or if we
     are in Thumb mode.  */
  if (TARGET_THUMB)
    return false;

  /* All indirect calls are within range, since we load the address into a
     register. */
  if (decl == NULL)
    return true;
  /* APPLE LOCAL end ARM indirect sibcalls */

  /* Get the calling method.  */
  if (lookup_attribute ("short_call", TYPE_ATTRIBUTES (TREE_TYPE (decl))))
    call_type = CALL_SHORT;
  else if (lookup_attribute ("long_call", TYPE_ATTRIBUTES (TREE_TYPE (decl))))
    call_type = CALL_LONG;

  /* Cannot tail-call to long calls, since these are out of range of
     a branch instruction.  However, if not compiling PIC, we know
     we can reach the symbol if it is in this compilation unit.  */
  if (call_type == CALL_LONG && (flag_pic || !TREE_ASM_WRITTEN (decl)))
    return false;

  /* If we are interworking and the function is not declared static
     then we can't tail-call it unless we know that it exists in this
     compilation unit (since it might be a Thumb routine).  */
  /* APPLE LOCAL begin ARM interworking */
  if (TREE_PUBLIC (decl) && !TREE_ASM_WRITTEN (decl) && TARGET_INTERWORK)
    {
      if (TARGET_MACHO)
	return false;
      else if (!arm_arch5)
	return false;
    }
  /* APPLE LOCAL end ARM interworking */

  /* APPLE LOCAL begin ARM 4956366 */
  /* If it's weak, the function called may end up being from a different
     compilation unit.  */
  if (arm_cpp_interwork && TREE_PUBLIC (decl) && DECL_WEAK (decl))
    return false;
  /* APPLE LOCAL end ARM 4956366 */

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  func_type = arm_current_func_type ();
  /* Never tailcall from an ISR routine - it needs a special exit sequence.  */
  if (IS_INTERRUPT (func_type))
    return false;

  /* Never tailcall if function may be called with a misaligned SP.  */
  if (IS_STACKALIGN (func_type))
    return false;

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  /* Everything else is ok.  */
  return true;
}


/* Addressing mode support functions.  */

/* Return nonzero if X is a legitimate immediate operand when compiling
   for PIC.  We know that X satisfies CONSTANT_P and flag_pic is true.  */
int
legitimate_pic_operand_p (rtx x)
{
  if (GET_CODE (x) == SYMBOL_REF
      || (GET_CODE (x) == CONST
	  && GET_CODE (XEXP (x, 0)) == PLUS
	  && GET_CODE (XEXP (XEXP (x, 0), 0)) == SYMBOL_REF))
    return 0;

  return 1;
}

rtx
legitimize_pic_address (rtx orig, enum machine_mode mode, rtx reg)
{
  if (GET_CODE (orig) == SYMBOL_REF
      || GET_CODE (orig) == LABEL_REF)
    {
#ifndef AOF_ASSEMBLER
      rtx pic_ref, address;
#endif
      /* APPLE LOCAL ARM pic support */
      rtx norig, l1 = NULL_RTX;
      rtx insn;
      int subregs = 0;
      /* APPLE LOCAL ARM pic support */
      bool avoid_ind = true;

      /* If this function doesn't have a pic register, create one now.
	 A lot of the logic here is made obscure by the fact that this
	 routine gets called as part of the rtx cost estimation
	 process.  We don't want those calls to affect any assumptions
	 about the real function; and further, we can't call
	 entry_of_function() until we start the real expansion
	 process.  */
      /* APPLE LOCAL ARM pic support */
      if (!TARGET_MACHO && !current_function_uses_pic_offset_table)
	{
	  gcc_assert (!no_new_pseudos);
	  if (arm_pic_register != INVALID_REGNUM)
	    {
	      cfun->machine->pic_reg = gen_rtx_REG (Pmode, arm_pic_register);

	      /* Play games to avoid marking the function as needing pic
		 if we are being called as part of the cost-estimation
		 process.  */
	      if (!ir_type())
		current_function_uses_pic_offset_table = 1;
	    }
	  else
	    {
	      rtx seq;

	      cfun->machine->pic_reg = gen_reg_rtx (Pmode);

	      /* Play games to avoid marking the function as needing pic
		 if we are being called as part of the cost-estimation
		 process.  */
	      if (!ir_type())
		{
		  current_function_uses_pic_offset_table = 1;
		  start_sequence ();

		  arm_load_pic_register (0UL);

		  seq = get_insns ();
		  end_sequence ();
		  emit_insn_after (seq, entry_of_function ());
		}
	    }
	}

      if (reg == 0)
	{
	  gcc_assert (!no_new_pseudos);
	  reg = gen_reg_rtx (Pmode);

	  subregs = 1;
	}

#ifdef AOF_ASSEMBLER
      /* The AOF assembler can generate relocations for these directly, and
	 understands that the PIC register has to be added into the offset.  */
      insn = emit_insn (gen_pic_load_addr_based (reg, orig));
#else
      if (subregs)
	address = gen_reg_rtx (Pmode);
      else
	address = reg;

      /* APPLE LOCAL begin ARM pic support */
      norig = orig;
#if TARGET_MACHO
      if (TARGET_MACHO)
	{
	  if (GET_CODE (orig) == SYMBOL_REF
	      || GET_CODE (orig) == LABEL_REF)
	    {
	      rtx x, ptr_ref = orig;

	      l1 = gen_label_rtx ();

	      if (GET_CODE (orig) == SYMBOL_REF)
		{
		  bool defined = machopic_data_defined_p (orig);

		  if (defined && MACHO_DYNAMIC_NO_PIC_P)
		    return orig;

		  if (! defined)
		    {
		      avoid_ind = false;
		      ptr_ref = gen_rtx_SYMBOL_REF (Pmode,
						    machopic_indirection_name (orig, false));
		      SET_SYMBOL_REF_DECL (ptr_ref, SYMBOL_REF_DECL (orig));
		      SYMBOL_REF_FLAGS (ptr_ref) |= MACHO_SYMBOL_FLAG_DEFINED;
		    }
		}
	      else
		{
		  if (MACHO_DYNAMIC_NO_PIC_P)
		    return orig;
		}

	      if (! MACHO_DYNAMIC_NO_PIC_P)
		{
		  x = plus_constant (gen_rtx_LABEL_REF (Pmode, l1), TARGET_ARM ? 8 : 4);
		  ptr_ref = gen_rtx_CONST (Pmode, gen_rtx_MINUS (Pmode, ptr_ref, x));
		}
	      norig = ptr_ref;
	    }
	}
#endif
      if (TARGET_MACHO && ! MACHO_DYNAMIC_NO_PIC_P)
	{
	  if (GET_CODE (orig) == SYMBOL_REF
	      || GET_CODE (orig) == LABEL_REF)
	    {
	      if (TARGET_ARM)
		{
		  emit_insn (gen_pic_load_addr_arm (address, norig, l1));
		  emit_insn (gen_pic_add_dot_plus_eight (address, l1, address));
		}
              /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
              else if (TARGET_THUMB2)
                {
		  emit_insn (gen_pic_load_addr_thumb2 (address, norig, l1));
		  emit_insn (gen_pic_add_dot_plus_four (address, l1, address));
                }
	      else /* TARGET_THUMB1 */
		{
		  emit_insn (gen_pic_load_addr_thumb1 (address, norig, l1));
		  emit_insn (gen_pic_add_dot_plus_four (address, l1, address));
		}
              /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	    }
	  else
	    abort ();
	}
      else
	{
	  if (TARGET_ARM)
	    emit_insn (gen_pic_load_addr_arm (address, norig, l1));
          /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
	  else if (TARGET_THUMB2)
            emit_insn (gen_pic_load_addr_thumb2 (address, norig, l1));
          else /* TARGET_THUMB1 */
	    emit_insn (gen_pic_load_addr_thumb1 (address, norig, l1));
          /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	}
      /* APPLE LOCAL end ARM pic support */

      if ((GET_CODE (orig) == LABEL_REF
	   || (GET_CODE (orig) == SYMBOL_REF &&
	       SYMBOL_REF_LOCAL_P (orig)))
	  && NEED_GOT_RELOC)
	pic_ref = gen_rtx_PLUS (Pmode, cfun->machine->pic_reg, address);
      else
	{
	  /* APPLE LOCAL begin ARM pic support */
	  if (! TARGET_MACHO)
	    pic_ref = gen_const_mem (Pmode,
				     gen_rtx_PLUS (Pmode, cfun->machine->pic_reg,
						   address));
	  else if (avoid_ind)
	    pic_ref = address;
	  else
	    pic_ref = gen_const_mem (Pmode, address);
	  /* APPLE LOCAL end ARM pic support */
	}

      insn = emit_move_insn (reg, pic_ref);
#endif
      /* Put a REG_EQUAL note on this insn, so that it can be optimized
	 by loop.  */
      REG_NOTES (insn) = gen_rtx_EXPR_LIST (REG_EQUAL, orig,
					    REG_NOTES (insn));
      return reg;
    }
  else if (GET_CODE (orig) == CONST)
    {
      rtx base, offset;

      if (GET_CODE (XEXP (orig, 0)) == PLUS
	  && XEXP (XEXP (orig, 0), 0) == cfun->machine->pic_reg)
	return orig;

      if (GET_CODE (XEXP (orig, 0)) == UNSPEC
	  && XINT (XEXP (orig, 0), 1) == UNSPEC_TLS)
	return orig;

      /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
      /* Handle the case where we have:
         const (plus (UNSPEC_TLS) (ADDEND)).  The ADDEND must be a
         CONST_INT.  */
      if (GET_CODE (XEXP (orig, 0)) == PLUS
          && GET_CODE (XEXP (XEXP (orig, 0), 0)) == UNSPEC
          && XINT (XEXP (XEXP (orig, 0), 0), 1) == UNSPEC_TLS)
        {
	  gcc_assert (GET_CODE (XEXP (XEXP (orig, 0), 1)) == CONST_INT);
	  return orig;
	}

      /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
      if (reg == 0)
	{
	  gcc_assert (!no_new_pseudos);
	  reg = gen_reg_rtx (Pmode);
	}

      gcc_assert (GET_CODE (XEXP (orig, 0)) == PLUS);

      base = legitimize_pic_address (XEXP (XEXP (orig, 0), 0), Pmode, reg);
      offset = legitimize_pic_address (XEXP (XEXP (orig, 0), 1), Pmode,
				       base == reg ? 0 : reg);

      /* APPLE LOCAL begin 6327222 */
      /* #if 0 for now so it's here for reference since this is a tricky
         bit. */
#if 0
      if (GET_CODE (offset) == CONST_INT)
	{
	  /* The base register doesn't really matter, we only want to
	     test the index for the appropriate mode.  */
	  if (!arm_legitimate_index_p (mode, offset, SET, 0))
	    {
	      gcc_assert (!no_new_pseudos);
	      offset = force_reg (Pmode, offset);
	    }

	  if (GET_CODE (offset) == CONST_INT)
	    return plus_constant (base, INTVAL (offset));
	}
#endif
      /* APPLE LOCAL end 6327222 */
      if (GET_MODE_SIZE (mode) > 4
	  && (GET_MODE_CLASS (mode) == MODE_INT
	      || TARGET_SOFT_FLOAT))
	{
	  emit_insn (gen_addsi3 (reg, base, offset));
	  return reg;
	}

      return gen_rtx_PLUS (Pmode, base, offset);
    }

  return orig;
}


/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
/* Find a spare register to use during the prolog of a function.  */

static int
thumb_find_work_register (unsigned long pushed_regs_mask)
{
  int reg;

  /* Check the argument registers first as these are call-used.  The
     register allocation order means that sometimes r3 might be used
     but earlier argument registers might not, so check them all.  */
  for (reg = LAST_ARG_REGNUM; reg >= 0; reg --)
    if (!regs_ever_live[reg])
      return reg;

  /* Before going on to check the call-saved registers we can try a couple
     more ways of deducing that r3 is available.  The first is when we are
     pushing anonymous arguments onto the stack and we have less than 4
     registers worth of fixed arguments(*).  In this case r3 will be part of
     the variable argument list and so we can be sure that it will be
     pushed right at the start of the function.  Hence it will be available
     for the rest of the prologue.
     (*): ie current_function_pretend_args_size is greater than 0.  */
  if (cfun->machine->uses_anonymous_args
      && current_function_pretend_args_size > 0)
    return LAST_ARG_REGNUM;

  /* The other case is when we have fixed arguments but less than 4 registers
     worth.  In this case r3 might be used in the body of the function, but
     it is not being used to convey an argument into the function.  In theory
     we could just check current_function_args_size to see how many bytes are
     being passed in argument registers, but it seems that it is unreliable.
     Sometimes it will have the value 0 when in fact arguments are being
     passed.  (See testcase execute/20021111-1.c for an example).  So we also
     check the args_info.nregs field as well.  The problem with this field is
     that it makes no allowances for arguments that are passed to the
     function but which are not used.  Hence we could miss an opportunity
     when a function has an unused argument in r3.  But it is better to be
     safe than to be sorry.  */
  if (! cfun->machine->uses_anonymous_args
      && current_function_args_size >= 0
      && current_function_args_size <= (LAST_ARG_REGNUM * UNITS_PER_WORD)
      && cfun->args_info.nregs < 4)
    return LAST_ARG_REGNUM;

  /* Otherwise look for a call-saved register that is going to be pushed.  */
  for (reg = LAST_LO_REGNUM; reg > LAST_ARG_REGNUM; reg --)
    if (pushed_regs_mask & (1 << reg))
      return reg;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (TARGET_THUMB2)
    {
      /* Thumb-2 can use high regs.  */
      for (reg = FIRST_HI_REGNUM; reg < 15; reg ++)
	if (pushed_regs_mask & (1 << reg))
	  return reg;
    }
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  /* Something went wrong - thumb_compute_save_reg_mask()
     should have arranged for a suitable register to be pushed.  */
  gcc_unreachable ();
}

static GTY(()) int pic_labelno;

/* Generate code to load the PIC register.  In thumb mode SCRATCH is a
   low register.  */

void
arm_load_pic_register (unsigned long saved_regs ATTRIBUTE_UNUSED)
{
#ifndef AOF_ASSEMBLER
  rtx l1, labelno, pic_tmp, pic_tmp2, pic_rtx;
  rtx global_offset_table;

  if (current_function_uses_pic_offset_table == 0 || TARGET_SINGLE_PIC_BASE)
    return;

  gcc_assert (flag_pic);

  /* We use an UNSPEC rather than a LABEL_REF because this label never appears
     in the code stream.  */

  labelno = GEN_INT (pic_labelno++);
  l1 = gen_rtx_UNSPEC (Pmode, gen_rtvec (1, labelno), UNSPEC_PIC_LABEL);
  l1 = gen_rtx_CONST (VOIDmode, l1);

  global_offset_table = gen_rtx_SYMBOL_REF (Pmode, "_GLOBAL_OFFSET_TABLE_");
  /* On the ARM the PC register contains 'dot + 8' at the time of the
     addition, on the Thumb it is 'dot + 4'.  */
  pic_tmp = plus_constant (l1, TARGET_ARM ? 8 : 4);
  if (GOT_PCREL)
    pic_tmp2 = gen_rtx_CONST (VOIDmode,
			    gen_rtx_PLUS (Pmode, global_offset_table, pc_rtx));
  else
    pic_tmp2 = gen_rtx_CONST (VOIDmode, global_offset_table);

  pic_rtx = gen_rtx_CONST (Pmode, gen_rtx_MINUS (Pmode, pic_tmp2, pic_tmp));

  if (TARGET_ARM)
    {
      /* APPLE LOCAL begin ARM pic support */
      emit_insn (gen_pic_load_addr_arm (cfun->machine->pic_reg, pic_rtx, l1));
      emit_insn (gen_pic_add_dot_plus_eight (cfun->machine->pic_reg, l1,
					     cfun->machine->pic_reg));
      /* APPLE LOCAL end ARM pic support */
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from Codesourcery */
  else if (TARGET_THUMB2)
    {
      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      /* Thumb-2 only allows very limited access to the PC.  Calculate the
       address in a temporary register.  */
      if (arm_pic_register != INVALID_REGNUM)
        {       
          pic_tmp = gen_rtx_REG (SImode,
                                 thumb_find_work_register (saved_regs));
        }
      else    
        {     
          gcc_assert (!no_new_pseudos);
          pic_tmp = gen_reg_rtx (Pmode);
        } 

      emit_insn (gen_pic_load_addr_thumb2 (cfun->machine->pic_reg, 
                              pic_rtx, l1));
      emit_insn (gen_pic_load_dot_plus_four (pic_tmp, labelno));
      emit_insn (gen_addsi3 (cfun->machine->pic_reg, cfun->machine->pic_reg,
                             pic_tmp));
    }
  else /* TARGET_THUMB1 */
    {
      /* APPLE LOCAL begin ARM pic support */
      if (arm_pic_register != INVALID_REGNUM
	  && REGNO (cfun->machine->pic_reg) > LAST_LO_REGNUM)
	{
	  /* We will have pushed the pic register, so we should always be
	     able to find a work register.  */
	  pic_tmp = gen_rtx_REG (SImode,
				 thumb_find_work_register (saved_regs));
	  emit_insn (gen_pic_load_addr_thumb1 (pic_tmp, pic_rtx, l1));
	  emit_insn (gen_movsi (pic_offset_table_rtx, pic_tmp));
	}
      else
	emit_insn (gen_pic_load_addr_thumb1 (cfun->machine->pic_reg, pic_rtx, l1));
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
      emit_insn (gen_pic_add_dot_plus_four (cfun->machine->pic_reg, l1,
					    cfun->machine->pic_reg));
      /* APPLE LOCAL end ARM pic support */
    }

  /* Need to emit this whether or not we obey regdecls,
     since setjmp/longjmp can cause life info to screw up.  */
  emit_insn (gen_rtx_USE (VOIDmode, cfun->machine->pic_reg));
#endif /* AOF_ASSEMBLER */
}


/* Return nonzero if X is valid as an ARM state addressing register.  */
static int
arm_address_register_rtx_p (rtx x, int strict_p)
{
  int regno;

  if (GET_CODE (x) != REG)
    return 0;

  regno = REGNO (x);

  if (strict_p)
    return ARM_REGNO_OK_FOR_BASE_P (regno);

  return (regno <= LAST_ARM_REGNUM
	  || regno >= FIRST_PSEUDO_REGISTER
	  || regno == FRAME_POINTER_REGNUM
	  || regno == ARG_POINTER_REGNUM);
}

/* Return TRUE if this rtx is the difference of a symbol and a label,
   and will reduce to a PC-relative relocation in the object file.
   Expressions like this can be left alone when generating PIC, rather
   than forced through the GOT.  */
static int
pcrel_constant_p (rtx x)
{
  if (GET_CODE (x) == MINUS)
    return symbol_mentioned_p (XEXP (x, 0)) && label_mentioned_p (XEXP (x, 1));

  return FALSE;
}

/* Return nonzero if X is a valid ARM state address operand.  */
int
arm_legitimate_address_p (enum machine_mode mode, rtx x, RTX_CODE outer,
			  int strict_p)
{
  bool use_ldrd;
  enum rtx_code code = GET_CODE (x);

  if (arm_address_register_rtx_p (x, strict_p))
    return 1;

  use_ldrd = (TARGET_LDRD
	      && (mode == DImode
		  || (mode == DFmode && (TARGET_SOFT_FLOAT || TARGET_VFP))));

  /* ALQAAHIRA LOCAL begin 6293989 */
  if (TARGET_NEON && VECTOR_MODE_P (mode)
      && (code == PRE_DEC || code == PRE_INC || code == POST_DEC))
    return 0;
  /* ALQAAHIRA LOCAL end 6293989 */

  if (code == POST_INC || code == PRE_DEC
      || ((code == PRE_INC || code == POST_DEC)
	  && (use_ldrd || GET_MODE_SIZE (mode) <= 4)))
    return arm_address_register_rtx_p (XEXP (x, 0), strict_p);

  else if ((code == POST_MODIFY || code == PRE_MODIFY)
	   && arm_address_register_rtx_p (XEXP (x, 0), strict_p)
	   && GET_CODE (XEXP (x, 1)) == PLUS
	   && rtx_equal_p (XEXP (XEXP (x, 1), 0), XEXP (x, 0)))
    {
      rtx addend = XEXP (XEXP (x, 1), 1);

      /* Don't allow ldrd post increment by register because it's hard
	 to fixup invalid register choices.  */
      if (use_ldrd
	  && GET_CODE (x) == POST_MODIFY
	  && GET_CODE (addend) == REG)
	return 0;

      return ((use_ldrd || GET_MODE_SIZE (mode) <= 4)
	      && arm_legitimate_index_p (mode, addend, outer, strict_p));
    }

  /* After reload constants split into minipools will have addresses
     from a LABEL_REF.  */
  else if (reload_completed
	   && (code == LABEL_REF
	       || (code == CONST
		   && GET_CODE (XEXP (x, 0)) == PLUS
		   && GET_CODE (XEXP (XEXP (x, 0), 0)) == LABEL_REF
		   && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT)))
    return 1;

  else if (mode == TImode || (TARGET_NEON && VALID_NEON_STRUCT_MODE (mode)))
    return 0;

  else if (code == PLUS)
    {
      rtx xop0 = XEXP (x, 0);
      rtx xop1 = XEXP (x, 1);

      return ((arm_address_register_rtx_p (xop0, strict_p)
	       && arm_legitimate_index_p (mode, xop1, outer, strict_p))
	      || (arm_address_register_rtx_p (xop1, strict_p)
		  && arm_legitimate_index_p (mode, xop0, outer, strict_p)));
    }

#if 0
  /* Reload currently can't handle MINUS, so disable this for now */
  else if (GET_CODE (x) == MINUS)
    {
      rtx xop0 = XEXP (x, 0);
      rtx xop1 = XEXP (x, 1);

      return (arm_address_register_rtx_p (xop0, strict_p)
	      && arm_legitimate_index_p (mode, xop1, outer, strict_p));
    }
#endif

  else if (GET_MODE_CLASS (mode) != MODE_FLOAT
	   && code == SYMBOL_REF
	   && CONSTANT_POOL_ADDRESS_P (x)
	   && ! (flag_pic
		 && symbol_mentioned_p (get_pool_constant (x))
		 && ! pcrel_constant_p (get_pool_constant (x))))
    return 1;

  return 0;
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Return nonzero if X is a valid Thumb-2 address operand.  */
int
thumb2_legitimate_address_p (enum machine_mode mode, rtx x, int strict_p)
{
  bool use_ldrd;
  enum rtx_code code = GET_CODE (x);
  
  if (arm_address_register_rtx_p (x, strict_p))
    return 1;

  use_ldrd = (TARGET_LDRD
	      && (mode == DImode
		  || (mode == DFmode && (TARGET_SOFT_FLOAT || TARGET_VFP))));

  /* ALQAAHIRA LOCAL begin 6293989 */
  if (TARGET_NEON && VECTOR_MODE_P (mode)
      && (code == PRE_DEC || code == PRE_INC || code == POST_DEC))
    return 0;
  /* ALQAAHIRA LOCAL end 6293989 */

  if (code == POST_INC || code == PRE_DEC
      || ((code == PRE_INC || code == POST_DEC)
	  && (use_ldrd || GET_MODE_SIZE (mode) <= 4)))
    return arm_address_register_rtx_p (XEXP (x, 0), strict_p);

  else if ((code == POST_MODIFY || code == PRE_MODIFY)
	   && arm_address_register_rtx_p (XEXP (x, 0), strict_p)
	   && GET_CODE (XEXP (x, 1)) == PLUS
	   && rtx_equal_p (XEXP (XEXP (x, 1), 0), XEXP (x, 0)))
    {
      /* Thumb-2 only has autoincrement by constant.  */
      rtx addend = XEXP (XEXP (x, 1), 1);
      HOST_WIDE_INT offset;

      if (GET_CODE (addend) != CONST_INT)
	return 0;

      offset = INTVAL(addend);
      if (GET_MODE_SIZE (mode) <= 4)
	return (offset > -256 && offset < 256);
      
      return (use_ldrd && offset > -1024 && offset < 1024
	      && (offset & 3) == 0);
    }

  /* After reload constants split into minipools will have addresses
     from a LABEL_REF.  */
  else if (reload_completed
	   && (code == LABEL_REF
	       || (code == CONST
		   && GET_CODE (XEXP (x, 0)) == PLUS
		   && GET_CODE (XEXP (XEXP (x, 0), 0)) == LABEL_REF
		   && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT)))
    return 1;

  /* ALQAAHIRA LOCAL v7 support. Merge from Codesourcery */
  else if (mode == TImode || (TARGET_NEON && VALID_NEON_STRUCT_MODE (mode)))
    return 0;

  else if (code == PLUS)
    {
      rtx xop0 = XEXP (x, 0);
      rtx xop1 = XEXP (x, 1);

      return ((arm_address_register_rtx_p (xop0, strict_p)
	       && thumb2_legitimate_index_p (mode, xop1, strict_p))
	      || (arm_address_register_rtx_p (xop1, strict_p)
		  && thumb2_legitimate_index_p (mode, xop0, strict_p)));
    }

  else if (GET_MODE_CLASS (mode) != MODE_FLOAT
	   && code == SYMBOL_REF
	   && CONSTANT_POOL_ADDRESS_P (x)
	   && ! (flag_pic
		 && symbol_mentioned_p (get_pool_constant (x))
		 && ! pcrel_constant_p (get_pool_constant (x))))
    return 1;

  return 0;
}

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* Return nonzero if INDEX is valid for an address index operand in
   ARM state.  */
static int
arm_legitimate_index_p (enum machine_mode mode, rtx index, RTX_CODE outer,
			int strict_p)
{
  HOST_WIDE_INT range;
  enum rtx_code code = GET_CODE (index);

  /* Standard coprocessor addressing modes.  */
  if (TARGET_HARD_FLOAT
      /* APPLE LOCAL ARM 4480764 */
      && (TARGET_FPA || TARGET_MAVERICK || TARGET_VFP)
      && (GET_MODE_CLASS (mode) == MODE_FLOAT
	  || (TARGET_MAVERICK && mode == DImode)))
    return (code == CONST_INT && INTVAL (index) < 1024
	    && INTVAL (index) > -1024
	    && (INTVAL (index) & 3) == 0);

  if (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (mode))
    {
      /* For DImode assume values will usually live in core regs
	 and only allow LDRD addressing modes.  */
      if (!TARGET_LDRD || mode != DImode)
	return (code == CONST_INT
		&& INTVAL (index) < 1024
		&& INTVAL (index) > -1024
		&& (INTVAL (index) & 3) == 0);
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  if (TARGET_NEON
      /* ALQAAHIRA LOCAL 6150882 use thumb2 by default for v7 */
      && VECTOR_MODE_P (mode)
      && (VALID_NEON_DREG_MODE (mode) || VALID_NEON_QREG_MODE (mode)))
    return (code == CONST_INT
	    && INTVAL (index) < 1016
	    && INTVAL (index) > -1024
	    && (INTVAL (index) & 3) == 0);

  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  if (arm_address_register_rtx_p (index, strict_p)
      && (GET_MODE_SIZE (mode) <= 4))
    return 1;

  if (mode == DImode || mode == DFmode)
    {
      if (code == CONST_INT)
	{
	  HOST_WIDE_INT val = INTVAL (index);

	  if (TARGET_LDRD)
	    return val > -256 && val < 256;
	  else
	    return val > -4096 && val < 4092;
	}

      return TARGET_LDRD && arm_address_register_rtx_p (index, strict_p);
    }

  if (GET_MODE_SIZE (mode) <= 4
      && ! (arm_arch4
	    && (mode == HImode
		|| (mode == QImode && outer == SIGN_EXTEND))))
    {
      if (code == MULT)
	{
	  rtx xiop0 = XEXP (index, 0);
	  rtx xiop1 = XEXP (index, 1);

	  return ((arm_address_register_rtx_p (xiop0, strict_p)
		   && power_of_two_operand (xiop1, SImode))
		  || (arm_address_register_rtx_p (xiop1, strict_p)
		      && power_of_two_operand (xiop0, SImode)));
	}
      else if (code == LSHIFTRT || code == ASHIFTRT
	       || code == ASHIFT || code == ROTATERT)
	{
	  rtx op = XEXP (index, 1);

	  return (arm_address_register_rtx_p (XEXP (index, 0), strict_p)
		  && GET_CODE (op) == CONST_INT
		  && INTVAL (op) > 0
		  && INTVAL (op) <= 31);
	}
    }

  /* For ARM v4 we may be doing a sign-extend operation during the
     load.  */
  if (arm_arch4)
    {
      if (mode == HImode || (outer == SIGN_EXTEND && mode == QImode))
	range = 256;
      else
	range = 4096;
    }
  else
    range = (mode == HImode) ? 4095 : 4096;

  return (code == CONST_INT
	  && INTVAL (index) < range
	  && INTVAL (index) > -range);
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Return true if OP is a valid index scaling factor for Thumb-2 address
   index operand.  i.e. 1, 2, 4 or 8.  */
static bool
thumb2_index_mul_operand (rtx op)
{
  HOST_WIDE_INT val;
  
  if (GET_CODE(op) != CONST_INT)
    return false;

  val = INTVAL(op);
  return (val == 1 || val == 2 || val == 4 || val == 8);
}
  
/* Return nonzero if INDEX is a valid Thumb-2 address index operand.  */
static int
thumb2_legitimate_index_p (enum machine_mode mode, rtx index, int strict_p)
{
  enum rtx_code code = GET_CODE (index);

  /* ??? Combine arm and thumb2 coprocessor addressing modes.  */
  /* Standard coprocessor addressing modes.  */
  if (TARGET_HARD_FLOAT
      && (TARGET_FPA || TARGET_MAVERICK)
      && (GET_MODE_CLASS (mode) == MODE_FLOAT
	  || (TARGET_MAVERICK && mode == DImode)))
    return (code == CONST_INT && INTVAL (index) < 1024
	    && INTVAL (index) > -1024
	    && (INTVAL (index) & 3) == 0);

  if (TARGET_REALLY_IWMMXT && VALID_IWMMXT_REG_MODE (mode))
    return (code == CONST_INT
	    && INTVAL (index) < 1024
	    && INTVAL (index) > -1024
	    && (INTVAL (index) & 3) == 0);

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  if (TARGET_NEON
      /* ALQAAHIRA LOCAL 6150882 use thumb2 by default for v7 */
      && VECTOR_MODE_P (mode)
      && (VALID_NEON_DREG_MODE (mode) || VALID_NEON_QREG_MODE (mode)))
    return (code == CONST_INT
	    && INTVAL (index) < 1016
	    && INTVAL (index) > -1024
	    && (INTVAL (index) & 3) == 0);

  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  if (arm_address_register_rtx_p (index, strict_p)
      && (GET_MODE_SIZE (mode) <= 4))
    return 1;

  if (mode == DImode || mode == DFmode)
    {
      HOST_WIDE_INT val = INTVAL (index);
      /* ??? Can we assume ldrd for thumb2?  */
      /* Thumb-2 ldrd only has reg+const addressing modes.  */
      if (code != CONST_INT)
	return 0;

      /* ldrd supports offsets of +-1020.
         However the ldr fallback does not.  */
      return val > -256 && val < 256 && (val & 3) == 0;
    }

  if (code == MULT)
    {
      rtx xiop0 = XEXP (index, 0);
      rtx xiop1 = XEXP (index, 1);

      return ((arm_address_register_rtx_p (xiop0, strict_p)
	       && thumb2_index_mul_operand (xiop1))
	      || (arm_address_register_rtx_p (xiop1, strict_p)
		  && thumb2_index_mul_operand (xiop0)));
    }
  else if (code == ASHIFT)
    {
      rtx op = XEXP (index, 1);

      return (arm_address_register_rtx_p (XEXP (index, 0), strict_p)
	      && GET_CODE (op) == CONST_INT
	      && INTVAL (op) > 0
	      && INTVAL (op) <= 3);
    }

  return (code == CONST_INT
	  && INTVAL (index) < 4096
	  && INTVAL (index) > -256);
}

/* Return nonzero if X is valid as a 16-bit Thumb state base register.  */
static int
thumb1_base_register_rtx_p (rtx x, enum machine_mode mode, int strict_p)
{
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  int regno;

  if (GET_CODE (x) != REG)
    return 0;

  regno = REGNO (x);

  if (strict_p)
    /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
    return THUMB1_REGNO_MODE_OK_FOR_BASE_P (regno, mode);

  return (regno <= LAST_LO_REGNUM
	  || regno > LAST_VIRTUAL_REGISTER
	  || regno == FRAME_POINTER_REGNUM
	  || (GET_MODE_SIZE (mode) >= 4
	      && (regno == STACK_POINTER_REGNUM
		  || regno >= FIRST_PSEUDO_REGISTER
		  || x == hard_frame_pointer_rtx
		  || x == arg_pointer_rtx)));
}
 
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Return nonzero if x is a legitimate index register.  This is the case
   for any base register that can access a QImode object.  */
inline static int
thumb1_index_register_rtx_p (rtx x, int strict_p)
{
  return thumb1_base_register_rtx_p (x, QImode, strict_p);
}
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
/* Return nonzero if x is a legitimate 16-bit Thumb-state address.

   The AP may be eliminated to either the SP or the FP, so we use the
   least common denominator, e.g. SImode, and offsets from 0 to 64.

   ??? Verify whether the above is the right approach.

   ??? Also, the FP may be eliminated to the SP, so perhaps that
   needs special handling also.

   ??? Look at how the mips16 port solves this problem.  It probably uses
   better ways to solve some of these problems.

   Although it is not incorrect, we don't accept QImode and HImode
   addresses based on the frame pointer or arg pointer until the
   reload pass starts.  This is so that eliminating such addresses
   into stack based ones won't produce impossible code.  */
int
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
thumb1_legitimate_address_p (enum machine_mode mode, rtx x, int strict_p)
{
  /* ??? Not clear if this is right.  Experiment.  */
  if (GET_MODE_SIZE (mode) < 4
      && !(reload_in_progress || reload_completed)
      && (reg_mentioned_p (frame_pointer_rtx, x)
	  || reg_mentioned_p (arg_pointer_rtx, x)
	  || reg_mentioned_p (virtual_incoming_args_rtx, x)
	  || reg_mentioned_p (virtual_outgoing_args_rtx, x)
	  || reg_mentioned_p (virtual_stack_dynamic_rtx, x)
	  || reg_mentioned_p (virtual_stack_vars_rtx, x)))
    return 0;

  /* Accept any base register.  SP only in SImode or larger.  */
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  else if (thumb1_base_register_rtx_p (x, mode, strict_p))
    return 1;

  /* This is PC relative data before arm_reorg runs.  */
  else if (GET_MODE_SIZE (mode) >= 4 && CONSTANT_P (x)
	   && GET_CODE (x) == SYMBOL_REF
           && CONSTANT_POOL_ADDRESS_P (x) && !flag_pic)
    return 1;

  /* This is PC relative data after arm_reorg runs.  */
  else if (GET_MODE_SIZE (mode) >= 4 && reload_completed
	   && (GET_CODE (x) == LABEL_REF
	       || (GET_CODE (x) == CONST
		   && GET_CODE (XEXP (x, 0)) == PLUS
		   && GET_CODE (XEXP (XEXP (x, 0), 0)) == LABEL_REF
		   && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT)))
    return 1;

  /* Post-inc indexing only supported for SImode and larger.  */
  else if (GET_CODE (x) == POST_INC && GET_MODE_SIZE (mode) >= 4
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	   && thumb1_index_register_rtx_p (XEXP (x, 0), strict_p))
    return 1;

  else if (GET_CODE (x) == PLUS)
    {
      /* REG+REG address can be any two index registers.  */
      /* We disallow FRAME+REG addressing since we know that FRAME
	 will be replaced with STACK, and SP relative addressing only
	 permits SP+OFFSET.  */
      if (GET_MODE_SIZE (mode) <= 4
	  && XEXP (x, 0) != frame_pointer_rtx
	  && XEXP (x, 1) != frame_pointer_rtx
          /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
	  && thumb1_index_register_rtx_p (XEXP (x, 0), strict_p)
	  && thumb1_index_register_rtx_p (XEXP (x, 1), strict_p))
          /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	return 1;

      /* REG+const has 5-7 bit offset for non-SP registers.  */
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      else if ((thumb1_index_register_rtx_p (XEXP (x, 0), strict_p)
		|| XEXP (x, 0) == arg_pointer_rtx)
	       && GET_CODE (XEXP (x, 1)) == CONST_INT
	       && thumb_legitimate_offset_p (mode, INTVAL (XEXP (x, 1))))
	return 1;

      /* REG+const has 10 bit offset for SP, but only SImode and
	 larger is supported.  */
      /* ??? Should probably check for DI/DFmode overflow here
	 just like GO_IF_LEGITIMATE_OFFSET does.  */
      else if (GET_CODE (XEXP (x, 0)) == REG
	       && REGNO (XEXP (x, 0)) == STACK_POINTER_REGNUM
	       && GET_MODE_SIZE (mode) >= 4
	       && GET_CODE (XEXP (x, 1)) == CONST_INT
	       && INTVAL (XEXP (x, 1)) >= 0
	       && INTVAL (XEXP (x, 1)) + GET_MODE_SIZE (mode) <= 1024
	       && (INTVAL (XEXP (x, 1)) & 3) == 0)
	return 1;

      else if (GET_CODE (XEXP (x, 0)) == REG
	       && REGNO (XEXP (x, 0)) == FRAME_POINTER_REGNUM
	       && GET_MODE_SIZE (mode) >= 4
	       && GET_CODE (XEXP (x, 1)) == CONST_INT
	       && (INTVAL (XEXP (x, 1)) & 3) == 0)
	return 1;
    }

  else if (GET_MODE_CLASS (mode) != MODE_FLOAT
	   && GET_MODE_SIZE (mode) == 4
	   && GET_CODE (x) == SYMBOL_REF
	   && CONSTANT_POOL_ADDRESS_P (x)
	   && ! (flag_pic
		 && symbol_mentioned_p (get_pool_constant (x))
		 && ! pcrel_constant_p (get_pool_constant (x))))
    return 1;

  return 0;
}

/* Return nonzero if VAL can be used as an offset in a Thumb-state address
   instruction of mode MODE.  */
int
thumb_legitimate_offset_p (enum machine_mode mode, HOST_WIDE_INT val)
{
  switch (GET_MODE_SIZE (mode))
    {
    case 1:
      return val >= 0 && val < 32;

    case 2:
      return val >= 0 && val < 64 && (val & 1) == 0;

    default:
      return (val >= 0
	      && (val + GET_MODE_SIZE (mode)) <= 128
	      && (val & 3) == 0);
    }
}

/* Build the SYMBOL_REF for __tls_get_addr.  */

static GTY(()) rtx tls_get_addr_libfunc;

static rtx
get_tls_get_addr (void)
{
  if (!tls_get_addr_libfunc)
    tls_get_addr_libfunc = init_one_libfunc ("__tls_get_addr");
  return tls_get_addr_libfunc;
}

static rtx
arm_load_tp (rtx target)
{
  if (!target)
    target = gen_reg_rtx (SImode);

  if (TARGET_HARD_TP)
    {
      /* Can return in any reg.  */
      emit_insn (gen_load_tp_hard (target));
    }
  else
    {
      /* Always returned in r0.  Immediately copy the result into a pseudo,
	 otherwise other uses of r0 (e.g. setting up function arguments) may
	 clobber the value.  */

      rtx tmp;

      emit_insn (gen_load_tp_soft ());

      tmp = gen_rtx_REG (SImode, 0);
      emit_move_insn (target, tmp);
    }
  return target;
}

static rtx
load_tls_operand (rtx x, rtx reg)
{
  rtx tmp;

  if (reg == NULL_RTX)
    reg = gen_reg_rtx (SImode);

  tmp = gen_rtx_CONST (SImode, x);

  emit_move_insn (reg, tmp);

  return reg;
}

static rtx
arm_call_tls_get_addr (rtx x, rtx reg, rtx *valuep, int reloc)
{
  rtx insns, label, labelno, sum;

  start_sequence ();

  labelno = GEN_INT (pic_labelno++);
  label = gen_rtx_UNSPEC (Pmode, gen_rtvec (1, labelno), UNSPEC_PIC_LABEL);
  label = gen_rtx_CONST (VOIDmode, label);

  sum = gen_rtx_UNSPEC (Pmode,
			gen_rtvec (4, x, GEN_INT (reloc), label,
				   GEN_INT (TARGET_ARM ? 8 : 4)),
			UNSPEC_TLS);
  reg = load_tls_operand (sum, reg);

  if (TARGET_ARM)
    emit_insn (gen_pic_add_dot_plus_eight (reg, reg, labelno));
  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  else if (TARGET_THUMB2)
    {
      rtx tmp;
      /* Thumb-2 only allows very limited access to the PC.  Calculate
	 the address in a temporary register.  */
      tmp = gen_reg_rtx (SImode);
      emit_insn (gen_pic_load_dot_plus_four (tmp, labelno));
      emit_insn (gen_addsi3(reg, reg, tmp));
    }
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  else /* TARGET_THUMB1 */
    emit_insn (gen_pic_add_dot_plus_four (reg, reg, labelno));

  *valuep = emit_library_call_value (get_tls_get_addr (), NULL_RTX, LCT_PURE, /* LCT_CONST?  */
				     Pmode, 1, reg, Pmode);

  insns = get_insns ();
  end_sequence ();

  return insns;
}

rtx
legitimize_tls_address (rtx x, rtx reg)
{
  rtx dest, tp, label, labelno, sum, insns, ret, eqv, addend;
  unsigned int model = SYMBOL_REF_TLS_MODEL (x);

  switch (model)
    {
    case TLS_MODEL_GLOBAL_DYNAMIC:
      insns = arm_call_tls_get_addr (x, reg, &ret, TLS_GD32);
      dest = gen_reg_rtx (Pmode);
      emit_libcall_block (insns, dest, ret, x);
      return dest;

    case TLS_MODEL_LOCAL_DYNAMIC:
      insns = arm_call_tls_get_addr (x, reg, &ret, TLS_LDM32);

      /* Attach a unique REG_EQUIV, to allow the RTL optimizers to
	 share the LDM result with other LD model accesses.  */
      eqv = gen_rtx_UNSPEC (Pmode, gen_rtvec (1, const1_rtx),
			    UNSPEC_TLS);
      dest = gen_reg_rtx (Pmode);
      emit_libcall_block (insns, dest, ret, eqv);

      /* Load the addend.  */
      addend = gen_rtx_UNSPEC (Pmode, gen_rtvec (2, x, GEN_INT (TLS_LDO32)),
			       UNSPEC_TLS);
      addend = force_reg (SImode, gen_rtx_CONST (SImode, addend));
      return gen_rtx_PLUS (Pmode, dest, addend);

    case TLS_MODEL_INITIAL_EXEC:
      labelno = GEN_INT (pic_labelno++);
      label = gen_rtx_UNSPEC (Pmode, gen_rtvec (1, labelno), UNSPEC_PIC_LABEL);
      label = gen_rtx_CONST (VOIDmode, label);
      sum = gen_rtx_UNSPEC (Pmode,
			    gen_rtvec (4, x, GEN_INT (TLS_IE32), label,
				       GEN_INT (TARGET_ARM ? 8 : 4)),
			    UNSPEC_TLS);
      reg = load_tls_operand (sum, reg);

      if (TARGET_ARM)
	emit_insn (gen_tls_load_dot_plus_eight (reg, reg, labelno));
      else if (TARGET_THUMB2)
	{
	  rtx tmp;
	  /* Thumb-2 only allows very limited access to the PC.  Calculate
	     the address in a temporary register.  */
	  tmp = gen_reg_rtx (SImode);
	  emit_insn (gen_pic_load_dot_plus_four (tmp, labelno));
	  emit_insn (gen_addsi3(reg, reg, tmp));
	  emit_move_insn (reg, gen_const_mem (SImode, reg));
	}
      else
	{
	  emit_insn (gen_pic_add_dot_plus_four (reg, reg, labelno));
	  emit_move_insn (reg, gen_const_mem (SImode, reg));
	}

      tp = arm_load_tp (NULL_RTX);

      return gen_rtx_PLUS (Pmode, tp, reg);

    case TLS_MODEL_LOCAL_EXEC:
      tp = arm_load_tp (NULL_RTX);

      reg = gen_rtx_UNSPEC (Pmode,
			    gen_rtvec (2, x, GEN_INT (TLS_LE32)),
			    UNSPEC_TLS);
      reg = force_reg (SImode, gen_rtx_CONST (SImode, reg));

      return gen_rtx_PLUS (Pmode, tp, reg);

    default:
      abort ();
    }
}

/* Try machine-dependent ways of modifying an illegitimate address
   to be legitimate.  If we find one, return the new, valid address.  */
rtx
arm_legitimize_address (rtx x, rtx orig_x, enum machine_mode mode)
{
  if (arm_tls_symbol_p (x))
    return legitimize_tls_address (x, NULL_RTX);

  /* APPLE LOCAL begin ARM addresses involving large constants */
  if (flag_pic)
    {
      /* We need to find and carefully transform any SYMBOL and LABEL
	 references; so go back to the original address expression.  */
      rtx new_x = legitimize_pic_address (orig_x, mode, NULL_RTX);

      if (new_x != orig_x)
	x = new_x;
    }
  else if (GET_CODE (x) == PLUS)
    {
      rtx xop0 = XEXP (x, 0);
      rtx xop1 = XEXP (x, 1);

      if (CONSTANT_P (xop0) && !symbol_mentioned_p (xop0))
	xop0 = force_reg (SImode, xop0);

      if (CONSTANT_P (xop1) && !symbol_mentioned_p (xop1)
	  && GET_CODE (xop1) != CONST_INT)
	xop1 = force_reg (SImode, xop1);

      if (GET_CODE (xop1) == CONST_INT)
	{
	  HOST_WIDE_INT n, low_n;
	  rtx base_reg, val;

	  /* Look for
	     (+ (+ (foo, SFP) const)).  It is better to rearrange this as
	     (+ (foo (+ (SFP, const))).   The eventual SP + const1 + const will
	     get folded. */

	  if (GET_CODE (xop0) == PLUS)
	    {
	      rtx xop00 = XEXP (xop0, 0);
	      rtx xop01 = XEXP (xop0, 1);

	      if (xop01 == virtual_stack_vars_rtx)
		{
		  base_reg = gen_reg_rtx (SImode);
		  val = force_operand (gen_rtx_PLUS (SImode, xop01, xop1), 
					NULL_RTX);
		  emit_move_insn (base_reg, val);
		  /* Canonical form requires some non-reg ops to be first. */
		  x = gen_rtx_PLUS (SImode, xop00, base_reg);
		  return x;
		}
	    }

	  n = INTVAL (xop1);
	  /* The size of constant that fits in a load or store instruction
	     is different for different sized operations.  Break N into
	     low_n (the part that will fit in the instruction) and n
	     (the part that won't). */
	  /* VFP addressing modes actually allow greater offsets, but for
	     now we just stick with the lowest common denominator.  */
	  if (mode == DImode
	      || ((TARGET_SOFT_FLOAT || TARGET_VFP) && mode == DFmode))
	    {
	      low_n = n & 0x0f;
	      n &= ~0x0f;
	      if (low_n > 4)
		{
		  n += 16;
		  low_n -= 16;
		}
	    }
	  else if ((mode == HImode || mode == QImode) && arm_arch4)
	    {
	      low_n = n >= 0 ? (n & 0xff) : -((-n) & 0xff);
	      n -= low_n;
	    }
	  else
	    {
	      low_n = ((mode) == TImode ? 0
		       : n >= 0 ? (n & 0xfff) : -((-n) & 0xfff));
	      n -= low_n;
	    }

	  if (n != 0)
	    {
	      /* Emit an auxiliary instruction to compute base+high_part
		 into a register base_reg, then return base_reg+low_part. */
	      base_reg = gen_reg_rtx (SImode);
	      val = force_operand (plus_constant (xop0, n), NULL_RTX);
	      emit_move_insn (base_reg, val);
	      x = plus_constant (base_reg, low_n);
	    }
	  else if (xop0 != XEXP (x, 0) || xop1 != XEXP (x, 1))
	    x = gen_rtx_PLUS (SImode, xop0, xop1);
	}
      else if (xop0 != XEXP (x, 0) || xop1 != XEXP (x, 1))
	x = gen_rtx_PLUS (SImode, xop0, xop1);
    }

  /* XXX We don't allow MINUS any more -- see comment in
     arm_legitimate_address_p ().  */
  else if (GET_CODE (x) == MINUS)
    {
      rtx xop0 = XEXP (x, 0);
      rtx xop1 = XEXP (x, 1);

      if (CONSTANT_P (xop0))
	xop0 = force_reg (SImode, xop0);

      if (CONSTANT_P (xop1) && ! symbol_mentioned_p (xop1))
	xop1 = force_reg (SImode, xop1);

      if (xop0 != XEXP (x, 0) || xop1 != XEXP (x, 1))
	x = gen_rtx_MINUS (SImode, xop0, xop1);
    }

  /* Make sure to take full advantage of the pre-indexed addressing mode
     with absolute addresses which often allows for the base register to
     be factorized for multiple adjacent memory references, and it might
     even allows for the mini pool to be avoided entirely. */
  else if (GET_CODE (x) == CONST_INT && optimize > 0)
    {
      unsigned int bits;
      HOST_WIDE_INT mask, base, index;
      rtx base_reg;

      /* ldr and ldrb can use a 12 bit index, ldrsb and the rest can only
	 use a 8 bit index. So let's use a 12 bit index for SImode only and
	 hope that arm_gen_constant will enable ldrb to use more bits. */
      bits = (mode == SImode) ? 12 : 8;
      mask = (1 << bits) - 1;
      base = INTVAL (x) & ~mask;
      index = INTVAL (x) & mask;
      if (bit_count (base & 0xffffffff) > (32 - bits)/2)
	{
	  /* It'll most probably be more efficient to generate the base
	     with more bits set and use a negative index instead. */
	  base |= mask;
	  index -= mask;
	}
      base_reg = force_reg (SImode, GEN_INT (base));
      x = plus_constant (base_reg, index);
    }
  /* APPLE LOCAL end ARM addresses involving large constants */

  return x;
}


/* Try machine-dependent ways of modifying an illegitimate Thumb address
   to be legitimate.  If we find one, return the new, valid address.  */
rtx
thumb_legitimize_address (rtx x, rtx orig_x, enum machine_mode mode)
{
  if (arm_tls_symbol_p (x))
    return legitimize_tls_address (x, NULL_RTX);

  if (GET_CODE (x) == PLUS
      && GET_CODE (XEXP (x, 1)) == CONST_INT
      && (INTVAL (XEXP (x, 1)) >= 32 * GET_MODE_SIZE (mode)
	  || INTVAL (XEXP (x, 1)) < 0))
    {
      rtx xop0 = XEXP (x, 0);
      rtx xop1 = XEXP (x, 1);
      HOST_WIDE_INT offset = INTVAL (xop1);

      /* Try and fold the offset into a biasing of the base register and
	 then offsetting that.  Don't do this when optimizing for space
	 since it can cause too many CSEs.  */
      if (optimize_size && offset >= 0
	  && offset < 256 + 31 * GET_MODE_SIZE (mode))
	{
	  HOST_WIDE_INT delta;

	  if (offset >= 256)
	    delta = offset - (256 - GET_MODE_SIZE (mode));
	  else if (offset < 32 * GET_MODE_SIZE (mode) + 8)
	    delta = 31 * GET_MODE_SIZE (mode);
	  else
	    delta = offset & (~31 * GET_MODE_SIZE (mode));

	  xop0 = force_operand (plus_constant (xop0, offset - delta),
				NULL_RTX);
	  x = plus_constant (xop0, delta);
	}
      else if (offset < 0 && offset > -256)
	/* Small negative offsets are best done with a subtract before the
	   dereference, forcing these into a register normally takes two
	   instructions.  */
	x = force_operand (x, NULL_RTX);
      else
	{
	  /* For the remaining cases, force the constant into a register.  */
	  xop1 = force_reg (SImode, xop1);
	  x = gen_rtx_PLUS (SImode, xop0, xop1);
	}
    }
  else if (GET_CODE (x) == PLUS
	   && s_register_operand (XEXP (x, 1), SImode)
	   && !s_register_operand (XEXP (x, 0), SImode))
    {
      rtx xop0 = force_operand (XEXP (x, 0), NULL_RTX);

      x = gen_rtx_PLUS (SImode, xop0, XEXP (x, 1));
    }

  if (flag_pic)
    {
      /* We need to find and carefully transform any SYMBOL and LABEL
	 references; so go back to the original address expression.  */
      rtx new_x = legitimize_pic_address (orig_x, mode, NULL_RTX);

      if (new_x != orig_x)
	x = new_x;
    }

  return x;
}

rtx
thumb_legitimize_reload_address (rtx *x_p,
				 enum machine_mode mode,
				 int opnum, int type,
				 int ind_levels ATTRIBUTE_UNUSED)
{
  rtx x = *x_p;

  if (GET_CODE (x) == PLUS
      && GET_MODE_SIZE (mode) < 4
      && REG_P (XEXP (x, 0))
      && XEXP (x, 0) == stack_pointer_rtx
      && GET_CODE (XEXP (x, 1)) == CONST_INT
      && !thumb_legitimate_offset_p (mode, INTVAL (XEXP (x, 1))))
    {
      rtx orig_x = x;

      x = copy_rtx (x);
      push_reload (orig_x, NULL_RTX, x_p, NULL, MODE_BASE_REG_CLASS (mode),
		   Pmode, VOIDmode, 0, 0, opnum, type);
      return x;
    }

  /* If both registers are hi-regs, then it's better to reload the
     entire expression rather than each register individually.  That
     only requires one reload register rather than two.  */
  if (GET_CODE (x) == PLUS
      && REG_P (XEXP (x, 0))
      && REG_P (XEXP (x, 1))
      && !REG_MODE_OK_FOR_REG_BASE_P (XEXP (x, 0), mode)
      && !REG_MODE_OK_FOR_REG_BASE_P (XEXP (x, 1), mode))
    {
      rtx orig_x = x;

      x = copy_rtx (x);
      push_reload (orig_x, NULL_RTX, x_p, NULL, MODE_BASE_REG_CLASS (mode),
		   Pmode, VOIDmode, 0, 0, opnum, type);
      return x;
    }

  return NULL;
}

/* Test for various thread-local symbols.  */

/* Return TRUE if X is a thread-local symbol.  */

static bool
arm_tls_symbol_p (rtx x)
{
  if (! TARGET_HAVE_TLS)
    return false;

  if (GET_CODE (x) != SYMBOL_REF)
    return false;

  return SYMBOL_REF_TLS_MODEL (x) != 0;
}

/* Helper for arm_tls_referenced_p.  */

static int
arm_tls_operand_p_1 (rtx *x, void *data ATTRIBUTE_UNUSED)
{
  if (GET_CODE (*x) == SYMBOL_REF)
    return SYMBOL_REF_TLS_MODEL (*x) != 0;

  /* Don't recurse into UNSPEC_TLS looking for TLS symbols; these are
     TLS offsets, not real symbol references.  */
  if (GET_CODE (*x) == UNSPEC
      && XINT (*x, 1) == UNSPEC_TLS)
    return -1;

  return 0;
}

/* Return TRUE if X contains any TLS symbol references.  */

bool
arm_tls_referenced_p (rtx x)
{
  if (! TARGET_HAVE_TLS)
    return false;

  return for_each_rtx (&x, arm_tls_operand_p_1, NULL);
}

/* APPLE LOCAL begin ARM -mdynamic-no-pic support */
static bool
arm_cannot_force_const_mem (rtx x)
{
  return arm_tls_referenced_p (x)
	 || ! LEGITIMATE_INDIRECT_OPERAND_P (x);
}
/* APPLE LOCAL end ARM -mdynamic-no-pic support */

#define REG_OR_SUBREG_REG(X)						\
  (GET_CODE (X) == REG							\
   || (GET_CODE (X) == SUBREG && GET_CODE (SUBREG_REG (X)) == REG))

#define REG_OR_SUBREG_RTX(X)			\
   (GET_CODE (X) == REG ? (X) : SUBREG_REG (X))

#ifndef COSTS_N_INSNS
#define COSTS_N_INSNS(N) ((N) * 4 - 2)
#endif
static inline int
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
thumb1_rtx_costs (rtx x, enum rtx_code code, enum rtx_code outer)
{
  enum machine_mode mode = GET_MODE (x);

  switch (code)
    {
    case ASHIFT:
    case ASHIFTRT:
    case LSHIFTRT:
    case ROTATERT:
    case PLUS:
    case MINUS:
    case COMPARE:
    case NEG:
    case NOT:
      return COSTS_N_INSNS (1);

    case MULT:
      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  int cycles = 0;
	  unsigned HOST_WIDE_INT i = INTVAL (XEXP (x, 1));

	  while (i)
	    {
	      i >>= 2;
	      cycles++;
	    }
	  return COSTS_N_INSNS (2) + cycles;
	}
      return COSTS_N_INSNS (1) + 16;

    case SET:
      return (COSTS_N_INSNS (1)
	      + 4 * ((GET_CODE (SET_SRC (x)) == MEM)
		     + GET_CODE (SET_DEST (x)) == MEM));

    case CONST_INT:
      if (outer == SET)
	{
	  if ((unsigned HOST_WIDE_INT) INTVAL (x) < 256)
	    return 0;
	  if (thumb_shiftable_const (INTVAL (x)))
	    return COSTS_N_INSNS (2);
	  return COSTS_N_INSNS (3);
	}
      else if ((outer == PLUS || outer == COMPARE)
	       && INTVAL (x) < 256 && INTVAL (x) > -256)
	return 0;
      else if (outer == AND
	       && INTVAL (x) < 256 && INTVAL (x) >= -256)
	return COSTS_N_INSNS (1);
      else if (outer == ASHIFT || outer == ASHIFTRT
	       || outer == LSHIFTRT)
	return 0;
      return COSTS_N_INSNS (2);

    case CONST:
    case CONST_DOUBLE:
    case LABEL_REF:
    case SYMBOL_REF:
      return COSTS_N_INSNS (3);

    case UDIV:
    case UMOD:
    case DIV:
    case MOD:
      return 100;

    case TRUNCATE:
      return 99;

    case AND:
    case XOR:
    case IOR:
      /* XXX guess.  */
      return 8;

    case MEM:
      /* XXX another guess.  */
      /* Memory costs quite a lot for the first word, but subsequent words
	 load at the equivalent of a single insn each.  */
      return (10 + 4 * ((GET_MODE_SIZE (mode) - 1) / UNITS_PER_WORD)
	      + ((GET_CODE (x) == SYMBOL_REF && CONSTANT_POOL_ADDRESS_P (x))
		 ? 4 : 0));

    case IF_THEN_ELSE:
      /* XXX a guess.  */
      if (GET_CODE (XEXP (x, 1)) == PC || GET_CODE (XEXP (x, 2)) == PC)
	return 14;
      return 2;

    case ZERO_EXTEND:
      /* XXX still guessing.  */
      switch (GET_MODE (XEXP (x, 0)))
	{
	case QImode:
	  return (1 + (mode == DImode ? 4 : 0)
		  + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	case HImode:
	  return (4 + (mode == DImode ? 4 : 0)
		  + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	case SImode:
	  return (1 + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	default:
	  return 99;
	}

    default:
      return 99;
    }
}

/* APPLE LOCAL begin ARM size variant of thumb costs */
/* This is very much a work in progress; it is just thumb_rtx_costs
   with modifications for size as discovered.  Currently, the costs
   for MULT, AND, XOR, IOR have been fixed; all of these are single
   instructions.  (Not for DImode, but that's not taken into account
   anywhere here.)  */

static inline int
thumb1_size_rtx_costs (rtx x, enum rtx_code code, enum rtx_code outer)
{
  enum machine_mode mode = GET_MODE (x);

  switch (code)
    {
    case ASHIFT:
    case ASHIFTRT:
    case LSHIFTRT:
    case ROTATERT:
    case PLUS:
    case MINUS:
    case COMPARE:
    case NEG:
    case NOT:
    case AND:
    case XOR:
    case IOR:
    case MULT:
      return COSTS_N_INSNS (1);

    case SET:
      return (COSTS_N_INSNS (1)
	      + 4 * ((GET_CODE (SET_SRC (x)) == MEM)
		     + GET_CODE (SET_DEST (x)) == MEM));

    case CONST_INT:
      if (outer == SET)
	{
	  if ((unsigned HOST_WIDE_INT) INTVAL (x) < 256)
	    return 0;
	  if (thumb_shiftable_const (INTVAL (x)))
	    return COSTS_N_INSNS (2);
	  return COSTS_N_INSNS (3);
	}
      else if ((outer == PLUS || outer == COMPARE)
	       && INTVAL (x) < 256 && INTVAL (x) > -256)
	return 0;
      else if (outer == AND
	       && INTVAL (x) < 256 && INTVAL (x) >= -256)
	return COSTS_N_INSNS (1);
      else if (outer == ASHIFT || outer == ASHIFTRT
	       || outer == LSHIFTRT)
	return 0;
      return COSTS_N_INSNS (2);

    case CONST:
    case CONST_DOUBLE:
    case LABEL_REF:
    case SYMBOL_REF:
      return COSTS_N_INSNS (3);

    case UDIV:
    case UMOD:
    case DIV:
    case MOD:
      return 100;

    case TRUNCATE:
      return 99;

    case MEM:
      /* XXX another guess.  */
      /* Memory costs quite a lot for the first word, but subsequent words
	 load at the equivalent of a single insn each.  */
      return (10 + 4 * ((GET_MODE_SIZE (mode) - 1) / UNITS_PER_WORD)
	      + ((GET_CODE (x) == SYMBOL_REF && CONSTANT_POOL_ADDRESS_P (x))
		 ? 4 : 0));

    case IF_THEN_ELSE:
      /* XXX a guess.  */
      if (GET_CODE (XEXP (x, 1)) == PC || GET_CODE (XEXP (x, 2)) == PC)
	return 14;
      return 2;

    case ZERO_EXTEND:
      /* XXX still guessing.  */
      switch (GET_MODE (XEXP (x, 0)))
	{
	case QImode:
	  return (1 + (mode == DImode ? 4 : 0)
		  + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	case HImode:
	  return (4 + (mode == DImode ? 4 : 0)
		  + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	case SImode:
	  return (1 + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	default:
	  return 99;
	}

    default:
      return 99;
    }
}
/* APPLE LOCAL end ARM size variant of thumb costs */

/* Worker routine for arm_rtx_costs.  */
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
/* ??? This needs updating for thumb2.  */
static inline int
arm_rtx_costs_1 (rtx x, enum rtx_code code, enum rtx_code outer)
{
  enum machine_mode mode = GET_MODE (x);
  enum rtx_code subcode;
  int extra_cost;

  switch (code)
    {
    case MEM:
      /* Memory costs quite a lot for the first word, but subsequent words
	 load at the equivalent of a single insn each.  */
      return (10 + 4 * ((GET_MODE_SIZE (mode) - 1) / UNITS_PER_WORD)
	      + (GET_CODE (x) == SYMBOL_REF
		 && CONSTANT_POOL_ADDRESS_P (x) ? 4 : 0));

    case DIV:
    case MOD:
    case UDIV:
    case UMOD:
      return optimize_size ? COSTS_N_INSNS (2) : 100;

    case ROTATE:
      if (mode == SImode && GET_CODE (XEXP (x, 1)) == REG)
	return 4;
      /* Fall through */
    case ROTATERT:
      if (mode != SImode)
	return 8;
      /* Fall through */
    case ASHIFT: case LSHIFTRT: case ASHIFTRT:
      if (mode == DImode)
	return (8 + (GET_CODE (XEXP (x, 1)) == CONST_INT ? 0 : 8)
		+ ((GET_CODE (XEXP (x, 0)) == REG
		    || (GET_CODE (XEXP (x, 0)) == SUBREG
			&& GET_CODE (SUBREG_REG (XEXP (x, 0))) == REG))
		   ? 0 : 8));
      return (1 + ((GET_CODE (XEXP (x, 0)) == REG
		    || (GET_CODE (XEXP (x, 0)) == SUBREG
			&& GET_CODE (SUBREG_REG (XEXP (x, 0))) == REG))
		   ? 0 : 4)
	      + ((GET_CODE (XEXP (x, 1)) == REG
		  || (GET_CODE (XEXP (x, 1)) == SUBREG
		      && GET_CODE (SUBREG_REG (XEXP (x, 1))) == REG)
		  || (GET_CODE (XEXP (x, 1)) == CONST_INT))
		 ? 0 : 4));

    case MINUS:
      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      if (GET_CODE (XEXP (x, 1)) == MULT && mode == SImode && arm_arch_thumb2)
	{
	  extra_cost = rtx_cost (XEXP (x, 1), code);
	  if (!REG_OR_SUBREG_REG (XEXP (x, 0)))
	    extra_cost += 4 * ARM_NUM_REGS (mode);
	  return extra_cost;
	}

      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
      if (mode == DImode)
	return (4 + (REG_OR_SUBREG_REG (XEXP (x, 1)) ? 0 : 8)
		+ ((REG_OR_SUBREG_REG (XEXP (x, 0))
		    || (GET_CODE (XEXP (x, 0)) == CONST_INT
		       && const_ok_for_arm (INTVAL (XEXP (x, 0)))))
		   ? 0 : 8));

      if (GET_MODE_CLASS (mode) == MODE_FLOAT)
	return (2 + ((REG_OR_SUBREG_REG (XEXP (x, 1))
		      || (GET_CODE (XEXP (x, 1)) == CONST_DOUBLE
			  && arm_const_double_rtx (XEXP (x, 1))))
		     ? 0 : 8)
		+ ((REG_OR_SUBREG_REG (XEXP (x, 0))
		    || (GET_CODE (XEXP (x, 0)) == CONST_DOUBLE
			&& arm_const_double_rtx (XEXP (x, 0))))
		   ? 0 : 8));

      if (((GET_CODE (XEXP (x, 0)) == CONST_INT
	    && const_ok_for_arm (INTVAL (XEXP (x, 0)))
	    && REG_OR_SUBREG_REG (XEXP (x, 1))))
	  || (((subcode = GET_CODE (XEXP (x, 1))) == ASHIFT
	       || subcode == ASHIFTRT || subcode == LSHIFTRT
	       || subcode == ROTATE || subcode == ROTATERT
	       || (subcode == MULT
		   && GET_CODE (XEXP (XEXP (x, 1), 1)) == CONST_INT
		   && ((INTVAL (XEXP (XEXP (x, 1), 1)) &
			(INTVAL (XEXP (XEXP (x, 1), 1)) - 1)) == 0)))
	      && REG_OR_SUBREG_REG (XEXP (XEXP (x, 1), 0))
	      && (REG_OR_SUBREG_REG (XEXP (XEXP (x, 1), 1))
		  || GET_CODE (XEXP (XEXP (x, 1), 1)) == CONST_INT)
	      && REG_OR_SUBREG_REG (XEXP (x, 0))))
	return 1;
      /* Fall through */

    case PLUS:
      if (GET_CODE (XEXP (x, 0)) == MULT)
	{
	  extra_cost = rtx_cost (XEXP (x, 0), code);
	  if (!REG_OR_SUBREG_REG (XEXP (x, 1)))
	    extra_cost += 4 * ARM_NUM_REGS (mode);
	  return extra_cost;
	}

      if (GET_MODE_CLASS (mode) == MODE_FLOAT)
	return (2 + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : 8)
		+ ((REG_OR_SUBREG_REG (XEXP (x, 1))
		    || (GET_CODE (XEXP (x, 1)) == CONST_DOUBLE
			&& arm_const_double_rtx (XEXP (x, 1))))
		   ? 0 : 8));

      /* Fall through */
    case AND: case XOR: case IOR:
      extra_cost = 0;

      /* Normally the frame registers will be spilt into reg+const during
	 reload, so it is a bad idea to combine them with other instructions,
	 since then they might not be moved outside of loops.  As a compromise
	 we allow integration with ops that have a constant as their second
	 operand.  */
      if ((REG_OR_SUBREG_REG (XEXP (x, 0))
	   && ARM_FRAME_RTX (REG_OR_SUBREG_RTX (XEXP (x, 0)))
	   && GET_CODE (XEXP (x, 1)) != CONST_INT)
	  || (REG_OR_SUBREG_REG (XEXP (x, 0))
	      && ARM_FRAME_RTX (REG_OR_SUBREG_RTX (XEXP (x, 0)))))
	extra_cost = 4;

      if (mode == DImode)
	return (4 + extra_cost + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : 8)
		+ ((REG_OR_SUBREG_REG (XEXP (x, 1))
		    || (GET_CODE (XEXP (x, 1)) == CONST_INT
			&& const_ok_for_op (INTVAL (XEXP (x, 1)), code)))
		   ? 0 : 8));

      if (REG_OR_SUBREG_REG (XEXP (x, 0)))
	return (1 + (GET_CODE (XEXP (x, 1)) == CONST_INT ? 0 : extra_cost)
		+ ((REG_OR_SUBREG_REG (XEXP (x, 1))
		    || (GET_CODE (XEXP (x, 1)) == CONST_INT
			&& const_ok_for_op (INTVAL (XEXP (x, 1)), code)))
		   ? 0 : 4));

      /* APPLE LOCAL begin ARM 4652753 */
      /* If the previous insn feeds into the shifted operand of this one,
	 there is a 1 cycle delay.  We can't tell here whether this will
	 be the case or not.  Model it for now, as this seems to lead to
	 better decisions about splitting up multiply-by-constant. */
      else if (REG_OR_SUBREG_REG (XEXP (x, 1)))
	return (1 + extra_cost
		+ ((((subcode = GET_CODE (XEXP (x, 0))) == ASHIFT
		     || subcode == LSHIFTRT || subcode == ASHIFTRT
		     || subcode == ROTATE || subcode == ROTATERT
		     || (subcode == MULT
			 && GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT
			 && ((INTVAL (XEXP (XEXP (x, 0), 1)) &
			      (INTVAL (XEXP (XEXP (x, 0), 1)) - 1)) == 0)))
		    && (REG_OR_SUBREG_REG (XEXP (XEXP (x, 0), 0)))
		    && ((REG_OR_SUBREG_REG (XEXP (XEXP (x, 0), 1)))
			|| GET_CODE (XEXP (XEXP (x, 0), 1)) == CONST_INT))
		   ? 1 : 4));
      /* APPLE LOCAL end ARM 4652753 */

      return 8;

    case MULT:
      /* This should have been handled by the CPU specific routines.  */
      gcc_unreachable ();

    case TRUNCATE:
      if (arm_arch3m && mode == SImode
	  && GET_CODE (XEXP (x, 0)) == LSHIFTRT
	  && GET_CODE (XEXP (XEXP (x, 0), 0)) == MULT
	  && (GET_CODE (XEXP (XEXP (XEXP (x, 0), 0), 0))
	      == GET_CODE (XEXP (XEXP (XEXP (x, 0), 0), 1)))
	  && (GET_CODE (XEXP (XEXP (XEXP (x, 0), 0), 0)) == ZERO_EXTEND
	      || GET_CODE (XEXP (XEXP (XEXP (x, 0), 0), 0)) == SIGN_EXTEND))
	return 8;
      return 99;

    case NEG:
      if (GET_MODE_CLASS (mode) == MODE_FLOAT)
	return 4 + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : 6);
      /* Fall through */
    case NOT:
      if (mode == DImode)
	return 4 + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : 4);

      return 1 + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : 4);

    case IF_THEN_ELSE:
      if (GET_CODE (XEXP (x, 1)) == PC || GET_CODE (XEXP (x, 2)) == PC)
	return 14;
      return 2;

    case COMPARE:
      return 1;

    case ABS:
      return 4 + (mode == DImode ? 4 : 0);

    case SIGN_EXTEND:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      /* ??? value extensions are cheaper on armv6. */
      if (GET_MODE (XEXP (x, 0)) == QImode)
	return (4 + (mode == DImode ? 4 : 0)
		+ (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));
      /* Fall through */
    case ZERO_EXTEND:
      switch (GET_MODE (XEXP (x, 0)))
	{
	case QImode:
	  return (1 + (mode == DImode ? 4 : 0)
		  + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	case HImode:
	  return (4 + (mode == DImode ? 4 : 0)
		  + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	case SImode:
	  return (1 + (GET_CODE (XEXP (x, 0)) == MEM ? 10 : 0));

	case V8QImode:
	case V4HImode:
	case V2SImode:
	case V4QImode:
	case V2HImode:
	    return 1;

	default:
	  gcc_unreachable ();
	}
      gcc_unreachable ();

    case CONST_INT:
      if (const_ok_for_arm (INTVAL (x)))
	return outer == SET ? 2 : -1;
      else if (outer == AND
	       && const_ok_for_arm (~INTVAL (x)))
	return -1;
      else if ((outer == COMPARE
		|| outer == PLUS || outer == MINUS)
	       && const_ok_for_arm (-INTVAL (x)))
	return -1;
      else
	return 5;

    case CONST:
    case LABEL_REF:
    case SYMBOL_REF:
      return 6;

    case CONST_DOUBLE:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      if (arm_const_double_rtx (x) || vfp3_const_double_rtx (x))
	return outer == SET ? 2 : -1;
      else if ((outer == COMPARE || outer == PLUS)
	       && neg_const_double_rtx_ok_for_fpa (x))
	return -1;
      return 7;

    default:
      return 99;
    }
}

/* RTX costs when optimizing for size.  */
static bool
arm_size_rtx_costs (rtx x, int code, int outer_code, int *total)
{
  enum machine_mode mode = GET_MODE (x);

  if (TARGET_THUMB)
    {
      /* APPLE LOCAL end ARM size variant of thumb costs */
      *total = thumb1_size_rtx_costs (x, code, outer_code);
      return true;
    }

  switch (code)
    {
    case MEM:
      /* A memory access costs 1 insn if the mode is small, or the address is
	 a single register, otherwise it costs one insn per word.  */
      if (REG_P (XEXP (x, 0)))
	*total = COSTS_N_INSNS (1);
      else
	*total = COSTS_N_INSNS (ARM_NUM_REGS (mode));
      return true;

    case DIV:
    case MOD:
    case UDIV:
    case UMOD:
      /* Needs a libcall, so it costs about this.  */
      *total = COSTS_N_INSNS (2);
      return false;

    case ROTATE:
      if (mode == SImode && GET_CODE (XEXP (x, 1)) == REG)
	{
	  *total = COSTS_N_INSNS (2) + rtx_cost (XEXP (x, 0), code);
	  return true;
	}
      /* Fall through */
    case ROTATERT:
    case ASHIFT:
    case LSHIFTRT:
    case ASHIFTRT:
      if (mode == DImode && GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  *total = COSTS_N_INSNS (3) + rtx_cost (XEXP (x, 0), code);
	  return true;
	}
      else if (mode == SImode)
	{
	  *total = COSTS_N_INSNS (1) + rtx_cost (XEXP (x, 0), code);
	  /* Slightly disparage register shifts, but not by much.  */
	  if (GET_CODE (XEXP (x, 1)) != CONST_INT)
	    *total += 1 + rtx_cost (XEXP (x, 1), code);
	  return true;
	}

      /* Needs a libcall.  */
      *total = COSTS_N_INSNS (2);
      return false;

    case MINUS:
      if (TARGET_HARD_FLOAT && GET_MODE_CLASS (mode) == MODE_FLOAT)
	{
	  *total = COSTS_N_INSNS (1);
	  return false;
	}

      if (mode == SImode)
	{
	  enum rtx_code subcode0 = GET_CODE (XEXP (x, 0));
	  enum rtx_code subcode1 = GET_CODE (XEXP (x, 1));

	  if (subcode0 == ROTATE || subcode0 == ROTATERT || subcode0 == ASHIFT
	      || subcode0 == LSHIFTRT || subcode0 == ASHIFTRT
	      || subcode1 == ROTATE || subcode1 == ROTATERT
	      || subcode1 == ASHIFT || subcode1 == LSHIFTRT
	      || subcode1 == ASHIFTRT)
	    {
	      /* It's just the cost of the two operands.  */
	      *total = 0;
	      return false;
	    }

	  *total = COSTS_N_INSNS (1);
	  return false;
	}

      *total = COSTS_N_INSNS (ARM_NUM_REGS (mode));
      return false;

    case PLUS:
      if (TARGET_HARD_FLOAT && GET_MODE_CLASS (mode) == MODE_FLOAT)
	{
	  *total = COSTS_N_INSNS (1);
	  return false;
	}

      /* Fall through */
    case AND: case XOR: case IOR:
      if (mode == SImode)
	{
	  enum rtx_code subcode = GET_CODE (XEXP (x, 0));

	  if (subcode == ROTATE || subcode == ROTATERT || subcode == ASHIFT
	      || subcode == LSHIFTRT || subcode == ASHIFTRT
	      || (code == AND && subcode == NOT))
	    {
	      /* It's just the cost of the two operands.  */
	      *total = 0;
	      return false;
	    }
	}

      *total = COSTS_N_INSNS (ARM_NUM_REGS (mode));
      return false;

    /* APPLE LOCAL begin DImode multiply enhancement */
    case MULT:
      if (mode == DImode)
	{
	  if (((GET_CODE (XEXP (x, 0)) == SIGN_EXTEND
	           && GET_CODE (XEXP (x, 1)) == SIGN_EXTEND)
	        || (GET_CODE (XEXP (x, 0)) == ZERO_EXTEND
	            && GET_CODE (XEXP (x, 1)) == ZERO_EXTEND))
	       && GET_MODE (XEXP (XEXP (x, 0), 0)) == SImode
	       && GET_MODE (XEXP (XEXP (x, 1), 0)) == SImode)
	    {
	      /* SMULL, etc., do sign extend better than free */
	      *total = COSTS_N_INSNS (1)
	               + rtx_cost (XEXP (XEXP (x, 0), 0), MULT)
	               + rtx_cost (XEXP (XEXP (x, 1), 0), MULT);
	      return true;
	    }
	  else
	    {
	      /* broken into 3 insns later, plus cost of kids */
	      /** does not allow for Cirrus instruction **/
	      *total = COSTS_N_INSNS (3);
	      return false;
	    }
	}
      *total = COSTS_N_INSNS (ARM_NUM_REGS (mode));
      return false;
    /* APPLE LOCAL end DImode multiply enhancement */

    case NEG:
      if (TARGET_HARD_FLOAT && GET_MODE_CLASS (mode) == MODE_FLOAT)
	*total = COSTS_N_INSNS (1);
      /* Fall through */
    case NOT:
      *total = COSTS_N_INSNS (ARM_NUM_REGS (mode));

      return false;

    case IF_THEN_ELSE:
      *total = 0;
      return false;

    case COMPARE:
      if (cc_register (XEXP (x, 0), VOIDmode))
	* total = 0;
      else
	*total = COSTS_N_INSNS (1);
      return false;

    case ABS:
      if (TARGET_HARD_FLOAT && GET_MODE_CLASS (mode) == MODE_FLOAT)
	*total = COSTS_N_INSNS (1);
      else
	*total = COSTS_N_INSNS (1 + ARM_NUM_REGS (mode));
      return false;

    case SIGN_EXTEND:
      *total = 0;
      if (GET_MODE_SIZE (GET_MODE (XEXP (x, 0))) < 4)
	{
	  if (!(arm_arch4 && MEM_P (XEXP (x, 0))))
	    *total += COSTS_N_INSNS (arm_arch6 ? 1 : 2);
	}
      if (mode == DImode)
	*total += COSTS_N_INSNS (1);
      return false;

    case ZERO_EXTEND:
      *total = 0;
      if (!(arm_arch4 && MEM_P (XEXP (x, 0))))
	{
	  switch (GET_MODE (XEXP (x, 0)))
	    {
	    case QImode:
	      *total += COSTS_N_INSNS (1);
	      break;

	    case HImode:
	      *total += COSTS_N_INSNS (arm_arch6 ? 1 : 2);

	    case SImode:
	      break;

	    default:
	      *total += COSTS_N_INSNS (2);
	    }
	}

      if (mode == DImode)
	*total += COSTS_N_INSNS (1);

      return false;

    case CONST_INT:
      if (const_ok_for_arm (INTVAL (x)))
	*total = COSTS_N_INSNS (outer_code == SET ? 1 : 0);
      else if (const_ok_for_arm (~INTVAL (x)))
	*total = COSTS_N_INSNS (outer_code == AND ? 0 : 1);
      else if (const_ok_for_arm (-INTVAL (x)))
	{
	  if (outer_code == COMPARE || outer_code == PLUS
	      || outer_code == MINUS)
	    *total = 0;
	  else
	    *total = COSTS_N_INSNS (1);
	}
      else
	*total = COSTS_N_INSNS (2);
      return true;

    case CONST:
    case LABEL_REF:
    case SYMBOL_REF:
      *total = COSTS_N_INSNS (2);
      return true;

    case CONST_DOUBLE:
      *total = COSTS_N_INSNS (4);
      return true;

    default:
      if (mode != VOIDmode)
	*total = COSTS_N_INSNS (ARM_NUM_REGS (mode));
      else
	*total = COSTS_N_INSNS (4); /* How knows?  */
      return false;
    }
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* RTX costs for cores with a slow MUL implementation.  Thumb-2 is not
   supported on any "slowmul" cores, so it can be ignored.  */
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

static bool
arm_slowmul_rtx_costs (rtx x, int code, int outer_code, int *total)
{
  enum machine_mode mode = GET_MODE (x);

  if (TARGET_THUMB)
    {
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      *total = thumb1_rtx_costs (x, code, outer_code);
      return true;
    }

  switch (code)
    {
    case MULT:
      if (GET_MODE_CLASS (mode) == MODE_FLOAT
	  || mode == DImode)
	{
	  *total = 30;
	  return true;
	}

      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  unsigned HOST_WIDE_INT i = (INTVAL (XEXP (x, 1))
				      & (unsigned HOST_WIDE_INT) 0xffffffff);
	  int cost, const_ok = const_ok_for_arm (i);
	  int j, booth_unit_size;

	  /* Tune as appropriate.  */
	  cost = const_ok ? 4 : 8;
	  booth_unit_size = 2;
	  for (j = 0; i && j < 32; j += booth_unit_size)
	    {
	      i >>= booth_unit_size;
	      cost += 2;
	    }

	  *total = cost;
	  return true;
	}

      *total = 30 + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : 4)
	          + (REG_OR_SUBREG_REG (XEXP (x, 1)) ? 0 : 4);
      return true;

    default:
      *total = arm_rtx_costs_1 (x, code, outer_code);
      return true;
    }
}


/* RTX cost for cores with a fast multiply unit (M variants).  */

static bool
arm_fastmul_rtx_costs (rtx x, int code, int outer_code, int *total)
{
  enum machine_mode mode = GET_MODE (x);

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (TARGET_THUMB1)
    {
      *total = thumb1_rtx_costs (x, code, outer_code);
      return true;
    }

  /* ??? should thumb2 use different costs?  */
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  switch (code)
    {
    case MULT:
      /* There is no point basing this on the tuning, since it is always the
	 fast variant if it exists at all.  */
      if (mode == DImode
	  && (GET_CODE (XEXP (x, 0)) == GET_CODE (XEXP (x, 1)))
	  && (GET_CODE (XEXP (x, 0)) == ZERO_EXTEND
	      || GET_CODE (XEXP (x, 0)) == SIGN_EXTEND))
	{
	  *total = 8;
	  return true;
	}


      if (GET_MODE_CLASS (mode) == MODE_FLOAT
	  || mode == DImode)
	{
	  *total = 30;
	  return true;
	}

      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  unsigned HOST_WIDE_INT i = (INTVAL (XEXP (x, 1))
				      & (unsigned HOST_WIDE_INT) 0xffffffff);
	  int cost, const_ok = const_ok_for_arm (i);
	  int j, booth_unit_size;

	  /* Tune as appropriate.  */
	  cost = const_ok ? 4 : 8;
	  booth_unit_size = 8;
	  for (j = 0; i && j < 32; j += booth_unit_size)
	    {
	      i >>= booth_unit_size;
	      cost += 2;
	    }

	  *total = cost;
	  return true;
	}

      *total = 8 + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : 4)
	         + (REG_OR_SUBREG_REG (XEXP (x, 1)) ? 0 : 4);
      return true;

    default:
      *total = arm_rtx_costs_1 (x, code, outer_code);
      return true;
    }
}


/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* RTX cost for XScale CPUs.  Thumb-2 is not supported on any xscale cores,
   so it can be ignored.  */
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

static bool
arm_xscale_rtx_costs (rtx x, int code, int outer_code, int *total)
{
  enum machine_mode mode = GET_MODE (x);

  if (TARGET_THUMB)
    {
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      *total = thumb1_rtx_costs (x, code, outer_code);
      return true;
    }

  switch (code)
    {
    case MULT:
      /* There is no point basing this on the tuning, since it is always the
	 fast variant if it exists at all.  */
      if (mode == DImode
	  && (GET_CODE (XEXP (x, 0)) == GET_CODE (XEXP (x, 1)))
	  && (GET_CODE (XEXP (x, 0)) == ZERO_EXTEND
	      || GET_CODE (XEXP (x, 0)) == SIGN_EXTEND))
	{
	  *total = 8;
	  return true;
	}


      if (GET_MODE_CLASS (mode) == MODE_FLOAT
	  || mode == DImode)
	{
	  *total = 30;
	  return true;
	}

      if (GET_CODE (XEXP (x, 1)) == CONST_INT)
	{
	  unsigned HOST_WIDE_INT i = (INTVAL (XEXP (x, 1))
				      & (unsigned HOST_WIDE_INT) 0xffffffff);
	  int cost, const_ok = const_ok_for_arm (i);
	  unsigned HOST_WIDE_INT masked_const;

	  /* The cost will be related to two insns.
	     First a load of the constant (MOV or LDR), then a multiply.  */
	  cost = 2;
	  if (! const_ok)
	    cost += 1;      /* LDR is probably more expensive because
			       of longer result latency.  */
	  masked_const = i & 0xffff8000;
	  if (masked_const != 0 && masked_const != 0xffff8000)
	    {
	      masked_const = i & 0xf8000000;
	      if (masked_const == 0 || masked_const == 0xf8000000)
		cost += 1;
	      else
		cost += 2;
	    }
	  *total = cost;
	  return true;
	}

      *total = 8 + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : 4)
		 + (REG_OR_SUBREG_REG (XEXP (x, 1)) ? 0 : 4);
      return true;

    case COMPARE:
      /* A COMPARE of a MULT is slow on XScale; the muls instruction
	 will stall until the multiplication is complete.  */
      if (GET_CODE (XEXP (x, 0)) == MULT)
	*total = 4 + rtx_cost (XEXP (x, 0), code);
      else
	*total = arm_rtx_costs_1 (x, code, outer_code);
      return true;

    default:
      *total = arm_rtx_costs_1 (x, code, outer_code);
      return true;
    }
}


/* RTX costs for 9e (and later) cores.  */

static bool
arm_9e_rtx_costs (rtx x, int code, int outer_code, int *total)
{
  enum machine_mode mode = GET_MODE (x);
  int nonreg_cost;
  int cost;

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (TARGET_THUMB1)
    {
      switch (code)
	{
	case MULT:
	  *total = COSTS_N_INSNS (3);
	  return true;

	default:
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  *total = thumb1_rtx_costs (x, code, outer_code);
	  return true;
	}
    }

  switch (code)
    {
    case MULT:
      /* There is no point basing this on the tuning, since it is always the
	 fast variant if it exists at all.  */
      if (mode == DImode
	  && (GET_CODE (XEXP (x, 0)) == GET_CODE (XEXP (x, 1)))
	  && (GET_CODE (XEXP (x, 0)) == ZERO_EXTEND
	      || GET_CODE (XEXP (x, 0)) == SIGN_EXTEND))
	{
	  *total = 3;
	  return true;
	}


      if (GET_MODE_CLASS (mode) == MODE_FLOAT)
	{
	  *total = 30;
	  return true;
	}
      if (mode == DImode)
	{
	  cost = 7;
	  nonreg_cost = 8;
	}
      else
	{
	  cost = 2;
	  nonreg_cost = 4;
	}


      *total = cost + (REG_OR_SUBREG_REG (XEXP (x, 0)) ? 0 : nonreg_cost)
		    + (REG_OR_SUBREG_REG (XEXP (x, 1)) ? 0 : nonreg_cost);
      return true;

    default:
      *total = arm_rtx_costs_1 (x, code, outer_code);
      return true;
    }
}
/* All address computations that can be done are free, but rtx cost returns
   the same for practically all of them.  So we weight the different types
   of address here in the order (most pref first):
   PRE/POST_INC/DEC, SHIFT or NON-INT sum, INT sum, REG, MEM or LABEL.  */
static inline int
arm_arm_address_cost (rtx x)
{
  enum rtx_code c  = GET_CODE (x);

  if (c == PRE_INC || c == PRE_DEC || c == POST_INC || c == POST_DEC)
    return 0;
  if (c == MEM || c == LABEL_REF || c == SYMBOL_REF)
    return 10;

  if (c == PLUS || c == MINUS)
    {
      if (GET_CODE (XEXP (x, 0)) == CONST_INT)
	return 2;

      if (ARITHMETIC_P (XEXP (x, 0)) || ARITHMETIC_P (XEXP (x, 1)))
	return 3;

      return 4;
    }

  return 6;
}

static inline int
arm_thumb_address_cost (rtx x)
{
  enum rtx_code c  = GET_CODE (x);

  if (c == REG)
    return 1;
  if (c == PLUS
      && GET_CODE (XEXP (x, 0)) == REG
      && GET_CODE (XEXP (x, 1)) == CONST_INT)
    return 1;

  return 2;
}

static int
arm_address_cost (rtx x)
{
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  return TARGET_32BIT ? arm_arm_address_cost (x) : arm_thumb_address_cost (x);
}

static int
arm_adjust_cost (rtx insn, rtx link, rtx dep, int cost)
{
  rtx i_pat, d_pat;

  /* Some true dependencies can have a higher cost depending
     on precisely how certain input operands are used.  */
  if (arm_tune_xscale
      && REG_NOTE_KIND (link) == 0
      && recog_memoized (insn) >= 0
      && recog_memoized (dep) >= 0)
    {
      int shift_opnum = get_attr_shift (insn);
      enum attr_type attr_type = get_attr_type (dep);

      /* If nonzero, SHIFT_OPNUM contains the operand number of a shifted
	 operand for INSN.  If we have a shifted input operand and the
	 instruction we depend on is another ALU instruction, then we may
	 have to account for an additional stall.  */
      if (shift_opnum != 0
	  && (attr_type == TYPE_ALU_SHIFT || attr_type == TYPE_ALU_SHIFT_REG))
	{
	  rtx shifted_operand;
	  int opno;

	  /* Get the shifted operand.  */
	  extract_insn (insn);
	  shifted_operand = recog_data.operand[shift_opnum];

	  /* Iterate over all the operands in DEP.  If we write an operand
	     that overlaps with SHIFTED_OPERAND, then we have increase the
	     cost of this dependency.  */
	  extract_insn (dep);
	  preprocess_constraints ();
	  for (opno = 0; opno < recog_data.n_operands; opno++)
	    {
	      /* We can ignore strict inputs.  */
	      if (recog_data.operand_type[opno] == OP_IN)
		continue;

	      if (reg_overlap_mentioned_p (recog_data.operand[opno],
					   shifted_operand))
		return 2;
	    }
	}
    }

  /* XXX This is not strictly true for the FPA.  */
  if (REG_NOTE_KIND (link) == REG_DEP_ANTI
      || REG_NOTE_KIND (link) == REG_DEP_OUTPUT)
    return 0;

  /* Call insns don't incur a stall, even if they follow a load.  */
  if (REG_NOTE_KIND (link) == 0
      && GET_CODE (insn) == CALL_INSN)
    return 1;

  if ((i_pat = single_set (insn)) != NULL
      && GET_CODE (SET_SRC (i_pat)) == MEM
      && (d_pat = single_set (dep)) != NULL
      && GET_CODE (SET_DEST (d_pat)) == MEM)
    {
      rtx src_mem = XEXP (SET_SRC (i_pat), 0);
      /* This is a load after a store, there is no conflict if the load reads
	 from a cached area.  Assume that loads from the stack, and from the
	 constant pool are cached, and that others will miss.  This is a
	 hack.  */

      if ((GET_CODE (src_mem) == SYMBOL_REF && CONSTANT_POOL_ADDRESS_P (src_mem))
	  || reg_mentioned_p (stack_pointer_rtx, src_mem)
	  || reg_mentioned_p (frame_pointer_rtx, src_mem)
	  || reg_mentioned_p (hard_frame_pointer_rtx, src_mem))
	return 1;
    }

  return cost;
}

static int fp_consts_inited = 0;

/* Only zero is valid for VFP.  Other values are also valid for FPA.  */
static const char * const strings_fp[8] =
{
  "0",   "1",   "2",   "3",
  "4",   "5",   "0.5", "10"
};

static REAL_VALUE_TYPE values_fp[8];

static void
init_fp_table (void)
{
  int i;
  REAL_VALUE_TYPE r;

  if (TARGET_VFP)
    fp_consts_inited = 1;
  else
    fp_consts_inited = 8;

  for (i = 0; i < fp_consts_inited; i++)
    {
      r = REAL_VALUE_ATOF (strings_fp[i], DFmode);
      values_fp[i] = r;
    }
}

/* Return TRUE if rtx X is a valid immediate FP constant.  */
int
arm_const_double_rtx (rtx x)
{
  REAL_VALUE_TYPE r;
  int i;

  if (!fp_consts_inited)
    init_fp_table ();

  REAL_VALUE_FROM_CONST_DOUBLE (r, x);
  if (REAL_VALUE_MINUS_ZERO (r))
    return 0;

  for (i = 0; i < fp_consts_inited; i++)
    if (REAL_VALUES_EQUAL (r, values_fp[i]))
      return 1;

  return 0;
}

/* Return TRUE if rtx X is a valid immediate FPA constant.  */
int
neg_const_double_rtx_ok_for_fpa (rtx x)
{
  REAL_VALUE_TYPE r;
  int i;

  if (!fp_consts_inited)
    init_fp_table ();

  REAL_VALUE_FROM_CONST_DOUBLE (r, x);
  r = REAL_VALUE_NEGATE (r);
  if (REAL_VALUE_MINUS_ZERO (r))
    return 0;

  for (i = 0; i < 8; i++)
    if (REAL_VALUES_EQUAL (r, values_fp[i]))
      return 1;

  return 0;
}


/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* VFPv3 has a fairly wide range of representable immediates, formed from
   "quarter-precision" floating-point values. These can be evaluated using this
   formula (with ^ for exponentiation):

     -1^s * n * 2^-r

   Where 's' is a sign bit (0/1), 'n' and 'r' are integers such that
   16 <= n <= 31 and 0 <= r <= 7.

   These values are mapped onto an 8-bit integer ABCDEFGH s.t.

     - A (most-significant) is the sign bit.
     - BCD are the exponent (encoded as r XOR 3).
     - EFGH are the mantissa (encoded as n - 16).
*/

/* Return an integer index for a VFPv3 immediate operand X suitable for the
   fconst[sd] instruction, or -1 if X isn't suitable.  */
static int
vfp3_const_double_index (rtx x)
{
  REAL_VALUE_TYPE r, m;
  int sign, exponent;
  unsigned HOST_WIDE_INT mantissa, mant_hi;
  unsigned HOST_WIDE_INT mask;
  HOST_WIDE_INT m1, m2;
  int point_pos = 2 * HOST_BITS_PER_WIDE_INT - 1;

  if (!TARGET_VFP3 || GET_CODE (x) != CONST_DOUBLE)
    return -1;

  REAL_VALUE_FROM_CONST_DOUBLE (r, x);

  /* We can't represent these things, so detect them first.  */
  if (REAL_VALUE_ISINF (r) || REAL_VALUE_ISNAN (r) || REAL_VALUE_MINUS_ZERO (r))
    return -1;

  /* Extract sign, exponent and mantissa.  */
  sign = REAL_VALUE_NEGATIVE (r) ? 1 : 0;
  r = REAL_VALUE_ABS (r);
  exponent = REAL_EXP (&r);
  /* For the mantissa, we expand into two HOST_WIDE_INTS, apart from the
     highest (sign) bit, with a fixed binary point at bit point_pos.
     WARNING: If there's ever a VFP version which uses more than 2 * H_W_I - 1
     bits for the mantissa, this may fail (low bits would be lost).  */
  real_ldexp (&m, &r, point_pos - exponent);
  REAL_VALUE_TO_INT (&m1, &m2, m);
  mantissa = m1;
  mant_hi = m2;

  /* If there are bits set in the low part of the mantissa, we can't
     represent this value.  */
  if (mantissa != 0)
    return -1;

  /* Now make it so that mantissa contains the most-significant bits, and move
     the point_pos to indicate that the least-significant bits have been
     discarded.  */
  point_pos -= HOST_BITS_PER_WIDE_INT;
  mantissa = mant_hi;

  /* We can permit four significant bits of mantissa only, plus a high bit
     which is always 1.  */
  mask = ((unsigned HOST_WIDE_INT)1 << (point_pos - 5)) - 1;
  if ((mantissa & mask) != 0)
    return -1;

  /* Now we know the mantissa is in range, chop off the unneeded bits.  */
  mantissa >>= point_pos - 5;

  /* The mantissa may be zero. Disallow that case. (It's possible to load the
     floating-point immediate zero with Neon using an integer-zero load, but
     that case is handled elsewhere.)  */
  if (mantissa == 0)
    return -1;

  gcc_assert (mantissa >= 16 && mantissa <= 31);

  /* The value of 5 here would be 4 if GCC used IEEE754-like encoding (where
     normalised significands are in the range [1, 2). (Our mantissa is shifted
     left 4 places at this point relative to normalised IEEE754 values).  GCC
     internally uses [0.5, 1) (see real.c), so the exponent returned from
     REAL_EXP must be altered.  */
  exponent = 5 - exponent;

  if (exponent < 0 || exponent > 7)
    return -1;

  /* Sign, mantissa and exponent are now in the correct form to plug into the
     formulae described in the comment above.  */
  return (sign << 7) | ((exponent ^ 3) << 4) | (mantissa - 16);
}

/* Return TRUE if rtx X is a valid immediate VFPv3 constant.  */
int
vfp3_const_double_rtx (rtx x)
{
  if (!TARGET_VFP3)
    return 0;

  return vfp3_const_double_index (x) != -1;
}

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
/* Recognize immediates which can be used in various Neon instructions. Legal
   immediates are described by the following table (for VMVN variants, the
   bitwise inverse of the constant shown is recognized. In either case, VMOV
   is output and the correct instruction to use for a given constant is chosen
   by the assembler). The constant shown is replicated across all elements of
   the destination vector.
   
   insn elems variant constant (binary)
   ---- ----- ------- -----------------
   vmov  i32     0    00000000 00000000 00000000 abcdefgh
   vmov  i32     1    00000000 00000000 abcdefgh 00000000
   vmov  i32     2    00000000 abcdefgh 00000000 00000000
   vmov  i32     3    abcdefgh 00000000 00000000 00000000
   vmov  i16     4    00000000 abcdefgh
   vmov  i16     5    abcdefgh 00000000
   vmvn  i32     6    00000000 00000000 00000000 abcdefgh
   vmvn  i32     7    00000000 00000000 abcdefgh 00000000
   vmvn  i32     8    00000000 abcdefgh 00000000 00000000
   vmvn  i32     9    abcdefgh 00000000 00000000 00000000
   vmvn  i16    10    00000000 abcdefgh
   vmvn  i16    11    abcdefgh 00000000
   vmov  i32    12    00000000 00000000 abcdefgh 11111111
   vmvn  i32    13    00000000 00000000 abcdefgh 11111111
   vmov  i32    14    00000000 abcdefgh 11111111 11111111
   vmvn  i32    15    00000000 abcdefgh 11111111 11111111
   vmov   i8    16    abcdefgh
   vmov  i64    17    aaaaaaaa bbbbbbbb cccccccc dddddddd
                      eeeeeeee ffffffff gggggggg hhhhhhhh
   vmov  f32    18    aBbbbbbc defgh000 00000000 00000000

   For case 18, B = !b. Representable values are exactly those accepted by
   vfp3_const_double_index, but are output as floating-point numbers rather
   than indices.
   
   Variants 0-5 (inclusive) may also be used as immediates for the second
   operand of VORR/VBIC instructions.
   
   The INVERSE argument causes the bitwise inverse of the given operand to be
   recognized instead (used for recognizing legal immediates for the VAND/VORN
   pseudo-instructions). If INVERSE is true, the value placed in *MODCONST is
   *not* inverted (i.e. the pseudo-instruction forms vand/vorn should still be
   output, rather than the real insns vbic/vorr).
   
   INVERSE makes no difference to the recognition of float vectors.
   
   The return value is the variant of immediate as shown in the above table, or
   -1 if the given value doesn't match any of the listed patterns.
*/
static int
neon_valid_immediate (rtx op, enum machine_mode mode, int inverse,
		      rtx *modconst, int *elementwidth)
{
#define CHECK(STRIDE, ELSIZE, CLASS, TEST)	\
  matches = 1;					\
  for (i = 0; i < idx; i += (STRIDE))		\
    if (!(TEST))				\
      matches = 0;				\
  if (matches)					\
    {						\
      immtype = (CLASS);			\
      elsize = (ELSIZE);			\
      break;					\
    }

  unsigned int i, elsize, idx = 0, n_elts = CONST_VECTOR_NUNITS (op);
  unsigned int innersize = GET_MODE_SIZE (GET_MODE_INNER (mode));
  unsigned char bytes[16];
  int immtype = -1, matches;
  unsigned int invmask = inverse ? 0xff : 0;
  
  /* Vectors of float constants.  */
  if (GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT)
    {
      rtx el0 = CONST_VECTOR_ELT (op, 0);
      REAL_VALUE_TYPE r0;

      if (!vfp3_const_double_rtx (el0))
        return -1;

      REAL_VALUE_FROM_CONST_DOUBLE (r0, el0);

      for (i = 1; i < n_elts; i++)
        {
          rtx elt = CONST_VECTOR_ELT (op, i);
          REAL_VALUE_TYPE re;
          
          REAL_VALUE_FROM_CONST_DOUBLE (re, elt);

          if (!REAL_VALUES_EQUAL (r0, re))
            return -1;
        }

      if (modconst)
        *modconst = CONST_VECTOR_ELT (op, 0);
      
      if (elementwidth)
        *elementwidth = 0;
      
      return 18;
    }
  
  /* Splat vector constant out into a byte vector.  */
  for (i = 0; i < n_elts; i++)
    {
      rtx el = CONST_VECTOR_ELT (op, i);
      unsigned HOST_WIDE_INT elpart;
      unsigned int part, parts;

      if (GET_CODE (el) == CONST_INT)
        {
          elpart = INTVAL (el);
          parts = 1;
        }
      else if (GET_CODE (el) == CONST_DOUBLE)
        {
          elpart = CONST_DOUBLE_LOW (el);
          parts = 2;
        }
      else
        gcc_unreachable ();
      
      for (part = 0; part < parts; part++)
        {
          unsigned int byte;
          for (byte = 0; byte < innersize; byte++)
            {
              bytes[idx++] = (elpart & 0xff) ^ invmask;
              elpart >>= BITS_PER_UNIT;
            }
          if (GET_CODE (el) == CONST_DOUBLE)
            elpart = CONST_DOUBLE_HIGH (el);
        }
    }
  
  /* Sanity check.  */
  gcc_assert (idx == GET_MODE_SIZE (mode));
  
  do
    {
      CHECK (4, 32, 0, bytes[i] == bytes[0] && bytes[i + 1] == 0
		       && bytes[i + 2] == 0 && bytes[i + 3] == 0);

      CHECK (4, 32, 1, bytes[i] == 0 && bytes[i + 1] == bytes[1]
		       && bytes[i + 2] == 0 && bytes[i + 3] == 0);

      CHECK (4, 32, 2, bytes[i] == 0 && bytes[i + 1] == 0
		       && bytes[i + 2] == bytes[2] && bytes[i + 3] == 0);

      CHECK (4, 32, 3, bytes[i] == 0 && bytes[i + 1] == 0
		       && bytes[i + 2] == 0 && bytes[i + 3] == bytes[3]);

      CHECK (2, 16, 4, bytes[i] == bytes[0] && bytes[i + 1] == 0);

      CHECK (2, 16, 5, bytes[i] == 0 && bytes[i + 1] == bytes[1]);

      CHECK (4, 32, 6, bytes[i] == bytes[0] && bytes[i + 1] == 0xff
		       && bytes[i + 2] == 0xff && bytes[i + 3] == 0xff);

      CHECK (4, 32, 7, bytes[i] == 0xff && bytes[i + 1] == bytes[1]
		       && bytes[i + 2] == 0xff && bytes[i + 3] == 0xff);
                   
      CHECK (4, 32, 8, bytes[i] == 0xff && bytes[i + 1] == 0xff
		       && bytes[i + 2] == bytes[2] && bytes[i + 3] == 0xff);

      CHECK (4, 32, 9, bytes[i] == 0xff && bytes[i + 1] == 0xff
		       && bytes[i + 2] == 0xff && bytes[i + 3] == bytes[3]);
      
      CHECK (2, 16, 10, bytes[i] == bytes[0] && bytes[i + 1] == 0xff);

      CHECK (2, 16, 11, bytes[i] == 0xff && bytes[i + 1] == bytes[1]);
                    
      CHECK (4, 32, 12, bytes[i] == 0xff && bytes[i + 1] == bytes[1]
			&& bytes[i + 2] == 0 && bytes[i + 3] == 0);

      CHECK (4, 32, 13, bytes[i] == 0 && bytes[i + 1] == bytes[1]
			&& bytes[i + 2] == 0xff && bytes[i + 3] == 0xff);
      
      CHECK (4, 32, 14, bytes[i] == 0xff && bytes[i + 1] == 0xff
			&& bytes[i + 2] == bytes[2] && bytes[i + 3] == 0);
                    
      CHECK (4, 32, 15, bytes[i] == 0 && bytes[i + 1] == 0
			&& bytes[i + 2] == bytes[2] && bytes[i + 3] == 0xff);
                    
      CHECK (1, 8, 16, bytes[i] == bytes[0]);

      CHECK (1, 64, 17, (bytes[i] == 0 || bytes[i] == 0xff)
			&& bytes[i] == bytes[(i + 8) % idx]);
    }
  while (0);

  if (immtype == -1)
    return -1;

  if (elementwidth)
    *elementwidth = elsize;
  
  if (modconst)
    {
      unsigned HOST_WIDE_INT imm = 0;

      /* Un-invert bytes of recognized vector, if neccessary.  */
      if (invmask != 0)
        for (i = 0; i < idx; i++)
          bytes[i] ^= invmask;

      if (immtype == 17)
        {
          /* FIXME: Broken on 32-bit H_W_I hosts.  */
          gcc_assert (sizeof (HOST_WIDE_INT) == 8);
          
          for (i = 0; i < 8; i++)
            imm |= (unsigned HOST_WIDE_INT) (bytes[i] ? 0xff : 0)
                   << (i * BITS_PER_UNIT);

          *modconst = GEN_INT (imm);
        }
      else
        {
          unsigned HOST_WIDE_INT imm = 0;

          for (i = 0; i < elsize / BITS_PER_UNIT; i++)
            imm |= (unsigned HOST_WIDE_INT) bytes[i] << (i * BITS_PER_UNIT);

          *modconst = GEN_INT (imm);
        }
    }
  
  return immtype;
#undef CHECK
}

/* Return TRUE if rtx X is legal for use as either a Neon VMOV (or, implicitly,
   VMVN) immediate. Write back width per element to *ELEMENTWIDTH (or zero for
   float elements), and a modified constant (whatever should be output for a
   VMOV) in *MODCONST.  */

int
neon_immediate_valid_for_move (rtx op, enum machine_mode mode,
			       rtx *modconst, int *elementwidth)
{
  rtx tmpconst;
  int tmpwidth;
  int retval = neon_valid_immediate (op, mode, 0, &tmpconst, &tmpwidth);
  
  if (retval == -1)
    return 0;
  
  if (modconst)
    *modconst = tmpconst;
  
  if (elementwidth)
    *elementwidth = tmpwidth;
  
  return 1;
}

/* Return TRUE if rtx X is legal for use in a VORR or VBIC instruction.  If
   the immediate is valid, write a constant suitable for using as an operand
   to VORR/VBIC/VAND/VORN to *MODCONST and the corresponding element width to
   *ELEMENTWIDTH. See neon_valid_immediate for description of INVERSE.  */

int
neon_immediate_valid_for_logic (rtx op, enum machine_mode mode, int inverse,
				rtx *modconst, int *elementwidth)
{
  rtx tmpconst;
  int tmpwidth;
  int retval = neon_valid_immediate (op, mode, inverse, &tmpconst, &tmpwidth);

  if (retval < 0 || retval > 5)
    return 0;
  
  if (modconst)
    *modconst = tmpconst;
  
  if (elementwidth)
    *elementwidth = tmpwidth;
  
  return 1;
}

/* Return a string suitable for output of Neon immediate logic operation
   MNEM.  */

char *
neon_output_logic_immediate (const char *mnem, rtx *op2, enum machine_mode mode,
			     int inverse, int quad)
{
  int width, is_valid;
  static char templ[40];
  
  is_valid = neon_immediate_valid_for_logic (*op2, mode, inverse, op2, &width);
  
  gcc_assert (is_valid != 0);
  
  if (quad)
    sprintf (templ, "%s.i%d\t%%q0, %%2", mnem, width);
  else
    sprintf (templ, "%s.i%d\t%%P0, %%2", mnem, width);
  
  return templ;
}

/* Output a sequence of pairwise operations to implement a reduction.
   NOTE: We do "too much work" here, because pairwise operations work on two
   registers-worth of operands in one go. Unfortunately we can't exploit those
   extra calculations to do the full operation in fewer steps, I don't think.
   Although all vector elements of the result but the first are ignored, we
   actually calculate the same result in each of the elements. An alternative
   such as initially loading a vector with zero to use as each of the second
   operands would use up an additional register and take an extra instruction,
   for no particular gain.  */

void
neon_pairwise_reduce (rtx op0, rtx op1, enum machine_mode mode,
		      rtx (*reduc) (rtx, rtx, rtx))
{
  enum machine_mode inner = GET_MODE_INNER (mode);
  unsigned int i, parts = GET_MODE_SIZE (mode) / GET_MODE_SIZE (inner);
  rtx tmpsum = op1;
  
  for (i = parts / 2; i >= 1; i /= 2)
    {
      rtx dest = (i == 1) ? op0 : gen_reg_rtx (mode);
      emit_insn (reduc (dest, tmpsum, tmpsum));
      tmpsum = dest;
    }
}

/* Initialise a vector with non-constant elements.  FIXME: We can do better
   than the current implementation (building a vector on the stack and then
   loading it) in many cases.  See rs6000.c.  */

void
neon_expand_vector_init (rtx target, rtx vals)
{
  enum machine_mode mode = GET_MODE (target);
  enum machine_mode inner = GET_MODE_INNER (mode);
  unsigned int i, n_elts = GET_MODE_NUNITS (mode);
  rtx mem;

  gcc_assert (VECTOR_MODE_P (mode));

  mem = assign_stack_temp (mode, GET_MODE_SIZE (mode), 0);
  for (i = 0; i < n_elts; i++)
    emit_move_insn (adjust_address_nv (mem, inner, i * GET_MODE_SIZE (inner)),
                   XVECEXP (vals, 0, i));

  emit_move_insn (target, mem);
}

/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

/* Predicates for `match_operand' and `match_operator'.  */

/* Return nonzero if OP is a valid Cirrus memory address pattern.  */
int
cirrus_memory_offset (rtx op)
{
  /* Reject eliminable registers.  */
  if (! (reload_in_progress || reload_completed)
      && (   reg_mentioned_p (frame_pointer_rtx, op)
	  || reg_mentioned_p (arg_pointer_rtx, op)
	  || reg_mentioned_p (virtual_incoming_args_rtx, op)
	  || reg_mentioned_p (virtual_outgoing_args_rtx, op)
	  || reg_mentioned_p (virtual_stack_dynamic_rtx, op)
	  || reg_mentioned_p (virtual_stack_vars_rtx, op)))
    return 0;

  if (GET_CODE (op) == MEM)
    {
      rtx ind;

      ind = XEXP (op, 0);

      /* Match: (mem (reg)).  */
      if (GET_CODE (ind) == REG)
	return 1;

      /* Match:
	 (mem (plus (reg)
	            (const))).  */
      if (GET_CODE (ind) == PLUS
	  && GET_CODE (XEXP (ind, 0)) == REG
	  && REG_MODE_OK_FOR_BASE_P (XEXP (ind, 0), VOIDmode)
	  && GET_CODE (XEXP (ind, 1)) == CONST_INT)
	return 1;
    }

  return 0;
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Return TRUE if OP is a valid coprocessor memory address pattern.
   WB is true if full writeback address modes are allowed and is false
   if limited writeback address modes (POST_INC and PRE_DEC) are
   allowed.  */
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

int
arm_coproc_mem_operand (rtx op, bool wb)
{
  rtx ind;

  /* Reject eliminable registers.  */
  if (! (reload_in_progress || reload_completed)
      && (   reg_mentioned_p (frame_pointer_rtx, op)
	  || reg_mentioned_p (arg_pointer_rtx, op)
	  || reg_mentioned_p (virtual_incoming_args_rtx, op)
	  || reg_mentioned_p (virtual_outgoing_args_rtx, op)
	  || reg_mentioned_p (virtual_stack_dynamic_rtx, op)
	  || reg_mentioned_p (virtual_stack_vars_rtx, op)))
    return FALSE;

  /* Constants are converted into offsets from labels.  */
  if (GET_CODE (op) != MEM)
    return FALSE;

  ind = XEXP (op, 0);

  if (reload_completed
      && (GET_CODE (ind) == LABEL_REF
	  || (GET_CODE (ind) == CONST
	      && GET_CODE (XEXP (ind, 0)) == PLUS
	      && GET_CODE (XEXP (XEXP (ind, 0), 0)) == LABEL_REF
	      && GET_CODE (XEXP (XEXP (ind, 0), 1)) == CONST_INT)))
    return TRUE;

  /* Match: (mem (reg)).  */
  if (GET_CODE (ind) == REG)
    return arm_address_register_rtx_p (ind, 0);

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* Autoincremment addressing modes.  POST_INC and PRE_DEC are
     acceptable in any case (subject to verification by
     arm_address_register_rtx_p).  We need WB to be true to accept
     PRE_INC and POST_DEC.  */
  if (GET_CODE (ind) == POST_INC
      || GET_CODE (ind) == PRE_DEC
      || (wb
	  && (GET_CODE (ind) == PRE_INC
	      || GET_CODE (ind) == POST_DEC)))
    return arm_address_register_rtx_p (XEXP (ind, 0), 0);
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

  if (wb
      && (GET_CODE (ind) == POST_MODIFY || GET_CODE (ind) == PRE_MODIFY)
      && arm_address_register_rtx_p (XEXP (ind, 0), 0)
      && GET_CODE (XEXP (ind, 1)) == PLUS
      && rtx_equal_p (XEXP (XEXP (ind, 1), 0), XEXP (ind, 0)))
    ind = XEXP (ind, 1);

  /* Match:
     (plus (reg)
	   (const)).  */
  if (GET_CODE (ind) == PLUS
      && GET_CODE (XEXP (ind, 0)) == REG
      && REG_MODE_OK_FOR_BASE_P (XEXP (ind, 0), VOIDmode)
      && GET_CODE (XEXP (ind, 1)) == CONST_INT
      && INTVAL (XEXP (ind, 1)) > -1024
      && INTVAL (XEXP (ind, 1)) <  1024
      && (INTVAL (XEXP (ind, 1)) & 3) == 0)
    return TRUE;

  return FALSE;
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
/* Return TRUE if OP is a memory operand which we can load or store a vector
   to/from. If CORE is true, we're moving from ARM registers not Neon
   registers.  */
int
neon_vector_mem_operand (rtx op, bool core)
{
  rtx ind;

  /* Reject eliminable registers.  */
  if (! (reload_in_progress || reload_completed)
      && (   reg_mentioned_p (frame_pointer_rtx, op)
	  || reg_mentioned_p (arg_pointer_rtx, op)
	  || reg_mentioned_p (virtual_incoming_args_rtx, op)
	  || reg_mentioned_p (virtual_outgoing_args_rtx, op)
	  || reg_mentioned_p (virtual_stack_dynamic_rtx, op)
	  || reg_mentioned_p (virtual_stack_vars_rtx, op)))
    return FALSE;

  /* Constants are converted into offsets from labels.  */
  if (GET_CODE (op) != MEM)
    return FALSE;

  ind = XEXP (op, 0);

  if (reload_completed
      && (GET_CODE (ind) == LABEL_REF
	  || (GET_CODE (ind) == CONST
	      && GET_CODE (XEXP (ind, 0)) == PLUS
	      && GET_CODE (XEXP (XEXP (ind, 0), 0)) == LABEL_REF
	      && GET_CODE (XEXP (XEXP (ind, 0), 1)) == CONST_INT)))
    return TRUE;

  /* Match: (mem (reg)).  */
  if (GET_CODE (ind) == REG)
    return arm_address_register_rtx_p (ind, 0);

  /* Allow post-increment with Neon registers.  */
  if (!core && GET_CODE (ind) == POST_INC)
    return arm_address_register_rtx_p (XEXP (ind, 0), 0);

#if 0
  /* FIXME: We can support this too if we use VLD1/VST1.  */
  if (!core
      && GET_CODE (ind) == POST_MODIFY
      && arm_address_register_rtx_p (XEXP (ind, 0), 0)
      && GET_CODE (XEXP (ind, 1)) == PLUS
      && rtx_equal_p (XEXP (XEXP (ind, 1), 0), XEXP (ind, 0)))
    ind = XEXP (ind, 1);
#endif

  /* Match:
     (plus (reg)
          (const)).  */
  if (!core
      && GET_CODE (ind) == PLUS
      && GET_CODE (XEXP (ind, 0)) == REG
      && REG_MODE_OK_FOR_BASE_P (XEXP (ind, 0), VOIDmode)
      /* ALQAAHIRA LOCAL begin 6160917 */
      /* Make call consistent with the ones used in neon_reload_{in,out} */
      && arm_legitimate_index_p (GET_MODE (op), XEXP (ind, 1), SET, 0))
      /* ALQAAHIRA LOCAL end 6160917 */
    return TRUE;

  return FALSE;
}

/* Return TRUE if OP is a mem suitable for loading/storing a Neon struct
   type.  */
int
neon_struct_mem_operand (rtx op)
{
  rtx ind;

  /* Reject eliminable registers.  */
  if (! (reload_in_progress || reload_completed)
      && (   reg_mentioned_p (frame_pointer_rtx, op)
	  || reg_mentioned_p (arg_pointer_rtx, op)
	  || reg_mentioned_p (virtual_incoming_args_rtx, op)
	  || reg_mentioned_p (virtual_outgoing_args_rtx, op)
	  || reg_mentioned_p (virtual_stack_dynamic_rtx, op)
	  || reg_mentioned_p (virtual_stack_vars_rtx, op)))
    return FALSE;

  /* Constants are converted into offsets from labels.  */
  if (GET_CODE (op) != MEM)
    return FALSE;

  ind = XEXP (op, 0);

  if (reload_completed
      && (GET_CODE (ind) == LABEL_REF
	  || (GET_CODE (ind) == CONST
	      && GET_CODE (XEXP (ind, 0)) == PLUS
	      && GET_CODE (XEXP (XEXP (ind, 0), 0)) == LABEL_REF
	      && GET_CODE (XEXP (XEXP (ind, 0), 1)) == CONST_INT)))
    return TRUE;

  /* Match: (mem (reg)).  */
  if (GET_CODE (ind) == REG)
    return arm_address_register_rtx_p (ind, 0);

  return FALSE;
}

/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
/* Return true if X is a register that will be eliminated later on.  */
int
arm_eliminable_register (rtx x)
{
  return REG_P (x) && (REGNO (x) == FRAME_POINTER_REGNUM
		       || REGNO (x) == ARG_POINTER_REGNUM
		       || (REGNO (x) >= FIRST_VIRTUAL_REGISTER
			   && REGNO (x) <= LAST_VIRTUAL_REGISTER));
}

/* Return GENERAL_REGS if a scratch register required to reload x to/from
   coprocessor registers.  Otherwise return NO_REGS.  */

enum reg_class
coproc_secondary_reload_class (enum machine_mode mode, rtx x, bool wb)
{
  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  if (TARGET_NEON
      && (GET_MODE_CLASS (mode) == MODE_VECTOR_INT
          || GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT)
      && neon_vector_mem_operand (x, FALSE))
     return NO_REGS;

  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  if (arm_coproc_mem_operand (x, wb) || s_register_operand (x, mode))
    return NO_REGS;

  return GENERAL_REGS;
}

/* Values which must be returned in the most-significant end of the return
   register.  */

static bool
arm_return_in_msb (tree valtype)
{
  return (TARGET_AAPCS_BASED
          && BYTES_BIG_ENDIAN
          && (AGGREGATE_TYPE_P (valtype)
              || TREE_CODE (valtype) == COMPLEX_TYPE));
}

/* Returns TRUE if INSN is an "LDR REG, ADDR" instruction.
   Use by the Cirrus Maverick code which has to workaround
   a hardware bug triggered by such instructions.  */
static bool
arm_memory_load_p (rtx insn)
{
  rtx body, lhs, rhs;;

  if (insn == NULL_RTX || GET_CODE (insn) != INSN)
    return false;

  body = PATTERN (insn);

  if (GET_CODE (body) != SET)
    return false;

  lhs = XEXP (body, 0);
  rhs = XEXP (body, 1);

  lhs = REG_OR_SUBREG_RTX (lhs);

  /* If the destination is not a general purpose
     register we do not have to worry.  */
  if (GET_CODE (lhs) != REG
      || REGNO_REG_CLASS (REGNO (lhs)) != GENERAL_REGS)
    return false;

  /* As well as loads from memory we also have to react
     to loads of invalid constants which will be turned
     into loads from the minipool.  */
  return (GET_CODE (rhs) == MEM
	  || GET_CODE (rhs) == SYMBOL_REF
	  || note_invalid_constants (insn, -1, false));
}

/* Return TRUE if INSN is a Cirrus instruction.  */
static bool
arm_cirrus_insn_p (rtx insn)
{
  enum attr_cirrus attr;

  /* get_attr cannot accept USE or CLOBBER.  */
  if (!insn
      || GET_CODE (insn) != INSN
      || GET_CODE (PATTERN (insn)) == USE
      || GET_CODE (PATTERN (insn)) == CLOBBER)
    return 0;

  attr = get_attr_cirrus (insn);

  return attr != CIRRUS_NOT;
}

/* Cirrus reorg for invalid instruction combinations.  */
static void
cirrus_reorg (rtx first)
{
  enum attr_cirrus attr;
  rtx body = PATTERN (first);
  rtx t;
  int nops;

  /* Any branch must be followed by 2 non Cirrus instructions.  */
  if (GET_CODE (first) == JUMP_INSN && GET_CODE (body) != RETURN)
    {
      nops = 0;
      t = next_nonnote_insn (first);

      if (arm_cirrus_insn_p (t))
	++ nops;

      if (arm_cirrus_insn_p (next_nonnote_insn (t)))
	++ nops;

      while (nops --)
	emit_insn_after (gen_nop (), first);

      return;
    }

  /* (float (blah)) is in parallel with a clobber.  */
  if (GET_CODE (body) == PARALLEL && XVECLEN (body, 0) > 0)
    body = XVECEXP (body, 0, 0);

  if (GET_CODE (body) == SET)
    {
      rtx lhs = XEXP (body, 0), rhs = XEXP (body, 1);

      /* cfldrd, cfldr64, cfstrd, cfstr64 must
	 be followed by a non Cirrus insn.  */
      if (get_attr_cirrus (first) == CIRRUS_DOUBLE)
	{
	  if (arm_cirrus_insn_p (next_nonnote_insn (first)))
	    emit_insn_after (gen_nop (), first);

	  return;
	}
      else if (arm_memory_load_p (first))
	{
	  unsigned int arm_regno;

	  /* Any ldr/cfmvdlr, ldr/cfmvdhr, ldr/cfmvsr, ldr/cfmv64lr,
	     ldr/cfmv64hr combination where the Rd field is the same
	     in both instructions must be split with a non Cirrus
	     insn.  Example:

	     ldr r0, blah
	     nop
	     cfmvsr mvf0, r0.  */

	  /* Get Arm register number for ldr insn.  */
	  if (GET_CODE (lhs) == REG)
	    arm_regno = REGNO (lhs);
	  else
	    {
	      gcc_assert (GET_CODE (rhs) == REG);
	      arm_regno = REGNO (rhs);
	    }

	  /* Next insn.  */
	  first = next_nonnote_insn (first);

	  if (! arm_cirrus_insn_p (first))
	    return;

	  body = PATTERN (first);

          /* (float (blah)) is in parallel with a clobber.  */
          if (GET_CODE (body) == PARALLEL && XVECLEN (body, 0))
	    body = XVECEXP (body, 0, 0);

	  if (GET_CODE (body) == FLOAT)
	    body = XEXP (body, 0);

	  if (get_attr_cirrus (first) == CIRRUS_MOVE
	      && GET_CODE (XEXP (body, 1)) == REG
	      && arm_regno == REGNO (XEXP (body, 1)))
	    emit_insn_after (gen_nop (), first);

	  return;
	}
    }

  /* get_attr cannot accept USE or CLOBBER.  */
  if (!first
      || GET_CODE (first) != INSN
      || GET_CODE (PATTERN (first)) == USE
      || GET_CODE (PATTERN (first)) == CLOBBER)
    return;

  attr = get_attr_cirrus (first);

  /* Any coprocessor compare instruction (cfcmps, cfcmpd, ...)
     must be followed by a non-coprocessor instruction.  */
  if (attr == CIRRUS_COMPARE)
    {
      nops = 0;

      t = next_nonnote_insn (first);

      if (arm_cirrus_insn_p (t))
	++ nops;

      if (arm_cirrus_insn_p (next_nonnote_insn (t)))
	++ nops;

      while (nops --)
	emit_insn_after (gen_nop (), first);

      return;
    }
}

/* APPLE LOCAL begin ARM -mdynamic-no-pic support */
/* Return TRUE if X references a SYMBOL_REF.  */
int
symbol_mentioned_p (rtx x)
{
  return symbol_mentioned_with_filter (x, 0);
}

/* Return TRUE if X references a non-local SYMBOL_REF.  */
int
non_local_symbol_mentioned_p (rtx x)
{
  return symbol_mentioned_with_filter (x, 1);
}

/* Return TRUE if X references a SYMBOL_REF.  If filter_local is set,
   then references to local symbols (as per machopic_data_defined_p)
   are ignored.  */
static int
symbol_mentioned_with_filter (rtx x, int filter_local)
{
  const char * fmt;
  int i;

  if (GET_CODE (x) == SYMBOL_REF)
    {
#if TARGET_MACHO
      if (filter_local && machopic_data_defined_p (x))
	return 0;
      else
#endif
	return 1;
    }

  /* UNSPEC_TLS entries for a symbol include the SYMBOL_REF, but they
     are constant offsets, not symbols.  */
  if (GET_CODE (x) == UNSPEC && XINT (x, 1) == UNSPEC_TLS)
    return 0;

  fmt = GET_RTX_FORMAT (GET_CODE (x));

  for (i = GET_RTX_LENGTH (GET_CODE (x)) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'E')
	{
	  int j;

	  for (j = XVECLEN (x, i) - 1; j >= 0; j--)
	    if (symbol_mentioned_with_filter (XVECEXP (x, i, j),
					      filter_local))
	      return 1;
	}
      else if (fmt[i] == 'e'
	       && symbol_mentioned_with_filter (XEXP (x, i),
						filter_local))
	return 1;
    }

  return 0;
}
/* APPLE LOCAL end ARM -mdynmaic-no-pic support */

/* Return TRUE if X references a LABEL_REF.  */
int
label_mentioned_p (rtx x)
{
  const char * fmt;
  int i;

  if (GET_CODE (x) == LABEL_REF)
    return 1;

  /* UNSPEC_TLS entries for a symbol include a LABEL_REF for the referencing
     instruction, but they are constant offsets, not symbols.  */
  if (GET_CODE (x) == UNSPEC && XINT (x, 1) == UNSPEC_TLS)
    return 0;

  fmt = GET_RTX_FORMAT (GET_CODE (x));
  for (i = GET_RTX_LENGTH (GET_CODE (x)) - 1; i >= 0; i--)
    {
      if (fmt[i] == 'E')
	{
	  int j;

	  for (j = XVECLEN (x, i) - 1; j >= 0; j--)
	    if (label_mentioned_p (XVECEXP (x, i, j)))
	      return 1;
	}
      else if (fmt[i] == 'e' && label_mentioned_p (XEXP (x, i)))
	return 1;
    }

  return 0;
}

int
tls_mentioned_p (rtx x)
{
  switch (GET_CODE (x))
    {
    case CONST:
      return tls_mentioned_p (XEXP (x, 0));

    case UNSPEC:
      if (XINT (x, 1) == UNSPEC_TLS)
	return 1;

    default:
      return 0;
    }
}

/* Must not copy a SET whose source operand is PC-relative.  */

static bool
arm_cannot_copy_insn_p (rtx insn)
{
  rtx pat = PATTERN (insn);

  /* APPLE LOCAL begin ARM pic support */
  if (GET_CODE (pat) == SET)
    {
      rtx rhs = SET_SRC (pat);
      rtx lhs = SET_DEST (pat);

      if (GET_CODE (rhs) == UNSPEC
	  && XINT (rhs, 1) == UNSPEC_PIC_BASE)
	return TRUE;

      if (GET_CODE (rhs) == MEM
	  && GET_CODE (XEXP (rhs, 0)) == UNSPEC
	  && XINT (XEXP (rhs, 0), 1) == UNSPEC_PIC_BASE)
	return TRUE;

      if (GET_CODE (lhs) == MEM
	  && GET_CODE (XEXP (lhs, 0)) == UNSPEC
	  && XINT (XEXP (lhs, 0), 1) == UNSPEC_PIC_BASE)
	return TRUE;
    }
  /* APPLE LOCAL end ARM pic support */

  if (GET_CODE (pat) == PARALLEL
      && GET_CODE (XVECEXP (pat, 0, 0)) == SET)
    {
      rtx rhs = SET_SRC (XVECEXP (pat, 0, 0));

      if (GET_CODE (rhs) == UNSPEC
	  && XINT (rhs, 1) == UNSPEC_PIC_BASE)
	return TRUE;

      if (GET_CODE (rhs) == MEM
	  && GET_CODE (XEXP (rhs, 0)) == UNSPEC
	  && XINT (XEXP (rhs, 0), 1) == UNSPEC_PIC_BASE)
	return TRUE;
    }

  return FALSE;
}

enum rtx_code
minmax_code (rtx x)
{
  enum rtx_code code = GET_CODE (x);

  switch (code)
    {
    case SMAX:
      return GE;
    case SMIN:
      return LE;
    case UMIN:
      return LEU;
    case UMAX:
      return GEU;
    default:
      gcc_unreachable ();
    }
}

/* Return 1 if memory locations are adjacent.  */
int
adjacent_mem_locations (rtx a, rtx b)
{
  /* We don't guarantee to preserve the order of these memory refs.  */
  if (volatile_refs_p (a) || volatile_refs_p (b))
    return 0;

  if ((GET_CODE (XEXP (a, 0)) == REG
       || (GET_CODE (XEXP (a, 0)) == PLUS
	   && GET_CODE (XEXP (XEXP (a, 0), 1)) == CONST_INT))
      && (GET_CODE (XEXP (b, 0)) == REG
	  || (GET_CODE (XEXP (b, 0)) == PLUS
	      && GET_CODE (XEXP (XEXP (b, 0), 1)) == CONST_INT)))
    {
      HOST_WIDE_INT val0 = 0, val1 = 0;
      rtx reg0, reg1;
      int val_diff;

      if (GET_CODE (XEXP (a, 0)) == PLUS)
        {
	  reg0 = XEXP (XEXP (a, 0), 0);
	  val0 = INTVAL (XEXP (XEXP (a, 0), 1));
        }
      else
	reg0 = XEXP (a, 0);

      if (GET_CODE (XEXP (b, 0)) == PLUS)
        {
	  reg1 = XEXP (XEXP (b, 0), 0);
	  val1 = INTVAL (XEXP (XEXP (b, 0), 1));
        }
      else
	reg1 = XEXP (b, 0);

      /* Don't accept any offset that will require multiple
	 instructions to handle, since this would cause the
	 arith_adjacentmem pattern to output an overlong sequence.  */
      if (!const_ok_for_op (PLUS, val0) || !const_ok_for_op (PLUS, val1))
	return 0;

      /* Don't allow an eliminable register: register elimination can make
	 the offset too large.  */
      if (arm_eliminable_register (reg0))
	return 0;

      val_diff = val1 - val0;

      if (arm_ld_sched)
	{
	  /* If the target has load delay slots, then there's no benefit
	     to using an ldm instruction unless the offset is zero and
	     we are optimizing for size.  */
	  return (optimize_size && (REGNO (reg0) == REGNO (reg1))
		  && (val0 == 0 || val1 == 0 || val0 == 4 || val1 == 4)
		  && (val_diff == 4 || val_diff == -4));
	}

      return ((REGNO (reg0) == REGNO (reg1))
	      && (val_diff == 4 || val_diff == -4));
    }

  return 0;
}

int
load_multiple_sequence (rtx *operands, int nops, int *regs, int *base,
			HOST_WIDE_INT *load_offset)
{
  int unsorted_regs[4];
  HOST_WIDE_INT unsorted_offsets[4];
  int order[4];
  int base_reg = -1;
  int i;

  /* Can only handle 2, 3, or 4 insns at present,
     though could be easily extended if required.  */
  gcc_assert (nops >= 2 && nops <= 4);

  /* Loop over the operands and check that the memory references are
     suitable (i.e. immediate offsets from the same base register).  At
     the same time, extract the target register, and the memory
     offsets.  */
  for (i = 0; i < nops; i++)
    {
      rtx reg;
      rtx offset;

      /* Convert a subreg of a mem into the mem itself.  */
      if (GET_CODE (operands[nops + i]) == SUBREG)
	operands[nops + i] = alter_subreg (operands + (nops + i));

      gcc_assert (GET_CODE (operands[nops + i]) == MEM);

      /* Don't reorder volatile memory references; it doesn't seem worth
	 looking for the case where the order is ok anyway.  */
      if (MEM_VOLATILE_P (operands[nops + i]))
	return 0;

      offset = const0_rtx;

      if ((GET_CODE (reg = XEXP (operands[nops + i], 0)) == REG
	   || (GET_CODE (reg) == SUBREG
	       && GET_CODE (reg = SUBREG_REG (reg)) == REG))
	  || (GET_CODE (XEXP (operands[nops + i], 0)) == PLUS
	      && ((GET_CODE (reg = XEXP (XEXP (operands[nops + i], 0), 0))
		   == REG)
		  || (GET_CODE (reg) == SUBREG
		      && GET_CODE (reg = SUBREG_REG (reg)) == REG))
	      && (GET_CODE (offset = XEXP (XEXP (operands[nops + i], 0), 1))
		  == CONST_INT)))
	{
	  if (i == 0)
	    {
	      base_reg = REGNO (reg);
	      unsorted_regs[0] = (GET_CODE (operands[i]) == REG
				  ? REGNO (operands[i])
				  : REGNO (SUBREG_REG (operands[i])));
	      order[0] = 0;
	    }
	  else
	    {
	      if (base_reg != (int) REGNO (reg))
		/* Not addressed from the same base register.  */
		return 0;

	      unsorted_regs[i] = (GET_CODE (operands[i]) == REG
				  ? REGNO (operands[i])
				  : REGNO (SUBREG_REG (operands[i])));
	      if (unsorted_regs[i] < unsorted_regs[order[0]])
		order[0] = i;
	    }

	  /* If it isn't an integer register, or if it overwrites the
	     base register but isn't the last insn in the list, then
	     we can't do this.  */
	  if (unsorted_regs[i] < 0 || unsorted_regs[i] > 14
	      || (i != nops - 1 && unsorted_regs[i] == base_reg))
	    return 0;

	  unsorted_offsets[i] = INTVAL (offset);
	}
      else
	/* Not a suitable memory address.  */
	return 0;
    }

  /* All the useful information has now been extracted from the
     operands into unsorted_regs and unsorted_offsets; additionally,
     order[0] has been set to the lowest numbered register in the
     list.  Sort the registers into order, and check that the memory
     offsets are ascending and adjacent.  */

  for (i = 1; i < nops; i++)
    {
      int j;

      order[i] = order[i - 1];
      for (j = 0; j < nops; j++)
	if (unsorted_regs[j] > unsorted_regs[order[i - 1]]
	    && (order[i] == order[i - 1]
		|| unsorted_regs[j] < unsorted_regs[order[i]]))
	  order[i] = j;

      /* Have we found a suitable register? if not, one must be used more
	 than once.  */
      if (order[i] == order[i - 1])
	return 0;

      /* Is the memory address adjacent and ascending? */
      if (unsorted_offsets[order[i]] != unsorted_offsets[order[i - 1]] + 4)
	return 0;
    }

  if (base)
    {
      *base = base_reg;

      for (i = 0; i < nops; i++)
	regs[i] = unsorted_regs[order[i]];

      *load_offset = unsorted_offsets[order[0]];
    }

  if (unsorted_offsets[order[0]] == 0)
    return 1; /* ldmia */

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (TARGET_ARM && unsorted_offsets[order[0]] == 4)
    return 2; /* ldmib */

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (TARGET_ARM && unsorted_offsets[order[nops - 1]] == 0)
    return 3; /* ldmda */

  if (unsorted_offsets[order[nops - 1]] == -4)
    return 4; /* ldmdb */

  /* For ARM8,9 & StrongARM, 2 ldr instructions are faster than an ldm
     if the offset isn't small enough.  The reason 2 ldrs are faster
     is because these ARMs are able to do more than one cache access
     in a single cycle.  The ARM9 and StrongARM have Harvard caches,
     whilst the ARM8 has a double bandwidth cache.  This means that
     these cores can do both an instruction fetch and a data fetch in
     a single cycle, so the trick of calculating the address into a
     scratch register (one of the result regs) and then doing a load
     multiple actually becomes slower (and no smaller in code size).
     That is the transformation

 	ldr	rd1, [rbase + offset]
 	ldr	rd2, [rbase + offset + 4]

     to

 	add	rd1, rbase, offset
 	ldmia	rd1, {rd1, rd2}

     produces worse code -- '3 cycles + any stalls on rd2' instead of
     '2 cycles + any stalls on rd2'.  On ARMs with only one cache
     access per cycle, the first sequence could never complete in less
     than 6 cycles, whereas the ldm sequence would only take 5 and
     would make better use of sequential accesses if not hitting the
     cache.

     We cheat here and test 'arm_ld_sched' which we currently know to
     only be true for the ARM8, ARM9 and StrongARM.  If this ever
     changes, then the test below needs to be reworked.  */
  if (nops == 2 && arm_ld_sched)
    return 0;

  /* Can't do it without setting up the offset, only do this if it takes
     no more than one insn.  */
  return (const_ok_for_arm (unsorted_offsets[order[0]])
	  || const_ok_for_arm (-unsorted_offsets[order[0]])) ? 5 : 0;
}

const char *
emit_ldm_seq (rtx *operands, int nops)
{
  int regs[4];
  int base_reg;
  HOST_WIDE_INT offset;
  char buf[100];
  int i;

  switch (load_multiple_sequence (operands, nops, regs, &base_reg, &offset))
    {
    case 1:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "ldm%(ia%)\t");
      break;

    case 2:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "ldm%(ib%)\t");
      break;

    case 3:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "ldm%(da%)\t");
      break;

    case 4:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "ldm%(db%)\t");
      break;

    case 5:
      if (offset >= 0)
	sprintf (buf, "add%%?\t%s%s, %s%s, #%ld", REGISTER_PREFIX,
		 reg_names[regs[0]], REGISTER_PREFIX, reg_names[base_reg],
		 (long) offset);
      else
	sprintf (buf, "sub%%?\t%s%s, %s%s, #%ld", REGISTER_PREFIX,
		 reg_names[regs[0]], REGISTER_PREFIX, reg_names[base_reg],
		 (long) -offset);
      output_asm_insn (buf, operands);
      base_reg = regs[0];
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "ldm%(ia%)\t");
      break;

    default:
      gcc_unreachable ();
    }

  sprintf (buf + strlen (buf), "%s%s, {%s%s", REGISTER_PREFIX,
	   reg_names[base_reg], REGISTER_PREFIX, reg_names[regs[0]]);

  for (i = 1; i < nops; i++)
    sprintf (buf + strlen (buf), ", %s%s", REGISTER_PREFIX,
	     reg_names[regs[i]]);

  strcat (buf, "}\t%@ phole ldm");

  output_asm_insn (buf, operands);
  return "";
}

int
store_multiple_sequence (rtx *operands, int nops, int *regs, int *base,
			 HOST_WIDE_INT * load_offset)
{
  int unsorted_regs[4];
  HOST_WIDE_INT unsorted_offsets[4];
  int order[4];
  int base_reg = -1;
  int i;

  /* Can only handle 2, 3, or 4 insns at present, though could be easily
     extended if required.  */
  gcc_assert (nops >= 2 && nops <= 4);

  /* Loop over the operands and check that the memory references are
     suitable (i.e. immediate offsets from the same base register).  At
     the same time, extract the target register, and the memory
     offsets.  */
  for (i = 0; i < nops; i++)
    {
      rtx reg;
      rtx offset;

      /* Convert a subreg of a mem into the mem itself.  */
      if (GET_CODE (operands[nops + i]) == SUBREG)
	operands[nops + i] = alter_subreg (operands + (nops + i));

      gcc_assert (GET_CODE (operands[nops + i]) == MEM);

      /* Don't reorder volatile memory references; it doesn't seem worth
	 looking for the case where the order is ok anyway.  */
      if (MEM_VOLATILE_P (operands[nops + i]))
	return 0;

      offset = const0_rtx;

      if ((GET_CODE (reg = XEXP (operands[nops + i], 0)) == REG
	   || (GET_CODE (reg) == SUBREG
	       && GET_CODE (reg = SUBREG_REG (reg)) == REG))
	  || (GET_CODE (XEXP (operands[nops + i], 0)) == PLUS
	      && ((GET_CODE (reg = XEXP (XEXP (operands[nops + i], 0), 0))
		   == REG)
		  || (GET_CODE (reg) == SUBREG
		      && GET_CODE (reg = SUBREG_REG (reg)) == REG))
	      && (GET_CODE (offset = XEXP (XEXP (operands[nops + i], 0), 1))
		  == CONST_INT)))
	{
	  if (i == 0)
	    {
	      base_reg = REGNO (reg);
	      unsorted_regs[0] = (GET_CODE (operands[i]) == REG
				  ? REGNO (operands[i])
				  : REGNO (SUBREG_REG (operands[i])));
	      order[0] = 0;
	    }
	  else
	    {
	      if (base_reg != (int) REGNO (reg))
		/* Not addressed from the same base register.  */
		return 0;

	      unsorted_regs[i] = (GET_CODE (operands[i]) == REG
				  ? REGNO (operands[i])
				  : REGNO (SUBREG_REG (operands[i])));
	      if (unsorted_regs[i] < unsorted_regs[order[0]])
		order[0] = i;
	    }

	  /* If it isn't an integer register, then we can't do this.  */
	  if (unsorted_regs[i] < 0 || unsorted_regs[i] > 14)
	    return 0;

	  unsorted_offsets[i] = INTVAL (offset);
	}
      else
	/* Not a suitable memory address.  */
	return 0;
    }

  /* All the useful information has now been extracted from the
     operands into unsorted_regs and unsorted_offsets; additionally,
     order[0] has been set to the lowest numbered register in the
     list.  Sort the registers into order, and check that the memory
     offsets are ascending and adjacent.  */

  for (i = 1; i < nops; i++)
    {
      int j;

      order[i] = order[i - 1];
      for (j = 0; j < nops; j++)
	if (unsorted_regs[j] > unsorted_regs[order[i - 1]]
	    && (order[i] == order[i - 1]
		|| unsorted_regs[j] < unsorted_regs[order[i]]))
	  order[i] = j;

      /* Have we found a suitable register? if not, one must be used more
	 than once.  */
      if (order[i] == order[i - 1])
	return 0;

      /* Is the memory address adjacent and ascending? */
      if (unsorted_offsets[order[i]] != unsorted_offsets[order[i - 1]] + 4)
	return 0;
    }

  if (base)
    {
      *base = base_reg;

      for (i = 0; i < nops; i++)
	regs[i] = unsorted_regs[order[i]];

      *load_offset = unsorted_offsets[order[0]];
    }

  if (unsorted_offsets[order[0]] == 0)
    return 1; /* stmia */

  if (unsorted_offsets[order[0]] == 4)
    return 2; /* stmib */

  if (unsorted_offsets[order[nops - 1]] == 0)
    return 3; /* stmda */

  if (unsorted_offsets[order[nops - 1]] == -4)
    return 4; /* stmdb */

  return 0;
}

const char *
emit_stm_seq (rtx *operands, int nops)
{
  int regs[4];
  int base_reg;
  HOST_WIDE_INT offset;
  char buf[100];
  int i;

  switch (store_multiple_sequence (operands, nops, regs, &base_reg, &offset))
    {
    case 1:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "stm%(ia%)\t");
      break;

    case 2:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "stm%(ib%)\t");
      break;

    case 3:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "stm%(da%)\t");
      break;

    case 4:
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      strcpy (buf, "stm%(db%)\t");
      break;

    default:
      gcc_unreachable ();
    }

  sprintf (buf + strlen (buf), "%s%s, {%s%s", REGISTER_PREFIX,
	   reg_names[base_reg], REGISTER_PREFIX, reg_names[regs[0]]);

  for (i = 1; i < nops; i++)
    sprintf (buf + strlen (buf), ", %s%s", REGISTER_PREFIX,
	     reg_names[regs[i]]);

  strcat (buf, "}\t%@ phole stm");

  output_asm_insn (buf, operands);
  return "";
}

/* Routines for use in generating RTL.  */

rtx
arm_gen_load_multiple (int base_regno, int count, rtx from, int up,
		       int write_back, rtx basemem, HOST_WIDE_INT *offsetp)
{
  HOST_WIDE_INT offset = *offsetp;
  int i = 0, j;
  rtx result;
  int sign = up ? 1 : -1;
  rtx mem, addr;

  /* XScale has load-store double instructions, but they have stricter
     alignment requirements than load-store multiple, so we cannot
     use them.

     For XScale ldm requires 2 + NREGS cycles to complete and blocks
     the pipeline until completion.

	NREGS		CYCLES
	  1		  3
	  2		  4
	  3		  5
	  4		  6

     An ldr instruction takes 1-3 cycles, but does not block the
     pipeline.

	NREGS		CYCLES
	  1		 1-3
	  2		 2-6
	  3		 3-9
	  4		 4-12

     Best case ldr will always win.  However, the more ldr instructions
     we issue, the less likely we are to be able to schedule them well.
     Using ldr instructions also increases code size.

     As a compromise, we use ldr for counts of 1 or 2 regs, and ldm
     for counts of 3 or 4 regs.  */
  if (arm_tune_xscale && count <= 2 && ! optimize_size)
    {
      rtx seq;

      start_sequence ();

      for (i = 0; i < count; i++)
	{
	  addr = plus_constant (from, i * 4 * sign);
	  mem = adjust_automodify_address (basemem, SImode, addr, offset);
	  emit_move_insn (gen_rtx_REG (SImode, base_regno + i), mem);
	  offset += 4 * sign;
	}

      if (write_back)
	{
	  emit_move_insn (from, plus_constant (from, count * 4 * sign));
	  *offsetp = offset;
	}

      seq = get_insns ();
      end_sequence ();

      return seq;
    }

  result = gen_rtx_PARALLEL (VOIDmode,
			     rtvec_alloc (count + (write_back ? 1 : 0)));
  if (write_back)
    {
      XVECEXP (result, 0, 0)
	= gen_rtx_SET (VOIDmode, from, plus_constant (from, count * 4 * sign));
      i = 1;
      count++;
    }

  for (j = 0; i < count; i++, j++)
    {
      addr = plus_constant (from, j * 4 * sign);
      mem = adjust_automodify_address_nv (basemem, SImode, addr, offset);
      XVECEXP (result, 0, i)
	= gen_rtx_SET (VOIDmode, gen_rtx_REG (SImode, base_regno + j), mem);
      offset += 4 * sign;
    }

  if (write_back)
    *offsetp = offset;

  return result;
}

rtx
arm_gen_store_multiple (int base_regno, int count, rtx to, int up,
			int write_back, rtx basemem, HOST_WIDE_INT *offsetp)
{
  HOST_WIDE_INT offset = *offsetp;
  int i = 0, j;
  rtx result;
  int sign = up ? 1 : -1;
  rtx mem, addr;

  /* See arm_gen_load_multiple for discussion of
     the pros/cons of ldm/stm usage for XScale.  */
  if (arm_tune_xscale && count <= 2 && ! optimize_size)
    {
      rtx seq;

      start_sequence ();

      for (i = 0; i < count; i++)
	{
	  addr = plus_constant (to, i * 4 * sign);
	  mem = adjust_automodify_address (basemem, SImode, addr, offset);
	  emit_move_insn (mem, gen_rtx_REG (SImode, base_regno + i));
	  offset += 4 * sign;
	}

      if (write_back)
	{
	  emit_move_insn (to, plus_constant (to, count * 4 * sign));
	  *offsetp = offset;
	}

      seq = get_insns ();
      end_sequence ();

      return seq;
    }

  result = gen_rtx_PARALLEL (VOIDmode,
			     rtvec_alloc (count + (write_back ? 1 : 0)));
  if (write_back)
    {
      XVECEXP (result, 0, 0)
	= gen_rtx_SET (VOIDmode, to,
		       plus_constant (to, count * 4 * sign));
      i = 1;
      count++;
    }

  for (j = 0; i < count; i++, j++)
    {
      addr = plus_constant (to, j * 4 * sign);
      mem = adjust_automodify_address_nv (basemem, SImode, addr, offset);
      XVECEXP (result, 0, i)
	= gen_rtx_SET (VOIDmode, mem, gen_rtx_REG (SImode, base_regno + j));
      offset += 4 * sign;
    }

  if (write_back)
    *offsetp = offset;

  return result;
}

int
arm_gen_movmemqi (rtx *operands)
{
  HOST_WIDE_INT in_words_to_go, out_words_to_go, last_bytes;
  HOST_WIDE_INT srcoffset, dstoffset;
  int i;
  rtx src, dst, srcbase, dstbase;
  rtx part_bytes_reg = NULL;
  rtx mem;

  if (GET_CODE (operands[2]) != CONST_INT
      || GET_CODE (operands[3]) != CONST_INT
      || INTVAL (operands[2]) > 64
      || INTVAL (operands[3]) & 3)
    return 0;

  /* APPLE LOCAL begin ARM use memcpy more at -Os */
  /* At -Os we consider the size of repeated lod/sto vs memcpy call.  Both ways
     require getting source and dest addresses into regs.  Beyond that memcpy
     is 2 insns; lod/sto is at least 2, maybe more.  But lod/sto is faster so
     we prefer that when it is only 2 insns; that occurs when the size is
     1, 2, 4, 8, 12, or 16 only. */
  if (optimize_size
      && INTVAL (operands[2]) != 1
      && INTVAL (operands[2]) != 2
      && INTVAL (operands[2]) != 4
      && INTVAL (operands[2]) != 8
      && INTVAL (operands[2]) != 12
      && INTVAL (operands[2]) != 16)
    return 0;
  /* APPLE LOCAL end ARM use memcpy more at -Os */

  dstbase = operands[0];
  srcbase = operands[1];

  dst = copy_to_mode_reg (SImode, XEXP (dstbase, 0));
  src = copy_to_mode_reg (SImode, XEXP (srcbase, 0));

  in_words_to_go = ARM_NUM_INTS (INTVAL (operands[2]));
  out_words_to_go = INTVAL (operands[2]) / 4;
  last_bytes = INTVAL (operands[2]) & 3;
  dstoffset = srcoffset = 0;

  if (out_words_to_go != in_words_to_go && ((in_words_to_go - 1) & 3) != 0)
    part_bytes_reg = gen_rtx_REG (SImode, (in_words_to_go - 1) & 3);

  for (i = 0; in_words_to_go >= 2; i+=4)
    {
      if (in_words_to_go > 4)
	emit_insn (arm_gen_load_multiple (0, 4, src, TRUE, TRUE,
					  srcbase, &srcoffset));
      else
	emit_insn (arm_gen_load_multiple (0, in_words_to_go, src, TRUE,
					  FALSE, srcbase, &srcoffset));

      if (out_words_to_go)
	{
	  if (out_words_to_go > 4)
	    emit_insn (arm_gen_store_multiple (0, 4, dst, TRUE, TRUE,
					       dstbase, &dstoffset));
	  else if (out_words_to_go != 1)
	    emit_insn (arm_gen_store_multiple (0, out_words_to_go,
					       dst, TRUE,
					       (last_bytes == 0
						? FALSE : TRUE),
					       dstbase, &dstoffset));
	  else
	    {
	      mem = adjust_automodify_address (dstbase, SImode, dst, dstoffset);
	      emit_move_insn (mem, gen_rtx_REG (SImode, 0));
	      if (last_bytes != 0)
		{
		  emit_insn (gen_addsi3 (dst, dst, GEN_INT (4)));
		  dstoffset += 4;
		}
	    }
	}

      in_words_to_go -= in_words_to_go < 4 ? in_words_to_go : 4;
      out_words_to_go -= out_words_to_go < 4 ? out_words_to_go : 4;
    }

  /* OUT_WORDS_TO_GO will be zero here if there are byte stores to do.  */
  if (out_words_to_go)
    {
      rtx sreg;

      mem = adjust_automodify_address (srcbase, SImode, src, srcoffset);
      sreg = copy_to_reg (mem);

      mem = adjust_automodify_address (dstbase, SImode, dst, dstoffset);
      emit_move_insn (mem, sreg);
      in_words_to_go--;

      gcc_assert (!in_words_to_go);	/* Sanity check */
    }

  if (in_words_to_go)
    {
      gcc_assert (in_words_to_go > 0);

      mem = adjust_automodify_address (srcbase, SImode, src, srcoffset);
      part_bytes_reg = copy_to_mode_reg (SImode, mem);
    }

  gcc_assert (!last_bytes || part_bytes_reg);

  if (BYTES_BIG_ENDIAN && last_bytes)
    {
      rtx tmp = gen_reg_rtx (SImode);

      /* The bytes we want are in the top end of the word.  */
      emit_insn (gen_lshrsi3 (tmp, part_bytes_reg,
			      GEN_INT (8 * (4 - last_bytes))));
      part_bytes_reg = tmp;

      while (last_bytes)
	{
	  mem = adjust_automodify_address (dstbase, QImode,
					   plus_constant (dst, last_bytes - 1),
					   dstoffset + last_bytes - 1);
	  emit_move_insn (mem, gen_lowpart (QImode, part_bytes_reg));

	  if (--last_bytes)
	    {
	      tmp = gen_reg_rtx (SImode);
	      emit_insn (gen_lshrsi3 (tmp, part_bytes_reg, GEN_INT (8)));
	      part_bytes_reg = tmp;
	    }
	}

    }
  else
    {
      if (last_bytes > 1)
	{
	  mem = adjust_automodify_address (dstbase, HImode, dst, dstoffset);
	  emit_move_insn (mem, gen_lowpart (HImode, part_bytes_reg));
	  last_bytes -= 2;
	  if (last_bytes)
	    {
	      rtx tmp = gen_reg_rtx (SImode);
	      emit_insn (gen_addsi3 (dst, dst, const2_rtx));
	      emit_insn (gen_lshrsi3 (tmp, part_bytes_reg, GEN_INT (16)));
	      part_bytes_reg = tmp;
	      dstoffset += 2;
	    }
	}

      if (last_bytes)
	{
	  mem = adjust_automodify_address (dstbase, QImode, dst, dstoffset);
	  emit_move_insn (mem, gen_lowpart (QImode, part_bytes_reg));
	}
    }

  return 1;
}

/* Select a dominance comparison mode if possible for a test of the general
   form (OP (COND_OR (X) (Y)) (const_int 0)).  We support three forms.
   COND_OR == DOM_CC_X_AND_Y => (X && Y)
   COND_OR == DOM_CC_NX_OR_Y => ((! X) || Y)
   COND_OR == DOM_CC_X_OR_Y => (X || Y)
   In all cases OP will be either EQ or NE, but we don't need to know which
   here.  If we are unable to support a dominance comparison we return
   CC mode.  This will then fail to match for the RTL expressions that
   generate this call.  */
enum machine_mode
arm_select_dominance_cc_mode (rtx x, rtx y, HOST_WIDE_INT cond_or)
{
  enum rtx_code cond1, cond2;
  int swapped = 0;

  /* Currently we will probably get the wrong result if the individual
     comparisons are not simple.  This also ensures that it is safe to
     reverse a comparison if necessary.  */
  if ((arm_select_cc_mode (cond1 = GET_CODE (x), XEXP (x, 0), XEXP (x, 1))
       != CCmode)
      || (arm_select_cc_mode (cond2 = GET_CODE (y), XEXP (y, 0), XEXP (y, 1))
	  != CCmode))
    return CCmode;

  /* The if_then_else variant of this tests the second condition if the
     first passes, but is true if the first fails.  Reverse the first
     condition to get a true "inclusive-or" expression.  */
  if (cond_or == DOM_CC_NX_OR_Y)
    cond1 = reverse_condition (cond1);

  /* If the comparisons are not equal, and one doesn't dominate the other,
     then we can't do this.  */
  if (cond1 != cond2
      && !comparison_dominates_p (cond1, cond2)
      && (swapped = 1, !comparison_dominates_p (cond2, cond1)))
    return CCmode;

  if (swapped)
    {
      enum rtx_code temp = cond1;
      cond1 = cond2;
      cond2 = temp;
    }

  switch (cond1)
    {
    case EQ:
      if (cond_or == DOM_CC_X_AND_Y)
	return CC_DEQmode;

      switch (cond2)
	{
	case EQ: return CC_DEQmode;
	case LE: return CC_DLEmode;
	case LEU: return CC_DLEUmode;
	case GE: return CC_DGEmode;
	case GEU: return CC_DGEUmode;
	default: gcc_unreachable ();
	}

    case LT:
      if (cond_or == DOM_CC_X_AND_Y)
	return CC_DLTmode;

      switch (cond2)
	{
	case  LT:
	    return CC_DLTmode;
	case LE:
	  return CC_DLEmode;
	case NE:
	  return CC_DNEmode;
	default:
	  gcc_unreachable ();
	}

    case GT:
      if (cond_or == DOM_CC_X_AND_Y)
	return CC_DGTmode;

      switch (cond2)
	{
	case GT:
	  return CC_DGTmode;
	case GE:
	  return CC_DGEmode;
	case NE:
	  return CC_DNEmode;
	default:
	  gcc_unreachable ();
	}

    case LTU:
      if (cond_or == DOM_CC_X_AND_Y)
	return CC_DLTUmode;

      switch (cond2)
	{
	case LTU:
	  return CC_DLTUmode;
	case LEU:
	  return CC_DLEUmode;
	case NE:
	  return CC_DNEmode;
	default:
	  gcc_unreachable ();
	}

    case GTU:
      if (cond_or == DOM_CC_X_AND_Y)
	return CC_DGTUmode;

      switch (cond2)
	{
	case GTU:
	  return CC_DGTUmode;
	case GEU:
	  return CC_DGEUmode;
	case NE:
	  return CC_DNEmode;
	default:
	  gcc_unreachable ();
	}

    /* The remaining cases only occur when both comparisons are the
       same.  */
    case NE:
      gcc_assert (cond1 == cond2);
      return CC_DNEmode;

    case LE:
      gcc_assert (cond1 == cond2);
      return CC_DLEmode;

    case GE:
      gcc_assert (cond1 == cond2);
      return CC_DGEmode;

    case LEU:
      gcc_assert (cond1 == cond2);
      return CC_DLEUmode;

    case GEU:
      gcc_assert (cond1 == cond2);
      return CC_DGEUmode;

    default:
      gcc_unreachable ();
    }
}

enum machine_mode
arm_select_cc_mode (enum rtx_code op, rtx x, rtx y)
{
  /* All floating point compares return CCFP if it is an equality
     comparison, and CCFPE otherwise.  */
  if (GET_MODE_CLASS (GET_MODE (x)) == MODE_FLOAT)
    {
      switch (op)
	{
	case EQ:
	case NE:
	case UNORDERED:
	case ORDERED:
	case UNLT:
	case UNLE:
	case UNGT:
	case UNGE:
	case UNEQ:
	case LTGT:
	  return CCFPmode;

	case LT:
	case LE:
	case GT:
	case GE:
	  if (TARGET_HARD_FLOAT && TARGET_MAVERICK)
	    return CCFPmode;
	  return CCFPEmode;

	default:
	  gcc_unreachable ();
	}
    }

  /* A compare with a shifted operand.  Because of canonicalization, the
     comparison will have to be swapped when we emit the assembler.  */
  if (GET_MODE (y) == SImode && GET_CODE (y) == REG
      && (GET_CODE (x) == ASHIFT || GET_CODE (x) == ASHIFTRT
	  || GET_CODE (x) == LSHIFTRT || GET_CODE (x) == ROTATE
	  || GET_CODE (x) == ROTATERT))
    return CC_SWPmode;

  /* This operation is performed swapped, but since we only rely on the Z
     flag we don't need an additional mode.  */
  if (GET_MODE (y) == SImode && REG_P (y)
      && GET_CODE (x) == NEG
      && (op ==	EQ || op == NE))
    return CC_Zmode;

  /* This is a special case that is used by combine to allow a
     comparison of a shifted byte load to be split into a zero-extend
     followed by a comparison of the shifted integer (only valid for
     equalities and unsigned inequalities).  */
  if (GET_MODE (x) == SImode
      && GET_CODE (x) == ASHIFT
      && GET_CODE (XEXP (x, 1)) == CONST_INT && INTVAL (XEXP (x, 1)) == 24
      && GET_CODE (XEXP (x, 0)) == SUBREG
      && GET_CODE (SUBREG_REG (XEXP (x, 0))) == MEM
      && GET_MODE (SUBREG_REG (XEXP (x, 0))) == QImode
      && (op == EQ || op == NE
	  || op == GEU || op == GTU || op == LTU || op == LEU)
      && GET_CODE (y) == CONST_INT)
    return CC_Zmode;

  /* A construct for a conditional compare, if the false arm contains
     0, then both conditions must be true, otherwise either condition
     must be true.  Not all conditions are possible, so CCmode is
     returned if it can't be done.  */
  if (GET_CODE (x) == IF_THEN_ELSE
      && (XEXP (x, 2) == const0_rtx
	  || XEXP (x, 2) == const1_rtx)
      && COMPARISON_P (XEXP (x, 0))
      && COMPARISON_P (XEXP (x, 1)))
    return arm_select_dominance_cc_mode (XEXP (x, 0), XEXP (x, 1),
					 INTVAL (XEXP (x, 2)));

  /* Alternate canonicalizations of the above.  These are somewhat cleaner.  */
  if (GET_CODE (x) == AND
      && COMPARISON_P (XEXP (x, 0))
      && COMPARISON_P (XEXP (x, 1)))
    return arm_select_dominance_cc_mode (XEXP (x, 0), XEXP (x, 1),
					 DOM_CC_X_AND_Y);

  if (GET_CODE (x) == IOR
      && COMPARISON_P (XEXP (x, 0))
      && COMPARISON_P (XEXP (x, 1)))
    return arm_select_dominance_cc_mode (XEXP (x, 0), XEXP (x, 1),
					 DOM_CC_X_OR_Y);

  /* An operation (on Thumb) where we want to test for a single bit.
     This is done by shifting that bit up into the top bit of a
     scratch register; we can then branch on the sign bit.  */
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (TARGET_THUMB1
      && GET_MODE (x) == SImode
      && (op == EQ || op == NE)
      && GET_CODE (x) == ZERO_EXTRACT
      && XEXP (x, 1) == const1_rtx)
    return CC_Nmode;

  /* An operation that sets the condition codes as a side-effect, the
     V flag is not set correctly, so we can only use comparisons where
     this doesn't matter.  (For LT and GE we can use "mi" and "pl"
     instead.)  */
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  /* ??? Does the ZERO_EXTRACT case really apply to thumb2?  */
  if (GET_MODE (x) == SImode
      && y == const0_rtx
      && (op == EQ || op == NE || op == LT || op == GE)
      && (GET_CODE (x) == PLUS || GET_CODE (x) == MINUS
	  || GET_CODE (x) == AND || GET_CODE (x) == IOR
	  || GET_CODE (x) == XOR || GET_CODE (x) == MULT
	  || GET_CODE (x) == NOT || GET_CODE (x) == NEG
	  || GET_CODE (x) == LSHIFTRT
	  || GET_CODE (x) == ASHIFT || GET_CODE (x) == ASHIFTRT
	  || GET_CODE (x) == ROTATERT
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  || (TARGET_32BIT && GET_CODE (x) == ZERO_EXTRACT)))
    return CC_NOOVmode;

  if (GET_MODE (x) == QImode && (op == EQ || op == NE))
    return CC_Zmode;

  if (GET_MODE (x) == SImode && (op == LTU || op == GEU)
      && GET_CODE (x) == PLUS
      && (rtx_equal_p (XEXP (x, 0), y) || rtx_equal_p (XEXP (x, 1), y)))
    return CC_Cmode;

  return CCmode;
}

/* X and Y are two things to compare using CODE.  Emit the compare insn and
   return the rtx for register 0 in the proper mode.  FP means this is a
   floating point compare: I don't think that it is needed on the arm.  */
rtx
arm_gen_compare_reg (enum rtx_code code, rtx x, rtx y)
{
  enum machine_mode mode = SELECT_CC_MODE (code, x, y);
  rtx cc_reg = gen_rtx_REG (mode, CC_REGNUM);

  emit_set_insn (cc_reg, gen_rtx_COMPARE (mode, x, y));

  return cc_reg;
}

/* Generate a sequence of insns that will generate the correct return
   address mask depending on the physical architecture that the program
   is running on.  */
rtx
arm_gen_return_addr_mask (void)
{
  rtx reg = gen_reg_rtx (Pmode);

  emit_insn (gen_return_addr_mask (reg));
  return reg;
}

void
arm_reload_in_hi (rtx *operands)
{
  rtx ref = operands[1];
  rtx base, scratch;
  HOST_WIDE_INT offset = 0;

  if (GET_CODE (ref) == SUBREG)
    {
      offset = SUBREG_BYTE (ref);
      ref = SUBREG_REG (ref);
    }

  if (GET_CODE (ref) == REG)
    {
      /* We have a pseudo which has been spilt onto the stack; there
	 are two cases here: the first where there is a simple
	 stack-slot replacement and a second where the stack-slot is
	 out of range, or is used as a subreg.  */
      if (reg_equiv_mem[REGNO (ref)])
	{
	  ref = reg_equiv_mem[REGNO (ref)];
	  base = find_replacement (&XEXP (ref, 0));
	}
      else
	/* The slot is out of range, or was dressed up in a SUBREG.  */
	base = reg_equiv_address[REGNO (ref)];
    }
  else
    base = find_replacement (&XEXP (ref, 0));

  /* Handle the case where the address is too complex to be offset by 1.  */
  if (GET_CODE (base) == MINUS
      || (GET_CODE (base) == PLUS && GET_CODE (XEXP (base, 1)) != CONST_INT))
    {
      rtx base_plus = gen_rtx_REG (SImode, REGNO (operands[2]) + 1);

      emit_set_insn (base_plus, base);
      base = base_plus;
    }
  else if (GET_CODE (base) == PLUS)
    {
      /* The addend must be CONST_INT, or we would have dealt with it above.  */
      HOST_WIDE_INT hi, lo;

      offset += INTVAL (XEXP (base, 1));
      base = XEXP (base, 0);

      /* Rework the address into a legal sequence of insns.  */
      /* Valid range for lo is -4095 -> 4095 */
      lo = (offset >= 0
	    ? (offset & 0xfff)
	    : -((-offset) & 0xfff));

      /* Corner case, if lo is the max offset then we would be out of range
	 once we have added the additional 1 below, so bump the msb into the
	 pre-loading insn(s).  */
      if (lo == 4095)
	lo &= 0x7ff;

      hi = ((((offset - lo) & (HOST_WIDE_INT) 0xffffffff)
	     ^ (HOST_WIDE_INT) 0x80000000)
	    - (HOST_WIDE_INT) 0x80000000);

      gcc_assert (hi + lo == offset);

      if (hi != 0)
	{
	  rtx base_plus = gen_rtx_REG (SImode, REGNO (operands[2]) + 1);

	  /* Get the base address; addsi3 knows how to handle constants
	     that require more than one insn.  */
	  emit_insn (gen_addsi3 (base_plus, base, GEN_INT (hi)));
	  base = base_plus;
	  offset = lo;
	}
    }

  /* Operands[2] may overlap operands[0] (though it won't overlap
     operands[1]), that's why we asked for a DImode reg -- so we can
     use the bit that does not overlap.  */
  if (REGNO (operands[2]) == REGNO (operands[0]))
    scratch = gen_rtx_REG (SImode, REGNO (operands[2]) + 1);
  else
    scratch = gen_rtx_REG (SImode, REGNO (operands[2]));

  emit_insn (gen_zero_extendqisi2 (scratch,
				   gen_rtx_MEM (QImode,
						plus_constant (base,
							       offset))));
  emit_insn (gen_zero_extendqisi2 (gen_rtx_SUBREG (SImode, operands[0], 0),
				   gen_rtx_MEM (QImode,
						plus_constant (base,
							       offset + 1))));
  if (!BYTES_BIG_ENDIAN)
    emit_set_insn (gen_rtx_SUBREG (SImode, operands[0], 0),
		   gen_rtx_IOR (SImode,
				gen_rtx_ASHIFT
				(SImode,
				 gen_rtx_SUBREG (SImode, operands[0], 0),
				 GEN_INT (8)),
				scratch));
  else
    emit_set_insn (gen_rtx_SUBREG (SImode, operands[0], 0),
		   gen_rtx_IOR (SImode,
				gen_rtx_ASHIFT (SImode, scratch,
						GEN_INT (8)),
				gen_rtx_SUBREG (SImode, operands[0], 0)));
}

/* Handle storing a half-word to memory during reload by synthesizing as two
   byte stores.  Take care not to clobber the input values until after we
   have moved them somewhere safe.  This code assumes that if the DImode
   scratch in operands[2] overlaps either the input value or output address
   in some way, then that value must die in this insn (we absolutely need
   two scratch registers for some corner cases).  */
void
arm_reload_out_hi (rtx *operands)
{
  rtx ref = operands[0];
  rtx outval = operands[1];
  rtx base, scratch;
  HOST_WIDE_INT offset = 0;

  if (GET_CODE (ref) == SUBREG)
    {
      offset = SUBREG_BYTE (ref);
      ref = SUBREG_REG (ref);
    }

  if (GET_CODE (ref) == REG)
    {
      /* We have a pseudo which has been spilt onto the stack; there
	 are two cases here: the first where there is a simple
	 stack-slot replacement and a second where the stack-slot is
	 out of range, or is used as a subreg.  */
      if (reg_equiv_mem[REGNO (ref)])
	{
	  ref = reg_equiv_mem[REGNO (ref)];
	  base = find_replacement (&XEXP (ref, 0));
	}
      else
	/* The slot is out of range, or was dressed up in a SUBREG.  */
	base = reg_equiv_address[REGNO (ref)];
    }
  else
    base = find_replacement (&XEXP (ref, 0));

  scratch = gen_rtx_REG (SImode, REGNO (operands[2]));

  /* Handle the case where the address is too complex to be offset by 1.  */
  if (GET_CODE (base) == MINUS
      || (GET_CODE (base) == PLUS && GET_CODE (XEXP (base, 1)) != CONST_INT))
    {
      rtx base_plus = gen_rtx_REG (SImode, REGNO (operands[2]) + 1);

      /* Be careful not to destroy OUTVAL.  */
      if (reg_overlap_mentioned_p (base_plus, outval))
	{
	  /* Updating base_plus might destroy outval, see if we can
	     swap the scratch and base_plus.  */
	  if (!reg_overlap_mentioned_p (scratch, outval))
	    {
	      rtx tmp = scratch;
	      scratch = base_plus;
	      base_plus = tmp;
	    }
	  else
	    {
	      rtx scratch_hi = gen_rtx_REG (HImode, REGNO (operands[2]));

	      /* Be conservative and copy OUTVAL into the scratch now,
		 this should only be necessary if outval is a subreg
		 of something larger than a word.  */
	      /* XXX Might this clobber base?  I can't see how it can,
		 since scratch is known to overlap with OUTVAL, and
		 must be wider than a word.  */
	      emit_insn (gen_movhi (scratch_hi, outval));
	      outval = scratch_hi;
	    }
	}

      emit_set_insn (base_plus, base);
      base = base_plus;
    }
  else if (GET_CODE (base) == PLUS)
    {
      /* The addend must be CONST_INT, or we would have dealt with it above.  */
      HOST_WIDE_INT hi, lo;

      offset += INTVAL (XEXP (base, 1));
      base = XEXP (base, 0);

      /* Rework the address into a legal sequence of insns.  */
      /* Valid range for lo is -4095 -> 4095 */
      lo = (offset >= 0
	    ? (offset & 0xfff)
	    : -((-offset) & 0xfff));

      /* Corner case, if lo is the max offset then we would be out of range
	 once we have added the additional 1 below, so bump the msb into the
	 pre-loading insn(s).  */
      if (lo == 4095)
	lo &= 0x7ff;

      hi = ((((offset - lo) & (HOST_WIDE_INT) 0xffffffff)
	     ^ (HOST_WIDE_INT) 0x80000000)
	    - (HOST_WIDE_INT) 0x80000000);

      gcc_assert (hi + lo == offset);

      if (hi != 0)
	{
	  rtx base_plus = gen_rtx_REG (SImode, REGNO (operands[2]) + 1);

	  /* Be careful not to destroy OUTVAL.  */
	  if (reg_overlap_mentioned_p (base_plus, outval))
	    {
	      /* Updating base_plus might destroy outval, see if we
		 can swap the scratch and base_plus.  */
	      if (!reg_overlap_mentioned_p (scratch, outval))
		{
		  rtx tmp = scratch;
		  scratch = base_plus;
		  base_plus = tmp;
		}
	      else
		{
		  rtx scratch_hi = gen_rtx_REG (HImode, REGNO (operands[2]));

		  /* Be conservative and copy outval into scratch now,
		     this should only be necessary if outval is a
		     subreg of something larger than a word.  */
		  /* XXX Might this clobber base?  I can't see how it
		     can, since scratch is known to overlap with
		     outval.  */
		  emit_insn (gen_movhi (scratch_hi, outval));
		  outval = scratch_hi;
		}
	    }

	  /* Get the base address; addsi3 knows how to handle constants
	     that require more than one insn.  */
	  emit_insn (gen_addsi3 (base_plus, base, GEN_INT (hi)));
	  base = base_plus;
	  offset = lo;
	}
    }

  if (BYTES_BIG_ENDIAN)
    {
      emit_insn (gen_movqi (gen_rtx_MEM (QImode,
					 plus_constant (base, offset + 1)),
			    gen_lowpart (QImode, outval)));
      emit_insn (gen_lshrsi3 (scratch,
			      gen_rtx_SUBREG (SImode, outval, 0),
			      GEN_INT (8)));
      emit_insn (gen_movqi (gen_rtx_MEM (QImode, plus_constant (base, offset)),
			    gen_lowpart (QImode, scratch)));
    }
  else
    {
      emit_insn (gen_movqi (gen_rtx_MEM (QImode, plus_constant (base, offset)),
			    gen_lowpart (QImode, outval)));
      emit_insn (gen_lshrsi3 (scratch,
			      gen_rtx_SUBREG (SImode, outval, 0),
			      GEN_INT (8)));
      emit_insn (gen_movqi (gen_rtx_MEM (QImode,
					 plus_constant (base, offset + 1)),
			    gen_lowpart (QImode, scratch)));
    }
}

/* Return true if a type must be passed in memory. For AAPCS, small aggregates
   (padded to the size of a word) should be passed in a register.  */

static bool
arm_must_pass_in_stack (enum machine_mode mode, tree type)
{
  if (TARGET_AAPCS_BASED)
    return must_pass_in_stack_var_size (mode, type);
  else
    return must_pass_in_stack_var_size_or_pad (mode, type);
}


/* For use by FUNCTION_ARG_PADDING (MODE, TYPE).
   Return true if an argument passed on the stack should be padded upwards,
   i.e. if the least-significant byte has useful data.
   For legacy APCS ABIs we use the default.  For AAPCS based ABIs small
   aggregate types are placed in the lowest memory address.  */

bool
arm_pad_arg_upward (enum machine_mode mode, tree type)
{
  if (!TARGET_AAPCS_BASED)
    return DEFAULT_FUNCTION_ARG_PADDING(mode, type) == upward;

  if (type && BYTES_BIG_ENDIAN && INTEGRAL_TYPE_P (type))
    return false;

  return true;
}


/* Similarly, for use by BLOCK_REG_PADDING (MODE, TYPE, FIRST).
   For non-AAPCS, return !BYTES_BIG_ENDIAN if the least significant
   byte of the register has useful data, and return the opposite if the
   most significant byte does.
   For AAPCS, small aggregates and small complex types are always padded
   upwards.  */

bool
arm_pad_reg_upward (enum machine_mode mode ATTRIBUTE_UNUSED,
                    tree type, int first ATTRIBUTE_UNUSED)
{
  if (TARGET_AAPCS_BASED
      && BYTES_BIG_ENDIAN
      && (AGGREGATE_TYPE_P (type) || TREE_CODE (type) == COMPLEX_TYPE)
      && int_size_in_bytes (type) <= 4)
    return true;

  /* Otherwise, use default padding.  */
  return !BYTES_BIG_ENDIAN;
}


/* Print a symbolic form of X to the debug file, F.  */
static void
arm_print_value (FILE *f, rtx x)
{
  switch (GET_CODE (x))
    {
    case CONST_INT:
      fprintf (f, HOST_WIDE_INT_PRINT_HEX, INTVAL (x));
      return;

    case CONST_DOUBLE:
      fprintf (f, "<0x%lx,0x%lx>", (long)XWINT (x, 2), (long)XWINT (x, 3));
      return;

    case CONST_VECTOR:
      {
	int i;

	fprintf (f, "<");
	for (i = 0; i < CONST_VECTOR_NUNITS (x); i++)
	  {
	    fprintf (f, HOST_WIDE_INT_PRINT_HEX, INTVAL (CONST_VECTOR_ELT (x, i)));
	    if (i < (CONST_VECTOR_NUNITS (x) - 1))
	      fputc (',', f);
	  }
	fprintf (f, ">");
      }
      return;

    case CONST_STRING:
      fprintf (f, "\"%s\"", XSTR (x, 0));
      return;

    case SYMBOL_REF:
      fprintf (f, "`%s'", XSTR (x, 0));
      return;

    case LABEL_REF:
      fprintf (f, "L%d", INSN_UID (XEXP (x, 0)));
      return;

    case CONST:
      arm_print_value (f, XEXP (x, 0));
      return;

    case PLUS:
      arm_print_value (f, XEXP (x, 0));
      fprintf (f, "+");
      arm_print_value (f, XEXP (x, 1));
      return;

    case PC:
      fprintf (f, "pc");
      return;

    default:
      fprintf (f, "????");
      return;
    }
}

/* Routines for manipulation of the constant pool.  */

/* Arm instructions cannot load a large constant directly into a
   register; they have to come from a pc relative load.  The constant
   must therefore be placed in the addressable range of the pc
   relative load.  Depending on the precise pc relative load
   instruction the range is somewhere between 256 bytes and 4k.  This
   means that we often have to dump a constant inside a function, and
   generate code to branch around it.

   It is important to minimize this, since the branches will slow
   things down and make the code larger.

   Normally we can hide the table after an existing unconditional
   branch so that there is no interruption of the flow, but in the
   worst case the code looks like this:

	ldr	rn, L1
	...
	b	L2
	align
	L1:	.long value
	L2:
	...

	ldr	rn, L3
	...
	b	L4
	align
	L3:	.long value
	L4:
	...

   We fix this by performing a scan after scheduling, which notices
   which instructions need to have their operands fetched from the
   constant table and builds the table.

   The algorithm starts by building a table of all the constants that
   need fixing up and all the natural barriers in the function (places
   where a constant table can be dropped without breaking the flow).
   For each fixup we note how far the pc-relative replacement will be
   able to reach and the offset of the instruction into the function.

   Having built the table we then group the fixes together to form
   tables that are as large as possible (subject to addressing
   constraints) and emit each table of constants after the last
   barrier that is within range of all the instructions in the group.
   If a group does not contain a barrier, then we forcibly create one
   by inserting a jump instruction into the flow.  Once the table has
   been inserted, the insns are then modified to reference the
   relevant entry in the pool.

   Possible enhancements to the algorithm (not implemented) are:

   1) For some processors and object formats, there may be benefit in
   aligning the pools to the start of cache lines; this alignment
   would need to be taken into account when calculating addressability
   of a pool.  */

/* These typedefs are located at the start of this file, so that
   they can be used in the prototypes there.  This comment is to
   remind readers of that fact so that the following structures
   can be understood more easily.

     typedef struct minipool_node    Mnode;
     typedef struct minipool_fixup   Mfix;  */

struct minipool_node
{
  /* Doubly linked chain of entries.  */
  Mnode * next;
  Mnode * prev;
  /* The maximum offset into the code that this entry can be placed.  While
     pushing fixes for forward references, all entries are sorted in order
     of increasing max_address.  */
  HOST_WIDE_INT max_address;
  /* Similarly for an entry inserted for a backwards ref.  */
  HOST_WIDE_INT min_address;
  /* The number of fixes referencing this entry.  This can become zero
     if we "unpush" an entry.  In this case we ignore the entry when we
     come to emit the code.  */
  int refcount;
  /* The offset from the start of the minipool.  */
  HOST_WIDE_INT offset;
  /* The value in table.  */
  rtx value;
  /* The mode of value.  */
  enum machine_mode mode;
  /* The size of the value.  With iWMMXt enabled
     sizes > 4 also imply an alignment of 8-bytes.  */
  int fix_size;
};

struct minipool_fixup
{
  Mfix *            next;
  rtx               insn;
  HOST_WIDE_INT     address;
  rtx *             loc;
  enum machine_mode mode;
  int               fix_size;
  rtx               value;
  Mnode *           minipool;
  HOST_WIDE_INT     forwards;
  HOST_WIDE_INT     backwards;
};

/* Fixes less than a word need padding out to a word boundary.  */
#define MINIPOOL_FIX_SIZE(mode) \
  (GET_MODE_SIZE ((mode)) >= 4 ? GET_MODE_SIZE ((mode)) : 4)

/* APPLE LOCAL begin ARM 4790140 compact switch tables */
/* The miniLisp in attributes doesn't seem to be up to extracting
   a numeric datum from the argument; do it in code. */
void
arm_adjust_insn_length (rtx insn, int *length)
{
  rtx body = PATTERN (insn);
  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */

  /* Add two bytes to the length of conditionally executed Thumb-2
     instructions for the IT instruction.  */
  if (TARGET_THUMB2 && GET_CODE (PATTERN (insn)) == COND_EXEC)
    {
      *length += 2;
      return;
    }

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  if (GET_CODE (body) == UNSPEC_VOLATILE
      && (int) XEXP (body, 1) == VUNSPEC_POOL_STRING)
    {
      int len = TREE_STRING_LENGTH (SYMBOL_REF_DECL 
		    (XVECEXP (body, 0, 0)));
      len = (len + 3) & ~3;
      *length = len;
    }
  if (!TARGET_THUMB2 && GET_CODE (body) == ADDR_DIFF_VEC)
    {
      /* The obvious sizeof(elt)*nelts, plus sizeof(elt) for the
	 count.  */
      int len = (XVECLEN (body, 1) + 1) * GET_MODE_SIZE (GET_MODE (body));
      int insn_size = (TARGET_THUMB) ? 2 : 4;

      /* 32-bit thumb tables can have one halfword of padding.
	 If we knew the alignment + offset now, we could be correct
	 about this calculation.  Instead, we have to be
	 pessimistic.  */
      if (TARGET_THUMB
	  && GET_MODE_SIZE (GET_MODE (body)) == 4)
	len += 2;

      /* Round up to a multiple of instruction size.  */
      len = ((len + insn_size - 1) / insn_size) * insn_size;
      *length = len;
    }
  if (TARGET_THUMB
      /* ALQAAHIRA LOCAL 6279481 */
      && !TARGET_32BIT
      && GET_CODE (body) == UNSPEC_VOLATILE
      && (int) XEXP (body, 1) == VUNSPEC_EPILOGUE)
    {
      *length = handle_thumb_unexpanded_epilogue (false);
    }
  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (INSN_CODE (insn) == CODE_FOR_adjustable_thumb1_zero_extendhisi2
      || INSN_CODE (insn) == CODE_FOR_adjustable_thumb1_zero_extendhisi2_v6)
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    {
      rtx mem = XEXP (XEXP (body, 1), 0);
      if (GET_CODE (mem) == REG || GET_CODE (mem) == SUBREG)
	*length = 2;
      else
	{
	  gcc_assert (GET_CODE (mem) == MEM);
	  mem = XEXP (mem, 0);
	  if (GET_CODE (mem) == CONST)
	    mem = XEXP (mem, 0);
	  if (GET_CODE (mem) == PLUS
	      && GET_CODE (XEXP (mem, 0)) == REG
	      && REGNO (XEXP (mem, 0)) == SP_REGNUM)
	    *length = 4;
	  else
	    *length = 2;
	}
    }
  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (INSN_CODE (insn) == CODE_FOR_thumb1_extendhisi2
      || INSN_CODE (insn) == CODE_FOR_adjustable_thumb1_extendhisi2_insn_v6)
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    {
      rtx mem = XEXP (XEXP (XVECEXP (body, 0, 0), 1), 0);
      if (GET_CODE (mem) == REG || GET_CODE (mem) == SUBREG)
	*length = 2;
      else
	{
          gcc_assert (GET_CODE (mem) == MEM);
	  mem = XEXP (mem, 0);
	  if (GET_CODE (mem) == CONST)
	    mem = XEXP (mem, 0);
	  *length = 4;
	  if (GET_CODE (mem) == LABEL_REF)
	    *length = 2;
	  if (GET_CODE (mem) == PLUS)
	    {
	      if (GET_CODE (XEXP (mem, 0)) == LABEL_REF
		  && GET_CODE (XEXP (mem, 1)) == CONST_INT)
		*length = 2;
	      if (GET_CODE (XEXP (mem, 1)) == REG)
		*length = 2;
	    }
	}
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (INSN_CODE (insn) == CODE_FOR_adjustable_thumb1_extendqisi2)
    {
      rtx mem = XEXP (XEXP (body, 1), 0);
      if (GET_CODE (mem) == REG || GET_CODE (mem) == SUBREG)
	*length = 2;
      else
	{
          gcc_assert (GET_CODE (mem) == MEM);
	  mem = XEXP (mem, 0);
	  if (GET_CODE (mem) == CONST)
	    mem = XEXP (mem, 0);
	  if (GET_CODE (mem) == LABEL_REF)
	    *length = 2;
	  else if (GET_CODE (mem) == PLUS
	           && GET_CODE (XEXP (mem, 0)) == LABEL_REF)
	    *length = 2;
	  /* The "operand matches V constraint" case is not handled explicitly;
	     this can only generate valid code if the address is REG + REG,
	     so assume this is the case and let the code below handle it. */
	  else if (GET_CODE (mem) == PLUS)
	    {
	      if (GET_CODE (XEXP (mem, 0)) == REG)
		{
		  if (GET_CODE (XEXP (mem, 1)) == REG)
		    *length = 2;
	          else if (REGNO (XEXP (mem, 0)) == REGNO (XEXP (body, 0)))
		    *length = 6;
		  else
		    *length = 4;
		}
	      else
	        {
		  gcc_assert (GET_CODE (XEXP (mem, 1)) == REG);
		  if (REGNO (XEXP (mem, 1)) == REGNO (XEXP (body, 0)))
		    *length = 6;
		  else
		    *length = 4;
		}
	    }
	  else if (GET_CODE (mem) == REG && REGNO (XEXP (body, 0)) == REGNO (mem))
	    *length = 6;
	  else
	    *length = 4;
	}
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (INSN_CODE (insn) == CODE_FOR_adjustable_thumb1_extendqisi2_v6)
    {
      rtx mem = XEXP (XEXP (body, 1), 0);
      if (GET_CODE (mem) == REG || GET_CODE (mem) == SUBREG)
	*length = 2;
      else
	{
          gcc_assert (GET_CODE (mem) == MEM);
	  mem = XEXP (mem, 0);
	  if (GET_CODE (mem) == CONST)
	    mem = XEXP (mem, 0);
	  if (GET_CODE (mem) == LABEL_REF)
	    *length = 2;
	  else if (GET_CODE (mem) == PLUS
	           && GET_CODE (XEXP (mem, 0)) == LABEL_REF)
	    *length = 2;
	  /* The "operand matches V constraint" case is not handled explicitly;
	     this can only generate valid code if the address is REG + REG,
	     so assume this is the case and let the code below handle it. */
	  else if (GET_CODE (mem) == PLUS)
	    {
	      if (GET_CODE (XEXP (mem, 0)) == REG)
		{
		  if (GET_CODE (XEXP (mem, 1)) == REG)
		    *length = 2;
	          else if (REGNO (XEXP (mem, 0)) == REGNO (XEXP (body, 0)))
		    *length = 4;
		  else
		    *length = 4;
		}
	      else
	        {
		  gcc_assert (GET_CODE (XEXP (mem, 1)) == REG);
		  if (REGNO (XEXP (mem, 1)) == REGNO (XEXP (body, 0)))
		    *length = 4;
		  else
		    *length = 4;
		}
	    }
	  else if (GET_CODE (mem) == REG && REGNO (XEXP (body, 0)) == REGNO (mem))
	    *length = 4;
	  else
	    *length = 4;
	}
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (INSN_CODE (insn) == CODE_FOR_adjustable_thumb1_movhi_insn)
    {
      rtx mem = XEXP (body, 1);
      if (GET_CODE (mem) != MEM)
	*length = 2;
      else if (GET_CODE (XEXP (mem, 0)) == PLUS
	       && GET_CODE (XEXP (XEXP (mem, 0), 0)) == REG
	       && REGNO (XEXP (XEXP (mem, 0), 0)) == SP_REGNUM)
	*length = 4;
      else
	*length = 2;
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (INSN_CODE (insn) == CODE_FOR_adjustable_thumb1_movdi_insn)
    {
      rtx op0 = XEXP (body, 0);
      rtx op1 = XEXP (body, 1);

      /* case 3 */
      if (GET_CODE (op0) == MEM && 
	    (GET_CODE (XEXP (op0, 0)) == PRE_INC
	     || GET_CODE (XEXP (op0, 0)) == POST_INC))
	*length = 2;
      /* case 4 */
      else if (GET_CODE (op1) == MEM && 
	    (GET_CODE (XEXP (op1, 0)) == PRE_INC
	     || GET_CODE (XEXP (op1, 0)) == POST_INC))
	*length = 2;
      /* case 2 */
      else if (GET_CODE (op1) == CONST_INT
	       && !const_ok_for_arm (INTVAL (op1))
	       && INTVAL (op1) >= -4095
	       && INTVAL (op1) <= 4095
	       && thumb_low_register_operand (op0, GET_MODE (op0)))
	*length = 6;
      /* case 0, 1, 6, 7 */
      else if (GET_CODE (op1) != MEM)
	*length = 4;
      /* case 5 */
      else
	{
	  rtx addr = XEXP (op1, 0);
	  if (GET_CODE (addr) == REG)
	    *length = 4;
	  else if (GET_CODE (addr) == CONST)
	    *length = 4;
	  else if (GET_CODE (addr) == PLUS)
	    {
	      rtx base = XEXP (addr, 0);
	      rtx offset = XEXP (addr, 1);
	      if (CONSTANT_P (base))
		{
		  rtx temp = base;
		  base = offset;
		  offset = temp;
		}
	      if (GET_CODE (offset) == REG)
		*length = 6;
	      else
		*length = 4;
	    }
	  else if (GET_CODE (addr) == LABEL_REF)
	    *length = 4;
	  else
	    abort ();
	}
    }
}
/* APPLE LOCAL end ARM 4790140 compact switch tables */

static Mnode *	minipool_vector_head;
static Mnode *	minipool_vector_tail;
static rtx	minipool_vector_label;
static int	minipool_pad;

/* The linked list of all minipool fixes required for this function.  */
Mfix * 		minipool_fix_head;
Mfix * 		minipool_fix_tail;
/* The fix entry for the current minipool, once it has been placed.  */
Mfix *		minipool_barrier;

/* Determines if INSN is the start of a jump table.  Returns the end
   of the TABLE or NULL_RTX.  */
static rtx
is_jump_table (rtx insn)
{
  rtx table;

  if (GET_CODE (insn) == JUMP_INSN
      && JUMP_LABEL (insn) != NULL
      && ((table = next_real_insn (JUMP_LABEL (insn)))
	  == next_real_insn (insn))
      && table != NULL
      && GET_CODE (table) == JUMP_INSN
      && (GET_CODE (PATTERN (table)) == ADDR_VEC
	  || GET_CODE (PATTERN (table)) == ADDR_DIFF_VEC))
    return table;

  return NULL_RTX;
}

#ifndef JUMP_TABLES_IN_TEXT_SECTION
#define JUMP_TABLES_IN_TEXT_SECTION 0
#endif

static HOST_WIDE_INT
get_jump_table_size (rtx insn)
{
  /* ADDR_VECs only take room if read-only data does into the text
     section.  */
  if (JUMP_TABLES_IN_TEXT_SECTION || readonly_data_section == text_section)
    {
      rtx body = PATTERN (insn);
      int elt = GET_CODE (body) == ADDR_DIFF_VEC ? 1 : 0;
      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      HOST_WIDE_INT size;
      HOST_WIDE_INT modesize;

      modesize = GET_MODE_SIZE (GET_MODE (body));
      size = modesize * XVECLEN (body, elt);
      switch (modesize)
	{
	case 1:
	  /* Round up size  of TBB table to a hafword boundary.  */
	  size = (size + 1) & ~(HOST_WIDE_INT)1;
	  break;
	case 2:
	  /* No padding neccessary for TBH.  */
	  break;
	case 4:
	  /* Add two bytes for alignment on Thumb.  */
	  if (TARGET_THUMB)
	    size += 2;
	  break;
	default:
	  gcc_unreachable ();
	}
      return size;
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    }

  return 0;
}

/* Move a minipool fix MP from its current location to before MAX_MP.
   If MAX_MP is NULL, then MP doesn't need moving, but the addressing
   constraints may need updating.  */
static Mnode *
move_minipool_fix_forward_ref (Mnode *mp, Mnode *max_mp,
			       HOST_WIDE_INT max_address)
{
  /* The code below assumes these are different.  */
  gcc_assert (mp != max_mp);

  if (max_mp == NULL)
    {
      if (max_address < mp->max_address)
	mp->max_address = max_address;
    }
  else
    {
      if (max_address > max_mp->max_address - mp->fix_size)
	mp->max_address = max_mp->max_address - mp->fix_size;
      else
	mp->max_address = max_address;

      /* Unlink MP from its current position.  Since max_mp is non-null,
       mp->prev must be non-null.  */
      mp->prev->next = mp->next;
      if (mp->next != NULL)
	mp->next->prev = mp->prev;
      else
	minipool_vector_tail = mp->prev;

      /* Re-insert it before MAX_MP.  */
      mp->next = max_mp;
      mp->prev = max_mp->prev;
      max_mp->prev = mp;

      if (mp->prev != NULL)
	mp->prev->next = mp;
      else
	minipool_vector_head = mp;
    }

  /* Save the new entry.  */
  max_mp = mp;

  /* Scan over the preceding entries and adjust their addresses as
     required.  */
  while (mp->prev != NULL
	 && mp->prev->max_address > mp->max_address - mp->prev->fix_size)
    {
      mp->prev->max_address = mp->max_address - mp->prev->fix_size;
      mp = mp->prev;
    }

  return max_mp;
}

/* Add a constant to the minipool for a forward reference.  Returns the
   node added or NULL if the constant will not fit in this pool.  */
static Mnode *
add_minipool_forward_ref (Mfix *fix)
{
  /* If set, max_mp is the first pool_entry that has a lower
     constraint than the one we are trying to add.  */
  Mnode *       max_mp = NULL;
  HOST_WIDE_INT max_address = fix->address + fix->forwards - minipool_pad;
  Mnode *       mp;

  /* If the minipool starts before the end of FIX->INSN then this FIX
     can not be placed into the current pool.  Furthermore, adding the
     new constant pool entry may cause the pool to start FIX_SIZE bytes
     earlier.  */
  if (minipool_vector_head &&
      (fix->address + get_attr_length (fix->insn)
       >= minipool_vector_head->max_address - fix->fix_size))
    return NULL;

  /* Scan the pool to see if a constant with the same value has
     already been added.  While we are doing this, also note the
     location where we must insert the constant if it doesn't already
     exist.  */
  for (mp = minipool_vector_head; mp != NULL; mp = mp->next)
    {
      if (GET_CODE (fix->value) == GET_CODE (mp->value)
	  && fix->mode == mp->mode
	  && (GET_CODE (fix->value) != CODE_LABEL
	      || (CODE_LABEL_NUMBER (fix->value)
		  == CODE_LABEL_NUMBER (mp->value)))
	  && rtx_equal_p (fix->value, mp->value))
	{
	  /* More than one fix references this entry.  */
	  mp->refcount++;
	  return move_minipool_fix_forward_ref (mp, max_mp, max_address);
	}

      /* Note the insertion point if necessary.  */
      if (max_mp == NULL
	  && mp->max_address > max_address)
	max_mp = mp;

      /* If we are inserting an 8-bytes aligned quantity and
	 we have not already found an insertion point, then
	 make sure that all such 8-byte aligned quantities are
	 placed at the start of the pool.  */
      if (ARM_DOUBLEWORD_ALIGN
	  && max_mp == NULL
	  && fix->fix_size == 8
	  && mp->fix_size != 8)
	{
	  max_mp = mp;
	  max_address = mp->max_address;
	}
    }

  /* The value is not currently in the minipool, so we need to create
     a new entry for it.  If MAX_MP is NULL, the entry will be put on
     the end of the list since the placement is less constrained than
     any existing entry.  Otherwise, we insert the new fix before
     MAX_MP and, if necessary, adjust the constraints on the other
     entries.  */
  mp = XNEW (Mnode);
  mp->fix_size = fix->fix_size;
  mp->mode = fix->mode;
  mp->value = fix->value;
  mp->refcount = 1;
  /* Not yet required for a backwards ref.  */
  mp->min_address = -65536;

  if (max_mp == NULL)
    {
      mp->max_address = max_address;
      mp->next = NULL;
      mp->prev = minipool_vector_tail;

      if (mp->prev == NULL)
	{
	  minipool_vector_head = mp;
	  minipool_vector_label = gen_label_rtx ();
	}
      else
	mp->prev->next = mp;

      minipool_vector_tail = mp;
    }
  else
    {
      if (max_address > max_mp->max_address - mp->fix_size)
	mp->max_address = max_mp->max_address - mp->fix_size;
      else
	mp->max_address = max_address;

      mp->next = max_mp;
      mp->prev = max_mp->prev;
      max_mp->prev = mp;
      if (mp->prev != NULL)
	mp->prev->next = mp;
      else
	minipool_vector_head = mp;
    }

  /* Save the new entry.  */
  max_mp = mp;

  /* Scan over the preceding entries and adjust their addresses as
     required.  */
  while (mp->prev != NULL
	 && mp->prev->max_address > mp->max_address - mp->prev->fix_size)
    {
      mp->prev->max_address = mp->max_address - mp->prev->fix_size;
      mp = mp->prev;
    }

  return max_mp;
}

static Mnode *
move_minipool_fix_backward_ref (Mnode *mp, Mnode *min_mp,
				HOST_WIDE_INT  min_address)
{
  HOST_WIDE_INT offset;

  /* The code below assumes these are different.  */
  gcc_assert (mp != min_mp);

  if (min_mp == NULL)
    {
      if (min_address > mp->min_address)
	mp->min_address = min_address;
    }
  else
    {
      /* We will adjust this below if it is too loose.  */
      mp->min_address = min_address;

      /* Unlink MP from its current position.  Since min_mp is non-null,
	 mp->next must be non-null.  */
      mp->next->prev = mp->prev;
      if (mp->prev != NULL)
	mp->prev->next = mp->next;
      else
	minipool_vector_head = mp->next;

      /* Reinsert it after MIN_MP.  */
      mp->prev = min_mp;
      mp->next = min_mp->next;
      min_mp->next = mp;
      if (mp->next != NULL)
	mp->next->prev = mp;
      else
	minipool_vector_tail = mp;
    }

  min_mp = mp;

  offset = 0;
  for (mp = minipool_vector_head; mp != NULL; mp = mp->next)
    {
      mp->offset = offset;
      if (mp->refcount > 0)
	offset += mp->fix_size;

      if (mp->next && mp->next->min_address < mp->min_address + mp->fix_size)
	mp->next->min_address = mp->min_address + mp->fix_size;
    }

  return min_mp;
}

/* Add a constant to the minipool for a backward reference.  Returns the
   node added or NULL if the constant will not fit in this pool.

   Note that the code for insertion for a backwards reference can be
   somewhat confusing because the calculated offsets for each fix do
   not take into account the size of the pool (which is still under
   construction.  */
static Mnode *
add_minipool_backward_ref (Mfix *fix)
{
  /* If set, min_mp is the last pool_entry that has a lower constraint
     than the one we are trying to add.  */
  Mnode *min_mp = NULL;
  /* This can be negative, since it is only a constraint.  */
  HOST_WIDE_INT  min_address = fix->address - fix->backwards;
  Mnode *mp;

  /* If we can't reach the current pool from this insn, or if we can't
     insert this entry at the end of the pool without pushing other
     fixes out of range, then we don't try.  This ensures that we
     can't fail later on.  */
  if (min_address >= minipool_barrier->address
      || (minipool_vector_tail->min_address + fix->fix_size
	  >= minipool_barrier->address))
    return NULL;

  /* Scan the pool to see if a constant with the same value has
     already been added.  While we are doing this, also note the
     location where we must insert the constant if it doesn't already
     exist.  */
  for (mp = minipool_vector_tail; mp != NULL; mp = mp->prev)
    {
      if (GET_CODE (fix->value) == GET_CODE (mp->value)
	  && fix->mode == mp->mode
	  && (GET_CODE (fix->value) != CODE_LABEL
	      || (CODE_LABEL_NUMBER (fix->value)
		  == CODE_LABEL_NUMBER (mp->value)))
	  && rtx_equal_p (fix->value, mp->value)
	  /* Check that there is enough slack to move this entry to the
	     end of the table (this is conservative).  */
	  && (mp->max_address
	      > (minipool_barrier->address
		 + minipool_vector_tail->offset
		 + minipool_vector_tail->fix_size)))
	{
	  mp->refcount++;
	  return move_minipool_fix_backward_ref (mp, min_mp, min_address);
	}

      if (min_mp != NULL)
	mp->min_address += fix->fix_size;
      else
	{
	  /* Note the insertion point if necessary.  */
	  if (mp->min_address < min_address)
	    {
	      /* For now, we do not allow the insertion of 8-byte alignment
		 requiring nodes anywhere but at the start of the pool.  */
	      if (ARM_DOUBLEWORD_ALIGN
		  && fix->fix_size == 8 && mp->fix_size != 8)
		return NULL;
	      else
		min_mp = mp;
	    }
	  else if (mp->max_address
		   < minipool_barrier->address + mp->offset + fix->fix_size)
	    {
	      /* Inserting before this entry would push the fix beyond
		 its maximum address (which can happen if we have
		 re-located a forwards fix); force the new fix to come
		 after it.  */
	      min_mp = mp;
	      min_address = mp->min_address + fix->fix_size;
	    }
	  /* If we are inserting an 8-bytes aligned quantity and
	     we have not already found an insertion point, then
	     make sure that all such 8-byte aligned quantities are
	     placed at the start of the pool.  */
	  else if (ARM_DOUBLEWORD_ALIGN
		   && min_mp == NULL
		   && fix->fix_size == 8
		   && mp->fix_size < 8)
	    {
	      min_mp = mp;
	      min_address = mp->min_address + fix->fix_size;
	    }
	}
    }

  /* We need to create a new entry.  */
  mp = XNEW (Mnode);
  mp->fix_size = fix->fix_size;
  mp->mode = fix->mode;
  mp->value = fix->value;
  mp->refcount = 1;
  mp->max_address = minipool_barrier->address + 65536;

  mp->min_address = min_address;

  if (min_mp == NULL)
    {
      mp->prev = NULL;
      mp->next = minipool_vector_head;

      if (mp->next == NULL)
	{
	  minipool_vector_tail = mp;
	  minipool_vector_label = gen_label_rtx ();
	}
      else
	mp->next->prev = mp;

      minipool_vector_head = mp;
    }
  else
    {
      mp->next = min_mp->next;
      mp->prev = min_mp;
      min_mp->next = mp;

      if (mp->next != NULL)
	mp->next->prev = mp;
      else
	minipool_vector_tail = mp;
    }

  /* Save the new entry.  */
  min_mp = mp;

  if (mp->prev)
    mp = mp->prev;
  else
    mp->offset = 0;

  /* Scan over the following entries and adjust their offsets.  */
  while (mp->next != NULL)
    {
      if (mp->next->min_address < mp->min_address + mp->fix_size)
	mp->next->min_address = mp->min_address + mp->fix_size;

      if (mp->refcount)
	mp->next->offset = mp->offset + mp->fix_size;
      else
	mp->next->offset = mp->offset;

      mp = mp->next;
    }

  return min_mp;
}

static void
assign_minipool_offsets (Mfix *barrier)
{
  HOST_WIDE_INT offset = 0;
  Mnode *mp;

  minipool_barrier = barrier;

  for (mp = minipool_vector_head; mp != NULL; mp = mp->next)
    {
      mp->offset = offset;

      if (mp->refcount > 0)
	offset += mp->fix_size;
    }
}

/* Output the literal table */
static void
dump_minipool (rtx scan)
{
  Mnode * mp;
  Mnode * nmp;
  int align64 = 0;

  if (ARM_DOUBLEWORD_ALIGN)
    for (mp = minipool_vector_head; mp != NULL; mp = mp->next)
      if (mp->refcount > 0 && mp->fix_size == 8)
	{
	  align64 = 1;
	  break;
	}

  if (dump_file)
    fprintf (dump_file,
	     ";; Emitting minipool after insn %u; address %ld; align %d (bytes)\n",
	     INSN_UID (scan), (unsigned long) minipool_barrier->address, align64 ? 8 : 4);

  scan = emit_label_after (gen_label_rtx (), scan);
  scan = emit_insn_after (align64 ? gen_align_8 () : gen_align_4 (), scan);
  scan = emit_label_after (minipool_vector_label, scan);

  for (mp = minipool_vector_head; mp != NULL; mp = nmp)
    {
      if (mp->refcount > 0)
	{
	  if (dump_file)
	    {
	      fprintf (dump_file,
		       ";;  Offset %u, min %ld, max %ld ",
		       (unsigned) mp->offset, (unsigned long) mp->min_address,
		       (unsigned long) mp->max_address);
	      arm_print_value (dump_file, mp->value);
	      fputc ('\n', dump_file);
	    }

	  switch (mp->fix_size)
	    {
#ifdef HAVE_consttable_1
	    case 1:
	      scan = emit_insn_after (gen_consttable_1 (mp->value), scan);
	      break;

#endif
#ifdef HAVE_consttable_2
	    case 2:
	      scan = emit_insn_after (gen_consttable_2 (mp->value), scan);
	      break;

#endif
#ifdef HAVE_consttable_4
	    case 4:
	      scan = emit_insn_after (gen_consttable_4 (mp->value), scan);
	      break;

#endif
#ifdef HAVE_consttable_8
	    case 8:
	      scan = emit_insn_after (gen_consttable_8 (mp->value), scan);
	      break;

#endif
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
#ifdef HAVE_consttable_16
	    case 16:
              scan = emit_insn_after (gen_consttable_16 (mp->value), scan);
              break;

#endif
/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
	    default:
	      gcc_unreachable ();
	    }
	}

      nmp = mp->next;
      free (mp);
    }

  minipool_vector_head = minipool_vector_tail = NULL;
  scan = emit_insn_after (gen_consttable_end (), scan);
  scan = emit_barrier_after (scan);
}

/* Return the cost of forcibly inserting a barrier after INSN.  */
static int
arm_barrier_cost (rtx insn)
{
  /* Basing the location of the pool on the loop depth is preferable,
     but at the moment, the basic block information seems to be
     corrupt by this stage of the compilation.  */
  int base_cost = 50;
  rtx next = next_nonnote_insn (insn);

  if (next != NULL && GET_CODE (next) == CODE_LABEL)
    base_cost -= 20;

  switch (GET_CODE (insn))
    {
    case CODE_LABEL:
      /* It will always be better to place the table before the label, rather
	 than after it.  */
      return 50;

    case INSN:
    case CALL_INSN:
      return base_cost;

    case JUMP_INSN:
      return base_cost - 10;

    default:
      return base_cost + 10;
    }
}

/* Find the best place in the insn stream in the range
   (FIX->address,MAX_ADDRESS) to forcibly insert a minipool barrier.
   Create the barrier by inserting a jump and add a new fix entry for
   it.  */
static Mfix *
create_fix_barrier (Mfix *fix, HOST_WIDE_INT max_address)
{
  HOST_WIDE_INT count = 0;
  rtx barrier;
  rtx from = fix->insn;
  /* The instruction after which we will insert the jump.  */
  rtx selected = NULL;
  int selected_cost;
  /* The address at which the jump instruction will be placed.  */
  HOST_WIDE_INT selected_address;
  Mfix * new_fix;
  HOST_WIDE_INT max_count = max_address - fix->address;
  rtx label = gen_label_rtx ();

  selected_cost = arm_barrier_cost (from);
  selected_address = fix->address;

  while (from && count < max_count)
    {
      rtx tmp;
      int new_cost;

      /* This code shouldn't have been called if there was a natural barrier
	 within range.  */
      gcc_assert (GET_CODE (from) != BARRIER);

      /* Count the length of this insn.  */
      count += get_attr_length (from);

      /* APPLE LOCAL begin ARM 6008578 */
      if (LABEL_P (from))
	count += get_label_pad (from, fix->address + count);
      /* APPLE LOCAL end ARM 6008578 */

      /* If there is a jump table, add its length.  */
      tmp = is_jump_table (from);
      if (tmp != NULL)
	{
	  count += get_jump_table_size (tmp);

	  /* Jump tables aren't in a basic block, so base the cost on
	     the dispatch insn.  If we select this location, we will
	     still put the pool after the table.  */
	  new_cost = arm_barrier_cost (from);

	  if (count < max_count 
	      && (!selected || new_cost <= selected_cost))
	    {
	      selected = tmp;
	      selected_cost = new_cost;
	      selected_address = fix->address + count;
	    }

	  /* Continue after the dispatch table.  */
	  from = NEXT_INSN (tmp);
	  continue;
	}

      new_cost = arm_barrier_cost (from);

      if (count < max_count
	  && (!selected || new_cost <= selected_cost))
	{
	  selected = from;
	  selected_cost = new_cost;
	  selected_address = fix->address + count;
	}

      from = NEXT_INSN (from);
    }

  /* Make sure that we found a place to insert the jump.  */
  gcc_assert (selected);

  /* Create a new JUMP_INSN that branches around a barrier.  */
  from = emit_jump_insn_after (gen_jump (label), selected);
  JUMP_LABEL (from) = label;
  barrier = emit_barrier_after (from);
  emit_label_after (label, barrier);

  /* Create a minipool barrier entry for the new barrier.  */
  new_fix = (Mfix *) obstack_alloc (&minipool_obstack, sizeof (* new_fix));
  new_fix->insn = barrier;
  new_fix->address = selected_address;
  new_fix->next = fix->next;
  fix->next = new_fix;

  return new_fix;
}

/* Record that there is a natural barrier in the insn stream at
   ADDRESS.  */
static void
push_minipool_barrier (rtx insn, HOST_WIDE_INT address)
{
  Mfix * fix = (Mfix *) obstack_alloc (&minipool_obstack, sizeof (* fix));

  fix->insn = insn;
  fix->address = address;

  fix->next = NULL;
  if (minipool_fix_head != NULL)
    minipool_fix_tail->next = fix;
  else
    minipool_fix_head = fix;

  minipool_fix_tail = fix;
}

/* Record INSN, which will need fixing up to load a value from the
   minipool.  ADDRESS is the offset of the insn since the start of the
   function; LOC is a pointer to the part of the insn which requires
   fixing; VALUE is the constant that must be loaded, which is of type
   MODE.  */
static void
push_minipool_fix (rtx insn, HOST_WIDE_INT address, rtx *loc,
		   enum machine_mode mode, rtx value)
{
  Mfix * fix = (Mfix *) obstack_alloc (&minipool_obstack, sizeof (* fix));

#ifdef AOF_ASSEMBLER
  /* PIC symbol references need to be converted into offsets into the
     based area.  */
  /* XXX This shouldn't be done here.  */
  if (flag_pic && GET_CODE (value) == SYMBOL_REF)
    value = aof_pic_entry (value);
#endif /* AOF_ASSEMBLER */

  fix->insn = insn;
  fix->address = address;
  fix->loc = loc;
  fix->mode = mode;
  fix->fix_size = MINIPOOL_FIX_SIZE (mode);
  fix->value = value;
  fix->forwards = get_attr_pool_range (insn);
  fix->backwards = get_attr_neg_pool_range (insn);
  fix->minipool = NULL;

  /* If an insn doesn't have a range defined for it, then it isn't
     expecting to be reworked by this code.  Better to stop now than
     to generate duff assembly code.  */
  gcc_assert (fix->forwards || fix->backwards);

  /* If an entry requires 8-byte alignment then assume all constant pools
     require 4 bytes of padding.  Trying to do this later on a per-pool
     basis is awkward because existing pool entries have to be modified.  */
  if (ARM_DOUBLEWORD_ALIGN && fix->fix_size == 8)
    minipool_pad = 4;

  if (dump_file)
    {
      fprintf (dump_file,
	       ";; %smode fixup for i%d; addr %lu, range (%ld,%ld): ",
	       GET_MODE_NAME (mode),
	       INSN_UID (insn), (unsigned long) address,
	       -1 * (long)fix->backwards, (long)fix->forwards);
      arm_print_value (dump_file, fix->value);
      fprintf (dump_file, "\n");
    }

  /* Add it to the chain of fixes.  */
  fix->next = NULL;

  if (minipool_fix_head != NULL)
    minipool_fix_tail->next = fix;
  else
    minipool_fix_head = fix;

  minipool_fix_tail = fix;
}

/* Return the cost of synthesizing a 64-bit constant VAL inline.
   Returns the number of insns needed, or 99 if we don't know how to
   do it.  */
int
arm_const_double_inline_cost (rtx val)
{
  rtx lowpart, highpart;
  enum machine_mode mode;

  mode = GET_MODE (val);

  if (mode == VOIDmode)
    mode = DImode;

  gcc_assert (GET_MODE_SIZE (mode) == 8);

  lowpart = gen_lowpart (SImode, val);
  highpart = gen_highpart_mode (SImode, mode, val);

  gcc_assert (GET_CODE (lowpart) == CONST_INT);
  gcc_assert (GET_CODE (highpart) == CONST_INT);

  return (arm_gen_constant (SET, SImode, NULL_RTX, INTVAL (lowpart),
			    NULL_RTX, NULL_RTX, 0, 0)
	  + arm_gen_constant (SET, SImode, NULL_RTX, INTVAL (highpart),
			      NULL_RTX, NULL_RTX, 0, 0));
}

/* APPLE LOCAL begin 5831562 long long constants */
/* Return true if a 64-bit constant consists of two 32-bit halves,
   each of which is a valid immediate data-processing operand.
   (This differs from other 64-bit evaluations in that ~const is
   not considered.)
*/

bool
const64_ok_for_arm_immediate (rtx val)
{
  rtx lowpart, highpart;
  enum machine_mode mode;

  if (!TARGET_ARM)
    return false;

  mode = GET_MODE (val);

  if (mode == VOIDmode)
    mode = DImode;

  gcc_assert (GET_MODE_SIZE (mode) == 8);

  lowpart = gen_lowpart (SImode, val);
  highpart = gen_highpart_mode (SImode, mode, val);

  gcc_assert (GET_CODE (lowpart) == CONST_INT);
  gcc_assert (GET_CODE (highpart) == CONST_INT);

  return (const_ok_for_arm (INTVAL (lowpart))
	  && const_ok_for_arm (INTVAL (highpart)));
}

/* As above, but allow for constants whose negative value
   fits as well.  Both halves must match either as themselves
   or as negated.  */
bool
const64_ok_for_arm_add (rtx val)
{
  rtx lowpart, highpart, lowpart_neg, highpart_neg, val_neg;
  enum machine_mode mode;

  if (!TARGET_ARM)
    return false;

  mode = GET_MODE (val);

  if (mode == VOIDmode)
    mode = DImode;

  gcc_assert (GET_MODE_SIZE (mode) == 8);

  lowpart = gen_lowpart (SImode, val);
  highpart = gen_highpart_mode (SImode, mode, val);

  val_neg = negate_rtx (mode, val);
  lowpart_neg = gen_lowpart (SImode, val_neg);
  highpart_neg = gen_highpart_mode (SImode, mode, val_neg);

  gcc_assert (GET_CODE (lowpart) == CONST_INT);
  gcc_assert (GET_CODE (highpart) == CONST_INT);

  return ((const_ok_for_arm (INTVAL (lowpart))
	   && const_ok_for_arm (INTVAL (highpart)))
	  || (const_ok_for_arm (INTVAL (lowpart_neg))
	      && const_ok_for_arm (INTVAL (highpart_neg))));
}
/* APPLE LOCAL end 5831562 long long constants */

/* Return true if it is worthwhile to split a 64-bit constant into two
   32-bit operations.  This is the case if optimizing for size, or
   if we have load delay slots, or if one 32-bit part can be done with
   a single data operation.  */
bool
arm_const_double_by_parts (rtx val)
{
  enum machine_mode mode = GET_MODE (val);
  rtx part;

  if (optimize_size || arm_ld_sched)
    return true;

  if (mode == VOIDmode)
    mode = DImode;

  part = gen_highpart_mode (SImode, mode, val);

  gcc_assert (GET_CODE (part) == CONST_INT);

  if (const_ok_for_arm (INTVAL (part))
      || const_ok_for_arm (~INTVAL (part)))
    return true;

  part = gen_lowpart (SImode, val);

  gcc_assert (GET_CODE (part) == CONST_INT);

  if (const_ok_for_arm (INTVAL (part))
      || const_ok_for_arm (~INTVAL (part)))
    return true;

  return false;
}

/* Scan INSN and note any of its operands that need fixing.
   If DO_PUSHES is false we do not actually push any of the fixups
   needed.  The function returns TRUE if any fixups were needed/pushed.
   This is used by arm_memory_load_p() which needs to know about loads
   of constants that will be converted into minipool loads.  */
static bool
note_invalid_constants (rtx insn, HOST_WIDE_INT address, int do_pushes)
{
  bool result = false;
  int opno;

  extract_insn (insn);

  if (!constrain_operands (1))
    fatal_insn_not_found (insn);

  if (recog_data.n_alternatives == 0)
    return false;

  /* Fill in recog_op_alt with information about the constraints of
     this insn.  */
  preprocess_constraints ();

  for (opno = 0; opno < recog_data.n_operands; opno++)
    {
      /* Things we need to fix can only occur in inputs.  */
      if (recog_data.operand_type[opno] != OP_IN)
	continue;

      /* If this alternative is a memory reference, then any mention
	 of constants in this alternative is really to fool reload
	 into allowing us to accept one there.  We need to fix them up
	 now so that we output the right code.  */
      if (recog_op_alt[opno][which_alternative].memory_ok)
	{
	  rtx op = recog_data.operand[opno];

	  if (CONSTANT_P (op))
	    {
	      if (do_pushes)
		push_minipool_fix (insn, address, recog_data.operand_loc[opno],
				   recog_data.operand_mode[opno], op);
	      result = true;
	    }
	  else if (GET_CODE (op) == MEM
		   && GET_CODE (XEXP (op, 0)) == SYMBOL_REF
		   && CONSTANT_POOL_ADDRESS_P (XEXP (op, 0)))
	    {
	      if (do_pushes)
		{
		  rtx cop = avoid_constant_pool_reference (op);

		  /* Casting the address of something to a mode narrower
		     than a word can cause avoid_constant_pool_reference()
		     to return the pool reference itself.  That's no good to
		     us here.  Lets just hope that we can use the
		     constant pool value directly.  */
		  if (op == cop)
		    cop = get_pool_constant (XEXP (op, 0));

		  push_minipool_fix (insn, address,
				     recog_data.operand_loc[opno],
				     recog_data.operand_mode[opno], cop);
		}

	      result = true;
	    }
	}
    }

  return result;
}

/* APPLE LOCAL begin ARM 6008578 */
/* Return the bytes of padding that will be inserted to align
   the label INSN given the current pc ADDRESS.  */
static HOST_WIDE_INT get_label_pad (rtx insn, HOST_WIDE_INT address)
{
  int label_align, max_skip;
  unsigned HOST_WIDE_INT align_mask;
  int pad_needed;

  gcc_assert (LABEL_P (insn));

  label_align = LABEL_ALIGN_LOG (insn);
  max_skip = LABEL_MAX_SKIP (insn);
  align_mask = ((unsigned int) 1 << label_align) - 1;

  /* Already aligned.  */
  if ((address & align_mask) == 0)
    return 0;

  pad_needed = ((address | align_mask) + 1) - address;

  /* We would have to insert more than max_skip bytes to
     align this label.  */
  if (max_skip && (pad_needed > max_skip))
    return 0;

  return pad_needed;
}
/* APPLE LOCAL end ARM 6008578 */

/* Gcc puts the pool in the wrong place for ARM, since we can only
   load addresses a limited distance around the pc.  We do some
   special munging to move the constant pool values to the correct
   point in the code.  */
static void
arm_reorg (void)
{
  rtx insn;
  HOST_WIDE_INT address = 0;
  Mfix * fix;

  minipool_fix_head = minipool_fix_tail = NULL;

/* APPLE LOCAL begin ARM compact switch tables */
/* This is actually lurking bug I think, alignment matters.  */
  if (TARGET_THUMB)
    address = count_thumb_unexpanded_prologue ();
/* APPLE LOCAL end ARM compact switch tables */

  /* The first insn must always be a note, or the code below won't
     scan it properly.  */
  insn = get_insns ();
  gcc_assert (GET_CODE (insn) == NOTE);
  minipool_pad = 0;

  /* Scan all the insns and record the operands that will need fixing.  */
  for (insn = next_nonnote_insn (insn); insn; insn = next_nonnote_insn (insn))
    {
      if (TARGET_CIRRUS_FIX_INVALID_INSNS
          && (arm_cirrus_insn_p (insn)
	      || GET_CODE (insn) == JUMP_INSN
	      || arm_memory_load_p (insn)))
	cirrus_reorg (insn);

      if (GET_CODE (insn) == BARRIER)
	push_minipool_barrier (insn, address);
      /* APPLE LOCAL begin ARM 6008578 */
      else if (LABEL_P (insn))
	address += get_label_pad (insn, address);
      /* APPLE LOCAL end ARM 6008578 */
      else if (INSN_P (insn))
	{
	  rtx table;

	  note_invalid_constants (insn, address, true);
	  address += get_attr_length (insn);

	  /* If the insn is a vector jump, add the size of the table
	     and skip the table.  */
	  if ((table = is_jump_table (insn)) != NULL)
	    {
	      address += get_jump_table_size (table);
	      insn = table;
	    }
	}
    }

  fix = minipool_fix_head;

  /* Now scan the fixups and perform the required changes.  */
  while (fix)
    {
      Mfix * ftmp;
      Mfix * fdel;
      Mfix *  last_added_fix;
      Mfix * last_barrier = NULL;
      Mfix * this_fix;

      /* Skip any further barriers before the next fix.  */
      while (fix && GET_CODE (fix->insn) == BARRIER)
	fix = fix->next;

      /* No more fixes.  */
      if (fix == NULL)
	break;

      last_added_fix = NULL;

      for (ftmp = fix; ftmp; ftmp = ftmp->next)
	{
	  if (GET_CODE (ftmp->insn) == BARRIER)
	    {
	      if (ftmp->address >= minipool_vector_head->max_address)
		break;

	      last_barrier = ftmp;
	    }
	  else if ((ftmp->minipool = add_minipool_forward_ref (ftmp)) == NULL)
	    break;

	  last_added_fix = ftmp;  /* Keep track of the last fix added.  */
	}

      /* If we found a barrier, drop back to that; any fixes that we
	 could have reached but come after the barrier will now go in
	 the next mini-pool.  */
      if (last_barrier != NULL)
	{
	  /* Reduce the refcount for those fixes that won't go into this
	     pool after all.  */
	  for (fdel = last_barrier->next;
	       fdel && fdel != ftmp;
	       fdel = fdel->next)
	    {
	      fdel->minipool->refcount--;
	      fdel->minipool = NULL;
	    }

	  ftmp = last_barrier;
	}
      else
        {
	  /* ftmp is first fix that we can't fit into this pool and
	     there no natural barriers that we could use.  Insert a
	     new barrier in the code somewhere between the previous
	     fix and this one, and arrange to jump around it.  */
	  HOST_WIDE_INT max_address;

	  /* The last item on the list of fixes must be a barrier, so
	     we can never run off the end of the list of fixes without
	     last_barrier being set.  */
	  gcc_assert (ftmp);

	  max_address = minipool_vector_head->max_address;
	  /* Check that there isn't another fix that is in range that
	     we couldn't fit into this pool because the pool was
	     already too large: we need to put the pool before such an
	     instruction.  The pool itself may come just after the
	     fix because create_fix_barrier also allows space for a
	     jump instruction.  */
	  if (ftmp->address < max_address)
	    max_address = ftmp->address + 1;

	  last_barrier = create_fix_barrier (last_added_fix, max_address);
	}

      assign_minipool_offsets (last_barrier);

      while (ftmp)
	{
	  if (GET_CODE (ftmp->insn) != BARRIER
	      && ((ftmp->minipool = add_minipool_backward_ref (ftmp))
		  == NULL))
	    break;

	  ftmp = ftmp->next;
	}

      /* Scan over the fixes we have identified for this pool, fixing them
	 up and adding the constants to the pool itself.  */
      for (this_fix = fix; this_fix && ftmp != this_fix;
	   this_fix = this_fix->next)
	if (GET_CODE (this_fix->insn) != BARRIER)
	  {
	    rtx addr
	      = plus_constant (gen_rtx_LABEL_REF (VOIDmode,
						  minipool_vector_label),
			       this_fix->minipool->offset);
	    *this_fix->loc = gen_rtx_MEM (this_fix->mode, addr);
	  }

      dump_minipool (last_barrier->insn);
      fix = ftmp;
    }

  /* From now on we must synthesize any constants that we can't handle
     directly.  This can happen if the RTL gets split during final
     instruction generation.  */
  after_arm_reorg = 1;

  /* Free the minipool memory.  */
  obstack_free (&minipool_obstack, minipool_startobj);
}

/* Routines to output assembly language.  */

/* If the rtx is the correct value then return the string of the number.
   In this way we can ensure that valid double constants are generated even
   when cross compiling.  */
const char *
fp_immediate_constant (rtx x)
{
  REAL_VALUE_TYPE r;
  int i;

  if (!fp_consts_inited)
    init_fp_table ();

  REAL_VALUE_FROM_CONST_DOUBLE (r, x);
  for (i = 0; i < 8; i++)
    if (REAL_VALUES_EQUAL (r, values_fp[i]))
      return strings_fp[i];

  gcc_unreachable ();
}

/* As for fp_immediate_constant, but value is passed directly, not in rtx.  */
static const char *
fp_const_from_val (REAL_VALUE_TYPE *r)
{
  int i;

  if (!fp_consts_inited)
    init_fp_table ();

  for (i = 0; i < 8; i++)
    if (REAL_VALUES_EQUAL (*r, values_fp[i]))
      return strings_fp[i];

  gcc_unreachable ();
}

/* Output the operands of a LDM/STM instruction to STREAM.
   MASK is the ARM register set mask of which only bits 0-15 are important.
   REG is the base register, either the frame pointer or the stack pointer,
   INSTR is the possibly suffixed load or store instruction.  */

static void
print_multi_reg (FILE *stream, const char *instr, unsigned reg,
                unsigned long mask)
{
  unsigned i;
  bool not_first = FALSE;

  fputc ('\t', stream);
  asm_fprintf (stream, instr, reg);
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  fputc ('{', stream);

  for (i = 0; i <= LAST_ARM_REGNUM; i++)
    if (mask & (1 << i))
      {
        /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
        if (not_first)
          fprintf (stream, ", ");

        asm_fprintf (stream, "%r", i);
        not_first = TRUE;
        /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
      }

  fprintf (stream, "}\n");
}


/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Output a FLDMD instruction to STREAM.
   BASE if the register containing the address.
   REG and COUNT specify the register range.
   Extra registers may be added to avoid hardware bugs.

   We output FLDMD even for ARMv5 VFP implementations.  Although
   FLDMD is technically not supported until ARMv6, it is believed
   that all VFP implementations support its use in this context.  */

static void
vfp_output_fldmd (FILE * stream, unsigned int base, int reg, int count)
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
{
  int i;

  /* Workaround ARM10 VFPr1 bug.  */
  if (count == 2 && !arm_arch6)
    {
      if (reg == 15)
	reg--;
      count++;
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* FLDMD may not load more than 16 doubleword registers at a time. Split the
     load into multiple parts if we have to handle more than 16 registers.  */
  if (count > 16)
    {
      vfp_output_fldmd (stream, base, reg, 16);
      vfp_output_fldmd (stream, base, reg + 16, count - 16);
      return;
    }

  fputc ('\t', stream);
  asm_fprintf (stream, "fldmfdd\t%r!, {", base);
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

  for (i = reg; i < reg + count; i++)
    {
      if (i > reg)
	fputs (", ", stream);
      asm_fprintf (stream, "d%d", i);
    }
  fputs ("}\n", stream);

}


/* Output the assembly for a store multiple.  */

const char *
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
vfp_output_fstmd (rtx * operands)
{
  char pattern[100];
  int p;
  int base;
  int i;

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  strcpy (pattern, "fstmfdd\t%m0!, {%P1");
  p = strlen (pattern);

  gcc_assert (GET_CODE (operands[1]) == REG);

  base = (REGNO (operands[1]) - FIRST_VFP_REGNUM) / 2;
  for (i = 1; i < XVECLEN (operands[2], 0); i++)
    {
      p += sprintf (&pattern[p], ", d%d", base + i);
    }
  strcpy (&pattern[p], "}");

  output_asm_insn (pattern, operands);
  return "";
}


/* Emit RTL to save block of VFP register pairs to the stack.  Returns the
   number of bytes pushed.  */

static int
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
vfp_emit_fstmd (int base_reg, int count)
{
  rtx par;
  rtx dwarf;
  rtx tmp, reg;
  int i;

  /* Workaround ARM10 VFPr1 bug.  Data corruption can occur when exactly two
     register pairs are stored by a store multiple insn.  We avoid this
     by pushing an extra pair.  */
  if (count == 2 && !arm_arch6)
    {
      if (base_reg == LAST_VFP_REGNUM - 3)
	base_reg -= 2;
      count++;
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* FSTMD may not store more than 16 doubleword registers at once.  Split
     larger stores into multiple parts (up to a maximum of two, in
     practice).  */
  if (count > 16)
    {
      int saved;
      /* NOTE: base_reg is an internal register number, so each D register
         counts as 2.  */
      saved = vfp_emit_fstmd (base_reg + 32, count - 16);
      saved += vfp_emit_fstmd (base_reg, 16);
      return saved;
    }

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  par = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc (count));
  dwarf = gen_rtx_SEQUENCE (VOIDmode, rtvec_alloc (count + 1));

  reg = gen_rtx_REG (DFmode, base_reg);
  base_reg += 2;

  XVECEXP (par, 0, 0)
    = gen_rtx_SET (VOIDmode,
		   gen_frame_mem (BLKmode,
				  gen_rtx_PRE_DEC (BLKmode,
						   stack_pointer_rtx)),
		   gen_rtx_UNSPEC (BLKmode,
				   gen_rtvec (1, reg),
				   UNSPEC_PUSH_MULT));

  tmp = gen_rtx_SET (VOIDmode, stack_pointer_rtx,
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
		     plus_constant (stack_pointer_rtx, -(count * 8)));
  RTX_FRAME_RELATED_P (tmp) = 1;
  XVECEXP (dwarf, 0, 0) = tmp;

  tmp = gen_rtx_SET (VOIDmode,
		     gen_frame_mem (DFmode, stack_pointer_rtx),
		     reg);
  RTX_FRAME_RELATED_P (tmp) = 1;
  XVECEXP (dwarf, 0, 1) = tmp;

  for (i = 1; i < count; i++)
    {
      reg = gen_rtx_REG (DFmode, base_reg);
      base_reg += 2;
      XVECEXP (par, 0, i) = gen_rtx_USE (VOIDmode, reg);

      tmp = gen_rtx_SET (VOIDmode,
			 gen_frame_mem (DFmode,
					plus_constant (stack_pointer_rtx,
						       i * 8)),
			 reg);
      RTX_FRAME_RELATED_P (tmp) = 1;
      XVECEXP (dwarf, 0, i + 1) = tmp;
    }

  par = emit_insn (par);
  REG_NOTES (par) = gen_rtx_EXPR_LIST (REG_FRAME_RELATED_EXPR, dwarf,
				       REG_NOTES (par));
  RTX_FRAME_RELATED_P (par) = 1;

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  return count * 8;
}


/* Output a 'call' insn.  */
const char *
output_call (rtx *operands)
{
  gcc_assert (!arm_arch5); /* Patterns should call blx <reg> directly.  */

  /* Handle calls to lr using ip (which may be clobbered in subr anyway).  */
  if (REGNO (operands[0]) == LR_REGNUM)
    {
      operands[0] = gen_rtx_REG (SImode, IP_REGNUM);
      output_asm_insn ("mov%?\t%0, %|lr", operands);
    }

  output_asm_insn ("mov%?\t%|lr, %|pc", operands);

  if (TARGET_INTERWORK || arm_arch4t)
    output_asm_insn ("bx%?\t%0", operands);
  else
    output_asm_insn ("mov%?\t%|pc, %0", operands);

  return "";
}

/* Output a 'call' insn that is a reference in memory.  */
const char *
output_call_mem (rtx *operands)
{
  if (TARGET_INTERWORK && !arm_arch5)
    {
      output_asm_insn ("ldr%?\t%|ip, %0", operands);
      output_asm_insn ("mov%?\t%|lr, %|pc", operands);
      output_asm_insn ("bx%?\t%|ip", operands);
    }
  else if (regno_use_in (LR_REGNUM, operands[0]))
    {
      /* LR is used in the memory address.  We load the address in the
	 first instruction.  It's safe to use IP as the target of the
	 load since the call will kill it anyway.  */
      output_asm_insn ("ldr%?\t%|ip, %0", operands);
      if (arm_arch5)
	output_asm_insn ("blx%?\t%|ip", operands);
      else
	{
	  output_asm_insn ("mov%?\t%|lr, %|pc", operands);
	  if (arm_arch4t)
	    output_asm_insn ("bx%?\t%|ip", operands);
	  else
	    output_asm_insn ("mov%?\t%|pc, %|ip", operands);
	}
    }
  else
    {
      output_asm_insn ("mov%?\t%|lr, %|pc", operands);
      output_asm_insn ("ldr%?\t%|pc, %0", operands);
    }

  return "";
}


/* Output a move from arm registers to an fpa registers.
   OPERANDS[0] is an fpa register.
   OPERANDS[1] is the first registers of an arm register pair.  */
const char *
output_mov_long_double_fpa_from_arm (rtx *operands)
{
  int arm_reg0 = REGNO (operands[1]);
  rtx ops[3];

  gcc_assert (arm_reg0 != IP_REGNUM);

  ops[0] = gen_rtx_REG (SImode, arm_reg0);
  ops[1] = gen_rtx_REG (SImode, 1 + arm_reg0);
  ops[2] = gen_rtx_REG (SImode, 2 + arm_reg0);

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  output_asm_insn ("stm%(fd%)\t%|sp!, {%0, %1, %2}", ops);
  output_asm_insn ("ldf%?e\t%0, [%|sp], #12", operands);

  return "";
}

/* Output a move from an fpa register to arm registers.
   OPERANDS[0] is the first registers of an arm register pair.
   OPERANDS[1] is an fpa register.  */
const char *
output_mov_long_double_arm_from_fpa (rtx *operands)
{
  int arm_reg0 = REGNO (operands[0]);
  rtx ops[3];

  gcc_assert (arm_reg0 != IP_REGNUM);

  ops[0] = gen_rtx_REG (SImode, arm_reg0);
  ops[1] = gen_rtx_REG (SImode, 1 + arm_reg0);
  ops[2] = gen_rtx_REG (SImode, 2 + arm_reg0);

  output_asm_insn ("stf%?e\t%1, [%|sp, #-12]!", operands);
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  output_asm_insn ("ldm%(fd%)\t%|sp!, {%0, %1, %2}", ops);
  return "";
}

/* Output a move from arm registers to arm registers of a long double
   OPERANDS[0] is the destination.
   OPERANDS[1] is the source.  */
const char *
output_mov_long_double_arm_from_arm (rtx *operands)
{
  /* We have to be careful here because the two might overlap.  */
  int dest_start = REGNO (operands[0]);
  int src_start = REGNO (operands[1]);
  rtx ops[2];
  int i;

  if (dest_start < src_start)
    {
      for (i = 0; i < 3; i++)
	{
	  ops[0] = gen_rtx_REG (SImode, dest_start + i);
	  ops[1] = gen_rtx_REG (SImode, src_start + i);
	  output_asm_insn ("mov%?\t%0, %1", ops);
	}
    }
  else
    {
      for (i = 2; i >= 0; i--)
	{
	  ops[0] = gen_rtx_REG (SImode, dest_start + i);
	  ops[1] = gen_rtx_REG (SImode, src_start + i);
	  output_asm_insn ("mov%?\t%0, %1", ops);
	}
    }

  return "";
}


/* Output a move from arm registers to an fpa registers.
   OPERANDS[0] is an fpa register.
   OPERANDS[1] is the first registers of an arm register pair.  */
const char *
output_mov_double_fpa_from_arm (rtx *operands)
{
  int arm_reg0 = REGNO (operands[1]);
  rtx ops[2];

  gcc_assert (arm_reg0 != IP_REGNUM);

  ops[0] = gen_rtx_REG (SImode, arm_reg0);
  ops[1] = gen_rtx_REG (SImode, 1 + arm_reg0);
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  output_asm_insn ("stm%(fd%)\t%|sp!, {%0, %1}", ops);
  output_asm_insn ("ldf%?d\t%0, [%|sp], #8", operands);
  return "";
}

/* Output a move from an fpa register to arm registers.
   OPERANDS[0] is the first registers of an arm register pair.
   OPERANDS[1] is an fpa register.  */
const char *
output_mov_double_arm_from_fpa (rtx *operands)
{
  int arm_reg0 = REGNO (operands[0]);
  rtx ops[2];

  gcc_assert (arm_reg0 != IP_REGNUM);

  ops[0] = gen_rtx_REG (SImode, arm_reg0);
  ops[1] = gen_rtx_REG (SImode, 1 + arm_reg0);
  output_asm_insn ("stf%?d\t%1, [%|sp, #-8]!", operands);
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  output_asm_insn ("ldm%(fd%)\t%|sp!, {%0, %1}", ops);
  return "";
}

/* Output a move between double words.
   It must be REG<-REG, REG<-CONST_DOUBLE, REG<-CONST_INT, REG<-MEM
   or MEM<-REG and all MEMs must be offsettable addresses.  */
const char *
output_move_double (rtx *operands)
{
  enum rtx_code code0 = GET_CODE (operands[0]);
  enum rtx_code code1 = GET_CODE (operands[1]);
  rtx otherops[3];

  if (code0 == REG)
    {
      int reg0 = REGNO (operands[0]);

      otherops[0] = gen_rtx_REG (SImode, 1 + reg0);

      gcc_assert (code1 == MEM);  /* Constraints should ensure this.  */

      switch (GET_CODE (XEXP (operands[1], 0)))
	{
	case REG:
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("ldm%(ia%)\t%m1, %M0", operands);
	  break;

	case PRE_INC:
	  gcc_assert (TARGET_LDRD);
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("ldr%(d%)\t%0, [%m1, #8]!", operands);
	  break;

	case PRE_DEC:
          /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
	  if (TARGET_LDRD)
	    output_asm_insn ("ldr%(d%)\t%0, [%m1, #-8]!", operands);
	  else
	    output_asm_insn ("ldm%(db%)\t%m1!, %M0", operands);
          /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	  break;

	case POST_INC:
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("ldm%(ia%)\t%m1!, %M0", operands);
	  break;

	case POST_DEC:
	  gcc_assert (TARGET_LDRD);
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("ldr%(d%)\t%0, [%m1], #-8", operands);
	  break;

	case PRE_MODIFY:
	case POST_MODIFY:
	  otherops[0] = operands[0];
	  otherops[1] = XEXP (XEXP (XEXP (operands[1], 0), 1), 0);
	  otherops[2] = XEXP (XEXP (XEXP (operands[1], 0), 1), 1);

          /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
	  if (GET_CODE (XEXP (operands[1], 0)) == PRE_MODIFY)
	    {
	      if (reg_overlap_mentioned_p (otherops[0], otherops[2]))
		{
		  /* Registers overlap so split out the increment.  */
		  output_asm_insn ("add%?\t%1, %1, %2", otherops);
		  output_asm_insn ("ldr%(d%)\t%0, [%1] @split", otherops);
		}
	      else
		{
		  /* IWMMXT allows offsets larger than ARM ldrd can handle,
		     fix these up with a pair of ldr.  */
		  if (TARGET_ARM && GET_CODE (otherops[2]) == CONST_INT
		      && (INTVAL(otherops[2]) <= -256
			  || INTVAL(otherops[2]) >= 256))
		    {
		      output_asm_insn ("ldr%?\t%0, [%1, %2]!", otherops);
		      otherops[0] = gen_rtx_REG (SImode, 1 + reg0);
		      output_asm_insn ("ldr%?\t%0, [%1, #4]", otherops);
		    }
		  else
		    output_asm_insn ("ldr%(d%)\t%0, [%1, %2]!", otherops);
		}
	    }
	  else
	    {
	      /* IWMMXT allows offsets larger than ARM ldrd can handle,
		 fix these up with a pair of ldr.  */
	      if (TARGET_ARM && GET_CODE (otherops[2]) == CONST_INT
		  && (INTVAL(otherops[2]) <= -256
		      || INTVAL(otherops[2]) >= 256))
		{
		  otherops[0] = gen_rtx_REG (SImode, 1 + reg0);
		  output_asm_insn ("ldr%?\t%0, [%1, #4]", otherops);
		  otherops[0] = operands[0];
		  output_asm_insn ("ldr%?\t%0, [%1], %2", otherops);
		}
	      else
		/* We only allow constant increments, so this is safe.  */
		output_asm_insn ("ldr%(d%)\t%0, [%1], %2", otherops);
	    }
          /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	  break;

	case LABEL_REF:
	case CONST:
	  output_asm_insn ("adr%?\t%0, %1", operands);
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("ldm%(ia%)\t%0, %M0", operands);
	  break;

          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
          /* ??? This needs checking for thumb2.  */
	default:
	  if (arm_add_operand (XEXP (XEXP (operands[1], 0), 1),
			       GET_MODE (XEXP (XEXP (operands[1], 0), 1))))
	    {
	      otherops[0] = operands[0];
	      otherops[1] = XEXP (XEXP (operands[1], 0), 0);
	      otherops[2] = XEXP (XEXP (operands[1], 0), 1);

	      if (GET_CODE (XEXP (operands[1], 0)) == PLUS)
		{
		  if (GET_CODE (otherops[2]) == CONST_INT)
		    {
                      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
		      switch ((int) INTVAL (otherops[2]))
			{
			case -8:
			  output_asm_insn ("ldm%(db%)\t%1, %M0", otherops);
			  return "";
			case -4:
			  if (TARGET_THUMB2)
			    break;
			  output_asm_insn ("ldm%(da%)\t%1, %M0", otherops);
			  return "";
			case 4:
			  if (TARGET_THUMB2)
			    break;
			  output_asm_insn ("ldm%(ib%)\t%1, %M0", otherops);
			  return "";
			}
                      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
		    }
		  if (TARGET_LDRD
		      && (GET_CODE (otherops[2]) == REG
			  || (GET_CODE (otherops[2]) == CONST_INT
			      && INTVAL (otherops[2]) > -256
			      && INTVAL (otherops[2]) < 256)))
		    {
		      if (reg_overlap_mentioned_p (otherops[0],
						   otherops[2]))
			{
			  /* Swap base and index registers over to
			     avoid a conflict.  */
			  otherops[1] = XEXP (XEXP (operands[1], 0), 1);
			  otherops[2] = XEXP (XEXP (operands[1], 0), 0);
			}
		      /* If both registers conflict, it will usually
			 have been fixed by a splitter.  */
		      if (reg_overlap_mentioned_p (otherops[0], otherops[2]))
			{
			  output_asm_insn ("add%?\t%1, %1, %2", otherops);
                          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
			  output_asm_insn ("ldr%(d%)\t%0, [%1]",
					   otherops);
			}
		      else
                        /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
			output_asm_insn ("ldr%(d%)\t%0, [%1, %2]", otherops);
		      return "";
		    }

		  if (GET_CODE (otherops[2]) == CONST_INT)
		    {
		      if (!(const_ok_for_arm (INTVAL (otherops[2]))))
			output_asm_insn ("sub%?\t%0, %1, #%n2", otherops);
		      else
			output_asm_insn ("add%?\t%0, %1, %2", otherops);
		    }
		  else
		    output_asm_insn ("add%?\t%0, %1, %2", otherops);
		}
	      else
		output_asm_insn ("sub%?\t%0, %1, %2", otherops);

              /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	      return "ldm%(ia%)\t%0, %M0";
	    }
	  else
	    {
	      otherops[1] = adjust_address (operands[1], SImode, 4);
	      /* Take care of overlapping base/data reg.  */
	      if (reg_mentioned_p (operands[0], operands[1]))
		{
		  output_asm_insn ("ldr%?\t%0, %1", otherops);
		  output_asm_insn ("ldr%?\t%0, %1", operands);
		}
	      else
		{
		  output_asm_insn ("ldr%?\t%0, %1", operands);
		  output_asm_insn ("ldr%?\t%0, %1", otherops);
		}
	    }
	}
    }
  else
    {
      /* Constraints should ensure this.  */
      gcc_assert (code0 == MEM && code1 == REG);
      gcc_assert (REGNO (operands[1]) != IP_REGNUM);

      switch (GET_CODE (XEXP (operands[0], 0)))
        {
	case REG:
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("stm%(ia%)\t%m0, %M1", operands);
	  break;

        case PRE_INC:
	  gcc_assert (TARGET_LDRD);
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("str%(d%)\t%1, [%m0, #8]!", operands);
	  break;

        case PRE_DEC:
          /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
	  if (TARGET_LDRD)
	    output_asm_insn ("str%(d%)\t%1, [%m0, #-8]!", operands);
	  else
	    output_asm_insn ("stm%(db%)\t%m0!, %M1", operands);
          /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	  break;

        case POST_INC:
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("stm%(ia%)\t%m0!, %M1", operands);
	  break;

        case POST_DEC:
	  gcc_assert (TARGET_LDRD);
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  output_asm_insn ("str%(d%)\t%1, [%m0], #-8", operands);
	  break;

	case PRE_MODIFY:
	case POST_MODIFY:
	  otherops[0] = operands[1];
	  otherops[1] = XEXP (XEXP (XEXP (operands[0], 0), 1), 0);
	  otherops[2] = XEXP (XEXP (XEXP (operands[0], 0), 1), 1);

          /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
	  /* IWMMXT allows offsets larger than ARM ldrd can handle,
	     fix these up with a pair of ldr.  */
	  if (TARGET_ARM && GET_CODE (otherops[2]) == CONST_INT
          /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	      && (INTVAL(otherops[2]) <= -256
		  || INTVAL(otherops[2]) >= 256))
	    {
	      rtx reg1;
	      reg1 = gen_rtx_REG (SImode, 1 + REGNO (operands[1]));
	      if (GET_CODE (XEXP (operands[0], 0)) == PRE_MODIFY)
		{
		  output_asm_insn ("ldr%?\t%0, [%1, %2]!", otherops);
		  otherops[0] = reg1;
		  output_asm_insn ("ldr%?\t%0, [%1, #4]", otherops);
		}
	      else
		{
		  otherops[0] = reg1;
		  output_asm_insn ("ldr%?\t%0, [%1, #4]", otherops);
		  otherops[0] = operands[1];
		  output_asm_insn ("ldr%?\t%0, [%1], %2", otherops);
		}
	    }
	  else if (GET_CODE (XEXP (operands[0], 0)) == PRE_MODIFY)
            /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	    output_asm_insn ("str%(d%)\t%0, [%1, %2]!", otherops);
	  else
            /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	    output_asm_insn ("str%(d%)\t%0, [%1], %2", otherops);
	  break;

	case PLUS:
	  otherops[2] = XEXP (XEXP (operands[0], 0), 1);
	  if (GET_CODE (otherops[2]) == CONST_INT)
	    {
              /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
	      switch ((int) INTVAL (XEXP (XEXP (operands[0], 0), 1)))
		{
		case -8:
		  output_asm_insn ("stm%(db%)\t%m0, %M1", operands);
		  return "";

		case -4:
		  if (TARGET_THUMB2)
		    break;
		  output_asm_insn ("stm%(da%)\t%m0, %M1", operands);
		  return "";

		case 4:
		  if (TARGET_THUMB2)
		    break;
		  output_asm_insn ("stm%(ib%)\t%m0, %M1", operands);
		  return "";
		}
              /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	    }
	  if (TARGET_LDRD
	      && (GET_CODE (otherops[2]) == REG
		  || (GET_CODE (otherops[2]) == CONST_INT
		      && INTVAL (otherops[2]) > -256
		      && INTVAL (otherops[2]) < 256)))
	    {
	      otherops[0] = operands[1];
	      otherops[1] = XEXP (XEXP (operands[0], 0), 0);
              /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	      output_asm_insn ("str%(d%)\t%0, [%1, %2]", otherops);
	      return "";
	    }
	  /* Fall through */

        default:
	  otherops[0] = adjust_address (operands[0], SImode, 4);
	  otherops[1] = gen_rtx_REG (SImode, 1 + REGNO (operands[1]));
	  output_asm_insn ("str%?\t%1, %0", operands);
	  output_asm_insn ("str%?\t%1, %0", otherops);
	}
    }

  return "";
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
/* Output a move, load or store for quad-word vectors in ARM registers.  Only
   handles MEMs accepted by neon_vector_mem_operand with CORE=true.  */

const char *
output_move_quad (rtx *operands)
{
  if (REG_P (operands[0]))
    {
      /* Load, or reg->reg move.  */

      if (MEM_P (operands[1]))
        {
          switch (GET_CODE (XEXP (operands[1], 0)))
            {
            case REG:
              output_asm_insn ("ldm%(ia%)\t%m1, %M0", operands);
              break;

            case LABEL_REF:
            case CONST:
              output_asm_insn ("adr%?\t%0, %1", operands);
              output_asm_insn ("ldm%(ia%)\t%0, %M0", operands);
              break;

            default:
              gcc_unreachable ();
            }
        }
      else
        {
          rtx ops[2];
          int dest, src, i;

          gcc_assert (REG_P (operands[1]));

          dest = REGNO (operands[0]);
          src = REGNO (operands[1]);
          
          /* This seems pretty dumb, but hopefully GCC won't try to do it
             very often.  */
          if (dest < src)
            for (i = 0; i < 4; i++)
              {
                ops[0] = gen_rtx_REG (SImode, dest + i);
                ops[1] = gen_rtx_REG (SImode, src + i);
                output_asm_insn ("mov%?\t%0, %1", ops);
              }
          else
            for (i = 3; i >= 0; i--)
              {
                ops[0] = gen_rtx_REG (SImode, dest + i);
                ops[1] = gen_rtx_REG (SImode, src + i);
                output_asm_insn ("mov%?\t%0, %1", ops);
              }
        }
    }
  else 
    {
      gcc_assert (MEM_P (operands[0]));
      gcc_assert (REG_P (operands[1]));
      gcc_assert (!reg_overlap_mentioned_p (operands[1], operands[0]));
      
      switch (GET_CODE (XEXP (operands[0], 0)))
        {
        case REG:
          output_asm_insn ("stm%(ia%)\t%m0, %M1", operands);
          break;

        default:
          gcc_unreachable ();
        }
    }
  
  return "";
}

/* Output a VFP load or store instruction.  */

const char *
output_move_vfp (rtx *operands)
{
  rtx reg, mem, addr, ops[2];
  int load = REG_P (operands[0]);
  int dp = GET_MODE_SIZE (GET_MODE (operands[0])) == 8;
  int integer_p = GET_MODE_CLASS (GET_MODE (operands[0])) == MODE_INT;
  const char *template;
  char buff[50];
  enum machine_mode mode;

  reg = operands[!load];
  mem = operands[load];

  mode = GET_MODE (reg);

  gcc_assert (REG_P (reg));
  gcc_assert (IS_VFP_REGNUM (REGNO (reg)));
  gcc_assert (mode == SFmode
	      || mode == DFmode
	      || mode == SImode
	      || mode == DImode
              || (TARGET_NEON && VALID_NEON_DREG_MODE (mode)));
  gcc_assert (MEM_P (mem));

  addr = XEXP (mem, 0);

  switch (GET_CODE (addr))
    {
    case PRE_DEC:
      template = "f%smdb%c%%?\t%%0!, {%%%s1}%s";
      ops[0] = XEXP (addr, 0);
      ops[1] = reg;
      break;

    case POST_INC:
      template = "f%smia%c%%?\t%%0!, {%%%s1}%s";
      ops[0] = XEXP (addr, 0);
      ops[1] = reg;
      break;

    default:
      template = "f%s%c%%?\t%%%s0, %%1%s";
      ops[0] = reg;
      ops[1] = mem;
      break;
    }

  sprintf (buff, template,
	   load ? "ld" : "st",
	   dp ? 'd' : 's',
	   dp ? "P" : "",
	   integer_p ? "\t%@ int" : "");
  output_asm_insn (buff, ops);

  return "";
}

/* Output a Neon quad-word load or store, or a load or store for
   larger structure modes. We could also support post-modify
   forms using VLD1/VST1, but we don't do that yet.
   WARNING, FIXME: The ordering of elements in memory is going to be weird in
   big-endian mode at present, because we use VSTM instead of VST1, to make
   it easy to make vector stores via ARM registers write values in the same
   order as stores direct from Neon registers.  For example, the byte ordering
   of a quadword vector with 16-byte elements like this:

     [e7:e6:e5:e4:e3:e2:e1:e0]  (highest-numbered element first)

   will be (with lowest address first, h = most-significant byte,
   l = least-significant byte of element):

     [e3h, e3l, e2h, e2l, e1h, e1l, e0h, e0l,
      e7h, e7l, e6h, e6l, e5h, e5l, e4h, e4l]
   
   When necessary, quadword registers (dN, dN+1) are moved to ARM registers from
   rN in the order:
   
     dN -> (rN+1, rN), dN+1 -> (rN+3, rN+2)
   
   So that STM/LDM can be used on vectors in ARM registers, and the same memory
   layout will result as if VSTM/VLDM were used.

   This memory format (in BE mode) is very likely to change in the future.  */

const char *
output_move_neon (rtx *operands)
{
  rtx reg, mem, addr, ops[2];
  int regno, load = REG_P (operands[0]);
  const char *template;
  char buff[50];
  enum machine_mode mode;
  
  reg = operands[!load];
  mem = operands[load];
  
  mode = GET_MODE (reg);
  
  gcc_assert (REG_P (reg));
  regno = REGNO (reg);
  gcc_assert (VFP_REGNO_OK_FOR_DOUBLE (regno)
	      || NEON_REGNO_OK_FOR_QUAD (regno));
  gcc_assert (VALID_NEON_DREG_MODE (mode)
	      || VALID_NEON_QREG_MODE (mode)
	      || VALID_NEON_STRUCT_MODE (mode));
  gcc_assert (MEM_P (mem));
  
  addr = XEXP (mem, 0);
  
  /* Strip off const from addresses like (const (plus (...))).  */
  if (GET_CODE (addr) == CONST && GET_CODE (XEXP (addr, 0)) == PLUS)
    addr = XEXP (addr, 0);
  
  switch (GET_CODE (addr))
    {
    case POST_INC:
      /* FIXME: We should be using vld1/vst1 here in BE mode?  */
      template = "v%smia%%?\t%%0!, %%h1";
      ops[0] = XEXP (addr, 0);
      ops[1] = reg;
      break;
    
    case POST_MODIFY:
      /* FIXME: Not currently enabled in neon_vector_mem_operand.  */
      gcc_unreachable ();

    case LABEL_REF:
    case PLUS:
      {
	int nregs = HARD_REGNO_NREGS (REGNO (reg), mode) / 2;
	int i;
	int overlap = -1;
	for (i = 0; i < nregs; i++)
	  {
	    /* We're only using DImode here because it's a convenient size.
	       FIXME: This will need updating if the memory format of vectors
	       changes.  */
	    ops[0] = gen_rtx_REG (DImode, REGNO (reg) + 2 * i);
	    ops[1] = adjust_address (mem, SImode, 8 * i);
	    if (reg_overlap_mentioned_p (ops[0], mem))
	      {
		gcc_assert (overlap == -1);
		overlap = i;
	      }
	    else
	      {
		sprintf (buff, "v%sr%%?\t%%P0, %%1", load ? "ld" : "st");
		output_asm_insn (buff, ops);
	      }
	  }
	if (overlap != -1)
	  {
	    ops[0] = gen_rtx_REG (DImode, REGNO (reg) + 2 * overlap);
	    ops[1] = adjust_address (mem, SImode, 8 * overlap);
	    sprintf (buff, "v%sr%%?\t%%P0, %%1", load ? "ld" : "st");
	    output_asm_insn (buff, ops);
	  }

        return "";
      }

    default:
      /* FIXME: See POST_INC.  */
      template = "v%smia%%?\t%%m0, %%h1";
      ops[0] = mem;
      ops[1] = reg;
    }
  
  sprintf (buff, template, load ? "ld" : "st");
  output_asm_insn (buff, ops);
  
  return "";
}

/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
/* Output an ADD r, s, #n where n may be too big for one instruction.
   If adding zero to one register, output nothing.  */
const char *
output_add_immediate (rtx *operands)
{
  HOST_WIDE_INT n = INTVAL (operands[2]);

  if (n != 0 || REGNO (operands[0]) != REGNO (operands[1]))
    {
      if (n < 0)
	output_multi_immediate (operands,
				"sub%?\t%0, %1, %2", "sub%?\t%0, %0, %2", 2,
				-n);
      else
	output_multi_immediate (operands,
				"add%?\t%0, %1, %2", "add%?\t%0, %0, %2", 2,
				n);
    }

  return "";
}

/* Output a multiple immediate operation.
   OPERANDS is the vector of operands referred to in the output patterns.
   INSTR1 is the output pattern to use for the first constant.
   INSTR2 is the output pattern to use for subsequent constants.
   IMMED_OP is the index of the constant slot in OPERANDS.
   N is the constant value.  */
static const char *
output_multi_immediate (rtx *operands, const char *instr1, const char *instr2,
			int immed_op, HOST_WIDE_INT n)
{
#if HOST_BITS_PER_WIDE_INT > 32
  n &= 0xffffffff;
#endif

  if (n == 0)
    {
      /* Quick and easy output.  */
      operands[immed_op] = const0_rtx;
      output_asm_insn (instr1, operands);
    }
  else
    {
      int i;
      const char * instr = instr1;

      /* Note that n is never zero here (which would give no output).  */
      for (i = 0; i < 32; i += 2)
	{
	  if (n & (3 << i))
	    {
	      operands[immed_op] = GEN_INT (n & (255 << i));
	      output_asm_insn (instr, operands);
	      instr = instr2;
	      i += 6;
	    }
	}
    }

  return "";
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Return the name of a shifter operation.  */
static const char *
arm_shift_nmem(enum rtx_code code)
{
  switch (code)
    {
    case ASHIFT:
      return ARM_LSL_NAME;

    case ASHIFTRT:
      return "asr";

    case LSHIFTRT:
      return "lsr";

    case ROTATERT:
      return "ror";

    default:
      abort();
    }
}

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* Return the appropriate ARM instruction for the operation code.
   The returned result should not be overwritten.  OP is the rtx of the
   operation.  SHIFT_FIRST_ARG is TRUE if the first argument of the operator
   was shifted.  */
const char *
arithmetic_instr (rtx op, int shift_first_arg)
{
  switch (GET_CODE (op))
    {
    case PLUS:
      return "add";

    case MINUS:
      return shift_first_arg ? "rsb" : "sub";

    case IOR:
      return "orr";

    case XOR:
      return "eor";

    case AND:
      return "and";

    case ASHIFT:
    case ASHIFTRT:
    case LSHIFTRT:
    case ROTATERT:
      return arm_shift_nmem(GET_CODE(op));

    default:
      gcc_unreachable ();
    }
}

/* Ensure valid constant shifts and return the appropriate shift mnemonic
   for the operation code.  The returned result should not be overwritten.
   OP is the rtx code of the shift.
   On exit, *AMOUNTP will be -1 if the shift is by a register, or a constant
   shift.  */
static const char *
shift_op (rtx op, HOST_WIDE_INT *amountp)
{
  const char * mnem;
  enum rtx_code code = GET_CODE (op);

  switch (GET_CODE (XEXP (op, 1)))
    {
    case REG:
    case SUBREG:
      *amountp = -1;
      break;

    case CONST_INT:
      *amountp = INTVAL (XEXP (op, 1));
      break;

    default:
      gcc_unreachable ();
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  switch (code)
    {
    case ROTATE:
      gcc_assert (*amountp != -1);
      *amountp = 32 - *amountp;
      code = ROTATERT;

      /* Fall through.  */

    case ASHIFT:
    case ASHIFTRT:
    case LSHIFTRT:
    case ROTATERT:
      mnem = arm_shift_nmem(code);
      break;

    case MULT:
      /* We never have to worry about the amount being other than a
	 power of 2, since this case can never be reloaded from a reg.  */
      gcc_assert (*amountp != -1);
      *amountp = int_log2 (*amountp);
      return ARM_LSL_NAME;

    default:
      gcc_unreachable ();
    }

  if (*amountp != -1)
    {
      /* This is not 100% correct, but follows from the desire to merge
	 multiplication by a power of 2 with the recognizer for a
	 shift.  >=32 is not a valid shift for "lsl", so we must try and
	 output a shift that produces the correct arithmetical result.
	 Using lsr #32 is identical except for the fact that the carry bit
	 is not set correctly if we set the flags; but we never use the
	 carry bit from such an operation, so we can ignore that.  */
      if (code == ROTATERT)
	/* Rotate is just modulo 32.  */
	*amountp &= 31;
      else if (*amountp != (*amountp & 31))
	{
	  if (code == ASHIFT)
	    mnem = "lsr";
	  *amountp = 32;
	}

      /* Shifts of 0 are no-ops.  */
      if (*amountp == 0)
	return NULL;
    }
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

  return mnem;
}

/* Obtain the shift from the POWER of two.  */

static HOST_WIDE_INT
int_log2 (HOST_WIDE_INT power)
{
  HOST_WIDE_INT shift = 0;

  while ((((HOST_WIDE_INT) 1 << shift) & power) == 0)
    {
      gcc_assert (shift <= 31);
      shift++;
    }

  return shift;
}

/* Output a .ascii pseudo-op, keeping track of lengths.  This is
   because /bin/as is horribly restrictive.  The judgement about
   whether or not each character is 'printable' (and can be output as
   is) or not (and must be printed with an octal escape) must be made
   with reference to the *host* character set -- the situation is
   similar to that discussed in the comments above pp_c_char in
   c-pretty-print.c.  */

#define MAX_ASCII_LEN 51

void
output_ascii_pseudo_op (FILE *stream, const unsigned char *p, int len)
{
  int i;
  int len_so_far = 0;

  fputs ("\t.ascii\t\"", stream);

  for (i = 0; i < len; i++)
    {
      int c = p[i];

      if (len_so_far >= MAX_ASCII_LEN)
	{
	  fputs ("\"\n\t.ascii\t\"", stream);
	  len_so_far = 0;
	}

      if (ISPRINT (c))
	{
	  if (c == '\\' || c == '\"')
	    {
	      putc ('\\', stream);
	      len_so_far++;
	    }
	  putc (c, stream);
	  len_so_far++;
	}
      else
	{
	  fprintf (stream, "\\%03o", c);
	  len_so_far += 4;
	}
    }

  fputs ("\"\n", stream);
}

/* Compute the register save mask for registers 0 through 12
   inclusive.  This code is used by arm_compute_save_reg_mask.  */

static unsigned long
arm_compute_save_reg0_reg12_mask (void)
{
  unsigned long func_type = arm_current_func_type ();
  unsigned long save_reg_mask = 0;
  unsigned int reg;

  if (IS_INTERRUPT (func_type))
    {
      unsigned int max_reg;
      /* Interrupt functions must not corrupt any registers,
	 even call clobbered ones.  If this is a leaf function
	 we can just examine the registers used by the RTL, but
	 otherwise we have to assume that whatever function is
	 called might clobber anything, and so we have to save
	 all the call-clobbered registers as well.  */
      if (ARM_FUNC_TYPE (func_type) == ARM_FT_FIQ)
	/* FIQ handlers have registers r8 - r12 banked, so
	   we only need to check r0 - r7, Normal ISRs only
	   bank r14 and r15, so we must check up to r12.
	   r13 is the stack pointer which is always preserved,
	   so we do not need to consider it here.  */
	max_reg = 7;
      else
	max_reg = 12;

      for (reg = 0; reg <= max_reg; reg++)
	if (regs_ever_live[reg]
	    || (! current_function_is_leaf && call_used_regs [reg]))
	  save_reg_mask |= (1 << reg);

      /* Also save the pic base register if necessary.  */
      if (flag_pic
	  && !TARGET_SINGLE_PIC_BASE
	  && arm_pic_register != INVALID_REGNUM
	  && current_function_uses_pic_offset_table)
	save_reg_mask |= 1 << PIC_OFFSET_TABLE_REGNUM;
    }
  else
    {
      /* APPLE LOCAL begin ARM custom frame layout */
      /* In the normal case we only need to save those registers
	 which are call saved and which are used by this function.  */
      for (reg = 0; reg <= 11; reg++)
	if (regs_ever_live[reg] && ! call_used_regs [reg])
	  save_reg_mask |= (1 << reg);

      /* Handle the frame pointer as a special case.  */
      if (frame_pointer_needed)
	save_reg_mask |= 1 << HARD_FRAME_POINTER_REGNUM;
      /* APPLE LOCAL end ARM use custom frame layout */

      /* If we aren't loading the PIC register,
	 don't stack it even though it may be live.  */
      if (flag_pic
	  && !TARGET_SINGLE_PIC_BASE
	  && arm_pic_register != INVALID_REGNUM
	  && (regs_ever_live[PIC_OFFSET_TABLE_REGNUM]
	      || current_function_uses_pic_offset_table))
	save_reg_mask |= 1 << PIC_OFFSET_TABLE_REGNUM;

      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      /* The prologue will copy SP into R0, so save it.  */
      if (IS_STACKALIGN (func_type))
	save_reg_mask |= 1;
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    }

  /* Save registers so the exception handler can modify them.  */
  if (current_function_calls_eh_return)
    {
      unsigned int i;

      for (i = 0; ; i++)
	{
	  reg = EH_RETURN_DATA_REGNO (i);
	  if (reg == INVALID_REGNUM)
	    break;
	  save_reg_mask |= 1 << reg;
	}
    }

  return save_reg_mask;
}

/* Compute a bit mask of which registers need to be
   saved on the stack for the current function.  */

static unsigned long
arm_compute_save_reg_mask (void)
{
  unsigned int save_reg_mask = 0;
  unsigned long func_type = arm_current_func_type ();
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  unsigned int reg;

  if (IS_NAKED (func_type))
    /* This should never really happen.  */
    return 0;

  /* APPLE LOCAL begin ARM use custom frame layout */
  /* Volatile functions do not return, so there
     is no need to save any other registers.  */
  if (!IS_VOLATILE (func_type))
    save_reg_mask |= arm_compute_save_reg0_reg12_mask ();
  /* APPLE LOCAL end ARM use custom frame layout */

  /* Decide if we need to save the link register.
     Interrupt routines have their own banked link register,
     so they never need to save it.
     Otherwise if we do not use the link register we do not need to save
     it.  If we are pushing other registers onto the stack however, we
     can save an instruction in the epilogue by pushing the link register
     now and then popping it back into the PC.  This incurs extra memory
     accesses though, so we only do it when optimizing for size, and only
     if we know that we will not need a fancy return sequence.  */
  if (regs_ever_live [LR_REGNUM]
	  || (save_reg_mask
	      && optimize_size
	      && ARM_FUNC_TYPE (func_type) == ARM_FT_NORMAL
	      && !current_function_calls_eh_return))
    save_reg_mask |= 1 << LR_REGNUM;

  if (cfun->machine->lr_save_eliminated)
    save_reg_mask &= ~ (1 << LR_REGNUM);

  /* APPLE LOCAL begin ARM custom frame layout */
  if (frame_pointer_needed)
    save_reg_mask |= (1 << LR_REGNUM | 1 << HARD_FRAME_POINTER_REGNUM);
  /* APPLE LOCAL end ARM custom frame layout */

  if (TARGET_REALLY_IWMMXT
      /* APPLE LOCAL ARM custom frame layout */
      && (!IS_VOLATILE (func_type))
      && ((bit_count (save_reg_mask)
	   + ARM_NUM_INTS (current_function_pretend_args_size)) % 2) != 0)
    {
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      /* moved definition of 'reg' to function level scope */
      /* The total number of registers that are going to be pushed
	 onto the stack is odd.  We need to ensure that the stack
	 is 64-bit aligned before we start to save iWMMXt registers,
	 and also before we start to create locals.  (A local variable
	 might be a double or long long which we will load/store using
	 an iWMMXt instruction).  Therefore we need to push another
	 ARM register, so that the stack will be 64-bit aligned.  We
	 try to avoid using the arg registers (r0 -r3) as they might be
	 used to pass values in a tail call.  */
      for (reg = 4; reg <= 12; reg++)
	if ((save_reg_mask & (1 << reg)) == 0)
	  break;

      if (reg <= 12)
	save_reg_mask |= (1 << reg);
      else
	{
	  cfun->machine->sibcall_blocked = 1;
	  save_reg_mask |= (1 << 3);
	}
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* We may need to push an additional register for use initializing the
     PIC base register.  */
  if (TARGET_THUMB2 && IS_NESTED (func_type) && flag_pic
      && (save_reg_mask & THUMB2_WORK_REGS) == 0)
    {
      reg = thumb_find_work_register (1 << 4);
      if (!call_used_regs[reg])
	save_reg_mask |= (1 << reg);
    }

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  return save_reg_mask;
}


/* Compute a bit mask of which registers need to be
   saved on the stack for the current function.  */
static unsigned long
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
thumb1_compute_save_reg_mask (void)
{
  unsigned long mask;
  unsigned reg;

  mask = 0;
  for (reg = 0; reg < 12; reg ++)
    if (regs_ever_live[reg] && !call_used_regs[reg])
      mask |= 1 << reg;

  /* APPLE LOCAL begin ARM thumb requires FP */
  if (frame_pointer_needed)
    mask |= 1 << THUMB_HARD_FRAME_POINTER_REGNUM;
  /* APPLE LOCAL end ARM thumb requires FP */

  if (flag_pic
      && !TARGET_SINGLE_PIC_BASE
      && arm_pic_register != INVALID_REGNUM
      && current_function_uses_pic_offset_table)
    mask |= 1 << PIC_OFFSET_TABLE_REGNUM;

  /* See if we might need r11 for calls to _interwork_r11_call_via_rN().  */
  if (!frame_pointer_needed && CALLER_INTERWORKING_SLOT_SIZE > 0)
    mask |= 1 << ARM_HARD_FRAME_POINTER_REGNUM;

  /* LR will also be pushed if any lo regs are pushed.  */
  if (mask & 0xff || thumb_force_lr_save ())
    mask |= (1 << LR_REGNUM);

  /* Make sure we have a low work register if we need one.
     We will need one if we are going to push a high register,
     but we are not currently intending to push a low register.  */
  if ((mask & 0xff) == 0
      && ((mask & 0x0f00) || TARGET_BACKTRACE))
    {
      /* Use thumb_find_work_register to choose which register
	 we will use.  If the register is live then we will
	 have to push it.  Use LAST_LO_REGNUM as our fallback
	 choice for the register to select.  */
      /* APPLE LOCAL ARM thumb requires FP */
      reg = thumb_find_work_register (1 << (LAST_LO_REGNUM - 1));

      if (! call_used_regs[reg])
	mask |= 1 << reg;
    }

  /* APPLE LOCAL begin ARM custom frame layout */
  /* Also need a scratch register in the case where the frame size is
     too big for the subtract instruction.  This is not exactly the right
     computation for frame size, there's a circular dependency on which
     registers get saved, but it should catch most of the problem cases
     and there is (very inefficient) code to handle the rare case where
     we didn't allocate a scratch reg and need one. */
  if (frame_pointer_needed && ((mask & 0x70) == 0)
      && (ROUND_UP_WORD (get_frame_size ())
            + current_function_outgoing_args_size) >= 512)
    mask |= 1 << (LAST_LO_REGNUM - 1);
  /* APPLE LOCAL end ARM custom frame layout */

  return mask;
}


/* Return the number of bytes required to save VFP registers.  */
static int
arm_get_vfp_saved_size (void)
{
  unsigned int regno;
  int count;
  int saved;

  saved = 0;
  /* Space for saved VFP registers.  */
  if (TARGET_HARD_FLOAT && TARGET_VFP)
    {
      count = 0;
      for (regno = FIRST_VFP_REGNUM;
	   regno < LAST_VFP_REGNUM;
	   regno += 2)
	{
	  if ((!regs_ever_live[regno] || call_used_regs[regno])
	      && (!regs_ever_live[regno + 1] || call_used_regs[regno + 1]))
	    {
	      if (count > 0)
		{
		  /* Workaround ARM10 VFPr1 bug.  */
		  if (count == 2 && !arm_arch6)
		    count++;
                  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
		  saved += count * 8;
		}
	      count = 0;
	    }
	  else
	    count++;
	}
      if (count > 0)
	{
	  if (count == 2 && !arm_arch6)
	    count++;
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  saved += count * 8;
	}
    }
  return saved;
}


/* Generate a function exit sequence.  If REALLY_RETURN is false, then do
   everything bar the final return instruction.  */
const char *
output_return_instruction (rtx operand, int really_return, int reverse)
{
  char conditional[10];
  char instr[100];
  unsigned reg;
  unsigned long live_regs_mask;
  unsigned long func_type;
  arm_stack_offsets *offsets;

  func_type = arm_current_func_type ();

  if (IS_NAKED (func_type))
    return "";

  if (IS_VOLATILE (func_type) && TARGET_ABORT_NORETURN)
    {
      /* If this function was declared non-returning, and we have
	 found a tail call, then we have to trust that the called
	 function won't return.  */
      if (really_return)
	{
	  rtx ops[2];

	  /* Otherwise, trap an attempted return by aborting.  */
	  ops[0] = operand;
	  ops[1] = gen_rtx_SYMBOL_REF (Pmode, NEED_PLT_RELOC ? "abort(PLT)"
				       : "abort");
	  assemble_external_libcall (ops[1]);
	  output_asm_insn (reverse ? "bl%D0\t%a1" : "bl%d0\t%a1", ops);
	}

      return "";
    }

  gcc_assert (!current_function_calls_alloca || really_return);

  sprintf (conditional, "%%?%%%c0", reverse ? 'D' : 'd');

  return_used_this_function = 1;

  live_regs_mask = arm_compute_save_reg_mask ();

  if (live_regs_mask)
    {
      const char * return_reg;

      /* If we do not have any special requirements for function exit
	 (e.g. interworking, or ISR) then we can load the return address
	 directly into the PC.  Otherwise we must load it into LR.  */
      if (really_return
	  /* APPLE LOCAL ARM interworking */
	  && (! TARGET_INTERWORK || arm_arch5))
	return_reg = reg_names[PC_REGNUM];
      else
	return_reg = reg_names[LR_REGNUM];

      if ((live_regs_mask & (1 << IP_REGNUM)) == (1 << IP_REGNUM))
	{
	  /* There are three possible reasons for the IP register
	     being saved.  1) a stack frame was created, in which case
	     IP contains the old stack pointer, or 2) an ISR routine
	     corrupted it, or 3) it was saved to align the stack on
	     iWMMXt.  In case 1, restore IP into SP, otherwise just
	     restore IP.  */
	  if (frame_pointer_needed)
	    {
	      live_regs_mask &= ~ (1 << IP_REGNUM);
	      live_regs_mask |=   (1 << SP_REGNUM);
	    }
	  else
	    gcc_assert (IS_INTERRUPT (func_type) || TARGET_REALLY_IWMMXT);
	}

      /* On some ARM architectures it is faster to use LDR rather than
	 LDM to load a single register.  On other architectures, the
	 cost is the same.  In 26 bit mode, or for exception handlers,
	 we have to use LDM to load the PC so that the CPSR is also
	 restored.  */
      for (reg = 0; reg <= LAST_ARM_REGNUM; reg++)
	if (live_regs_mask == (1U << reg))
	  break;

      if (reg <= LAST_ARM_REGNUM
	  && (reg != LR_REGNUM
	      || ! really_return
	      || ! IS_INTERRUPT (func_type)))
	{
	  sprintf (instr, "ldr%s\t%%|%s, [%%|sp], #4", conditional,
		   (reg == LR_REGNUM) ? return_reg : reg_names[reg]);
	}
      else
	{
	  char *p;
	  int first = 1;

	  /* Generate the load multiple instruction to restore the
	     registers.  Note we can get here, even if
	     frame_pointer_needed is true, but only if sp already
	     points to the base of the saved core registers.  */
	  if (live_regs_mask & (1 << SP_REGNUM))
	    {
	      unsigned HOST_WIDE_INT stack_adjust;

	      offsets = arm_get_frame_offsets ();
	      stack_adjust = offsets->outgoing_args - offsets->saved_regs;
	      gcc_assert (stack_adjust == 0 || stack_adjust == 4);

              /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	      if (stack_adjust && arm_arch5 && TARGET_ARM)
		sprintf (instr, "ldm%sib\t%%|sp, {", conditional);
	      else
		{
		  /* If we can't use ldmib (SA110 bug),
		     then try to pop r3 instead.  */
		  if (stack_adjust)
		    live_regs_mask |= 1 << 3;
		  sprintf (instr, "ldm%sfd\t%%|sp, {", conditional);
		}
	    }
	  else
	    sprintf (instr, "ldm%sfd\t%%|sp!, {", conditional);

	  p = instr + strlen (instr);

	  for (reg = 0; reg <= SP_REGNUM; reg++)
	    if (live_regs_mask & (1 << reg))
	      {
		int l = strlen (reg_names[reg]);

		if (first)
		  first = 0;
		else
		  {
		    memcpy (p, ", ", 2);
		    p += 2;
		  }

		memcpy (p, "%|", 2);
		memcpy (p + 2, reg_names[reg], l);
		p += l + 2;
	      }

	  if (live_regs_mask & (1 << LR_REGNUM))
	    {
	      sprintf (p, "%s%%|%s}", first ? "" : ", ", return_reg);
	      /* If returning from an interrupt, restore the CPSR.  */
	      if (IS_INTERRUPT (func_type))
		strcat (p, "^");
	    }
	  else
	    strcpy (p, "}");
	}

      output_asm_insn (instr, & operand);

      /* See if we need to generate an extra instruction to
	 perform the actual function return.  */
      if (really_return
	  && func_type != ARM_FT_INTERWORKED
	  && (live_regs_mask & (1 << LR_REGNUM)) != 0)
	{
	  /* The return has already been handled
	     by loading the LR into the PC.  */
	  really_return = 0;
	}
    }

  if (really_return)
    {
      switch ((int) ARM_FUNC_TYPE (func_type))
	{
	case ARM_FT_ISR:
	case ARM_FT_FIQ:
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  /* ??? This is wrong for unified assembly syntax.  */
	  sprintf (instr, "sub%ss\t%%|pc, %%|lr, #4", conditional);
	  break;

	case ARM_FT_INTERWORKED:
	  sprintf (instr, "bx%s\t%%|lr", conditional);
	  break;

	case ARM_FT_EXCEPTION:
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
   	  /* ??? This is wrong for unified assembly syntax.  */
	  sprintf (instr, "mov%ss\t%%|pc, %%|lr", conditional);
	  break;

	default:
	  /* Use bx if it's available.  */
	  if (arm_arch5 || arm_arch4t)
	    sprintf (instr, "bx%s\t%%|lr", conditional);
	  else
	    sprintf (instr, "mov%s\t%%|pc, %%|lr", conditional);
	  break;
	}

      output_asm_insn (instr, & operand);
    }

  return "";
}

/* Write the function name into the code section, directly preceding
   the function prologue.

   Code will be output similar to this:
     t0
	 .ascii "arm_poke_function_name", 0
	 .align
     t1
	 .word 0xff000000 + (t1 - t0)
     arm_poke_function_name
	 mov     ip, sp
	 stmfd   sp!, {fp, ip, lr, pc}
	 sub     fp, ip, #4

   When performing a stack backtrace, code can inspect the value
   of 'pc' stored at 'fp' + 0.  If the trace function then looks
   at location pc - 12 and the top 8 bits are set, then we know
   that there is a function name embedded immediately preceding this
   location and has length ((pc[-3]) & 0xff000000).

   We assume that pc is declared as a pointer to an unsigned long.

   It is of no benefit to output the function name if we are assembling
   a leaf function.  These function types will not contain a stack
   backtrace structure, therefore it is not possible to determine the
   function name.  */
void
arm_poke_function_name (FILE *stream, const char *name)
{
  unsigned long alignlength;
  unsigned long length;
  rtx           x;

  length      = strlen (name) + 1;
  alignlength = ROUND_UP_WORD (length);

  ASM_OUTPUT_ASCII (stream, name, length);
  ASM_OUTPUT_ALIGN (stream, 2);
  x = GEN_INT ((unsigned HOST_WIDE_INT) 0xff000000 + alignlength);
  assemble_aligned_integer (UNITS_PER_WORD, x);
}

/* Place some comments into the assembler stream
   describing the current function.  */
static void
arm_output_function_prologue (FILE *f, HOST_WIDE_INT frame_size)
{
  unsigned long func_type;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (TARGET_THUMB1)
    {
      thumb1_output_function_prologue (f, frame_size);
      return;
    }
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

  /* Sanity check.  */
  gcc_assert (!arm_ccfsm_state && !arm_target_insn);

  func_type = arm_current_func_type ();

  switch ((int) ARM_FUNC_TYPE (func_type))
    {
    default:
    case ARM_FT_NORMAL:
      break;
    case ARM_FT_INTERWORKED:
      asm_fprintf (f, "\t%@ Function supports interworking.\n");
      break;
    case ARM_FT_ISR:
      asm_fprintf (f, "\t%@ Interrupt Service Routine.\n");
      break;
    case ARM_FT_FIQ:
      asm_fprintf (f, "\t%@ Fast Interrupt Service Routine.\n");
      break;
    case ARM_FT_EXCEPTION:
      asm_fprintf (f, "\t%@ ARM Exception Handler.\n");
      break;
    }

  if (IS_NAKED (func_type))
    asm_fprintf (f, "\t%@ Naked Function: prologue and epilogue provided by programmer.\n");

  if (IS_VOLATILE (func_type))
    asm_fprintf (f, "\t%@ Volatile: function does not return.\n");

  if (IS_NESTED (func_type))
    asm_fprintf (f, "\t%@ Nested: function declared inside another function.\n");
  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (IS_STACKALIGN (func_type))
    asm_fprintf (f, "\t%@ Stack Align: May be called with mis-aligned SP.\n");
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

  asm_fprintf (f, "\t%@ args = %d, pretend = %d, frame = %wd\n",
	       current_function_args_size,
	       current_function_pretend_args_size, frame_size);

  asm_fprintf (f, "\t%@ frame_needed = %d, uses_anonymous_args = %d\n",
	       frame_pointer_needed,
	       cfun->machine->uses_anonymous_args);

  if (cfun->machine->lr_save_eliminated)
    asm_fprintf (f, "\t%@ link register save eliminated.\n");

  if (current_function_calls_eh_return)
    asm_fprintf (f, "\t@ Calls __builtin_eh_return.\n");

#ifdef AOF_ASSEMBLER
  if (flag_pic)
    asm_fprintf (f, "\tmov\t%r, %r\n", IP_REGNUM, PIC_OFFSET_TABLE_REGNUM);
#endif

  return_used_this_function = 0;
}

/* ALQAAHIRA LOCAL begin 6902937 out of order VFP restore */
static void
arm_output_epilogue_vfp_restore (void)
{
  int reg, start_reg;
  FILE * f = asm_out_file;
  start_reg = LAST_VFP_REGNUM - 1;
  for (reg = LAST_VFP_REGNUM - 1 ; reg >= FIRST_VFP_REGNUM; reg -= 2)
  {
    if ((!regs_ever_live[reg] || call_used_regs[reg])
        && (!regs_ever_live[reg + 1] || call_used_regs[reg + 1]))
    {
      if (start_reg != reg)
        vfp_output_fldmd (f, SP_REGNUM,
                          (reg - FIRST_VFP_REGNUM) / 2 + 1,
                          (start_reg - reg) / 2);
      start_reg = reg - 2;
    }
  }
  if (start_reg != reg)
    vfp_output_fldmd (f, SP_REGNUM,
                      (reg - FIRST_VFP_REGNUM + 2) / 2 + 1,
                      (start_reg - reg) / 2);
}
/* ALQAAHIRA LOCAL end 6902937 out of order VFP restore */

const char *
arm_output_epilogue (rtx sibling)
{
  int reg;
  unsigned long saved_regs_mask;
  unsigned long func_type;
  /* Floats_offset is the offset from the "virtual" frame.  In an APCS
     frame that is $fp + 4 for a non-variadic function.  */
  int floats_offset = 0;
  rtx operands[3];
  FILE * f = asm_out_file;
  unsigned int lrm_count = 0;
  int really_return = (sibling == NULL);
  int start_reg;
  arm_stack_offsets *offsets;
  /* ALQAAHIRA LOCAL 6196857 use pop for thumb-2 epilogue */
  const char *pop_insn;

  /* If we have already generated the return instruction
     then it is futile to generate anything else.  */
  if (use_return_insn (FALSE, sibling) && return_used_this_function)
    return "";

  func_type = arm_current_func_type ();

  if (IS_NAKED (func_type))
    /* Naked functions don't have epilogues.  */
    return "";

  if (IS_VOLATILE (func_type) && TARGET_ABORT_NORETURN)
    {
      rtx op;

      /* A volatile function should never return.  Call abort.  */
      op = gen_rtx_SYMBOL_REF (Pmode, NEED_PLT_RELOC ? "abort(PLT)" : "abort");
      assemble_external_libcall (op);
      output_asm_insn ("bl\t%a0", &op);

      return "";
    }

  /* If we are throwing an exception, then we really must be doing a
     return, so we can't tail-call.  */
  gcc_assert (!current_function_calls_eh_return || really_return);

  offsets = arm_get_frame_offsets ();
  saved_regs_mask = arm_compute_save_reg_mask ();

  if (TARGET_IWMMXT)
    lrm_count = bit_count (saved_regs_mask);

  floats_offset = offsets->saved_args;
  /* Compute how far away the floats will be.  */
  for (reg = 0; reg <= LAST_ARM_REGNUM; reg++)
    if (saved_regs_mask & (1 << reg))
      floats_offset += 4;

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (frame_pointer_needed && TARGET_32BIT)
    {
      /* This variable is for the Virtual Frame Pointer, not VFP regs.  */
      int vfp_offset = offsets->frame;
      /* APPLE LOCAL begin ARM custom frame layout */
      unsigned long regs_above_fp =
        inclusive_bitmask (ARM_HARD_FRAME_POINTER_REGNUM + 1, 11);
      /* APPLE LOCAL end ARM custom frame layout */

      if (arm_fpu_arch == FPUTYPE_FPA_EMU2)
	{
	  for (reg = LAST_FPA_REGNUM; reg >= FIRST_FPA_REGNUM; reg--)
	    if (regs_ever_live[reg] && !call_used_regs[reg])
	      {
		floats_offset += 12;
		asm_fprintf (f, "\tldfe\t%r, [%r, #-%d]\n",
			     reg, FP_REGNUM, floats_offset - vfp_offset);
	      }
	}
      else
	{
	  start_reg = LAST_FPA_REGNUM;

	  for (reg = LAST_FPA_REGNUM; reg >= FIRST_FPA_REGNUM; reg--)
	    {
	      if (regs_ever_live[reg] && !call_used_regs[reg])
		{
		  floats_offset += 12;

		  /* We can't unstack more than four registers at once.  */
		  if (start_reg - reg == 3)
		    {
		      asm_fprintf (f, "\tlfm\t%r, 4, [%r, #-%d]\n",
			           reg, FP_REGNUM, floats_offset - vfp_offset);
		      start_reg = reg - 1;
		    }
		}
	      else
		{
		  if (reg != start_reg)
		    asm_fprintf (f, "\tlfm\t%r, %d, [%r, #-%d]\n",
				 reg + 1, start_reg - reg,
				 FP_REGNUM, floats_offset - vfp_offset);
		  start_reg = reg - 1;
		}
	    }

	  /* Just in case the last register checked also needs unstacking.  */
	  if (reg != start_reg)
	    asm_fprintf (f, "\tlfm\t%r, %d, [%r, #-%d]\n",
			 reg + 1, start_reg - reg,
			 FP_REGNUM, floats_offset - vfp_offset);
	}

      if (TARGET_HARD_FLOAT && TARGET_VFP)
	{
	  int saved_size;

          /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
	  /* APPLE LOCAL begin 4809156 */
	  /* The fldmd insns do not have base+offset addressing modes,
	     so we use SP to hold the address.  (IP might have a
	     live value in it for indirect sibcalls, can't use that.)  */
          /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
	  saved_size = arm_get_vfp_saved_size ();

	  if (saved_size > 0)
	    {
	      floats_offset += saved_size;
	      asm_fprintf (f, "\tsub\t%r, %r, #%d\n", SP_REGNUM,
			   FP_REGNUM, floats_offset - vfp_offset);
	    }
          /* ALQAAHIRA LOCAL 6902937 out of order VFP restore */
          arm_output_epilogue_vfp_restore ();
	  /* APPLE LOCAL end 4809156 */
        }

      if (TARGET_IWMMXT)
	{
	  /* The frame pointer is guaranteed to be non-double-word aligned.
	     This is because it is set to (old_stack_pointer - 4) and the
	     old_stack_pointer was double word aligned.  Thus the offset to
	     the iWMMXt registers to be loaded must also be non-double-word
	     sized, so that the resultant address *is* double-word aligned.
	     We can ignore floats_offset since that was already included in
	     the live_regs_mask.  */
	  lrm_count += (lrm_count % 2 ? 2 : 1);

	  for (reg = LAST_IWMMXT_REGNUM; reg >= FIRST_IWMMXT_REGNUM; reg--)
	    if (regs_ever_live[reg] && !call_used_regs[reg])
	      {
		asm_fprintf (f, "\twldrd\t%r, [%r, #-%d]\n",
			     reg, FP_REGNUM, lrm_count * 4);
		lrm_count += 2;
	      }
	}

      /* APPLE LOCAL ARM custom frame layout */
      /* Removed lines.  */

      /* APPLE LOCAL begin ARM indirect sibcalls */
      /* If we have an indirect sibcall that uses a reg saved across calls, that reg will
	 be clobbered when we pop the old value off the stack.  Copy the value to IP
	 before doing the pop. */
      if (sibling)
	{
	  bool is_value;
	  int regno = indirect_sibreturn_reg (sibling, &is_value);
	  if (regno > 3 && regno != 12)
	    {
	      if (is_value)
		XEXP (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0), 0)
		    = gen_rtx_REG (SImode, IP_REGNUM);
	      else
		XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0), 0) 
		    = gen_rtx_REG (SImode, IP_REGNUM);
	      asm_fprintf (f, "\tmov\t%r, %r\n", IP_REGNUM, regno);
	    }
	  if (regno == -1)
	    {
	      rtx stack_reg, offset;
	      offset = indirect_sibreturn_mem (sibling, &stack_reg, &is_value);
	      if (offset)
		{
		  if (is_value)
		    XEXP (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0), 0)
			= gen_rtx_REG (SImode, IP_REGNUM);
		  else
		    XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0), 0) 
			= gen_rtx_REG (SImode, IP_REGNUM);
		  asm_fprintf (f, "\tldr\t%r, [%r, #%wd]\n", IP_REGNUM, 
		    REGNO (stack_reg), INTVAL (offset));
		}
	    }
	}
      /* APPLE LOCAL end ARM indirect sibcalls */

      /* We must use SP as the base register, because SP is one of the
         registers being restored.  If an interrupt or page fault
         happens in the ldm instruction, the SP might or might not
         have been restored.  That would be bad, as then SP will no
         longer indicate the safe area of stack, and we can get stack
         corruption.  Using SP as the base register means that it will
         be reset correctly to the original value, should an interrupt
         occur.  If the stack pointer already points at the right
         place, then omit the subtraction.  */

      /* APPLE LOCAL begin ARM custom frame layout */
      if ((offsets->outgoing_args - offsets->saved_args
	   != (signed) bit_count (saved_regs_mask) * 4)
	  || ! current_function_sp_is_unchanging)
	/* FP points 8 bytes into the frame.  */
	asm_fprintf (f, "\tsub\t%r, %r, #%d\n", SP_REGNUM, FP_REGNUM,
		     (bit_count (saved_regs_mask) - 2) * 4);

      /* If we can, restore the LR into the PC.  */
      if (ARM_FUNC_TYPE (func_type) == ARM_FT_NORMAL
	  && really_return
	  && current_function_pretend_args_size == 0
	  && saved_regs_mask & (1 << LR_REGNUM)
	  && !current_function_calls_eh_return)
	{
	  saved_regs_mask &= ~ (1 << LR_REGNUM);
	  saved_regs_mask |=   (1 << PC_REGNUM);
	}

      /* We mustn't be trying to restore SP from the stack.  */
      gcc_assert (! (saved_regs_mask & (1 << SP_REGNUM)));

      /* ALQAAHIRA LOCAL 6196857 begin use pop for thumb-2 epilogue */
      if (TARGET_ARM)
	pop_insn = "ldmfd\t%r!, ";
      else /* (TARGET_THUMB2) */
	pop_insn = "pop\t";

      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      if (saved_regs_mask & regs_above_fp)
	{
	  print_multi_reg (f, pop_insn, SP_REGNUM,
			   saved_regs_mask & regs_above_fp);
	  print_multi_reg (f, pop_insn, SP_REGNUM,
			   saved_regs_mask & ~regs_above_fp);
	}
      else
	print_multi_reg (f, pop_insn, SP_REGNUM, saved_regs_mask);
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
      /* ALQAAHIRA LOCAL 6196857 begin use pop for thumb-2 epilogue */

      if (current_function_pretend_args_size)
	{
	  /* Unwind the pre-pushed regs.  */
	  operands[0] = operands[1] = stack_pointer_rtx;
	  operands[2] = GEN_INT (current_function_pretend_args_size);
	  output_add_immediate (operands);
	}
      /* APPLE LOCAL end ARM custom frame layout */

      if (IS_INTERRUPT (func_type))
	/* Interrupt handlers will have pushed the
	   IP onto the stack, so restore it now.  */
        /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	print_multi_reg (f, "ldmfd\t%r!, ", SP_REGNUM, 1 << IP_REGNUM);
    }
  else
    {
      /* APPLE LOCAL begin ARM indirect sibcalls */
      int ip_ok = 1;

      /* If we have an indirect sibcall that uses a reg saved across calls, that reg will
	 be clobbered when we pop the old value off the stack.  Copy the value to IP
	 before doing the pop. */
      if (sibling)
	{
	  bool is_value;
	  int regno = indirect_sibreturn_reg (sibling, &is_value);
	  if (regno > 3 && regno != 12)
	    {
	      ip_ok = 0;
	      if (is_value)
		XEXP (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0), 0)
		    = gen_rtx_REG (SImode, IP_REGNUM);
	      else
		XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0), 0) 
		    = gen_rtx_REG (SImode, IP_REGNUM);
	      asm_fprintf (f, "\tmov\t%r, %r\n", IP_REGNUM, regno);
	    }
	  if (regno == -1)
	    {
	      rtx stack_reg, offset;
	      offset = indirect_sibreturn_mem (sibling, &stack_reg, &is_value);
	      if (offset)
		{
		  ip_ok = 0;
		  if (is_value)
		    XEXP (XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 1), 0), 0)
			= gen_rtx_REG (SImode, IP_REGNUM);
		  else
		    XEXP (XEXP (XVECEXP (PATTERN (sibling), 0, 0), 0), 0) 
			= gen_rtx_REG (SImode, IP_REGNUM);
		  asm_fprintf (f, "\tldr\t%r, [%r, #%wd]\n", IP_REGNUM, 
		    REGNO (stack_reg), INTVAL (offset));
		}
	    }
	}

      /* APPLE LOCAL begin ARM combine stack pop and register pop */
      /* Code here is probably making overly specific assumptions about modes. */
      /* Restore stack pointer if necessary.  */
      if (offsets->outgoing_args != offsets->saved_regs)
	{
	  int delta = offsets->outgoing_args - offsets->saved_regs;
	  int maxpopsize;
	  tree rettype = TREE_TYPE (TREE_TYPE (current_function_decl));
	  /* We can use R0 through R3 for this purpose, but not any regs that
	     contain (part of) the return value.  */
	  if (TYPE_MODE (rettype) == VOIDmode)
	    maxpopsize = 20;
	  else if (TYPE_MODE (rettype) == DFmode
		   || TYPE_MODE (rettype) == DImode)
	    maxpopsize = 12;
	  else
	    maxpopsize = 16;
	  /* We can also use R12 provided it was not used for the sibcall hack above,
	     and we are not saving any regs in the range R4...R11.  In the latter case
	     they are stored on the stack below the "empty" spot used for R12 and
	     the saved values would get clobbered.  */
	  if (saved_regs_mask 
	      & ((1<<4) | (1<<5) | (1<<6) | (1<<7) | (1<<8) | (1<<9) | (1<<10) | (1<<11)))
	    ip_ok = 0;
	  if (!ip_ok)
	    maxpopsize -= 4;
	  if (optimize_size 
	      && delta <= maxpopsize && delta % 4 == 0
	      && !TARGET_IWMMXT
	      && really_return
	      && TARGET_SOFT_FLOAT
	      && arm_fpu_arch == FPUTYPE_NONE
	      && !flag_pic
	      && !frame_pointer_needed)
	    {
	      int reg = ip_ok ? 12 : 3;
	      while (delta)
		{
		  saved_regs_mask |= (1 << reg);
		  reg = (reg == 12) ? 3 : reg - 1;
		  delta -= 4;
		}
	    }
	  else
	    {
	      operands[0] = operands[1] = stack_pointer_rtx;
	      operands[2] = GEN_INT (offsets->outgoing_args - offsets->saved_regs);
	      output_add_immediate (operands);
	    }
	}
      /* APPLE LOCAL end ARM combine stack pop and register pop */
      /* APPLE LOCAL end ARM indirect sibcalls */

      if (arm_fpu_arch == FPUTYPE_FPA_EMU2)
	{
	  for (reg = FIRST_FPA_REGNUM; reg <= LAST_FPA_REGNUM; reg++)
	    if (regs_ever_live[reg] && !call_used_regs[reg])
	      asm_fprintf (f, "\tldfe\t%r, [%r], #12\n",
			   reg, SP_REGNUM);
	}
      else
	{
	  start_reg = FIRST_FPA_REGNUM;

	  for (reg = FIRST_FPA_REGNUM; reg <= LAST_FPA_REGNUM; reg++)
	    {
	      if (regs_ever_live[reg] && !call_used_regs[reg])
		{
		  if (reg - start_reg == 3)
		    {
		      asm_fprintf (f, "\tlfmfd\t%r, 4, [%r]!\n",
				   start_reg, SP_REGNUM);
		      start_reg = reg + 1;
		    }
		}
	      else
		{
		  if (reg != start_reg)
		    asm_fprintf (f, "\tlfmfd\t%r, %d, [%r]!\n",
				 start_reg, reg - start_reg,
				 SP_REGNUM);

		  start_reg = reg + 1;
		}
	    }

	  /* Just in case the last register checked also needs unstacking.  */
	  if (reg != start_reg)
	    asm_fprintf (f, "\tlfmfd\t%r, %d, [%r]!\n",
			 start_reg, reg - start_reg, SP_REGNUM);
	}

      if (TARGET_HARD_FLOAT && TARGET_VFP)
	{
          /* ALQAAHIRA LOCAL 6902937 out of order VFP restore */
          arm_output_epilogue_vfp_restore ();
        }
      if (TARGET_IWMMXT)
	for (reg = FIRST_IWMMXT_REGNUM; reg <= LAST_IWMMXT_REGNUM; reg++)
	  if (regs_ever_live[reg] && !call_used_regs[reg])
	    asm_fprintf (f, "\twldrd\t%r, [%r], #8\n", reg, SP_REGNUM);

      /* If we can, restore the LR into the PC.  */
      if (ARM_FUNC_TYPE (func_type) == ARM_FT_NORMAL
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
	  && !IS_STACKALIGN (func_type)
	  && really_return
	  && current_function_pretend_args_size == 0
	  && saved_regs_mask & (1 << LR_REGNUM)
	  && !current_function_calls_eh_return)
	{
	  saved_regs_mask &= ~ (1 << LR_REGNUM);
	  saved_regs_mask |=   (1 << PC_REGNUM);
	}

      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      /* Load the registers off the stack.  If we only have one register
	 to load use the LDR instruction - it is faster.  For Thumb-2
	 always use pop and the assembler will pick the best instruction.*/
      if (TARGET_ARM && saved_regs_mask == (1 << LR_REGNUM))
	{
	  asm_fprintf (f, "\tldr\t%r, [%r], #4\n", LR_REGNUM, SP_REGNUM);
	}
      else if (saved_regs_mask)
	{
	  if (saved_regs_mask & (1 << SP_REGNUM))
	    /* Note - write back to the stack register is not enabled
	       (i.e. "ldmfd sp!...").  We know that the stack pointer is
	       in the list of registers and if we add writeback the
	       instruction becomes UNPREDICTABLE.  */
	    print_multi_reg (f, "ldmfd\t%r, ", SP_REGNUM, saved_regs_mask);
	  else if (TARGET_ARM)
	    print_multi_reg (f, "ldmfd\t%r!, ", SP_REGNUM, saved_regs_mask);
	  else
	    print_multi_reg (f, "pop\t", SP_REGNUM, saved_regs_mask);
	}
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

      if (current_function_pretend_args_size)
	{
	  /* Unwind the pre-pushed regs.  */
	  operands[0] = operands[1] = stack_pointer_rtx;
	  operands[2] = GEN_INT (current_function_pretend_args_size);
	  output_add_immediate (operands);
	}
    }

  /* We may have already restored PC directly from the stack.  */
  if (!really_return || saved_regs_mask & (1 << PC_REGNUM))
    return "";

  /* Stack adjustment for exception handler.  */
  if (current_function_calls_eh_return)
    asm_fprintf (f, "\tadd\t%r, %r, %r\n", SP_REGNUM, SP_REGNUM,
		 ARM_EH_STACKADJ_REGNUM);

  /* Generate the return instruction.  */
  switch ((int) ARM_FUNC_TYPE (func_type))
    {
    case ARM_FT_ISR:
    case ARM_FT_FIQ:
      asm_fprintf (f, "\tsubs\t%r, %r, #4\n", PC_REGNUM, LR_REGNUM);
      break;

    case ARM_FT_EXCEPTION:
      asm_fprintf (f, "\tmovs\t%r, %r\n", PC_REGNUM, LR_REGNUM);
      break;

    case ARM_FT_INTERWORKED:
      asm_fprintf (f, "\tbx\t%r\n", LR_REGNUM);
      break;

    default:
      if (IS_STACKALIGN (func_type))
	{
	  /* See comment in arm_expand_prologue.  */
	  asm_fprintf (f, "\tmov\t%r, %r\n", SP_REGNUM, 0);
	}
      if (arm_arch5 || arm_arch4t)
	asm_fprintf (f, "\tbx\t%r\n", LR_REGNUM);
      else
	asm_fprintf (f, "\tmov\t%r, %r\n", PC_REGNUM, LR_REGNUM);
      break;
    }

  return "";
}

static void
arm_output_function_epilogue (FILE *file ATTRIBUTE_UNUSED,
			      HOST_WIDE_INT frame_size ATTRIBUTE_UNUSED)
{
  arm_stack_offsets *offsets;

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (TARGET_THUMB1)
    {
      int regno;

      /* Emit any call-via-reg trampolines that are needed for v4t support
	 of call_reg and call_value_reg type insns.  */
      for (regno = 0; regno < LR_REGNUM; regno++)
	{
	  rtx label = cfun->machine->call_via[regno];

	  if (label != NULL)
	    {
	      switch_to_section (function_section (current_function_decl));
	      targetm.asm_out.internal_label (asm_out_file, "L",
					      CODE_LABEL_NUMBER (label));
	      asm_fprintf (asm_out_file, "\tbx\t%r\n", regno);
	    }
	}

      /* ??? Probably not safe to set this here, since it assumes that a
	 function will be emitted as assembly immediately after we generate
	 RTL for it.  This does not happen for inline functions.  */
      return_used_this_function = 0;
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  else /* TARGET_32BIT */
    {
      /* We need to take into account any stack-frame rounding.  */
      offsets = arm_get_frame_offsets ();

      gcc_assert (!use_return_insn (FALSE, NULL)
		  || !return_used_this_function
		  || offsets->saved_regs == offsets->outgoing_args
		  || frame_pointer_needed);

      /* Reset the ARM-specific per-function variables.  */
      after_arm_reorg = 0;
    }

/* APPLE LOCAL begin ARM label addresses */
#if TARGET_MACHO
  /* Mach-O doesn't support labels at the end of objects, so if
     it looks like we might want one, insert a NOP.  */
  {
    rtx insn = get_last_insn ();
    while (insn
	   && NOTE_P (insn)
	   && NOTE_LINE_NUMBER (insn) != NOTE_INSN_DELETED_LABEL)
      insn = PREV_INSN (insn);
    if (insn
	&& (LABEL_P (insn)
	    || (NOTE_P (insn)
		&& NOTE_LINE_NUMBER (insn) == NOTE_INSN_DELETED_LABEL)))
      fputs ("\tnop\n", file);
  }
#endif
/* APPLE LOCAL end ARM label addresses */
}

/* Generate and emit an insn that we will recognize as a push_multi.
   Unfortunately, since this insn does not reflect very well the actual
   semantics of the operation, we need to annotate the insn for the benefit
   of DWARF2 frame unwind information.  */
static rtx
emit_multi_reg_push (unsigned long mask)
{
  int num_regs = 0;
  int num_dwarf_regs;
  int i, j;
  rtx par;
  rtx dwarf;
  int dwarf_par_index;
  rtx tmp, reg;

  for (i = 0; i <= LAST_ARM_REGNUM; i++)
    if (mask & (1 << i))
      num_regs++;

  gcc_assert (num_regs && num_regs <= 16);

  /* We don't record the PC in the dwarf frame information.  */
  num_dwarf_regs = num_regs;
  if (mask & (1 << PC_REGNUM))
    num_dwarf_regs--;

  /* For the body of the insn we are going to generate an UNSPEC in
     parallel with several USEs.  This allows the insn to be recognized
     by the push_multi pattern in the arm.md file.  The insn looks
     something like this:

       (parallel [
           (set (mem:BLK (pre_dec:BLK (reg:SI sp)))
	        (unspec:BLK [(reg:SI r4)] UNSPEC_PUSH_MULT))
           (use (reg:SI 11 fp))
           (use (reg:SI 12 ip))
           (use (reg:SI 14 lr))
           (use (reg:SI 15 pc))
        ])

     For the frame note however, we try to be more explicit and actually
     show each register being stored into the stack frame, plus a (single)
     decrement of the stack pointer.  We do it this way in order to be
     friendly to the stack unwinding code, which only wants to see a single
     stack decrement per instruction.  The RTL we generate for the note looks
     something like this:

      (sequence [
           (set (reg:SI sp) (plus:SI (reg:SI sp) (const_int -20)))
           (set (mem:SI (reg:SI sp)) (reg:SI r4))
           (set (mem:SI (plus:SI (reg:SI sp) (const_int 4))) (reg:SI fp))
           (set (mem:SI (plus:SI (reg:SI sp) (const_int 8))) (reg:SI ip))
           (set (mem:SI (plus:SI (reg:SI sp) (const_int 12))) (reg:SI lr))
        ])

      This sequence is used both by the code to support stack unwinding for
      exceptions handlers and the code to generate dwarf2 frame debugging.  */

  par = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc (num_regs));
  dwarf = gen_rtx_SEQUENCE (VOIDmode, rtvec_alloc (num_dwarf_regs + 1));
  dwarf_par_index = 1;

  for (i = 0; i <= LAST_ARM_REGNUM; i++)
    {
      if (mask & (1 << i))
	{
	  reg = gen_rtx_REG (SImode, i);

	  XVECEXP (par, 0, 0)
	    = gen_rtx_SET (VOIDmode,
			   gen_frame_mem (BLKmode,
					  gen_rtx_PRE_DEC (BLKmode,
							   stack_pointer_rtx)),
			   gen_rtx_UNSPEC (BLKmode,
					   gen_rtvec (1, reg),
					   UNSPEC_PUSH_MULT));

	  if (i != PC_REGNUM)
	    {
	      tmp = gen_rtx_SET (VOIDmode,
				 gen_frame_mem (SImode, stack_pointer_rtx),
				 reg);
	      RTX_FRAME_RELATED_P (tmp) = 1;
	      XVECEXP (dwarf, 0, dwarf_par_index) = tmp;
	      dwarf_par_index++;
	    }

	  break;
	}
    }

  for (j = 1, i++; j < num_regs; i++)
    {
      if (mask & (1 << i))
	{
	  reg = gen_rtx_REG (SImode, i);

	  XVECEXP (par, 0, j) = gen_rtx_USE (VOIDmode, reg);

	  if (i != PC_REGNUM)
	    {
	      tmp
		= gen_rtx_SET (VOIDmode,
			       gen_frame_mem (SImode,
					      plus_constant (stack_pointer_rtx,
							     4 * j)),
			       reg);
	      RTX_FRAME_RELATED_P (tmp) = 1;
	      XVECEXP (dwarf, 0, dwarf_par_index++) = tmp;
	    }

	  j++;
	}
    }

  par = emit_insn (par);

  tmp = gen_rtx_SET (VOIDmode,
		     stack_pointer_rtx,
		     plus_constant (stack_pointer_rtx, -4 * num_regs));
  RTX_FRAME_RELATED_P (tmp) = 1;
  XVECEXP (dwarf, 0, 0) = tmp;

  REG_NOTES (par) = gen_rtx_EXPR_LIST (REG_FRAME_RELATED_EXPR, dwarf,
				       REG_NOTES (par));
  return par;
}

/* Calculate the size of the return value that is passed in registers.  */
static int
arm_size_return_regs (void)
{
  enum machine_mode mode;

  if (current_function_return_rtx != 0)
    mode = GET_MODE (current_function_return_rtx);
  else
    mode = DECL_MODE (DECL_RESULT (current_function_decl));

  return GET_MODE_SIZE (mode);
}

static rtx
emit_sfm (int base_reg, int count)
{
  rtx par;
  rtx dwarf;
  rtx tmp, reg;
  int i;

  par = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc (count));
  dwarf = gen_rtx_SEQUENCE (VOIDmode, rtvec_alloc (count + 1));

  reg = gen_rtx_REG (XFmode, base_reg++);

  XVECEXP (par, 0, 0)
    = gen_rtx_SET (VOIDmode,
		   gen_frame_mem (BLKmode,
				  gen_rtx_PRE_DEC (BLKmode,
						   stack_pointer_rtx)),
		   gen_rtx_UNSPEC (BLKmode,
				   gen_rtvec (1, reg),
				   UNSPEC_PUSH_MULT));
  tmp = gen_rtx_SET (VOIDmode,
		     gen_frame_mem (XFmode, stack_pointer_rtx), reg);
  RTX_FRAME_RELATED_P (tmp) = 1;
  XVECEXP (dwarf, 0, 1) = tmp;

  for (i = 1; i < count; i++)
    {
      reg = gen_rtx_REG (XFmode, base_reg++);
      XVECEXP (par, 0, i) = gen_rtx_USE (VOIDmode, reg);

      tmp = gen_rtx_SET (VOIDmode,
			 gen_frame_mem (XFmode,
					plus_constant (stack_pointer_rtx,
						       i * 12)),
			 reg);
      RTX_FRAME_RELATED_P (tmp) = 1;
      XVECEXP (dwarf, 0, i + 1) = tmp;
    }

  tmp = gen_rtx_SET (VOIDmode,
		     stack_pointer_rtx,
		     plus_constant (stack_pointer_rtx, -12 * count));

  RTX_FRAME_RELATED_P (tmp) = 1;
  XVECEXP (dwarf, 0, 0) = tmp;

  par = emit_insn (par);
  REG_NOTES (par) = gen_rtx_EXPR_LIST (REG_FRAME_RELATED_EXPR, dwarf,
				       REG_NOTES (par));
  return par;
}


/* Return true if the current function needs to save/restore LR.  */

static bool
thumb_force_lr_save (void)
{
  return !cfun->machine->lr_save_eliminated
	 && (!leaf_function_p ()
	     || thumb_far_jump_used_p ()
	     || regs_ever_live [LR_REGNUM]);
}


/* Compute the distance from register FROM to register TO.
   These can be the arg pointer (26), the soft frame pointer (25),
   the stack pointer (13) or the hard frame pointer (11).
   In thumb mode r7 is used as the soft frame pointer, if needed.
   Typical stack layout looks like this:

       old stack pointer -> |    |
                             ----
                            |    | \
                            |    |   saved arguments for
                            |    |   vararg functions
			    |    | /
                              --
   hard FP & arg pointer -> |    | \
                            |    |   stack
                            |    |   frame
                            |    | /
                              --
                            |    | \
                            |    |   call saved
                            |    |   registers
      soft frame pointer -> |    | /
                              --
                            |    | \
                            |    |   local
                            |    |   variables
     locals base pointer -> |    | /
                              --
                            |    | \
                            |    |   outgoing
                            |    |   arguments
   current stack pointer -> |    | /
                              --

  For a given function some or all of these stack components
  may not be needed, giving rise to the possibility of
  eliminating some of the registers.

  The values returned by this function must reflect the behavior
  of arm_expand_prologue() and arm_compute_save_reg_mask().

  The sign of the number returned reflects the direction of stack
  growth, so the values are positive for all eliminations except
  from the soft frame pointer to the hard frame pointer.

  SFP may point just inside the local variables block to ensure correct
  alignment.  */


/* Calculate stack offsets.  These are used to calculate register elimination
   offsets and in prologue/epilogue code.  */

static arm_stack_offsets *
arm_get_frame_offsets (void)
{
  struct arm_stack_offsets *offsets;
  unsigned long func_type;
  int leaf;
  int saved;
  HOST_WIDE_INT frame_size;

  offsets = &cfun->machine->stack_offsets;

  /* We need to know if we are a leaf function.  Unfortunately, it
     is possible to be called after start_sequence has been called,
     which causes get_insns to return the insns for the sequence,
     not the function, which will cause leaf_function_p to return
     the incorrect result.

     to know about leaf functions once reload has completed, and the
     frame size cannot be changed after that time, so we can safely
     use the cached value.  */

  if (reload_completed)
    return offsets;

  /* Initially this is the size of the local variables.  It will translated
     into an offset once we have determined the size of preceding data.  */
  frame_size = ROUND_UP_WORD (get_frame_size ());

  leaf = leaf_function_p ();

  /* Space for variadic functions.  */
  offsets->saved_args = current_function_pretend_args_size;

  /* APPLE LOCAL ARM custom frame layout */
  offsets->frame = offsets->saved_args + (frame_pointer_needed ? 8 : 0);

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (TARGET_32BIT)
    {
      unsigned int regno;

      saved = bit_count (arm_compute_save_reg_mask ()) * 4;

      /* We know that SP will be doubleword aligned on entry, and we must
	 preserve that condition at any subroutine call.  We also require the
	 soft frame pointer to be doubleword aligned.  */

      if (TARGET_REALLY_IWMMXT)
	{
	  /* Check for the call-saved iWMMXt registers.  */
	  for (regno = FIRST_IWMMXT_REGNUM;
	       regno <= LAST_IWMMXT_REGNUM;
	       regno++)
	    if (regs_ever_live [regno] && ! call_used_regs [regno])
	      saved += 8;
	}

      func_type = arm_current_func_type ();
      if (! IS_VOLATILE (func_type))
	{
	  /* Space for saved FPA registers.  */
	  for (regno = FIRST_FPA_REGNUM; regno <= LAST_FPA_REGNUM; regno++)
	  if (regs_ever_live[regno] && ! call_used_regs[regno])
	    saved += 12;

	  /* Space for saved VFP registers.  */
	  if (TARGET_HARD_FLOAT && TARGET_VFP)
	    saved += arm_get_vfp_saved_size ();
	}
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  else /* TARGET_THUMB1 */
    {
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      saved = bit_count (thumb1_compute_save_reg_mask ()) * 4;
      if (TARGET_BACKTRACE)
	saved += 16;
      /* APPLE LOCAL begin 6465387 exception handling interworking VFP save */
      /* Saved VFP registers in thumb mode aren't accounted for by
         thumb1_compute_save_reg_mask() */
      if (current_function_has_nonlocal_label && arm_arch6)
        saved += 64;
      /* APPLE LOCAL end 6465387 exception handling interworking VFP save */
    }

  /* Saved registers include the stack frame.  */
  offsets->saved_regs = offsets->saved_args + saved;
  offsets->soft_frame = offsets->saved_regs + CALLER_INTERWORKING_SLOT_SIZE;
  /* A leaf function does not need any stack alignment if it has nothing
     on the stack.  */
  if (leaf && frame_size == 0)
    {
      offsets->outgoing_args = offsets->soft_frame;
      offsets->locals_base = offsets->soft_frame;
      return offsets;
    }

  /* Ensure SFP has the correct alignment.  */
  if (ARM_DOUBLEWORD_ALIGN
      && (offsets->soft_frame & 7))
    offsets->soft_frame += 4;

  offsets->locals_base = offsets->soft_frame + frame_size;
  offsets->outgoing_args = (offsets->locals_base
			    + current_function_outgoing_args_size);

  if (ARM_DOUBLEWORD_ALIGN)
    {
      /* Ensure SP remains doubleword aligned.  */
      if (offsets->outgoing_args & 7)
	offsets->outgoing_args += 4;
      gcc_assert (!(offsets->outgoing_args & 7));
    }

  return offsets;
}


/* Calculate the relative offsets for the different stack pointers.  Positive
   offsets are in the direction of stack growth.  */

HOST_WIDE_INT
arm_compute_initial_elimination_offset (unsigned int from, unsigned int to)
{
  arm_stack_offsets *offsets;

  offsets = arm_get_frame_offsets ();

  /* OK, now we have enough information to compute the distances.
     There must be an entry in these switch tables for each pair
     of registers in ELIMINABLE_REGS, even if some of the entries
     seem to be redundant or useless.  */
  switch (from)
    {
    case ARG_POINTER_REGNUM:
      switch (to)
	{
	/* APPLE LOCAL ARM custom frame layout */
        /* Removed lines.  */

	case FRAME_POINTER_REGNUM:
	  /* This is the reverse of the soft frame pointer
	     to hard frame pointer elimination below.  */
	  return offsets->soft_frame - offsets->saved_args;

	/* APPLE LOCAL begin ARM custom frame layout */
	case HARD_FRAME_POINTER_REGNUM:
	  return offsets->frame - (offsets->saved_args + 4);
	/* APPLE LOCAL end ARM custom frame layout */

	case STACK_POINTER_REGNUM:
	  /* If nothing has been pushed on the stack at all
	     then this will return -4.  This *is* correct!  */
	  return offsets->outgoing_args - (offsets->saved_args + 4);

	default:
	  gcc_unreachable ();
	}
      gcc_unreachable ();

    case FRAME_POINTER_REGNUM:
      switch (to)
	{
	/* APPLE LOCAL begin ARM custom frame layout */
	case HARD_FRAME_POINTER_REGNUM:
	/* APPLE LOCAL end ARM custom frame layout */
	  /* The hard frame pointer points to the top entry in the
	     stack frame.  The soft frame pointer to the bottom entry
	     in the stack frame.  If there is no stack frame at all,
	     then they are identical.  */

	  return offsets->frame - offsets->soft_frame;

	case STACK_POINTER_REGNUM:
	  return offsets->outgoing_args - offsets->soft_frame;

	default:
	  gcc_unreachable ();
	}
      gcc_unreachable ();

    default:
      /* You cannot eliminate from the stack pointer.
	 In theory you could eliminate from the hard frame
	 pointer to the stack pointer, but this will never
	 happen, since if a stack frame is not needed the
	 hard frame pointer will never be used.  */
      gcc_unreachable ();
    }
}


/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Emit RTL to save coprocessor registers on funciton entry.  Returns the
   number of bytes pushed.  */

static int
arm_save_coproc_regs(void)
{
  int saved_size = 0;
  unsigned reg;
  unsigned start_reg;
  rtx insn;

  for (reg = LAST_IWMMXT_REGNUM; reg >= FIRST_IWMMXT_REGNUM; reg--)
    if (regs_ever_live[reg] && ! call_used_regs [reg])
      {
	insn = gen_rtx_PRE_DEC (V2SImode, stack_pointer_rtx);
	insn = gen_rtx_MEM (V2SImode, insn);
	insn = emit_set_insn (insn, gen_rtx_REG (V2SImode, reg));
	RTX_FRAME_RELATED_P (insn) = 1;
	saved_size += 8;
      }

  /* Save any floating point call-saved registers used by this
     function.  */
  if (arm_fpu_arch == FPUTYPE_FPA_EMU2)
    {
      for (reg = LAST_FPA_REGNUM; reg >= FIRST_FPA_REGNUM; reg--)
	if (regs_ever_live[reg] && !call_used_regs[reg])
	  {
	    insn = gen_rtx_PRE_DEC (XFmode, stack_pointer_rtx);
	    insn = gen_rtx_MEM (XFmode, insn);
	    insn = emit_set_insn (insn, gen_rtx_REG (XFmode, reg));
	    RTX_FRAME_RELATED_P (insn) = 1;
	    saved_size += 12;
	  }
    }
  else
    {
      start_reg = LAST_FPA_REGNUM;

      for (reg = LAST_FPA_REGNUM; reg >= FIRST_FPA_REGNUM; reg--)
	{
	  if (regs_ever_live[reg] && !call_used_regs[reg])
	    {
	      if (start_reg - reg == 3)
		{
		  insn = emit_sfm (reg, 4);
		  RTX_FRAME_RELATED_P (insn) = 1;
		  saved_size += 48;
		  start_reg = reg - 1;
		}
	    }
	  else
	    {
	      if (start_reg != reg)
		{
		  insn = emit_sfm (reg + 1, start_reg - reg);
		  RTX_FRAME_RELATED_P (insn) = 1;
		  saved_size += (start_reg - reg) * 12;
		}
	      start_reg = reg - 1;
	    }
	}

      if (start_reg != reg)
	{
	  insn = emit_sfm (reg + 1, start_reg - reg);
	  saved_size += (start_reg - reg) * 12;
	  RTX_FRAME_RELATED_P (insn) = 1;
	}
    }
  if (TARGET_HARD_FLOAT && TARGET_VFP)
    {
      start_reg = FIRST_VFP_REGNUM;

      for (reg = FIRST_VFP_REGNUM; reg < LAST_VFP_REGNUM; reg += 2)
	{
	  if ((!regs_ever_live[reg] || call_used_regs[reg])
	      && (!regs_ever_live[reg + 1] || call_used_regs[reg + 1]))
	    {
	      if (start_reg != reg)
		saved_size += vfp_emit_fstmd (start_reg,
					      (reg - start_reg) / 2);
	      start_reg = reg + 2;
	    }
	}
      if (start_reg != reg)
	saved_size += vfp_emit_fstmd (start_reg,
				      (reg - start_reg) / 2);
    }
  return saved_size;
}


/* Set the Thumb frame pointer from the stack pointer.  */

static void
thumb_set_frame_pointer (arm_stack_offsets *offsets)
{
  HOST_WIDE_INT amount;
  rtx insn, dwarf;

  amount = offsets->outgoing_args - offsets->locals_base;
  if (amount < 1024)
    insn = emit_insn (gen_addsi3 (hard_frame_pointer_rtx,
				  stack_pointer_rtx, GEN_INT (amount)));
  else
    {
      emit_insn (gen_movsi (hard_frame_pointer_rtx, GEN_INT (amount)));
      insn = emit_insn (gen_addsi3 (hard_frame_pointer_rtx,
				    hard_frame_pointer_rtx,
				    stack_pointer_rtx));
      dwarf = gen_rtx_SET (VOIDmode, hard_frame_pointer_rtx,
			   plus_constant (stack_pointer_rtx, amount));
      RTX_FRAME_RELATED_P (dwarf) = 1;
      REG_NOTES (insn) = gen_rtx_EXPR_LIST (REG_FRAME_RELATED_EXPR, dwarf,
					    REG_NOTES (insn));
    }

  RTX_FRAME_RELATED_P (insn) = 1;
}

/* Generate the prologue instructions for entry into an ARM or Thumb-2
   function.  */
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
void
arm_expand_prologue (void)
{
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  /* Remove 'reg' variable */
  rtx amount;
  rtx insn;
  rtx ip_rtx;
  unsigned long live_regs_mask;
  unsigned long func_type;
  /* APPLE LOCAL ARM custom frame layout */
  /* Remove unused variable definitions.  */
  int saved_regs = 0;
  unsigned HOST_WIDE_INT args_to_push;
  arm_stack_offsets *offsets;

  func_type = arm_current_func_type ();

  /* Naked functions don't have prologues.  */
  if (IS_NAKED (func_type))
    return;

  /* Make a copy of c_f_p_a_s as we may need to modify it locally.  */
  args_to_push = current_function_pretend_args_size;

  /* Compute which register we will have to save onto the stack.  */
  live_regs_mask = arm_compute_save_reg_mask ();

  ip_rtx = gen_rtx_REG (SImode, IP_REGNUM);

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (IS_STACKALIGN (func_type))
    {
      rtx dwarf;
      rtx r0;
      rtx r1;
      /* Handle a word-aligned stack pointer.  We generate the following:

	  mov r0, sp
	  bic r1, r0, #7
	  mov sp, r1
	  <save and restore r0 in normal prologue/epilogue>
	  mov sp, r0
	  bx lr

	 The unwinder doesn't need to know about the stack realignment.
	 Just tell it we saved SP in r0.  */
      gcc_assert (TARGET_THUMB2 && !arm_arch_notm && args_to_push == 0);

      r0 = gen_rtx_REG (SImode, 0);
      r1 = gen_rtx_REG (SImode, 1);
      dwarf = gen_rtx_UNSPEC (SImode, NULL_RTVEC, UNSPEC_STACK_ALIGN);
      dwarf = gen_rtx_SET (VOIDmode, r0, dwarf);
      insn = gen_movsi (r0, stack_pointer_rtx);
      RTX_FRAME_RELATED_P (insn) = 1;
      REG_NOTES (insn) = gen_rtx_EXPR_LIST (REG_FRAME_RELATED_EXPR,
					    dwarf, REG_NOTES (insn));
      emit_insn (insn);
      emit_insn (gen_andsi3 (r1, r0, GEN_INT (~(HOST_WIDE_INT)7)));
      emit_insn (gen_movsi (stack_pointer_rtx, r1));
    }

  if (frame_pointer_needed && TARGET_ARM)
    {
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
      if (IS_INTERRUPT (func_type))
	{
	  /* Interrupt functions must not corrupt any registers.
	     Creating a frame pointer however, corrupts the IP
	     register, so we must push it first.  */
	  insn = emit_multi_reg_push (1 << IP_REGNUM);

	  /* Do not set RTX_FRAME_RELATED_P on this insn.
	     The dwarf stack unwinding code only wants to see one
	     stack decrement per function, and this is not it.  If
	     this instruction is labeled as being part of the frame
	     creation sequence then dwarf2out_frame_debug_expr will
	     die when it encounters the assignment of IP to FP
	     later on, since the use of SP here establishes SP as
	     the CFA register and not IP.

	     Anyway this instruction is not really part of the stack
	     frame creation although it is part of the prologue.  */
	}
      /* APPLE LOCAL begin ARM custom frame layout */
      else if (IS_NESTED (func_type))
	{
	  /* Our prologue doesn't corrupt IP, so no need to save it.  */
	}
      /* APPLE LOCAL end ARM custom frame layout */
    }

  if (args_to_push)
    {
      /* Push the argument registers, or reserve space for them.  */
      if (cfun->machine->uses_anonymous_args)
	insn = emit_multi_reg_push
	  ((0xf0 >> (args_to_push / 4)) & 0xf);
      else
	insn = emit_insn
	  (gen_addsi3 (stack_pointer_rtx, stack_pointer_rtx,
		       GEN_INT (- args_to_push)));
      RTX_FRAME_RELATED_P (insn) = 1;
    }

  /* If this is an interrupt service routine, and the link register
     is going to be pushed, and we are not creating a stack frame,
     (which would involve an extra push of IP and a pop in the epilogue)
     subtracting four from LR now will mean that the function return
     can be done with a single instruction.  */
  if ((func_type == ARM_FT_ISR || func_type == ARM_FT_FIQ)
      && (live_regs_mask & (1 << LR_REGNUM)) != 0
      && ! frame_pointer_needed)
    {
      rtx lr = gen_rtx_REG (SImode, LR_REGNUM);
      
      emit_set_insn (lr, plus_constant (lr, -4));
    }

  /* APPLE LOCAL begin ARM peephole combine reg store and stack push */
  offsets = arm_get_frame_offsets ();

  if (live_regs_mask)
    {
      saved_regs += bit_count (live_regs_mask) * 4;

      /* Space optimization:  if we need a small amount of stack space, and
	 we're going to do a push, push some extra registers rather than
	 doing a separate subtract.  We can safely push R0 thru R3.  We can
	 also use R12 provided no regs in the range R4..R11 are being saved.
	 (Their saved values would be below the value of R12 on the stack,
	 and would get clobbered.) */
      /* The conditions here are probably overly restrictive. */
      if (optimize_size
	  && !flag_pic
	  && !frame_pointer_needed
	  && arm_fpu_arch == FPUTYPE_NONE
	  && TARGET_SOFT_FLOAT
	  && !TARGET_IWMMXT)
	{
	  int ip_ok = 1;
	  int delta = offsets->outgoing_args - offsets->saved_args - saved_regs;
	  if (delta < 0)
	    abort();
	  if (live_regs_mask
	      & ((1<<4) | (1<<5) | (1<<6) | (1<<7) | (1<<8) | (1<<9) | (1<<10) | (1<<11)))
	    ip_ok = 0;
	  if (delta <= (ip_ok ? 20 : 16) && delta % 4 == 0)
	    {
	      int reg = (ip_ok ? 12 : 3);
	      while (delta)
		{
		  delta -= 4;
		  live_regs_mask |= (1<<reg);
		  reg = (reg == 12) ? 3 : reg - 1;
		  saved_regs += 4;
		}
	    }
	}

      /* APPLE LOCAL begin ARM custom frame layout */
      if (frame_pointer_needed)
	{
	  unsigned long regs_above_fp =
	    inclusive_bitmask (ARM_HARD_FRAME_POINTER_REGNUM + 1, 11);
	  unsigned long initial_push_regs = live_regs_mask
					    & ~regs_above_fp;
	  unsigned long second_push_regs = live_regs_mask
					    & regs_above_fp;

	  /* Save everything up to the FP, and the LR  */
	  insn = emit_multi_reg_push (initial_push_regs);
	  /* rdar://6148015 */
	  RTX_FRAME_RELATED_P (insn) = 1;

	  /* Configure FP to point to the saved FP.  */
	  insn = emit_insn (
		   gen_addsi3 (hard_frame_pointer_rtx, stack_pointer_rtx,
			       GEN_INT ((bit_count (initial_push_regs) - 2)
					* 4)));
	  RTX_FRAME_RELATED_P (insn) = 1;

	  /* Prevent attempts to optimize away the frame pointer.  */
	  emit_insn (gen_rtx_USE (VOIDmode, hard_frame_pointer_rtx));

	  /* Push remaining regs.  */
	  if (second_push_regs)
	    {
	      insn = emit_multi_reg_push (second_push_regs);
	      /* rdar://6148015 */
	      RTX_FRAME_RELATED_P (insn) = 1;
	    }
	}
      else
	{
	  insn = emit_multi_reg_push (live_regs_mask);
	  RTX_FRAME_RELATED_P (insn) = 1;
	}
      /* APPLE LOCAL end ARM custom frame layout */
    }
  /* APPLE LOCAL end ARM peephole combine reg store and stack push */

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  if (! IS_VOLATILE (func_type))
    saved_regs += arm_save_coproc_regs ();

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  /* APPLE LOCAL ARM custom frame layout */
  /* Removed lines.  */

  /* APPLE LOCAL ARM peephole combine reg store and stack push */
  /* Remove call to arm_get_frame_offsets.  */
  if (offsets->outgoing_args != offsets->saved_args + saved_regs)
    {
      /* This add can produce multiple insns for a large constant, so we
	 need to get tricky.  */
      rtx last = get_last_insn ();

      amount = GEN_INT (offsets->saved_args + saved_regs
			- offsets->outgoing_args);

      insn = emit_insn (gen_addsi3 (stack_pointer_rtx, stack_pointer_rtx,
				    amount));
      do
	{
	  last = last ? NEXT_INSN (last) : get_insns ();
	  RTX_FRAME_RELATED_P (last) = 1;
	}
      while (last != insn);

      /* If the frame pointer is needed, emit a special barrier that
	 will prevent the scheduler from moving stores to the frame
	 before the stack adjustment.  */
      if (frame_pointer_needed)
	insn = emit_insn (gen_stack_tie (stack_pointer_rtx,
					 hard_frame_pointer_rtx));
    }


  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* ALQAAHIRA LOCAL add !live_regs_mask. that's handled above by apple code */
  if (frame_pointer_needed && TARGET_THUMB2 && !live_regs_mask)
    thumb_set_frame_pointer (offsets);

  if (flag_pic && arm_pic_register != INVALID_REGNUM)
    {
      unsigned long mask;

      mask = live_regs_mask;
      mask &= THUMB2_WORK_REGS;
      if (!IS_NESTED (func_type))
	mask |= (1 << IP_REGNUM);
      arm_load_pic_register (mask);
    }

  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  /* If we are profiling, make sure no instructions are scheduled before
     the call to mcount.  Similarly if the user has requested no
     scheduling in the prolog.  Similarly if we want non-call exceptions
     using the EABI unwinder, to prevent faulting instructions from being
     swapped with a stack adjustment.  */
  if (current_function_profile || !TARGET_SCHED_PROLOG
      || (ARM_EABI_UNWIND_TABLES && flag_non_call_exceptions))
    emit_insn (gen_blockage ());

  /* If the link register is being kept alive, with the return address in it,
     then make sure that it does not get reused by the ce2 pass.  */
  if ((live_regs_mask & (1 << LR_REGNUM)) == 0)
    {
      emit_insn (gen_prologue_use (gen_rtx_REG (SImode, LR_REGNUM)));
      cfun->machine->lr_save_eliminated = 1;
    }
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Print condition code to STREAM.  Helper function for arm_print_operand.  */
static void
arm_print_condition (FILE *stream)
{
  if (arm_ccfsm_state == 3 || arm_ccfsm_state == 4)
    {
      /* Branch conversion is not implemented for Thumb-2.  */
      if (TARGET_THUMB)
	{
	  output_operand_lossage ("predicated Thumb instruction");
	  return;
	}
      if (current_insn_predicate != NULL)
	{
	  output_operand_lossage
	    ("predicated instruction in conditional sequence");
	  return;
	}

      fputs (arm_condition_codes[arm_current_cc], stream);
    }
  else if (current_insn_predicate)
    {
      enum arm_cond_code code;

      if (TARGET_THUMB1)
	{
	  output_operand_lossage ("predicated Thumb instruction");
	  return;
	}

      code = get_arm_condition_code (current_insn_predicate);
      fputs (arm_condition_codes[code], stream);
    }
}


/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* If CODE is 'd', then the X is a condition operand and the instruction
   should only be executed if the condition is true.
   if CODE is 'D', then the X is a condition operand and the instruction
   should only be executed if the condition is false: however, if the mode
   of the comparison is CCFPEmode, then always execute the instruction -- we
   do this because in these circumstances !GE does not necessarily imply LT;
   in these cases the instruction pattern will take care to make sure that
   an instruction containing %d will follow, thereby undoing the effects of
   doing this instruction unconditionally.
   If CODE is 'N' then X is a floating point operand that must be negated
   before output.
   If CODE is 'B' then output a bitwise inverted value of X (a const int).
   If X is a REG and CODE is `M', output a ldm/stm style multi-reg.  */
void
arm_print_operand (FILE *stream, rtx x, int code)
{
  switch (code)
    {
    /* APPLE LOCAL begin ARM MACH assembler */
    /* ALQAAHIRA LOCAL change appple '.' code to '~' for v7 to avoid conflict */
    case '~':
#ifdef LOCAL_LABEL_PREFIX
      fputs (LOCAL_LABEL_PREFIX, stream);
#endif
      return;
    /* APPLE LOCAL end ARM MACH assembler */

    case '@':
      fputs (ASM_COMMENT_START, stream);
      return;

    case '_':
      fputs (user_label_prefix, stream);
      return;

    case '|':
      fputs (REGISTER_PREFIX, stream);
      return;

    /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
    case '?':
      arm_print_condition (stream);
      return;

    case '(':
      /* Nothing in unified syntax, otherwise the current condition code.  */
      if (!TARGET_UNIFIED_ASM)
	arm_print_condition (stream);
      break;

    case ')':
      /* The current condition code in unified syntax, otherwise nothing.  */
      if (TARGET_UNIFIED_ASM)
	arm_print_condition (stream);
      break;
  
    case '.':
      /* The current condition code for a condition code setting instruction.
	 Preceeded by 's' in unified syntax, otherwise followed by 's'.  */
      if (TARGET_UNIFIED_ASM)
	{
	  fputc('s', stream);
	  arm_print_condition (stream);
	}
      else
	{
	  arm_print_condition (stream);
	  fputc('s', stream);
	}
      return;

    case '!':
      /* If the instruction is conditionally executed then print
	 the current condition code, otherwise print 's'.  */
      gcc_assert (TARGET_THUMB2 && TARGET_UNIFIED_ASM);
      if (current_insn_predicate)
	arm_print_condition (stream);
      else
	fputc('s', stream);
      break;

    /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
    /* %# is a "break" sequence. It doesn't output anything, but is used to
       seperate e.g. operand numbers from following text, if that text consists
       of further digits which we don't want to be part of the operand
       number.  */
    case '#':
      return;
    /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

    case 'N':
      {
	REAL_VALUE_TYPE r;
	REAL_VALUE_FROM_CONST_DOUBLE (r, x);
	r = REAL_VALUE_NEGATE (r);
	fprintf (stream, "%s", fp_const_from_val (&r));
      }
      return;

    /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
    /* An integer without a preceding # sign.  */
    case 'c':
      gcc_assert (GET_CODE (x) == CONST_INT);
      fprintf (stream, HOST_WIDE_INT_PRINT_DEC, INTVAL (x));
      return;

    /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
    case 'B':
      if (GET_CODE (x) == CONST_INT)
	{
	  HOST_WIDE_INT val;
	  val = ARM_SIGN_EXTEND (~INTVAL (x));
	  fprintf (stream, HOST_WIDE_INT_PRINT_DEC, val);
	}
      else
	{
	  putc ('~', stream);
	  output_addr_const (stream, x);
	}
      return;

    /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
    case 'L':
      /* The low 16 bits of an immediate constant.  */
      fprintf (stream, HOST_WIDE_INT_PRINT_DEC, INTVAL(x) & 0xffff);
      return;

    /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    case 'i':
      fprintf (stream, "%s", arithmetic_instr (x, 1));
      return;

    /* Truncate Cirrus shift counts.  */
    case 's':
      if (GET_CODE (x) == CONST_INT)
	{
	  fprintf (stream, HOST_WIDE_INT_PRINT_DEC, INTVAL (x) & 0x3f);
	  return;
	}
      arm_print_operand (stream, x, 0);
      return;

    case 'I':
      fprintf (stream, "%s", arithmetic_instr (x, 0));
      return;

    case 'S':
      {
	HOST_WIDE_INT val;
	const char *shift;

	if (!shift_operator (x, SImode))
	  {
	    output_operand_lossage ("invalid shift operand");
	    break;
	  }

	shift = shift_op (x, &val);

	if (shift)
	  {
	    fprintf (stream, ", %s ", shift);
	    if (val == -1)
	      arm_print_operand (stream, XEXP (x, 1), 0);
	    else
	      fprintf (stream, "#" HOST_WIDE_INT_PRINT_DEC, val);
	  }
      }
      return;

      /* An explanation of the 'Q', 'R' and 'H' register operands:

	 In a pair of registers containing a DI or DF value the 'Q'
	 operand returns the register number of the register containing
	 the least significant part of the value.  The 'R' operand returns
	 the register number of the register containing the most
	 significant part of the value.

	 The 'H' operand returns the higher of the two register numbers.
	 On a run where WORDS_BIG_ENDIAN is true the 'H' operand is the
	 same as the 'Q' operand, since the most significant part of the
	 value is held in the lower number register.  The reverse is true
	 on systems where WORDS_BIG_ENDIAN is false.

	 The purpose of these operands is to distinguish between cases
	 where the endian-ness of the values is important (for example
	 when they are added together), and cases where the endian-ness
	 is irrelevant, but the order of register operations is important.
	 For example when loading a value from memory into a register
	 pair, the endian-ness does not matter.  Provided that the value
	 from the lower memory address is put into the lower numbered
	 register, and the value from the higher address is put into the
	 higher numbered register, the load will work regardless of whether
	 the value being loaded is big-wordian or little-wordian.  The
	 order of the two register loads can matter however, if the address
	 of the memory location is actually held in one of the registers
	 being overwritten by the load.  */
    case 'Q':
      if (GET_CODE (x) != REG || REGNO (x) > LAST_ARM_REGNUM)
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      asm_fprintf (stream, "%r", REGNO (x) + (WORDS_BIG_ENDIAN ? 1 : 0));
      return;

    case 'R':
      if (GET_CODE (x) != REG || REGNO (x) > LAST_ARM_REGNUM)
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      asm_fprintf (stream, "%r", REGNO (x) + (WORDS_BIG_ENDIAN ? 0 : 1));
      return;

    case 'H':
      if (GET_CODE (x) != REG || REGNO (x) > LAST_ARM_REGNUM)
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      asm_fprintf (stream, "%r", REGNO (x) + 1);
      return;

    /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
    case 'J':
      if (GET_CODE (x) != REG || REGNO (x) > LAST_ARM_REGNUM)
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      asm_fprintf (stream, "%r", REGNO (x) + (WORDS_BIG_ENDIAN ? 3 : 2));
      return;

    case 'K':
      if (GET_CODE (x) != REG || REGNO (x) > LAST_ARM_REGNUM)
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      asm_fprintf (stream, "%r", REGNO (x) + (WORDS_BIG_ENDIAN ? 2 : 3));
      return;

    /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
    case 'm':
      asm_fprintf (stream, "%r",
		   GET_CODE (XEXP (x, 0)) == REG
		   ? REGNO (XEXP (x, 0)) : REGNO (XEXP (XEXP (x, 0), 0)));
      return;

    case 'M':
      asm_fprintf (stream, "{%r-%r}",
		   REGNO (x),
		   REGNO (x) + ARM_NUM_REGS (GET_MODE (x)) - 1);
      return;

    /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
    /* Like 'M', but writing doubleword vector registers, for use by Neon
       insns.  */
    case 'h':
      {
        int regno = (REGNO (x) - FIRST_VFP_REGNUM) / 2;
        int numregs = ARM_NUM_REGS (GET_MODE (x)) / 2;
        if (numregs == 1)
          asm_fprintf (stream, "{d%d}", regno);
        else
          asm_fprintf (stream, "{d%d-d%d}", regno, regno + numregs - 1);
      }
      return;

    /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
    case 'd':
      /* CONST_TRUE_RTX means always -- that's the default.  */
      if (x == const_true_rtx)
	return;

      if (!COMPARISON_P (x))
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      fputs (arm_condition_codes[get_arm_condition_code (x)],
	     stream);
      return;

    case 'D':
      /* CONST_TRUE_RTX means not always -- i.e. never.  We shouldn't ever
	 want to do that.  */
      if (x == const_true_rtx)
	{
	  output_operand_lossage ("instruction never exectued");
	  return;
	}
      if (!COMPARISON_P (x))
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      fputs (arm_condition_codes[ARM_INVERSE_CONDITION_CODE
				 (get_arm_condition_code (x))],
	     stream);
      return;

    /* Cirrus registers can be accessed in a variety of ways:
         single floating point (f)
	 double floating point (d)
	 32bit integer         (fx)
	 64bit integer         (dx).  */
    case 'W':			/* Cirrus register in F mode.  */
    case 'X':			/* Cirrus register in D mode.  */
    case 'Y':			/* Cirrus register in FX mode.  */
    case 'Z':			/* Cirrus register in DX mode.  */
      gcc_assert (GET_CODE (x) == REG
		  && REGNO_REG_CLASS (REGNO (x)) == CIRRUS_REGS);

      fprintf (stream, "mv%s%s",
	       code == 'W' ? "f"
	       : code == 'X' ? "d"
	       : code == 'Y' ? "fx" : "dx", reg_names[REGNO (x)] + 2);

      return;

    /* Print cirrus register in the mode specified by the register's mode.  */
    case 'V':
      {
	int mode = GET_MODE (x);

	if (GET_CODE (x) != REG || REGNO_REG_CLASS (REGNO (x)) != CIRRUS_REGS)
	  {
	    output_operand_lossage ("invalid operand for code '%c'", code);
	    return;
	  }

	fprintf (stream, "mv%s%s",
		 mode == DFmode ? "d"
		 : mode == SImode ? "fx"
		 : mode == DImode ? "dx"
		 : "f", reg_names[REGNO (x)] + 2);

	return;
      }

    case 'U':
      if (GET_CODE (x) != REG
	  || REGNO (x) < FIRST_IWMMXT_GR_REGNUM
	  || REGNO (x) > LAST_IWMMXT_GR_REGNUM)
	/* Bad value for wCG register number.  */
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      else
	fprintf (stream, "%d", REGNO (x) - FIRST_IWMMXT_GR_REGNUM);
      return;

      /* Print an iWMMXt control register name.  */
    case 'w':
      if (GET_CODE (x) != CONST_INT
	  || INTVAL (x) < 0
	  || INTVAL (x) >= 16)
	/* Bad value for wC register number.  */
	{
	  output_operand_lossage ("invalid operand for code '%c'", code);
	  return;
	}

      else
	{
	  static const char * wc_reg_names [16] =
	    {
	      "wCID",  "wCon",  "wCSSF", "wCASF",
	      "wC4",   "wC5",   "wC6",   "wC7",
	      "wCGR0", "wCGR1", "wCGR2", "wCGR3",
	      "wC12",  "wC13",  "wC14",  "wC15"
	    };

	  fprintf (stream, wc_reg_names [INTVAL (x)]);
	}
      return;

    /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
    /* Print a VFP/Neon double precision or quad precision register name.  */
    case 'P':
    case 'q':
      {
	int mode = GET_MODE (x);
        int is_quad = (code == 'q');
	int regno;

	if (GET_MODE_SIZE (mode) != (is_quad ? 16 : 8))
	  {
	    output_operand_lossage ("invalid operand for code '%c'", code);
	    return;
	  }

	if (GET_CODE (x) != REG
	    || !IS_VFP_REGNUM (REGNO (x)))
	  {
	    output_operand_lossage ("invalid operand for code '%c'", code);
	    return;
	  }

	regno = REGNO (x);
	if ((is_quad && !NEON_REGNO_OK_FOR_QUAD (regno))
            || (!is_quad && !VFP_REGNO_OK_FOR_DOUBLE (regno)))
	  {
	    output_operand_lossage ("invalid operand for code '%c'", code);
	    return;
	  }

	fprintf (stream, "%c%d", is_quad ? 'q' : 'd',
	  (regno - FIRST_VFP_REGNUM) >> (is_quad ? 2 : 1));
      }
      return;

    /* ALQAAHIRA LOCAL 6150859 begin use NEON instructions for SF math */
    /* This code prints the double precision register name starting at
       register number of the indicated single precision register.  */
    case 'p':
      {
	int mode = GET_MODE (x);
	int regno;

	if (GET_CODE (x) != REG || !IS_VFP_REGNUM (REGNO (x))
	    || GET_MODE_SIZE (mode) != 4)
	  {
	    output_operand_lossage ("invalid operand for code '%c'", code);
	    return;
	  }

	regno = REGNO (x);
	if (((regno - FIRST_VFP_REGNUM) & 0x1) != 0)
	  {
	    output_operand_lossage ("invalid operand for code '%c'", code);
	    return;
	  }

	fprintf (stream, "d%d", (regno - FIRST_VFP_REGNUM) >> 1);
      }
      return;
    /* ALQAAHIRA LOCAL 6150859 end use NEON instructions for SF math */

    /* These two codes print the low/high doubleword register of a Neon quad
       register, respectively.  For pair-structure types, can also print
       low/high quadword registers.  */
    case 'e':
    case 'f':
      {
        int mode = GET_MODE (x);
        int regno;
        
        if ((GET_MODE_SIZE (mode) != 16
	     && GET_MODE_SIZE (mode) != 32) || GET_CODE (x) != REG)
          {
	    output_operand_lossage ("invalid operand for code '%c'", code);
	    return;
          }
        
        regno = REGNO (x);
        if (!NEON_REGNO_OK_FOR_QUAD (regno))
          {
	    output_operand_lossage ("invalid operand for code '%c'", code);
	    return;
          }
        
        if (GET_MODE_SIZE (mode) == 16)
          fprintf (stream, "d%d", ((regno - FIRST_VFP_REGNUM) >> 1)
				  + (code == 'f' ? 1 : 0));
        else
          fprintf (stream, "q%d", ((regno - FIRST_VFP_REGNUM) >> 2)
				  + (code == 'f' ? 1 : 0));
      }
      return;

    /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
    /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
    /* Print a VFPv3 floating-point constant, represented as an integer
       index.  */
    case 'G':
      {
        int index = vfp3_const_double_index (x);
	gcc_assert (index != -1);
	fprintf (stream, "%d", index);
      }
      return;

    /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
    /* Print bits representing opcode features for Neon.

       Bit 0 is 1 for signed, 0 for unsigned.  Floats count as signed
       and polynomials as unsigned.

       Bit 1 is 1 for rounding functions, 0 otherwise.

       Bit 2 is 1 for floats and polynomials, 0 for ordinary integers.  */

    /* Identify the type as 's', 'u', 'p' or 'f'.  */
    case 'T':
      {
        HOST_WIDE_INT bits = INTVAL (x);
        fputc ((bits & 1) != 0
	       ? ((bits & 4) != 0 ? 'f' : 's')
	       : ((bits & 4) != 0 ? 'p' : 'u'),
	       stream);
      }
      return;

    /* Likewise, but signed and unsigned integers are both 'i'.  */
    case 'F':
      {
        HOST_WIDE_INT bits = INTVAL (x);
        fputc ((bits & 4) != 0
	       ? ((bits & 1) != 0 ? 'f' : 'p')
	       : 'i',
	       stream);
      }
      return;

    /* As for 'T', but emit 'u' instead of 'p'.  */
    case 't':
      {
        HOST_WIDE_INT bits = INTVAL (x);
        fputc ((bits & 1) != 0
	       ? ((bits & 4) != 0 ? 'f' : 's')
	       : 'u',
	       stream);
      }
      return;

    /* Bit 1: rounding (vs none).  */
    case 'O':
      {
        HOST_WIDE_INT bits = INTVAL (x);
        fputs ((bits & 2) != 0 ? "r" : "", stream);
      }
      return;

    /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
    default:
      if (x == 0)
	{
	  output_operand_lossage ("missing operand");
	  return;
	}

      switch (GET_CODE (x))
	{
	case REG:
	  asm_fprintf (stream, "%r", REGNO (x));
	  break;

	case MEM:
	  output_memory_reference_mode = GET_MODE (x);
	  output_address (XEXP (x, 0));
	  break;

	case CONST_DOUBLE:
          /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
          if (TARGET_NEON)
            {
              char fpstr[20];
              real_to_decimal (fpstr, CONST_DOUBLE_REAL_VALUE (x),
			       sizeof (fpstr), 0, 1);
              fprintf (stream, "#%s", fpstr);
            }
          else
	    fprintf (stream, "#%s", fp_immediate_constant (x));
          /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
	  break;

	default:
	  gcc_assert (GET_CODE (x) != NEG);
	  fputc ('#', stream);
	  output_addr_const (stream, x);
	  break;
	}
    }
}

#ifndef AOF_ASSEMBLER
/* Target hook for assembling integer objects.  The ARM version needs to
   handle word-sized values specially.  */
static bool
arm_assemble_integer (rtx x, unsigned int size, int aligned_p)
{
  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  enum machine_mode mode;

  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  /* APPLE LOCAL begin ARM MACH assembler */
  /* We can always handle unaligned data with the normal pseudoops.  */
  if (TARGET_MACHO)
    aligned_p = 1;
  /* APPLE LOCAL end ARM MACH assembler */

  if (size == UNITS_PER_WORD && aligned_p)
    {
      /* APPLE LOCAL ARM MACH assembler */
      fputs ("\t" DOT_WORD "\t", asm_out_file);
      output_addr_const (asm_out_file, x);

      /* Mark symbols as position independent.  We only do this in the
	 .text segment, not in the .data segment.  */
      if (NEED_GOT_RELOC && flag_pic && making_const_table &&
	  (GET_CODE (x) == SYMBOL_REF || GET_CODE (x) == LABEL_REF))
	{
	  if (GET_CODE (x) == SYMBOL_REF
	      && (CONSTANT_POOL_ADDRESS_P (x)
		  || SYMBOL_REF_LOCAL_P (x)))
	    fputs ("(GOTOFF)", asm_out_file);
	  else if (GET_CODE (x) == LABEL_REF)
	    fputs ("(GOTOFF)", asm_out_file);
	  else
	    fputs ("(GOT)", asm_out_file);
	}
      fputc ('\n', asm_out_file);
      return true;
    }

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  mode = GET_MODE (x);

  if (arm_vector_mode_supported_p (mode))
    {
      int i, units;
      unsigned int invmask = 0, parts_per_word;

      gcc_assert (GET_CODE (x) == CONST_VECTOR);

      units = CONST_VECTOR_NUNITS (x);
      size = GET_MODE_SIZE (GET_MODE_INNER (mode));
      
      /* For big-endian Neon vectors, we must permute the vector to the form
         which, when loaded by a VLDR or VLDM instruction, will give a vector
         with the elements in the right order.  */
      if (TARGET_NEON && WORDS_BIG_ENDIAN)
        {
          parts_per_word = UNITS_PER_WORD / size;
          /* FIXME: This might be wrong for 64-bit vector elements, but we don't
             support those anywhere yet.  */
          invmask = (parts_per_word == 0) ? 0 : (1 << (parts_per_word - 1)) - 1;
        }
      
      if (GET_MODE_CLASS (mode) == MODE_VECTOR_INT)
      for (i = 0; i < units; i++)
	{
	    rtx elt = CONST_VECTOR_ELT (x, i ^ invmask);
	  assemble_integer
	    (elt, size, i == 0 ? BIGGEST_ALIGNMENT : size * BITS_PER_UNIT, 1);
	}
      else
        for (i = 0; i < units; i++)
          {
            rtx elt = CONST_VECTOR_ELT (x, i);
            REAL_VALUE_TYPE rval;
            
            REAL_VALUE_FROM_CONST_DOUBLE (rval, elt);
            
            assemble_real
              (rval, GET_MODE_INNER (mode),
              i == 0 ? BIGGEST_ALIGNMENT : size * BITS_PER_UNIT);
          }

      return true;
    }
  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

  return default_assemble_integer (x, size, aligned_p);
}


/* APPLE LOCAL ARM macho file format */
#ifdef OBJECT_FORMAT_ELF
/* Add a function to the list of static constructors.  */

static void
arm_elf_asm_constructor (rtx symbol, int priority ATTRIBUTE_UNUSED)
{
  if (!TARGET_AAPCS_BASED)
    {
      default_named_section_asm_out_constructor (symbol, priority);
      return;
    }

  /* Put these in the .init_array section, using a special relocation.  */
  switch_to_section (ctors_section);
  assemble_align (POINTER_SIZE);
  fputs ("\t.word\t", asm_out_file);
  output_addr_const (asm_out_file, symbol);
  fputs ("(target1)\n", asm_out_file);
}
/* APPLE LOCAL ARM macho file format */
#endif
#endif

/* A finite state machine takes care of noticing whether or not instructions
   can be conditionally executed, and thus decrease execution time and code
   size by deleting branch instructions.  The fsm is controlled by
   final_prescan_insn, and controls the actions of ASM_OUTPUT_OPCODE.  */

/* The state of the fsm controlling condition codes are:
   0: normal, do nothing special
   1: make ASM_OUTPUT_OPCODE not output this instruction
   2: make ASM_OUTPUT_OPCODE not output this instruction
   3: make instructions conditional
   4: make instructions conditional

   State transitions (state->state by whom under condition):
   0 -> 1 final_prescan_insn if the `target' is a label
   0 -> 2 final_prescan_insn if the `target' is an unconditional branch
   1 -> 3 ASM_OUTPUT_OPCODE after not having output the conditional branch
   2 -> 4 ASM_OUTPUT_OPCODE after not having output the conditional branch
   3 -> 0 (*targetm.asm_out.internal_label) if the `target' label is reached
          (the target label has CODE_LABEL_NUMBER equal to arm_target_label).
   4 -> 0 final_prescan_insn if the `target' unconditional branch is reached
          (the target insn is arm_target_insn).

   If the jump clobbers the conditions then we use states 2 and 4.

   A similar thing can be done with conditional return insns.

   XXX In case the `target' is an unconditional branch, this conditionalising
   of the instructions always reduces code size, but not always execution
   time.  But then, I want to reduce the code size to somewhere near what
   /bin/cc produces.  */

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* In addition to this, state is maintained for Thumb-2 COND_EXEC
   instructions.  When a COND_EXEC instruction is seen the subsequent
   instructions are scanned so that multiple conditional instructions can be
   combined into a single IT block.  arm_condexec_count and arm_condexec_mask
   specify the length and true/false mask for the IT block.  These will be
   decremented/zeroed by arm_asm_output_opcode as the insns are output.  */

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* Returns the index of the ARM condition code string in
   `arm_condition_codes'.  COMPARISON should be an rtx like
   `(eq (...) (...))'.  */
static enum arm_cond_code
get_arm_condition_code (rtx comparison)
{
  enum machine_mode mode = GET_MODE (XEXP (comparison, 0));
  int code;
  enum rtx_code comp_code = GET_CODE (comparison);

  if (GET_MODE_CLASS (mode) != MODE_CC)
    mode = SELECT_CC_MODE (comp_code, XEXP (comparison, 0),
			   XEXP (comparison, 1));

  switch (mode)
    {
    case CC_DNEmode: code = ARM_NE; goto dominance;
    case CC_DEQmode: code = ARM_EQ; goto dominance;
    case CC_DGEmode: code = ARM_GE; goto dominance;
    case CC_DGTmode: code = ARM_GT; goto dominance;
    case CC_DLEmode: code = ARM_LE; goto dominance;
    case CC_DLTmode: code = ARM_LT; goto dominance;
    case CC_DGEUmode: code = ARM_CS; goto dominance;
    case CC_DGTUmode: code = ARM_HI; goto dominance;
    case CC_DLEUmode: code = ARM_LS; goto dominance;
    case CC_DLTUmode: code = ARM_CC;

    dominance:
      gcc_assert (comp_code == EQ || comp_code == NE);

      if (comp_code == EQ)
	return ARM_INVERSE_CONDITION_CODE (code);
      return code;

    case CC_NOOVmode:
      switch (comp_code)
	{
	case NE: return ARM_NE;
	case EQ: return ARM_EQ;
	case GE: return ARM_PL;
	case LT: return ARM_MI;
	default: gcc_unreachable ();
	}

    case CC_Zmode:
      switch (comp_code)
	{
	case NE: return ARM_NE;
	case EQ: return ARM_EQ;
	default: gcc_unreachable ();
	}

    case CC_Nmode:
      switch (comp_code)
	{
	case NE: return ARM_MI;
	case EQ: return ARM_PL;
	default: gcc_unreachable ();
	}

    case CCFPEmode:
    case CCFPmode:
      /* These encodings assume that AC=1 in the FPA system control
	 byte.  This allows us to handle all cases except UNEQ and
	 LTGT.  */
      switch (comp_code)
	{
	case GE: return ARM_GE;
	case GT: return ARM_GT;
	case LE: return ARM_LS;
	case LT: return ARM_MI;
	case NE: return ARM_NE;
	case EQ: return ARM_EQ;
	case ORDERED: return ARM_VC;
	case UNORDERED: return ARM_VS;
	case UNLT: return ARM_LT;
	case UNLE: return ARM_LE;
	case UNGT: return ARM_HI;
	case UNGE: return ARM_PL;
	  /* UNEQ and LTGT do not have a representation.  */
	case UNEQ: /* Fall through.  */
	case LTGT: /* Fall through.  */
	default: gcc_unreachable ();
	}

    case CC_SWPmode:
      switch (comp_code)
	{
	case NE: return ARM_NE;
	case EQ: return ARM_EQ;
	case GE: return ARM_LE;
	case GT: return ARM_LT;
	case LE: return ARM_GE;
	case LT: return ARM_GT;
	case GEU: return ARM_LS;
	case GTU: return ARM_CC;
	case LEU: return ARM_CS;
	case LTU: return ARM_HI;
	default: gcc_unreachable ();
	}

    case CC_Cmode:
      switch (comp_code)
      {
      case LTU: return ARM_CS;
      case GEU: return ARM_CC;
      default: gcc_unreachable ();
      }

    case CCmode:
      switch (comp_code)
	{
	case NE: return ARM_NE;
	case EQ: return ARM_EQ;
	case GE: return ARM_GE;
	case GT: return ARM_GT;
	case LE: return ARM_LE;
	case LT: return ARM_LT;
	case GEU: return ARM_CS;
	case GTU: return ARM_HI;
	case LEU: return ARM_LS;
	case LTU: return ARM_CC;
	default: gcc_unreachable ();
	}

    default: gcc_unreachable ();
    }
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Tell arm_asm_ouput_opcode to output IT blocks for conditionally executed
   instructions.  */
void
thumb2_final_prescan_insn (rtx insn)
{
  rtx first_insn = insn;
  rtx body = PATTERN (insn);
  rtx predicate;
  enum arm_cond_code code;
  int n;
  int mask;

  /* Remove the previous insn from the count of insns to be output.  */
  if (arm_condexec_count)
      arm_condexec_count--;

  /* Nothing to do if we are already inside a conditional block.  */
  if (arm_condexec_count)
    return;

  if (GET_CODE (body) != COND_EXEC)
    return;

  /* Conditional jumps are implemented directly.  */
  if (GET_CODE (insn) == JUMP_INSN)
    return;

  predicate = COND_EXEC_TEST (body);
  arm_current_cc = get_arm_condition_code (predicate);

  n = get_attr_ce_count (insn);
  arm_condexec_count = 1;
  arm_condexec_mask = (1 << n) - 1;
  arm_condexec_masklen = n;
  /* See if subsequent instructions can be combined into the same block.  */
  for (;;)
    {
      insn = next_nonnote_insn (insn);

      /* Jumping into the middle of an IT block is illegal, so a label or
         barrier terminates the block.  */
      if (GET_CODE (insn) != INSN && GET_CODE(insn) != JUMP_INSN)
	break;

      body = PATTERN (insn);
      /* USE and CLOBBER aren't really insns, so just skip them.  */
      if (GET_CODE (body) == USE
	  || GET_CODE (body) == CLOBBER)
	continue;

      /* ??? Recognise conditional jumps, and combine them with IT blocks.  */
      if (GET_CODE (body) != COND_EXEC)
	break;
      /* Allow up to 4 conditionally executed instructions in a block.  */
      n = get_attr_ce_count (insn);
      if (arm_condexec_masklen + n > 4)
	break;

      predicate = COND_EXEC_TEST (body);
      code = get_arm_condition_code (predicate);
      mask = (1 << n) - 1;
      if (arm_current_cc == code)
	arm_condexec_mask |= (mask << arm_condexec_masklen);
      else if (arm_current_cc != ARM_INVERSE_CONDITION_CODE(code))
	break;

      arm_condexec_count++;
      arm_condexec_masklen += n;

      /* A jump must be the last instruction in a conditional block.  */
      if (GET_CODE(insn) == JUMP_INSN)
	break;
    }
  /* Restore recog_data (getting the attributes of other insns can
     destroy this array, but final.c assumes that it remains intact
     across this call).  */
  extract_constrain_insn_cached (first_insn);
}

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
void
arm_final_prescan_insn (rtx insn)
{
  /* BODY will hold the body of INSN.  */
  rtx body = PATTERN (insn);

  /* This will be 1 if trying to repeat the trick, and things need to be
     reversed if it appears to fail.  */
  int reverse = 0;

  /* JUMP_CLOBBERS will be one implies that the conditions if a branch is
     taken are clobbered, even if the rtl suggests otherwise.  It also
     means that we have to grub around within the jump expression to find
     out what the conditions are when the jump isn't taken.  */
  int jump_clobbers = 0;

  /* If we start with a return insn, we only succeed if we find another one.  */
  int seeking_return = 0;

  /* START_INSN will hold the insn from where we start looking.  This is the
     first insn after the following code_label if REVERSE is true.  */
  rtx start_insn = insn;

  /* If in state 4, check if the target branch is reached, in order to
     change back to state 0.  */
  if (arm_ccfsm_state == 4)
    {
      if (insn == arm_target_insn)
	{
	  arm_target_insn = NULL;
	  arm_ccfsm_state = 0;
	}
      return;
    }

  /* If in state 3, it is possible to repeat the trick, if this insn is an
     unconditional branch to a label, and immediately following this branch
     is the previous target label which is only used once, and the label this
     branch jumps to is not too far off.  */
  if (arm_ccfsm_state == 3)
    {
      if (simplejump_p (insn))
	{
	  start_insn = next_nonnote_insn (start_insn);
	  if (GET_CODE (start_insn) == BARRIER)
	    {
	      /* XXX Isn't this always a barrier?  */
	      start_insn = next_nonnote_insn (start_insn);
	    }
	  if (GET_CODE (start_insn) == CODE_LABEL
	      && CODE_LABEL_NUMBER (start_insn) == arm_target_label
	      && LABEL_NUSES (start_insn) == 1)
	    reverse = TRUE;
	  else
	    return;
	}
      else if (GET_CODE (body) == RETURN)
        {
	  start_insn = next_nonnote_insn (start_insn);
	  if (GET_CODE (start_insn) == BARRIER)
	    start_insn = next_nonnote_insn (start_insn);
	  if (GET_CODE (start_insn) == CODE_LABEL
	      && CODE_LABEL_NUMBER (start_insn) == arm_target_label
	      && LABEL_NUSES (start_insn) == 1)
	    {
	      reverse = TRUE;
	      seeking_return = 1;
	    }
	  else
	    return;
        }
      else
	return;
    }

  gcc_assert (!arm_ccfsm_state || reverse);
  if (GET_CODE (insn) != JUMP_INSN)
    return;

  /* This jump might be paralleled with a clobber of the condition codes
     the jump should always come first */
  if (GET_CODE (body) == PARALLEL && XVECLEN (body, 0) > 0)
    body = XVECEXP (body, 0, 0);

  if (reverse
      || (GET_CODE (body) == SET && GET_CODE (SET_DEST (body)) == PC
	  && GET_CODE (SET_SRC (body)) == IF_THEN_ELSE))
    {
      int insns_skipped;
      int fail = FALSE, succeed = FALSE;
      /* Flag which part of the IF_THEN_ELSE is the LABEL_REF.  */
      int then_not_else = TRUE;
      rtx this_insn = start_insn, label = 0;

      /* If the jump cannot be done with one instruction, we cannot
	 conditionally execute the instruction in the inverse case.  */
      if (get_attr_conds (insn) == CONDS_JUMP_CLOB)
	{
	  jump_clobbers = 1;
	  return;
	}

      /* Register the insn jumped to.  */
      if (reverse)
        {
	  if (!seeking_return)
	    label = XEXP (SET_SRC (body), 0);
        }
      else if (GET_CODE (XEXP (SET_SRC (body), 1)) == LABEL_REF)
	label = XEXP (XEXP (SET_SRC (body), 1), 0);
      else if (GET_CODE (XEXP (SET_SRC (body), 2)) == LABEL_REF)
	{
	  label = XEXP (XEXP (SET_SRC (body), 2), 0);
	  then_not_else = FALSE;
	}
      else if (GET_CODE (XEXP (SET_SRC (body), 1)) == RETURN)
	seeking_return = 1;
      else if (GET_CODE (XEXP (SET_SRC (body), 2)) == RETURN)
        {
	  seeking_return = 1;
	  then_not_else = FALSE;
        }
      else
	gcc_unreachable ();

      /* See how many insns this branch skips, and what kind of insns.  If all
	 insns are okay, and the label or unconditional branch to the same
	 label is not too far away, succeed.  */
      for (insns_skipped = 0;
	   !fail && !succeed && insns_skipped++ < max_insns_skipped;)
	{
	  rtx scanbody;

	  this_insn = next_nonnote_insn (this_insn);
	  if (!this_insn)
	    break;

	  switch (GET_CODE (this_insn))
	    {
	    case CODE_LABEL:
	      /* Succeed if it is the target label, otherwise fail since
		 control falls in from somewhere else.  */
	      if (this_insn == label)
		{
		  if (jump_clobbers)
		    {
		      arm_ccfsm_state = 2;
		      this_insn = next_nonnote_insn (this_insn);
		    }
		  else
		    arm_ccfsm_state = 1;
		  succeed = TRUE;
		}
	      else
		fail = TRUE;
	      break;

	    case BARRIER:
	      /* Succeed if the following insn is the target label.
		 Otherwise fail.
		 If return insns are used then the last insn in a function
		 will be a barrier.  */
	      this_insn = next_nonnote_insn (this_insn);
	      if (this_insn && this_insn == label)
		{
		  if (jump_clobbers)
		    {
		      arm_ccfsm_state = 2;
		      this_insn = next_nonnote_insn (this_insn);
		    }
		  else
		    arm_ccfsm_state = 1;
		  succeed = TRUE;
		}
	      else
		fail = TRUE;
	      break;

	    case CALL_INSN:
	      /* The AAPCS says that conditional calls should not be
		 used since they make interworking inefficient (the
		 linker can't transform BL<cond> into BLX).  That's
		 only a problem if the machine has BLX.  */
	      if (arm_arch5)
		{
		  fail = TRUE;
		  break;
		}

	      /* Succeed if the following insn is the target label, or
		 if the following two insns are a barrier and the
		 target label.  */
	      this_insn = next_nonnote_insn (this_insn);
	      if (this_insn && GET_CODE (this_insn) == BARRIER)
		this_insn = next_nonnote_insn (this_insn);

	      if (this_insn && this_insn == label
		  && insns_skipped < max_insns_skipped)
		{
		  if (jump_clobbers)
		    {
		      arm_ccfsm_state = 2;
		      this_insn = next_nonnote_insn (this_insn);
		    }
		  else
		    arm_ccfsm_state = 1;
		  succeed = TRUE;
		}
	      else
		fail = TRUE;
	      break;

	    case JUMP_INSN:
      	      /* If this is an unconditional branch to the same label, succeed.
		 If it is to another label, do nothing.  If it is conditional,
		 fail.  */
	      /* XXX Probably, the tests for SET and the PC are
		 unnecessary.  */

	      scanbody = PATTERN (this_insn);
	      if (GET_CODE (scanbody) == SET
		  && GET_CODE (SET_DEST (scanbody)) == PC)
		{
		  if (GET_CODE (SET_SRC (scanbody)) == LABEL_REF
		      && XEXP (SET_SRC (scanbody), 0) == label && !reverse)
		    {
		      arm_ccfsm_state = 2;
		      succeed = TRUE;
		    }
		  else if (GET_CODE (SET_SRC (scanbody)) == IF_THEN_ELSE)
		    fail = TRUE;
		}
	      /* Fail if a conditional return is undesirable (e.g. on a
		 StrongARM), but still allow this if optimizing for size.  */
	      else if (GET_CODE (scanbody) == RETURN
		       && !use_return_insn (TRUE, NULL)
		       && !optimize_size)
		fail = TRUE;
	      else if (GET_CODE (scanbody) == RETURN
		       && seeking_return)
	        {
		  arm_ccfsm_state = 2;
		  succeed = TRUE;
	        }
	      else if (GET_CODE (scanbody) == PARALLEL)
	        {
		  switch (get_attr_conds (this_insn))
		    {
		    case CONDS_NOCOND:
		      break;
		    default:
		      fail = TRUE;
		      break;
		    }
		}
	      else
		fail = TRUE;	/* Unrecognized jump (e.g. epilogue).  */

	      break;

	    case INSN:
	      /* Instructions using or affecting the condition codes make it
		 fail.  */
	      scanbody = PATTERN (this_insn);
	      if (!(GET_CODE (scanbody) == SET
		    || GET_CODE (scanbody) == PARALLEL)
		  || get_attr_conds (this_insn) != CONDS_NOCOND)
		fail = TRUE;

	      /* A conditional cirrus instruction must be followed by
		 a non Cirrus instruction.  However, since we
		 conditionalize instructions in this function and by
		 the time we get here we can't add instructions
		 (nops), because shorten_branches() has already been
		 called, we will disable conditionalizing Cirrus
		 instructions to be safe.  */
	      if (GET_CODE (scanbody) != USE
		  && GET_CODE (scanbody) != CLOBBER
		  && get_attr_cirrus (this_insn) != CIRRUS_NOT)
		fail = TRUE;

	      /* ALQAAHIRA LOCAL begin 6280380 */
	      /* While most ARM instructions are predicable, a few
		 like NEON instructions aren't... */
	      if (GET_CODE (scanbody) != USE
		  && GET_CODE (scanbody) != CLOBBER
		  && get_attr_predicable (this_insn) == PREDICABLE_NO)
		fail = TRUE;
	      /* ALQAAHIRA LOCAL end 6280380 */
	      break;

	    default:
	      break;
	    }
	}
      if (succeed)
	{
	  if ((!seeking_return) && (arm_ccfsm_state == 1 || reverse))
	    arm_target_label = CODE_LABEL_NUMBER (label);
	  else
	    {
	      gcc_assert (seeking_return || arm_ccfsm_state == 2);

	      while (this_insn && GET_CODE (PATTERN (this_insn)) == USE)
	        {
		  this_insn = next_nonnote_insn (this_insn);
		  gcc_assert (!this_insn
			      || (GET_CODE (this_insn) != BARRIER
				  && GET_CODE (this_insn) != CODE_LABEL));
	        }
	      if (!this_insn)
	        {
		  /* Oh, dear! we ran off the end.. give up.  */
                  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
		  extract_constrain_insn_cached (insn);
		  arm_ccfsm_state = 0;
		  arm_target_insn = NULL;
		  return;
	        }
	      arm_target_insn = this_insn;
	    }
	  if (jump_clobbers)
	    {
	      gcc_assert (!reverse);
	      arm_current_cc =
		  get_arm_condition_code (XEXP (XEXP (XEXP (SET_SRC (body),
							    0), 0), 1));
	      if (GET_CODE (XEXP (XEXP (SET_SRC (body), 0), 0)) == AND)
		arm_current_cc = ARM_INVERSE_CONDITION_CODE (arm_current_cc);
	      if (GET_CODE (XEXP (SET_SRC (body), 0)) == NE)
		arm_current_cc = ARM_INVERSE_CONDITION_CODE (arm_current_cc);
	    }
	  else
	    {
	      /* If REVERSE is true, ARM_CURRENT_CC needs to be inverted from
		 what it was.  */
	      if (!reverse)
		arm_current_cc = get_arm_condition_code (XEXP (SET_SRC (body),
							       0));
	    }

	  if (reverse || then_not_else)
	    arm_current_cc = ARM_INVERSE_CONDITION_CODE (arm_current_cc);
	}

      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      /* Restore recog_data (getting the attributes of other insns can
	 destroy this array, but final.c assumes that it remains intact
	 across this call.  */
      extract_constrain_insn_cached (insn);
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    }
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Output IT instructions.  */
void
thumb2_asm_output_opcode (FILE * stream)
{
  char buff[5];
  int n;

  if (arm_condexec_mask)
    {
      for (n = 0; n < arm_condexec_masklen; n++)
	buff[n] = (arm_condexec_mask & (1 << n)) ? 't' : 'e';
      buff[n] = 0;
      asm_fprintf(stream, "i%s\t%s\n\t", buff,
		  arm_condition_codes[arm_current_cc]);
      arm_condexec_mask = 0;
    }
}

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* Returns true if REGNO is a valid register
   for holding a quantity of type MODE.  */
int
arm_hard_regno_mode_ok (unsigned int regno, enum machine_mode mode)
{
  if (GET_MODE_CLASS (mode) == MODE_CC)
    return (regno == CC_REGNUM
	    || (TARGET_HARD_FLOAT && TARGET_VFP
		&& regno == VFPCC_REGNUM));

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (TARGET_THUMB1)
    /* For the Thumb we only allow values bigger than SImode in
       registers 0 - 6, so that there is always a second low
       register available to hold the upper part of the value.
       We probably we ought to ensure that the register is the
       start of an even numbered register pair.  */
    return (ARM_NUM_REGS (mode) < 2) || (regno < LAST_LO_REGNUM);

  if (TARGET_HARD_FLOAT && TARGET_MAVERICK
      && IS_CIRRUS_REGNUM (regno))
    /* We have outlawed SI values in Cirrus registers because they
       reside in the lower 32 bits, but SF values reside in the
       upper 32 bits.  This causes gcc all sorts of grief.  We can't
       even split the registers into pairs because Cirrus SI values
       get sign extended to 64bits-- aldyh.  */
    return (GET_MODE_CLASS (mode) == MODE_FLOAT) || (mode == DImode);

  if (TARGET_HARD_FLOAT && TARGET_VFP
      && IS_VFP_REGNUM (regno))
    {
      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      if (mode == SFmode || mode == SImode)
	return VFP_REGNO_OK_FOR_SINGLE (regno);

      if (mode == DFmode)
	return VFP_REGNO_OK_FOR_DOUBLE (regno);
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
      /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
      
      if (TARGET_NEON)
        return (VALID_NEON_DREG_MODE (mode) && VFP_REGNO_OK_FOR_DOUBLE (regno))
               || (VALID_NEON_QREG_MODE (mode)
                   && NEON_REGNO_OK_FOR_QUAD (regno))
	       || (mode == TImode && NEON_REGNO_OK_FOR_NREGS (regno, 2))
	       || (mode == EImode && NEON_REGNO_OK_FOR_NREGS (regno, 3))
	       || (mode == OImode && NEON_REGNO_OK_FOR_NREGS (regno, 4))
	       || (mode == CImode && NEON_REGNO_OK_FOR_NREGS (regno, 6))
	       || (mode == XImode && NEON_REGNO_OK_FOR_NREGS (regno, 8));
      
      /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
      return FALSE;
    }

  if (TARGET_REALLY_IWMMXT)
    {
      if (IS_IWMMXT_GR_REGNUM (regno))
	return mode == SImode;

      if (IS_IWMMXT_REGNUM (regno))
	return VALID_IWMMXT_REG_MODE (mode);
    }
  
  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  /* We allow any value to be stored in the general registers.
     Restrict doubleword quantities to even register pairs so that we can
     use ldrd.  Do not allow Neon structure opaque modes in general registers;
     they would use too many.  */
  if (regno <= LAST_ARM_REGNUM)
    return !(TARGET_LDRD && GET_MODE_SIZE (mode) > 4 && (regno & 1) != 0)
      && !VALID_NEON_STRUCT_MODE (mode);
  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

  if (regno == FRAME_POINTER_REGNUM
      || regno == ARG_POINTER_REGNUM)
    /* We only allow integers in the fake hard registers.  */
    return GET_MODE_CLASS (mode) == MODE_INT;

  /* The only registers left are the FPA registers
     which we only allow to hold FP values.  */
  return (TARGET_HARD_FLOAT && TARGET_FPA
	  && GET_MODE_CLASS (mode) == MODE_FLOAT
	  && regno >= FIRST_FPA_REGNUM
	  && regno <= LAST_FPA_REGNUM);
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* For efficiency and historical reasons LO_REGS, HI_REGS and CC_REGS are
   not used in arm mode.  */
int
arm_regno_class (int regno)
{
  if (TARGET_THUMB1)
    {
      if (regno == STACK_POINTER_REGNUM)
	return STACK_REG;
      if (regno == CC_REGNUM)
	return CC_REG;
      if (regno < 8)
	return LO_REGS;
      return HI_REGS;
    }

  if (TARGET_THUMB2 && regno < 8)
    return LO_REGS;

  if (   regno <= LAST_ARM_REGNUM
      || regno == FRAME_POINTER_REGNUM
      || regno == ARG_POINTER_REGNUM)
    return TARGET_THUMB2 ? HI_REGS : GENERAL_REGS;

  if (regno == CC_REGNUM || regno == VFPCC_REGNUM)
    return TARGET_THUMB2 ? CC_REG : NO_REGS;

  if (IS_CIRRUS_REGNUM (regno))
    return CIRRUS_REGS;

  if (IS_VFP_REGNUM (regno))
    {
      if (regno <= D7_VFP_REGNUM)
	return VFP_D0_D7_REGS;
      else if (regno <= LAST_LO_VFP_REGNUM)
        return VFP_LO_REGS;
      else
        return VFP_HI_REGS;
    }

  if (IS_IWMMXT_REGNUM (regno))
    return IWMMXT_REGS;

  if (IS_IWMMXT_GR_REGNUM (regno))
    return IWMMXT_GR_REGS;

  return FPA_REGS;
}
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */

/* Handle a special case when computing the offset
   of an argument from the frame pointer.  */
int
arm_debugger_arg_offset (int value, rtx addr)
{
  rtx insn;

  /* APPLE LOCAL begin ARM prefer SP to FP */
  /* If we generated a frame, but the offset is from the SP anyway, then
     we have to adjust the offset to be FP-relative, as that's what gdb
     will be expecting.  */
  if (frame_pointer_needed)
    {
      if ((GET_CODE (addr) == REG) && (REGNO (addr) == SP_REGNUM))
	return arm_local_debug_offset (addr);

      if (GET_CODE (addr) == PLUS
	  && GET_CODE (XEXP (addr, 0)) == REG
	  && REGNO (XEXP (addr, 0)) == SP_REGNUM)
	return arm_local_debug_offset (addr);
    }

  /* We are only interested if dbxout_parms() failed to compute the offset.  */
  if (value != 0)
    return value;
  /* APPLE LOCAL end ARM prefer SP to FP */

  /* We can only cope with the case where the address is held in a register.  */
  if (GET_CODE (addr) != REG)
    return 0;

  /* If we are using the frame pointer to point at the argument, then
     an offset of 0 is correct.  */
  if (REGNO (addr) == (unsigned) HARD_FRAME_POINTER_REGNUM)
    return 0;

  /* If we are using the stack pointer to point at the
     argument, then an offset of 0 is correct.  */
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  /* ??? Check this is consistent with thumb2 frame layout.  */
  if ((TARGET_THUMB || !frame_pointer_needed)
      && REGNO (addr) == SP_REGNUM)
    return 0;

  /* Oh dear.  The argument is pointed to by a register rather
     than being held in a register, or being stored at a known
     offset from the frame pointer.  Since GDB only understands
     those two kinds of argument we must translate the address
     held in the register into an offset from the frame pointer.
     We do this by searching through the insns for the function
     looking to see where this register gets its value.  If the
     register is initialized from the frame pointer plus an offset
     then we are in luck and we can continue, otherwise we give up.

     This code is exercised by producing debugging information
     for a function with arguments like this:

           double func (double a, double b, int c, double d) {return d;}

     Without this code the stab for parameter 'd' will be set to
     an offset of 0 from the frame pointer, rather than 8.  */

  /* The if() statement says:

     If the insn is a normal instruction
     and if the insn is setting the value in a register
     and if the register being set is the register holding the address of the argument
     and if the address is computing by an addition
     that involves adding to a register
     which is the frame pointer
     a constant integer

     then...  */

  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      if (   GET_CODE (insn) == INSN
	  && GET_CODE (PATTERN (insn)) == SET
	  && REGNO    (XEXP (PATTERN (insn), 0)) == REGNO (addr)
	  && GET_CODE (XEXP (PATTERN (insn), 1)) == PLUS
	  && GET_CODE (XEXP (XEXP (PATTERN (insn), 1), 0)) == REG
	  && REGNO    (XEXP (XEXP (PATTERN (insn), 1), 0)) == (unsigned) HARD_FRAME_POINTER_REGNUM
	  && GET_CODE (XEXP (XEXP (PATTERN (insn), 1), 1)) == CONST_INT
	     )
	{
	  value = INTVAL (XEXP (XEXP (PATTERN (insn), 1), 1));

	  break;
	}
    }

  if (value == 0)
    {
      debug_rtx (addr);
      warning (0, "unable to compute real location of stacked parameter");
      value = 8; /* XXX magic hack */
    }

  return value;
}

#define def_mbuiltin(MASK, NAME, TYPE, CODE)				\
  do									\
    {									\
      if ((MASK) & insn_flags)						\
        lang_hooks.builtin_function ((NAME), (TYPE), (CODE),		\
				     BUILT_IN_MD, NULL, NULL_TREE);	\
    }									\
  while (0)

struct builtin_description
{
  const unsigned int       mask;
  const enum insn_code     icode;
  const char * const       name;
  const enum arm_builtins  code;
  const enum rtx_code      comparison;
  const unsigned int       flag;
};

static const struct builtin_description bdesc_2arg[] =
{
#define IWMMXT_BUILTIN(code, string, builtin) \
  { FL_IWMMXT, CODE_FOR_##code, "__builtin_arm_" string, \
    ARM_BUILTIN_##builtin, 0, 0 },

  IWMMXT_BUILTIN (addv8qi3, "waddb", WADDB)
  IWMMXT_BUILTIN (addv4hi3, "waddh", WADDH)
  IWMMXT_BUILTIN (addv2si3, "waddw", WADDW)
  IWMMXT_BUILTIN (subv8qi3, "wsubb", WSUBB)
  IWMMXT_BUILTIN (subv4hi3, "wsubh", WSUBH)
  IWMMXT_BUILTIN (subv2si3, "wsubw", WSUBW)
  IWMMXT_BUILTIN (ssaddv8qi3, "waddbss", WADDSSB)
  IWMMXT_BUILTIN (ssaddv4hi3, "waddhss", WADDSSH)
  IWMMXT_BUILTIN (ssaddv2si3, "waddwss", WADDSSW)
  IWMMXT_BUILTIN (sssubv8qi3, "wsubbss", WSUBSSB)
  IWMMXT_BUILTIN (sssubv4hi3, "wsubhss", WSUBSSH)
  IWMMXT_BUILTIN (sssubv2si3, "wsubwss", WSUBSSW)
  IWMMXT_BUILTIN (usaddv8qi3, "waddbus", WADDUSB)
  IWMMXT_BUILTIN (usaddv4hi3, "waddhus", WADDUSH)
  IWMMXT_BUILTIN (usaddv2si3, "waddwus", WADDUSW)
  IWMMXT_BUILTIN (ussubv8qi3, "wsubbus", WSUBUSB)
  IWMMXT_BUILTIN (ussubv4hi3, "wsubhus", WSUBUSH)
  IWMMXT_BUILTIN (ussubv2si3, "wsubwus", WSUBUSW)
  IWMMXT_BUILTIN (mulv4hi3, "wmulul", WMULUL)
  IWMMXT_BUILTIN (smulv4hi3_highpart, "wmulsm", WMULSM)
  IWMMXT_BUILTIN (umulv4hi3_highpart, "wmulum", WMULUM)
  IWMMXT_BUILTIN (eqv8qi3, "wcmpeqb", WCMPEQB)
  IWMMXT_BUILTIN (eqv4hi3, "wcmpeqh", WCMPEQH)
  IWMMXT_BUILTIN (eqv2si3, "wcmpeqw", WCMPEQW)
  IWMMXT_BUILTIN (gtuv8qi3, "wcmpgtub", WCMPGTUB)
  IWMMXT_BUILTIN (gtuv4hi3, "wcmpgtuh", WCMPGTUH)
  IWMMXT_BUILTIN (gtuv2si3, "wcmpgtuw", WCMPGTUW)
  IWMMXT_BUILTIN (gtv8qi3, "wcmpgtsb", WCMPGTSB)
  IWMMXT_BUILTIN (gtv4hi3, "wcmpgtsh", WCMPGTSH)
  IWMMXT_BUILTIN (gtv2si3, "wcmpgtsw", WCMPGTSW)
  IWMMXT_BUILTIN (umaxv8qi3, "wmaxub", WMAXUB)
  IWMMXT_BUILTIN (smaxv8qi3, "wmaxsb", WMAXSB)
  IWMMXT_BUILTIN (umaxv4hi3, "wmaxuh", WMAXUH)
  IWMMXT_BUILTIN (smaxv4hi3, "wmaxsh", WMAXSH)
  IWMMXT_BUILTIN (umaxv2si3, "wmaxuw", WMAXUW)
  IWMMXT_BUILTIN (smaxv2si3, "wmaxsw", WMAXSW)
  IWMMXT_BUILTIN (uminv8qi3, "wminub", WMINUB)
  IWMMXT_BUILTIN (sminv8qi3, "wminsb", WMINSB)
  IWMMXT_BUILTIN (uminv4hi3, "wminuh", WMINUH)
  IWMMXT_BUILTIN (sminv4hi3, "wminsh", WMINSH)
  IWMMXT_BUILTIN (uminv2si3, "wminuw", WMINUW)
  IWMMXT_BUILTIN (sminv2si3, "wminsw", WMINSW)
  IWMMXT_BUILTIN (iwmmxt_anddi3, "wand", WAND)
  IWMMXT_BUILTIN (iwmmxt_nanddi3, "wandn", WANDN)
  IWMMXT_BUILTIN (iwmmxt_iordi3, "wor", WOR)
  IWMMXT_BUILTIN (iwmmxt_xordi3, "wxor", WXOR)
  IWMMXT_BUILTIN (iwmmxt_uavgv8qi3, "wavg2b", WAVG2B)
  IWMMXT_BUILTIN (iwmmxt_uavgv4hi3, "wavg2h", WAVG2H)
  IWMMXT_BUILTIN (iwmmxt_uavgrndv8qi3, "wavg2br", WAVG2BR)
  IWMMXT_BUILTIN (iwmmxt_uavgrndv4hi3, "wavg2hr", WAVG2HR)
  IWMMXT_BUILTIN (iwmmxt_wunpckilb, "wunpckilb", WUNPCKILB)
  IWMMXT_BUILTIN (iwmmxt_wunpckilh, "wunpckilh", WUNPCKILH)
  IWMMXT_BUILTIN (iwmmxt_wunpckilw, "wunpckilw", WUNPCKILW)
  IWMMXT_BUILTIN (iwmmxt_wunpckihb, "wunpckihb", WUNPCKIHB)
  IWMMXT_BUILTIN (iwmmxt_wunpckihh, "wunpckihh", WUNPCKIHH)
  IWMMXT_BUILTIN (iwmmxt_wunpckihw, "wunpckihw", WUNPCKIHW)
  IWMMXT_BUILTIN (iwmmxt_wmadds, "wmadds", WMADDS)
  IWMMXT_BUILTIN (iwmmxt_wmaddu, "wmaddu", WMADDU)

#define IWMMXT_BUILTIN2(code, builtin) \
  { FL_IWMMXT, CODE_FOR_##code, NULL, ARM_BUILTIN_##builtin, 0, 0 },

  IWMMXT_BUILTIN2 (iwmmxt_wpackhss, WPACKHSS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackwss, WPACKWSS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackdss, WPACKDSS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackhus, WPACKHUS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackwus, WPACKWUS)
  IWMMXT_BUILTIN2 (iwmmxt_wpackdus, WPACKDUS)
  IWMMXT_BUILTIN2 (ashlv4hi3_di,    WSLLH)
  IWMMXT_BUILTIN2 (ashlv4hi3,       WSLLHI)
  IWMMXT_BUILTIN2 (ashlv2si3_di,    WSLLW)
  IWMMXT_BUILTIN2 (ashlv2si3,       WSLLWI)
  IWMMXT_BUILTIN2 (ashldi3_di,      WSLLD)
  IWMMXT_BUILTIN2 (ashldi3_iwmmxt,  WSLLDI)
  IWMMXT_BUILTIN2 (lshrv4hi3_di,    WSRLH)
  IWMMXT_BUILTIN2 (lshrv4hi3,       WSRLHI)
  IWMMXT_BUILTIN2 (lshrv2si3_di,    WSRLW)
  IWMMXT_BUILTIN2 (lshrv2si3,       WSRLWI)
  IWMMXT_BUILTIN2 (lshrdi3_di,      WSRLD)
  IWMMXT_BUILTIN2 (lshrdi3_iwmmxt,  WSRLDI)
  IWMMXT_BUILTIN2 (ashrv4hi3_di,    WSRAH)
  IWMMXT_BUILTIN2 (ashrv4hi3,       WSRAHI)
  IWMMXT_BUILTIN2 (ashrv2si3_di,    WSRAW)
  IWMMXT_BUILTIN2 (ashrv2si3,       WSRAWI)
  IWMMXT_BUILTIN2 (ashrdi3_di,      WSRAD)
  IWMMXT_BUILTIN2 (ashrdi3_iwmmxt,  WSRADI)
  IWMMXT_BUILTIN2 (rorv4hi3_di,     WRORH)
  IWMMXT_BUILTIN2 (rorv4hi3,        WRORHI)
  IWMMXT_BUILTIN2 (rorv2si3_di,     WRORW)
  IWMMXT_BUILTIN2 (rorv2si3,        WRORWI)
  IWMMXT_BUILTIN2 (rordi3_di,       WRORD)
  IWMMXT_BUILTIN2 (rordi3,          WRORDI)
  IWMMXT_BUILTIN2 (iwmmxt_wmacuz,   WMACUZ)
  IWMMXT_BUILTIN2 (iwmmxt_wmacsz,   WMACSZ)
};

static const struct builtin_description bdesc_1arg[] =
{
  IWMMXT_BUILTIN (iwmmxt_tmovmskb, "tmovmskb", TMOVMSKB)
  IWMMXT_BUILTIN (iwmmxt_tmovmskh, "tmovmskh", TMOVMSKH)
  IWMMXT_BUILTIN (iwmmxt_tmovmskw, "tmovmskw", TMOVMSKW)
  IWMMXT_BUILTIN (iwmmxt_waccb, "waccb", WACCB)
  IWMMXT_BUILTIN (iwmmxt_wacch, "wacch", WACCH)
  IWMMXT_BUILTIN (iwmmxt_waccw, "waccw", WACCW)
  IWMMXT_BUILTIN (iwmmxt_wunpckehub, "wunpckehub", WUNPCKEHUB)
  IWMMXT_BUILTIN (iwmmxt_wunpckehuh, "wunpckehuh", WUNPCKEHUH)
  IWMMXT_BUILTIN (iwmmxt_wunpckehuw, "wunpckehuw", WUNPCKEHUW)
  IWMMXT_BUILTIN (iwmmxt_wunpckehsb, "wunpckehsb", WUNPCKEHSB)
  IWMMXT_BUILTIN (iwmmxt_wunpckehsh, "wunpckehsh", WUNPCKEHSH)
  IWMMXT_BUILTIN (iwmmxt_wunpckehsw, "wunpckehsw", WUNPCKEHSW)
  IWMMXT_BUILTIN (iwmmxt_wunpckelub, "wunpckelub", WUNPCKELUB)
  IWMMXT_BUILTIN (iwmmxt_wunpckeluh, "wunpckeluh", WUNPCKELUH)
  IWMMXT_BUILTIN (iwmmxt_wunpckeluw, "wunpckeluw", WUNPCKELUW)
  IWMMXT_BUILTIN (iwmmxt_wunpckelsb, "wunpckelsb", WUNPCKELSB)
  IWMMXT_BUILTIN (iwmmxt_wunpckelsh, "wunpckelsh", WUNPCKELSH)
  IWMMXT_BUILTIN (iwmmxt_wunpckelsw, "wunpckelsw", WUNPCKELSW)
};

/* Set up all the iWMMXt builtins.  This is
   not called if TARGET_IWMMXT is zero.  */

static void
arm_init_iwmmxt_builtins (void)
{
  const struct builtin_description * d;
  size_t i;
  tree endlink = void_list_node;

  tree V2SI_type_node = build_vector_type_for_mode (intSI_type_node, V2SImode);
  tree V4HI_type_node = build_vector_type_for_mode (intHI_type_node, V4HImode);
  tree V8QI_type_node = build_vector_type_for_mode (intQI_type_node, V8QImode);

  tree int_ftype_int
    = build_function_type (integer_type_node,
			   tree_cons (NULL_TREE, integer_type_node, endlink));
  tree v8qi_ftype_v8qi_v8qi_int
    = build_function_type (V8QI_type_node,
			   tree_cons (NULL_TREE, V8QI_type_node,
				      tree_cons (NULL_TREE, V8QI_type_node,
						 tree_cons (NULL_TREE,
							    integer_type_node,
							    endlink))));
  tree v4hi_ftype_v4hi_int
    = build_function_type (V4HI_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 endlink)));
  tree v2si_ftype_v2si_int
    = build_function_type (V2SI_type_node,
			   tree_cons (NULL_TREE, V2SI_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 endlink)));
  tree v2si_ftype_di_di
    = build_function_type (V2SI_type_node,
			   tree_cons (NULL_TREE, long_long_integer_type_node,
				      tree_cons (NULL_TREE, long_long_integer_type_node,
						 endlink)));
  tree di_ftype_di_int
    = build_function_type (long_long_integer_type_node,
			   tree_cons (NULL_TREE, long_long_integer_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 endlink)));
  tree di_ftype_di_int_int
    = build_function_type (long_long_integer_type_node,
			   tree_cons (NULL_TREE, long_long_integer_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 tree_cons (NULL_TREE,
							    integer_type_node,
							    endlink))));
  tree int_ftype_v8qi
    = build_function_type (integer_type_node,
			   tree_cons (NULL_TREE, V8QI_type_node,
				      endlink));
  tree int_ftype_v4hi
    = build_function_type (integer_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      endlink));
  tree int_ftype_v2si
    = build_function_type (integer_type_node,
			   tree_cons (NULL_TREE, V2SI_type_node,
				      endlink));
  tree int_ftype_v8qi_int
    = build_function_type (integer_type_node,
			   tree_cons (NULL_TREE, V8QI_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 endlink)));
  tree int_ftype_v4hi_int
    = build_function_type (integer_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 endlink)));
  tree int_ftype_v2si_int
    = build_function_type (integer_type_node,
			   tree_cons (NULL_TREE, V2SI_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 endlink)));
  tree v8qi_ftype_v8qi_int_int
    = build_function_type (V8QI_type_node,
			   tree_cons (NULL_TREE, V8QI_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 tree_cons (NULL_TREE,
							    integer_type_node,
							    endlink))));
  tree v4hi_ftype_v4hi_int_int
    = build_function_type (V4HI_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 tree_cons (NULL_TREE,
							    integer_type_node,
							    endlink))));
  tree v2si_ftype_v2si_int_int
    = build_function_type (V2SI_type_node,
			   tree_cons (NULL_TREE, V2SI_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 tree_cons (NULL_TREE,
							    integer_type_node,
							    endlink))));
  /* Miscellaneous.  */
  tree v8qi_ftype_v4hi_v4hi
    = build_function_type (V8QI_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      tree_cons (NULL_TREE, V4HI_type_node,
						 endlink)));
  tree v4hi_ftype_v2si_v2si
    = build_function_type (V4HI_type_node,
			   tree_cons (NULL_TREE, V2SI_type_node,
				      tree_cons (NULL_TREE, V2SI_type_node,
						 endlink)));
  tree v2si_ftype_v4hi_v4hi
    = build_function_type (V2SI_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      tree_cons (NULL_TREE, V4HI_type_node,
						 endlink)));
  tree v2si_ftype_v8qi_v8qi
    = build_function_type (V2SI_type_node,
			   tree_cons (NULL_TREE, V8QI_type_node,
				      tree_cons (NULL_TREE, V8QI_type_node,
						 endlink)));
  tree v4hi_ftype_v4hi_di
    = build_function_type (V4HI_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      tree_cons (NULL_TREE,
						 long_long_integer_type_node,
						 endlink)));
  tree v2si_ftype_v2si_di
    = build_function_type (V2SI_type_node,
			   tree_cons (NULL_TREE, V2SI_type_node,
				      tree_cons (NULL_TREE,
						 long_long_integer_type_node,
						 endlink)));
  tree void_ftype_int_int
    = build_function_type (void_type_node,
			   tree_cons (NULL_TREE, integer_type_node,
				      tree_cons (NULL_TREE, integer_type_node,
						 endlink)));
  tree di_ftype_void
    = build_function_type (long_long_unsigned_type_node, endlink);
  tree di_ftype_v8qi
    = build_function_type (long_long_integer_type_node,
			   tree_cons (NULL_TREE, V8QI_type_node,
				      endlink));
  tree di_ftype_v4hi
    = build_function_type (long_long_integer_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      endlink));
  tree di_ftype_v2si
    = build_function_type (long_long_integer_type_node,
			   tree_cons (NULL_TREE, V2SI_type_node,
				      endlink));
  tree v2si_ftype_v4hi
    = build_function_type (V2SI_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      endlink));
  tree v4hi_ftype_v8qi
    = build_function_type (V4HI_type_node,
			   tree_cons (NULL_TREE, V8QI_type_node,
				      endlink));

  tree di_ftype_di_v4hi_v4hi
    = build_function_type (long_long_unsigned_type_node,
			   tree_cons (NULL_TREE,
				      long_long_unsigned_type_node,
				      tree_cons (NULL_TREE, V4HI_type_node,
						 tree_cons (NULL_TREE,
							    V4HI_type_node,
							    endlink))));

  tree di_ftype_v4hi_v4hi
    = build_function_type (long_long_unsigned_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      tree_cons (NULL_TREE, V4HI_type_node,
						 endlink)));

  /* Normal vector binops.  */
  tree v8qi_ftype_v8qi_v8qi
    = build_function_type (V8QI_type_node,
			   tree_cons (NULL_TREE, V8QI_type_node,
				      tree_cons (NULL_TREE, V8QI_type_node,
						 endlink)));
  tree v4hi_ftype_v4hi_v4hi
    = build_function_type (V4HI_type_node,
			   tree_cons (NULL_TREE, V4HI_type_node,
				      tree_cons (NULL_TREE, V4HI_type_node,
						 endlink)));
  tree v2si_ftype_v2si_v2si
    = build_function_type (V2SI_type_node,
			   tree_cons (NULL_TREE, V2SI_type_node,
				      tree_cons (NULL_TREE, V2SI_type_node,
						 endlink)));
  tree di_ftype_di_di
    = build_function_type (long_long_unsigned_type_node,
			   tree_cons (NULL_TREE, long_long_unsigned_type_node,
				      tree_cons (NULL_TREE,
						 long_long_unsigned_type_node,
						 endlink)));

  /* Add all builtins that are more or less simple operations on two
     operands.  */
  for (i = 0, d = bdesc_2arg; i < ARRAY_SIZE (bdesc_2arg); i++, d++)
    {
      /* Use one of the operands; the target can have a different mode for
	 mask-generating compares.  */
      enum machine_mode mode;
      tree type;

      if (d->name == 0)
	continue;

      mode = insn_data[d->icode].operand[1].mode;

      switch (mode)
	{
	case V8QImode:
	  type = v8qi_ftype_v8qi_v8qi;
	  break;
	case V4HImode:
	  type = v4hi_ftype_v4hi_v4hi;
	  break;
	case V2SImode:
	  type = v2si_ftype_v2si_v2si;
	  break;
	case DImode:
	  type = di_ftype_di_di;
	  break;

	default:
	  gcc_unreachable ();
	}

      def_mbuiltin (d->mask, d->name, type, d->code);
    }

  /* Add the remaining MMX insns with somewhat more complicated types.  */
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wzero", di_ftype_void, ARM_BUILTIN_WZERO);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_setwcx", void_ftype_int_int, ARM_BUILTIN_SETWCX);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_getwcx", int_ftype_int, ARM_BUILTIN_GETWCX);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsllh", v4hi_ftype_v4hi_di, ARM_BUILTIN_WSLLH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsllw", v2si_ftype_v2si_di, ARM_BUILTIN_WSLLW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wslld", di_ftype_di_di, ARM_BUILTIN_WSLLD);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsllhi", v4hi_ftype_v4hi_int, ARM_BUILTIN_WSLLHI);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsllwi", v2si_ftype_v2si_int, ARM_BUILTIN_WSLLWI);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wslldi", di_ftype_di_int, ARM_BUILTIN_WSLLDI);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrlh", v4hi_ftype_v4hi_di, ARM_BUILTIN_WSRLH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrlw", v2si_ftype_v2si_di, ARM_BUILTIN_WSRLW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrld", di_ftype_di_di, ARM_BUILTIN_WSRLD);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrlhi", v4hi_ftype_v4hi_int, ARM_BUILTIN_WSRLHI);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrlwi", v2si_ftype_v2si_int, ARM_BUILTIN_WSRLWI);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrldi", di_ftype_di_int, ARM_BUILTIN_WSRLDI);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrah", v4hi_ftype_v4hi_di, ARM_BUILTIN_WSRAH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsraw", v2si_ftype_v2si_di, ARM_BUILTIN_WSRAW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrad", di_ftype_di_di, ARM_BUILTIN_WSRAD);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrahi", v4hi_ftype_v4hi_int, ARM_BUILTIN_WSRAHI);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsrawi", v2si_ftype_v2si_int, ARM_BUILTIN_WSRAWI);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsradi", di_ftype_di_int, ARM_BUILTIN_WSRADI);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wrorh", v4hi_ftype_v4hi_di, ARM_BUILTIN_WRORH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wrorw", v2si_ftype_v2si_di, ARM_BUILTIN_WRORW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wrord", di_ftype_di_di, ARM_BUILTIN_WRORD);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wrorhi", v4hi_ftype_v4hi_int, ARM_BUILTIN_WRORHI);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wrorwi", v2si_ftype_v2si_int, ARM_BUILTIN_WRORWI);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wrordi", di_ftype_di_int, ARM_BUILTIN_WRORDI);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wshufh", v4hi_ftype_v4hi_int, ARM_BUILTIN_WSHUFH);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsadb", v2si_ftype_v8qi_v8qi, ARM_BUILTIN_WSADB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsadh", v2si_ftype_v4hi_v4hi, ARM_BUILTIN_WSADH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsadbz", v2si_ftype_v8qi_v8qi, ARM_BUILTIN_WSADBZ);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wsadhz", v2si_ftype_v4hi_v4hi, ARM_BUILTIN_WSADHZ);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_textrmsb", int_ftype_v8qi_int, ARM_BUILTIN_TEXTRMSB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_textrmsh", int_ftype_v4hi_int, ARM_BUILTIN_TEXTRMSH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_textrmsw", int_ftype_v2si_int, ARM_BUILTIN_TEXTRMSW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_textrmub", int_ftype_v8qi_int, ARM_BUILTIN_TEXTRMUB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_textrmuh", int_ftype_v4hi_int, ARM_BUILTIN_TEXTRMUH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_textrmuw", int_ftype_v2si_int, ARM_BUILTIN_TEXTRMUW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tinsrb", v8qi_ftype_v8qi_int_int, ARM_BUILTIN_TINSRB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tinsrh", v4hi_ftype_v4hi_int_int, ARM_BUILTIN_TINSRH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tinsrw", v2si_ftype_v2si_int_int, ARM_BUILTIN_TINSRW);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_waccb", di_ftype_v8qi, ARM_BUILTIN_WACCB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wacch", di_ftype_v4hi, ARM_BUILTIN_WACCH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_waccw", di_ftype_v2si, ARM_BUILTIN_WACCW);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmovmskb", int_ftype_v8qi, ARM_BUILTIN_TMOVMSKB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmovmskh", int_ftype_v4hi, ARM_BUILTIN_TMOVMSKH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmovmskw", int_ftype_v2si, ARM_BUILTIN_TMOVMSKW);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wpackhss", v8qi_ftype_v4hi_v4hi, ARM_BUILTIN_WPACKHSS);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wpackhus", v8qi_ftype_v4hi_v4hi, ARM_BUILTIN_WPACKHUS);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wpackwus", v4hi_ftype_v2si_v2si, ARM_BUILTIN_WPACKWUS);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wpackwss", v4hi_ftype_v2si_v2si, ARM_BUILTIN_WPACKWSS);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wpackdus", v2si_ftype_di_di, ARM_BUILTIN_WPACKDUS);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wpackdss", v2si_ftype_di_di, ARM_BUILTIN_WPACKDSS);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckehub", v4hi_ftype_v8qi, ARM_BUILTIN_WUNPCKEHUB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckehuh", v2si_ftype_v4hi, ARM_BUILTIN_WUNPCKEHUH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckehuw", di_ftype_v2si, ARM_BUILTIN_WUNPCKEHUW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckehsb", v4hi_ftype_v8qi, ARM_BUILTIN_WUNPCKEHSB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckehsh", v2si_ftype_v4hi, ARM_BUILTIN_WUNPCKEHSH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckehsw", di_ftype_v2si, ARM_BUILTIN_WUNPCKEHSW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckelub", v4hi_ftype_v8qi, ARM_BUILTIN_WUNPCKELUB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckeluh", v2si_ftype_v4hi, ARM_BUILTIN_WUNPCKELUH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckeluw", di_ftype_v2si, ARM_BUILTIN_WUNPCKELUW);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckelsb", v4hi_ftype_v8qi, ARM_BUILTIN_WUNPCKELSB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckelsh", v2si_ftype_v4hi, ARM_BUILTIN_WUNPCKELSH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wunpckelsw", di_ftype_v2si, ARM_BUILTIN_WUNPCKELSW);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wmacs", di_ftype_di_v4hi_v4hi, ARM_BUILTIN_WMACS);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wmacsz", di_ftype_v4hi_v4hi, ARM_BUILTIN_WMACSZ);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wmacu", di_ftype_di_v4hi_v4hi, ARM_BUILTIN_WMACU);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_wmacuz", di_ftype_v4hi_v4hi, ARM_BUILTIN_WMACUZ);

  def_mbuiltin (FL_IWMMXT, "__builtin_arm_walign", v8qi_ftype_v8qi_v8qi_int, ARM_BUILTIN_WALIGN);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmia", di_ftype_di_int_int, ARM_BUILTIN_TMIA);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmiaph", di_ftype_di_int_int, ARM_BUILTIN_TMIAPH);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmiabb", di_ftype_di_int_int, ARM_BUILTIN_TMIABB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmiabt", di_ftype_di_int_int, ARM_BUILTIN_TMIABT);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmiatb", di_ftype_di_int_int, ARM_BUILTIN_TMIATB);
  def_mbuiltin (FL_IWMMXT, "__builtin_arm_tmiatt", di_ftype_di_int_int, ARM_BUILTIN_TMIATT);
}

static void
arm_init_tls_builtins (void)
{
  tree ftype;
  tree nothrow = tree_cons (get_identifier ("nothrow"), NULL, NULL);
  tree const_nothrow = tree_cons (get_identifier ("const"), NULL, nothrow);

  ftype = build_function_type (ptr_type_node, void_list_node);
  lang_hooks.builtin_function ("__builtin_thread_pointer", ftype,
			       ARM_BUILTIN_THREAD_POINTER, BUILT_IN_MD,
			       NULL, const_nothrow);
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
typedef enum {
  T_V8QI  = 0x0001,
  T_V4HI  = 0x0002,
  T_V2SI  = 0x0004,
  T_V2SF  = 0x0008,
  T_DI    = 0x0010,
  T_V16QI = 0x0020,
  T_V8HI  = 0x0040,
  T_V4SI  = 0x0080,
  T_V4SF  = 0x0100,
  T_V2DI  = 0x0200,
  T_TI	  = 0x0400,
  T_EI	  = 0x0800,
  T_OI	  = 0x1000
} neon_builtin_type_bits;

#define v8qi_UP  T_V8QI
#define v4hi_UP  T_V4HI
#define v2si_UP  T_V2SI
#define v2sf_UP  T_V2SF
#define di_UP    T_DI
#define v16qi_UP T_V16QI
#define v8hi_UP  T_V8HI
#define v4si_UP  T_V4SI
#define v4sf_UP  T_V4SF
#define v2di_UP  T_V2DI
#define ti_UP	 T_TI
#define ei_UP	 T_EI
#define oi_UP	 T_OI

#define UP(X) X##_UP

#define T_MAX 13

/* FIXME: Add other types of insn (loads & stores, etc.).  */
typedef enum {
  NEON_BINOP,
  NEON_TERNOP,
  NEON_UNOP,
  NEON_GETLANE,
  NEON_SETLANE,
  NEON_CREATE,
  NEON_DUP,
  NEON_DUPLANE,
  NEON_COMBINE,
  NEON_SPLIT,
  NEON_LANEMUL,
  NEON_LANEMULL,
  NEON_LANEMULH,
  NEON_LANEMAC,
  NEON_SCALARMUL,
  NEON_SCALARMULL,
  NEON_SCALARMULH,
  NEON_SCALARMAC,
  NEON_CONVERT,
  NEON_FIXCONV,
  NEON_SELECT,
  NEON_RESULTPAIR,
  NEON_REINTERP,
  NEON_VTBL,
  NEON_VTBX,
  NEON_LOAD1,
  NEON_LOAD1LANE,
  NEON_STORE1,
  NEON_STORE1LANE,
  NEON_LOADSTRUCT,
  NEON_LOADSTRUCTLANE,
  NEON_STORESTRUCT,
  NEON_STORESTRUCTLANE,
  NEON_LOGICBINOP,
  NEON_SHIFTINSERT,
  NEON_SHIFTIMM,
  NEON_SHIFTACC
} neon_itype;

typedef struct {
  const char *name;
  const neon_itype itype;
  const neon_builtin_type_bits bits;
  const enum insn_code codes[T_MAX];
  const unsigned int num_vars;
  unsigned int base_fcode;
} neon_builtin_datum;

#define CF(N,X) CODE_FOR_neon_##N##X

#define VAR1(T, N, A) \
  #N, NEON_##T, UP (A), { CF (N, A) }, 1, 0
#define VAR2(T, N, A, B) \
  #N, NEON_##T, UP (A) | UP (B), { CF (N, A), CF (N, B) }, 2, 0
#define VAR3(T, N, A, B, C) \
  #N, NEON_##T, UP (A) | UP (B) | UP (C), \
  { CF (N, A), CF (N, B), CF (N, C) }, 3, 0
#define VAR4(T, N, A, B, C, D) \
  #N, NEON_##T, UP (A) | UP (B) | UP (C) | UP (D), \
  { CF (N, A), CF (N, B), CF (N, C), CF (N, D) }, 4, 0
#define VAR5(T, N, A, B, C, D, E) \
  #N, NEON_##T, UP (A) | UP (B) | UP (C) | UP (D) | UP (E), \
  { CF (N, A), CF (N, B), CF (N, C), CF (N, D), CF (N, E) }, 5, 0
#define VAR6(T, N, A, B, C, D, E, F) \
  #N, NEON_##T, UP (A) | UP (B) | UP (C) | UP (D) | UP (E) | UP (F), \
  { CF (N, A), CF (N, B), CF (N, C), CF (N, D), CF (N, E), CF (N, F) }, 6, 0
#define VAR7(T, N, A, B, C, D, E, F, G) \
  #N, NEON_##T, UP (A) | UP (B) | UP (C) | UP (D) | UP (E) | UP (F) | UP (G), \
  { CF (N, A), CF (N, B), CF (N, C), CF (N, D), CF (N, E), CF (N, F), \
    CF (N, G) }, 7, 0
#define VAR8(T, N, A, B, C, D, E, F, G, H) \
  #N, NEON_##T, UP (A) | UP (B) | UP (C) | UP (D) | UP (E) | UP (F) | UP (G) \
                | UP (H), \
  { CF (N, A), CF (N, B), CF (N, C), CF (N, D), CF (N, E), CF (N, F), \
    CF (N, G), CF (N, H) }, 8, 0
#define VAR9(T, N, A, B, C, D, E, F, G, H, I) \
  #N, NEON_##T, UP (A) | UP (B) | UP (C) | UP (D) | UP (E) | UP (F) | UP (G) \
                | UP (H) | UP (I), \
  { CF (N, A), CF (N, B), CF (N, C), CF (N, D), CF (N, E), CF (N, F), \
    CF (N, G), CF (N, H), CF (N, I) }, 9, 0
#define VAR10(T, N, A, B, C, D, E, F, G, H, I, J) \
  #N, NEON_##T, UP (A) | UP (B) | UP (C) | UP (D) | UP (E) | UP (F) | UP (G) \
                | UP (H) | UP (I) | UP (J), \
  { CF (N, A), CF (N, B), CF (N, C), CF (N, D), CF (N, E), CF (N, F), \
    CF (N, G), CF (N, H), CF (N, I), CF (N, J) }, 10, 0

/* The mode entries in the following table correspond to the "key" type of the
   instruction variant, i.e. equivalent to that which would be specified after
   the assembler mnemonic, which usually refers to the last vector operand.
   (Signed/unsigned/polynomial types are not differentiated between though, and
   are all mapped onto the same mode for a given element size.) The modes
   listed per instruction should be the same as those defined for that
   instruction's pattern in neon.md.
   WARNING: Variants should be listed in the same increasing order as
   neon_builtin_type_bits.  */

static neon_builtin_datum neon_builtin_data[] =
{
  { VAR10 (BINOP, vadd,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR3 (BINOP, vaddl, v8qi, v4hi, v2si) },
  { VAR3 (BINOP, vaddw, v8qi, v4hi, v2si) },
  { VAR6 (BINOP, vhadd, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR8 (BINOP, vqadd, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR3 (BINOP, vaddhn, v8hi, v4si, v2di) },
  { VAR8 (BINOP, vmul, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR8 (TERNOP, vmla, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR3 (TERNOP, vmlal, v8qi, v4hi, v2si) },
  { VAR8 (TERNOP, vmls, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR3 (TERNOP, vmlsl, v8qi, v4hi, v2si) },
  { VAR4 (BINOP, vqdmulh, v4hi, v2si, v8hi, v4si) },
  { VAR2 (TERNOP, vqdmlal, v4hi, v2si) },
  { VAR2 (TERNOP, vqdmlsl, v4hi, v2si) },
  { VAR3 (BINOP, vmull, v8qi, v4hi, v2si) },
  { VAR2 (SCALARMULL, vmull_n, v4hi, v2si) },
  { VAR2 (LANEMULL, vmull_lane, v4hi, v2si) },
  { VAR2 (SCALARMULL, vqdmull_n, v4hi, v2si) },
  { VAR2 (LANEMULL, vqdmull_lane, v4hi, v2si) },
  { VAR4 (SCALARMULH, vqdmulh_n, v4hi, v2si, v8hi, v4si) },
  { VAR4 (LANEMULH, vqdmulh_lane, v4hi, v2si, v8hi, v4si) },
  { VAR2 (BINOP, vqdmull, v4hi, v2si) },
  { VAR8 (BINOP, vshl, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR8 (BINOP, vqshl, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR8 (SHIFTIMM, vshr_n, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR3 (SHIFTIMM, vshrn_n, v8hi, v4si, v2di) },
  { VAR3 (SHIFTIMM, vqshrn_n, v8hi, v4si, v2di) },
  { VAR3 (SHIFTIMM, vqshrun_n, v8hi, v4si, v2di) },
  { VAR8 (SHIFTIMM, vshl_n, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR8 (SHIFTIMM, vqshl_n, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR8 (SHIFTIMM, vqshlu_n, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR3 (SHIFTIMM, vshll_n, v8qi, v4hi, v2si) },
  { VAR8 (SHIFTACC, vsra_n, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR10 (BINOP, vsub,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR3 (BINOP, vsubl, v8qi, v4hi, v2si) },
  { VAR3 (BINOP, vsubw, v8qi, v4hi, v2si) },
  { VAR8 (BINOP, vqsub, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR6 (BINOP, vhsub, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR3 (BINOP, vsubhn, v8hi, v4si, v2di) },
  { VAR8 (BINOP, vceq, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR8 (BINOP, vcge, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR8 (BINOP, vcgt, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR2 (BINOP, vcage, v2sf, v4sf) },
  { VAR2 (BINOP, vcagt, v2sf, v4sf) },
  { VAR6 (BINOP, vtst, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR8 (BINOP, vabd, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR3 (BINOP, vabdl, v8qi, v4hi, v2si) },
  { VAR6 (TERNOP, vaba, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR3 (TERNOP, vabal, v8qi, v4hi, v2si) },
  { VAR8 (BINOP, vmax, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR8 (BINOP, vmin, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR4 (BINOP, vpadd, v8qi, v4hi, v2si, v2sf) },
  { VAR6 (UNOP, vpaddl, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR6 (BINOP, vpadal, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR4 (BINOP, vpmax, v8qi, v4hi, v2si, v2sf) },
  { VAR4 (BINOP, vpmin, v8qi, v4hi, v2si, v2sf) },
  { VAR2 (BINOP, vrecps, v2sf, v4sf) },
  { VAR2 (BINOP, vrsqrts, v2sf, v4sf) },
  { VAR8 (SHIFTINSERT, vsri_n, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR8 (SHIFTINSERT, vsli_n, v8qi, v4hi, v2si, di, v16qi, v8hi, v4si, v2di) },
  { VAR8 (UNOP, vabs, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR6 (UNOP, vqabs, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR8 (UNOP, vneg, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR6 (UNOP, vqneg, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR6 (UNOP, vcls, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR6 (UNOP, vclz, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  { VAR2 (UNOP, vcnt, v8qi, v16qi) },
  { VAR4 (UNOP, vrecpe, v2si, v2sf, v4si, v4sf) },
  { VAR4 (UNOP, vrsqrte, v2si, v2sf, v4si, v4sf) },
  { VAR6 (UNOP, vmvn, v8qi, v4hi, v2si, v16qi, v8hi, v4si) },
  /* FIXME: vget_lane supports more variants than this!  */
  { VAR10 (GETLANE, vget_lane,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (SETLANE, vset_lane,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR5 (CREATE, vcreate, v8qi, v4hi, v2si, v2sf, di) },
  { VAR10 (DUP, vdup_n,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (DUPLANE, vdup_lane,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR5 (COMBINE, vcombine, v8qi, v4hi, v2si, v2sf, di) },
  { VAR5 (SPLIT, vget_high, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR5 (SPLIT, vget_low, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR3 (UNOP, vmovn, v8hi, v4si, v2di) },
  { VAR3 (UNOP, vqmovn, v8hi, v4si, v2di) },
  { VAR3 (UNOP, vqmovun, v8hi, v4si, v2di) },
  { VAR3 (UNOP, vmovl, v8qi, v4hi, v2si) },
  { VAR6 (LANEMUL, vmul_lane, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR6 (LANEMAC, vmla_lane, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR2 (LANEMAC, vmlal_lane, v4hi, v2si) },
  { VAR2 (LANEMAC, vqdmlal_lane, v4hi, v2si) },
  { VAR6 (LANEMAC, vmls_lane, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR2 (LANEMAC, vmlsl_lane, v4hi, v2si) },
  { VAR2 (LANEMAC, vqdmlsl_lane, v4hi, v2si) },
  { VAR6 (SCALARMUL, vmul_n, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR6 (SCALARMAC, vmla_n, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR2 (SCALARMAC, vmlal_n, v4hi, v2si) },
  { VAR2 (SCALARMAC, vqdmlal_n, v4hi, v2si) },
  { VAR6 (SCALARMAC, vmls_n, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR2 (SCALARMAC, vmlsl_n, v4hi, v2si) },
  { VAR2 (SCALARMAC, vqdmlsl_n, v4hi, v2si) },
  { VAR10 (BINOP, vext,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR8 (UNOP, vrev64, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR4 (UNOP, vrev32, v8qi, v4hi, v16qi, v8hi) },
  { VAR2 (UNOP, vrev16, v8qi, v16qi) },
  { VAR4 (CONVERT, vcvt, v2si, v2sf, v4si, v4sf) },
  { VAR4 (FIXCONV, vcvt_n, v2si, v2sf, v4si, v4sf) },
  { VAR10 (SELECT, vbsl,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR1 (VTBL, vtbl1, v8qi) },
  { VAR1 (VTBL, vtbl2, v8qi) },
  { VAR1 (VTBL, vtbl3, v8qi) },
  { VAR1 (VTBL, vtbl4, v8qi) },
  { VAR1 (VTBX, vtbx1, v8qi) },
  { VAR1 (VTBX, vtbx2, v8qi) },
  { VAR1 (VTBX, vtbx3, v8qi) },
  { VAR1 (VTBX, vtbx4, v8qi) },
  { VAR8 (RESULTPAIR, vtrn, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR8 (RESULTPAIR, vzip, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR8 (RESULTPAIR, vuzp, v8qi, v4hi, v2si, v2sf, v16qi, v8hi, v4si, v4sf) },
  { VAR5 (REINTERP, vreinterpretv8qi, v8qi, v4hi, v2si, v2sf, di) },
  { VAR5 (REINTERP, vreinterpretv4hi, v8qi, v4hi, v2si, v2sf, di) },
  { VAR5 (REINTERP, vreinterpretv2si, v8qi, v4hi, v2si, v2sf, di) },
  { VAR5 (REINTERP, vreinterpretv2sf, v8qi, v4hi, v2si, v2sf, di) },
  { VAR5 (REINTERP, vreinterpretdi, v8qi, v4hi, v2si, v2sf, di) },
  { VAR5 (REINTERP, vreinterpretv16qi, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR5 (REINTERP, vreinterpretv8hi, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR5 (REINTERP, vreinterpretv4si, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR5 (REINTERP, vreinterpretv4sf, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR5 (REINTERP, vreinterpretv2di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (LOAD1, vld1,
           v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (LOAD1LANE, vld1_lane,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (LOAD1, vld1_dup,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (STORE1, vst1,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (STORE1LANE, vst1_lane,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR9 (LOADSTRUCT,
	  vld2, v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf) },
  { VAR7 (LOADSTRUCTLANE, vld2_lane,
	  v8qi, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR5 (LOADSTRUCT, vld2_dup, v8qi, v4hi, v2si, v2sf, di) },
  { VAR9 (STORESTRUCT, vst2,
	  v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf) },
  { VAR7 (STORESTRUCTLANE, vst2_lane,
	  v8qi, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR9 (LOADSTRUCT,
	  vld3, v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf) },
  { VAR7 (LOADSTRUCTLANE, vld3_lane,
	  v8qi, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR5 (LOADSTRUCT, vld3_dup, v8qi, v4hi, v2si, v2sf, di) },
  { VAR9 (STORESTRUCT, vst3,
	  v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf) },
  { VAR7 (STORESTRUCTLANE, vst3_lane,
	  v8qi, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR9 (LOADSTRUCT, vld4,
	  v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf) },
  { VAR7 (LOADSTRUCTLANE, vld4_lane,
	  v8qi, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR5 (LOADSTRUCT, vld4_dup, v8qi, v4hi, v2si, v2sf, di) },
  { VAR9 (STORESTRUCT, vst4,
	  v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf) },
  { VAR7 (STORESTRUCTLANE, vst4_lane,
	  v8qi, v4hi, v2si, v2sf, v8hi, v4si, v4sf) },
  { VAR10 (LOGICBINOP, vand,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (LOGICBINOP, vorr,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (BINOP, veor,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (LOGICBINOP, vbic,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) },
  { VAR10 (LOGICBINOP, vorn,
	   v8qi, v4hi, v2si, v2sf, di, v16qi, v8hi, v4si, v4sf, v2di) }
};

#undef CF
#undef VAR1
#undef VAR2
#undef VAR3
#undef VAR4
#undef VAR5
#undef VAR6
#undef VAR7
#undef VAR8
#undef VAR9
#undef VAR10

static int
valid_neon_mode (enum machine_mode mode)
{
  return VALID_NEON_DREG_MODE (mode) || VALID_NEON_QREG_MODE (mode);
}

static void
arm_init_neon_builtins (void)
{
#define qi_TN neon_intQI_type_node
#define hi_TN neon_intHI_type_node
#define pqi_TN neon_polyQI_type_node
#define qhi_TN neon_polyHI_type_node
#define si_TN neon_intSI_type_node
#define di_TN neon_intDI_type_node
#define ti_TN intTI_type_node
#define ei_TN intEI_type_node
#define oi_TN intOI_type_node
#define ci_TN intCI_type_node
#define xi_TN intXI_type_node

#define sf_TN neon_float_type_node

#define v8qi_TN V8QI_type_node
#define v4hi_TN V4HI_type_node
#define v2si_TN V2SI_type_node
#define v2sf_TN V2SF_type_node

#define v16qi_TN V16QI_type_node
#define v8hi_TN V8HI_type_node
#define v4si_TN V4SI_type_node
#define v4sf_TN V4SF_type_node
#define v2di_TN V2DI_type_node

#define pv8qi_TN V8QI_pointer_node
#define pv4hi_TN V4HI_pointer_node
#define pv2si_TN V2SI_pointer_node
#define pv2sf_TN V2SF_pointer_node
#define pdi_TN intDI_pointer_node

#define pv16qi_TN V16QI_pointer_node
#define pv8hi_TN V8HI_pointer_node
#define pv4si_TN V4SI_pointer_node
#define pv4sf_TN V4SF_pointer_node
#define pv2di_TN V2DI_pointer_node

#define void_TN void_type_node

#define TYPE2(A,B) \
  tree A##_##ftype##_##B = build_function_type_list (A##_TN, B##_TN, NULL)
#define TYPE3(A,B,C) \
  tree A##_##ftype##_##B##_##C = build_function_type_list (A##_TN, B##_TN, \
    C##_TN, NULL)
#define TYPE4(A,B,C,D) \
  tree A##_##ftype##_##B##_##C##_##D = build_function_type_list (A##_TN, \
    B##_TN, C##_TN, D##_TN, NULL)
#define TYPE5(A,B,C,D,E) \
  tree A##_##ftype##_##B##_##C##_##D##_##E = build_function_type_list (A##_TN, \
    B##_TN, C##_TN, D##_TN, E##_TN, NULL)
#define TYPE6(A,B,C,D,E,F) \
  tree A##_##ftype##_##B##_##C##_##D##_##E##_##F = build_function_type_list \
    (A##_TN, B##_TN, C##_TN, D##_TN, E##_TN, F##_TN, NULL)

  unsigned int i, fcode = ARM_BUILTIN_NEON_BASE;

  /* Create distinguished type nodes for NEON vector element types,
     and pointers to values of such types, so we can detect them later.  */
  tree neon_intQI_type_node = make_signed_type (GET_MODE_PRECISION (QImode));
  tree neon_intHI_type_node = make_signed_type (GET_MODE_PRECISION (HImode));
  tree neon_polyQI_type_node = make_signed_type (GET_MODE_PRECISION (QImode));
  tree neon_polyHI_type_node = make_signed_type (GET_MODE_PRECISION (HImode));
  tree neon_intSI_type_node = make_signed_type (GET_MODE_PRECISION (SImode));
  tree neon_intDI_type_node = make_signed_type (GET_MODE_PRECISION (DImode));
  tree neon_float_type_node = make_node (REAL_TYPE);
  TYPE_PRECISION (neon_float_type_node) = FLOAT_TYPE_SIZE;
  layout_type (neon_float_type_node);
  
  /* Define typedefs which exactly correspond to the modes we are basing vector
     types on.  If you change these names you'll need to change
     the table used by arm_mangle_vector_type too.  */
  (*lang_hooks.types.register_builtin_type) (neon_intQI_type_node,
					     "__builtin_neon_qi");
  (*lang_hooks.types.register_builtin_type) (neon_intHI_type_node,
					     "__builtin_neon_hi");
  (*lang_hooks.types.register_builtin_type) (neon_intSI_type_node,
					     "__builtin_neon_si");
  (*lang_hooks.types.register_builtin_type) (neon_float_type_node,
					     "__builtin_neon_sf");
  (*lang_hooks.types.register_builtin_type) (neon_intDI_type_node,
					     "__builtin_neon_di");

  (*lang_hooks.types.register_builtin_type) (neon_polyQI_type_node,
					     "__builtin_neon_poly8");
  (*lang_hooks.types.register_builtin_type) (neon_polyHI_type_node,
					     "__builtin_neon_poly16");

  tree intQI_pointer_node = build_pointer_type (neon_intQI_type_node);
  tree intHI_pointer_node = build_pointer_type (neon_intHI_type_node);
  tree intSI_pointer_node = build_pointer_type (neon_intSI_type_node);
  tree intDI_pointer_node = build_pointer_type (neon_intDI_type_node);
  tree float_pointer_node = build_pointer_type (neon_float_type_node);
 
  /* Next create constant-qualified versions of the above types.  */ 
  tree const_intQI_node = build_qualified_type (neon_intQI_type_node,
						TYPE_QUAL_CONST);
  tree const_intHI_node = build_qualified_type (neon_intHI_type_node,
						TYPE_QUAL_CONST);
  tree const_intSI_node = build_qualified_type (neon_intSI_type_node,
						TYPE_QUAL_CONST);
  tree const_intDI_node = build_qualified_type (neon_intDI_type_node,
						TYPE_QUAL_CONST);
  tree const_float_node = build_qualified_type (neon_float_type_node,
						TYPE_QUAL_CONST);

  tree const_intQI_pointer_node = build_pointer_type (const_intQI_node);
  tree const_intHI_pointer_node = build_pointer_type (const_intHI_node);
  tree const_intSI_pointer_node = build_pointer_type (const_intSI_node);
  tree const_intDI_pointer_node = build_pointer_type (const_intDI_node);
  tree const_float_pointer_node = build_pointer_type (const_float_node);

  /* Now create vector types based on our NEON element types.  */
  /* 64-bit vectors.  */
  tree V8QI_type_node =
    build_vector_type_for_mode (neon_intQI_type_node, V8QImode);
  tree V4HI_type_node =
    build_vector_type_for_mode (neon_intHI_type_node, V4HImode);
  tree V2SI_type_node =
    build_vector_type_for_mode (neon_intSI_type_node, V2SImode);
  tree V2SF_type_node =
    build_vector_type_for_mode (neon_float_type_node, V2SFmode);
  /* 128-bit vectors.  */
  tree V16QI_type_node =
    build_vector_type_for_mode (neon_intQI_type_node, V16QImode);
  tree V8HI_type_node =
    build_vector_type_for_mode (neon_intHI_type_node, V8HImode);
  tree V4SI_type_node =
    build_vector_type_for_mode (neon_intSI_type_node, V4SImode);
  tree V4SF_type_node =
    build_vector_type_for_mode (neon_float_type_node, V4SFmode);
  tree V2DI_type_node =
    build_vector_type_for_mode (neon_intDI_type_node, V2DImode);

  /* Unsigned integer types for various mode sizes.  */
  tree intUQI_type_node = make_unsigned_type (GET_MODE_PRECISION (QImode));
  tree intUHI_type_node = make_unsigned_type (GET_MODE_PRECISION (HImode));
  tree intUSI_type_node = make_unsigned_type (GET_MODE_PRECISION (SImode));
  tree intUDI_type_node = make_unsigned_type (GET_MODE_PRECISION (DImode));

  (*lang_hooks.types.register_builtin_type) (intUQI_type_node,
					     "__builtin_neon_uqi");
  (*lang_hooks.types.register_builtin_type) (intUHI_type_node,
					     "__builtin_neon_uhi");
  (*lang_hooks.types.register_builtin_type) (intUSI_type_node,
					     "__builtin_neon_usi");
  (*lang_hooks.types.register_builtin_type) (intUDI_type_node,
					     "__builtin_neon_udi");

  /* Opaque integer types for structures of vectors.  */
  tree intEI_type_node = make_signed_type (GET_MODE_PRECISION (EImode));
  tree intOI_type_node = make_signed_type (GET_MODE_PRECISION (OImode));
  tree intCI_type_node = make_signed_type (GET_MODE_PRECISION (CImode));
  tree intXI_type_node = make_signed_type (GET_MODE_PRECISION (XImode));

  (*lang_hooks.types.register_builtin_type) (intTI_type_node,
					     "__builtin_neon_ti");
  (*lang_hooks.types.register_builtin_type) (intEI_type_node,
					     "__builtin_neon_ei");
  (*lang_hooks.types.register_builtin_type) (intOI_type_node,
					     "__builtin_neon_oi");
  (*lang_hooks.types.register_builtin_type) (intCI_type_node,
					     "__builtin_neon_ci");
  (*lang_hooks.types.register_builtin_type) (intXI_type_node,
					     "__builtin_neon_xi");

  /* Pointers to vector types.  */
  tree V8QI_pointer_node = build_pointer_type (V8QI_type_node);
  tree V4HI_pointer_node = build_pointer_type (V4HI_type_node);
  tree V2SI_pointer_node = build_pointer_type (V2SI_type_node);
  tree V2SF_pointer_node = build_pointer_type (V2SF_type_node);
  tree V16QI_pointer_node = build_pointer_type (V16QI_type_node);
  tree V8HI_pointer_node = build_pointer_type (V8HI_type_node);
  tree V4SI_pointer_node = build_pointer_type (V4SI_type_node);
  tree V4SF_pointer_node = build_pointer_type (V4SF_type_node);
  tree V2DI_pointer_node = build_pointer_type (V2DI_type_node);

  /* Binops, all-doubleword arithmetic.  */
  TYPE4 (v8qi, v8qi, v8qi, si);
  TYPE4 (v4hi, v4hi, v4hi, si);
  TYPE4 (v2si, v2si, v2si, si);
  TYPE4 (v2sf, v2sf, v2sf, si);
  TYPE4 (di, di, di, si);
  
  /* Binops, all-quadword arithmetic.  */
  TYPE4 (v16qi, v16qi, v16qi, si);
  TYPE4 (v8hi, v8hi, v8hi, si);
  TYPE4 (v4si, v4si, v4si, si);
  TYPE4 (v4sf, v4sf, v4sf, si);
  TYPE4 (v2di, v2di, v2di, si);

  /* Binops, "long" operations (dest wider than operands).  */
  TYPE4 (v8hi, v8qi, v8qi, si);
  TYPE4 (v4si, v4hi, v4hi, si);
  TYPE4 (v2di, v2si, v2si, si);

  /* Binops, "wide" operations (dest and first operand wider than second
     operand).  */
  TYPE4 (v8hi, v8hi, v8qi, si);
  TYPE4 (v4si, v4si, v4hi, si);
  TYPE4 (v2di, v2di, v2si, si);

  /* Binops, "narrow" operations (dest narrower than operands).  */
  TYPE4 (v8qi, v8hi, v8hi, si);
  TYPE4 (v4hi, v4si, v4si, si);
  TYPE4 (v2si, v2di, v2di, si);

  /* Binops, comparisons (return type always an integer vector).  */
  TYPE4 (v2si, v2sf, v2sf, si);
  TYPE4 (v4si, v4sf, v4sf, si);

  /* Binops, dest and first operand elements wider (vpadal).  */
  TYPE4 (v4hi, v4hi, v8qi, si);
  TYPE4 (v2si, v2si, v4hi, si);
  TYPE4 (di, di, v2si, si);
  TYPE4 (v8hi, v8hi, v16qi, si);
  TYPE4 (v4si, v4si, v8hi, si);
  TYPE4 (v2di, v2di, v4si, si);

  /* Ternary operations, all-doubleword arithmetic.  */
  TYPE5 (v8qi, v8qi, v8qi, v8qi, si);
  TYPE5 (v4hi, v4hi, v4hi, v4hi, si);
  TYPE5 (v2si, v2si, v2si, v2si, si);
  TYPE5 (v2sf, v2sf, v2sf, v2sf, si);

  /* Ternary operations, all-quadword arithmetic.  */
  TYPE5 (v16qi, v16qi, v16qi, v16qi, si);
  TYPE5 (v8hi, v8hi, v8hi, v8hi, si);
  TYPE5 (v4si, v4si, v4si, v4si, si);
  TYPE5 (v4sf, v4sf, v4sf, v4sf, si);
  
  /* Ternary operations, "long" operations (dest and first operand
     wider than second and third operands).  */
  TYPE5 (v8hi, v8hi, v8qi, v8qi, si);
  TYPE5 (v4si, v4si, v4hi, v4hi, si);
  TYPE5 (v2di, v2di, v2si, v2si, si);
  
  /* Unops, all-doubleword arithmetic.  */
  TYPE3 (v8qi, v8qi, si);
  TYPE3 (v4hi, v4hi, si);
  TYPE3 (v2si, v2si, si);
  TYPE3 (v2sf, v2sf, si);
  TYPE3 (di, di, si);

  /* Unops, all-quadword arithmetic.  */
  TYPE3 (v16qi, v16qi, si);
  TYPE3 (v8hi, v8hi, si);
  TYPE3 (v4si, v4si, si);
  TYPE3 (v4sf, v4sf, si);
  TYPE3 (v2di, v2di, si);

  /* Unops, narrowing.  */
  TYPE3 (v8qi, v8hi, si);
  TYPE3 (v4hi, v4si, si);
  TYPE3 (v2si, v2di, si);

  /* Unops, widening.  */
  TYPE3 (v8hi, v8qi, si);
  TYPE3 (v4si, v4hi, si);
  TYPE3 (v2di, v2si, si);

  /* Unops, dest elements wider (vpaddl).  */
  TYPE3 (v4hi, v8qi, si);
  TYPE3 (v2si, v4hi, si);
  TYPE3 (di, v2si, si);
  TYPE3 (v8hi, v16qi, si);
  TYPE3 (v4si, v8hi, si);
  TYPE3 (v2di, v4si, si);

  /* Get-lane from doubleword insns (single-element result).  */
  TYPE4 (qi, v8qi, si, si);
  TYPE4 (hi, v4hi, si, si);
  TYPE4 (si, v2si, si, si);
  TYPE4 (sf, v2sf, si, si);
  TYPE4 (di, di, si, si);

  /* Get-lane from quadword insns.  */
  TYPE4 (qi, v16qi, si, si);
  TYPE4 (hi, v8hi, si, si);
  TYPE4 (si, v4si, si, si);
  TYPE4 (sf, v4sf, si, si);
  TYPE4 (di, v2di, si, si);

  /* Set lane in doubleword insns (single-element result).  */
  TYPE4 (v8qi, qi, v8qi, si);
  TYPE4 (v4hi, hi, v4hi, si);
  TYPE4 (v2si, si, v2si, si);
  TYPE4 (v2sf, sf, v2sf, si);

  /* Set lane in quadword insns.  */
  TYPE4 (v16qi, qi, v16qi, si);
  TYPE4 (v8hi, hi, v8hi, si);
  TYPE4 (v4si, si, v4si, si);
  TYPE4 (v4sf, sf, v4sf, si);
  TYPE4 (v2di, di, v2di, si);

  /* Create vector from bit pattern.  */
  TYPE2 (v8qi, di);
  TYPE2 (v4hi, di);
  TYPE2 (v2si, di);
  TYPE2 (v2sf, di);
  TYPE2 (di, di);

  /* Duplicate an ARM register into lanes of a vector.  */
  TYPE2 (v8qi, qi);
  TYPE2 (v4hi, hi);
  TYPE2 (v2si, si);
  TYPE2 (v2sf, sf);

  TYPE2 (v16qi, qi);
  TYPE2 (v8hi, hi);
  TYPE2 (v4si, si);
  TYPE2 (v4sf, sf);
  TYPE2 (v2di, di);

  /* Duplicate a lane of a vector to all lanes of another vector.  */
  TYPE3 (v16qi, v8qi, si);
  TYPE3 (v8hi, v4hi, si);
  TYPE3 (v4si, v2si, si);
  TYPE3 (v4sf, v2sf, si);
  TYPE3 (v2di, di, si);

  /* Combine doubleword vectors into quadword vectors.  */
  TYPE3 (v16qi, v8qi, v8qi);
  TYPE3 (v8hi, v4hi, v4hi);
  TYPE3 (v4si, v2si, v2si);
  TYPE3 (v4sf, v2sf, v2sf);
  TYPE3 (v2di, di, di);

  /* Split quadword vectors into high or low parts.  */
  TYPE2 (v8qi, v16qi);
  TYPE2 (v4hi, v8hi);
  TYPE2 (v2si, v4si);
  TYPE2 (v2sf, v4sf);
  TYPE2 (di, v2di);

  /* Conversions, int<->float.  */
  TYPE3 (v2si, v2sf, si);
  TYPE3 (v2sf, v2si, si);
  TYPE3 (v4si, v4sf, si);
  TYPE3 (v4sf, v4si, si);

  /* Conversions, fixed point<->float.  */
  TYPE4 (v2si, v2sf, si, si);
  TYPE4 (v2sf, v2si, si, si);
  TYPE4 (v4si, v4sf, si, si);
  TYPE4 (v4sf, v4si, si, si);
  
  /* Multiply by scalar (lane).  */
  TYPE5 (v4hi, v4hi, v4hi, si, si);
  TYPE5 (v2si, v2si, v2si, si, si);
  TYPE5 (v2sf, v2sf, v2sf, si, si);
  TYPE5 (v8hi, v8hi, v4hi, si, si);
  TYPE5 (v4si, v4si, v2si, si, si);
  TYPE5 (v4sf, v4sf, v2sf, si, si);

  /* Long multiply by scalar (lane).  */
  TYPE5 (v4si, v4hi, v4hi, si, si);
  TYPE5 (v2di, v2si, v2si, si, si);

  /* Multiply-accumulate etc. by scalar (lane).  */
  TYPE6 (v4hi, v4hi, v4hi, v4hi, si, si);
  TYPE6 (v2si, v2si, v2si, v2si, si, si);
  TYPE6 (v2sf, v2sf, v2sf, v2sf, si, si);
  TYPE6 (v8hi, v8hi, v8hi, v4hi, si, si);
  TYPE6 (v4si, v4si, v4si, v2si, si, si);
  TYPE6 (v4sf, v4sf, v4sf, v2sf, si, si);

  /* Multiply-accumulate, etc. by scalar (lane), widening.  */
  TYPE6 (v4si, v4si, v4hi, v4hi, si, si);
  TYPE6 (v2di, v2di, v2si, v2si, si, si);
  
  /* Multiply by scalar.  */
  TYPE4 (v4hi, v4hi, hi, si);
  TYPE4 (v2si, v2si, si, si);
  TYPE4 (v2sf, v2sf, sf, si);
  
  TYPE4 (v8hi, v8hi, hi, si);
  TYPE4 (v4si, v4si, si, si);
  TYPE4 (v4sf, v4sf, sf, si);

  /* Long multiply by scalar.  */
  TYPE4 (v4si, v4hi, hi, si);

  /* Multiply-accumulate etc. by scalar.  */
  TYPE5 (v4hi, v4hi, v4hi, hi, si);
 /* TYPE5 (v2si, v2si, v2si, si, si);*/
  TYPE5 (v2sf, v2sf, v2sf, sf, si);
  TYPE5 (v8hi, v8hi, v8hi, hi, si);
  TYPE5 (v4si, v4si, v4si, si, si);
  TYPE5 (v4sf, v4sf, v4sf, sf, si);

  /* Multiply-accumulate by scalar, widening.  */
  TYPE5 (v4si, v4si, v4hi, hi, si);
  TYPE5 (v2di, v2di, v2si, si, si);

  /* Bit select operations.  */
  TYPE4 (v8qi, v8qi, v8qi, v8qi);
  TYPE4 (v4hi, v4hi, v4hi, v4hi);
  TYPE4 (v2si, v2si, v2si, v2si);
  TYPE4 (v2sf, v2si, v2sf, v2sf);
  TYPE4 (di, di, di, di);

  TYPE4 (v16qi, v16qi, v16qi, v16qi);
  TYPE4 (v8hi, v8hi, v8hi, v8hi);
  TYPE4 (v4si, v4si, v4si, v4si);
  TYPE4 (v4sf, v4si, v4sf, v4sf);
  TYPE4 (v2di, v2di, v2di, v2di);

  /* Shift immediate operations.  */
  TYPE4 (v8qi, v8qi, si, si);
  TYPE4 (v4hi, v4hi, si, si);

  TYPE4 (v16qi, v16qi, si, si);
  TYPE4 (v8hi, v8hi, si, si);
  TYPE4 (v2di, v2di, si, si);

  /* Shift immediate, long operations.  */
  TYPE4 (v8hi, v8qi, si, si);
  TYPE4 (v4si, v4hi, si, si);
  TYPE4 (v2di, v2si, si, si);

  /* Shift immediate, narrowing operations.  */
  TYPE4 (v8qi, v8hi, si, si);
  TYPE4 (v4hi, v4si, si, si);
  TYPE4 (v2si, v2di, si, si);

  /* Shift + accumulate operations.  */
  TYPE5 (v8qi, v8qi, v8qi, si, si);
  TYPE5 (di, di, di, si, si);

  TYPE5 (v16qi, v16qi, v16qi, si, si);
  TYPE5 (v8hi, v8hi, v8hi, si, si);
  TYPE5 (v4sf, v4sf, v4sf, si, si);
  TYPE5 (v2di, v2di, v2di, si, si);

  /* Operations which return results as pairs.  */
  TYPE4 (void, pv8qi, v8qi, v8qi);
  TYPE4 (void, pv4hi, v4hi, v4hi);
  TYPE4 (void, pv2si, v2si, v2si);
  TYPE4 (void, pv2sf, v2sf, v2sf);
  TYPE4 (void, pdi, di, di);

  TYPE4 (void, pv16qi, v16qi, v16qi);
  TYPE4 (void, pv8hi, v8hi, v8hi);
  TYPE4 (void, pv4si, v4si, v4si);
  TYPE4 (void, pv4sf, v4sf, v4sf);
  TYPE4 (void, pv2di, v2di, v2di);

  /* Table look-up.  */
  TYPE3 (v8qi, v8qi, v8qi);
  TYPE3 (v8qi, ti, v8qi);
  TYPE3 (v8qi, ei, v8qi);
  TYPE3 (v8qi, oi, v8qi);
  
  /* Extended table look-up.  */
  /*TYPE4 (v8qi, v8qi, v8qi, v8qi);*/
  TYPE4 (v8qi, v8qi, ti, v8qi);
  TYPE4 (v8qi, v8qi, ei, v8qi);
  TYPE4 (v8qi, v8qi, oi, v8qi);

  /* Load operations, double-word.  */
  tree v8qi_ftype_const_qi_pointer =
    build_function_type_list (V8QI_type_node, const_intQI_pointer_node, NULL);
  tree v4hi_ftype_const_hi_pointer =
    build_function_type_list (V4HI_type_node, const_intHI_pointer_node, NULL);
  tree v2si_ftype_const_si_pointer =
    build_function_type_list (V2SI_type_node, const_intSI_pointer_node, NULL);
  tree di_ftype_const_di_pointer =
    build_function_type_list (intDI_type_node, const_intDI_pointer_node, NULL);
  tree v2sf_ftype_const_sf_pointer =
    build_function_type_list (V2SF_type_node, const_float_pointer_node, NULL);

  /* Load operations, quad-word.  */
  tree v16qi_ftype_const_qi_pointer =
    build_function_type_list (V16QI_type_node, const_intQI_pointer_node, NULL);
  tree v8hi_ftype_const_hi_pointer =
    build_function_type_list (V8HI_type_node, const_intHI_pointer_node, NULL);
  tree v4si_ftype_const_si_pointer =
    build_function_type_list (V4SI_type_node, const_intSI_pointer_node, NULL);
  tree v2di_ftype_const_di_pointer =
    build_function_type_list (V2DI_type_node, const_intDI_pointer_node, NULL);
  tree v4sf_ftype_const_sf_pointer =
    build_function_type_list (V4SF_type_node, const_float_pointer_node, NULL);

  /* Load lane operations, double-word.  */
  tree v8qi_ftype_const_qi_pointer_v8qi_si =
    build_function_type_list (V8QI_type_node, const_intQI_pointer_node,
			      V8QI_type_node, intSI_type_node, NULL);
  tree v4hi_ftype_const_hi_pointer_v4hi_si =
    build_function_type_list (V4HI_type_node, const_intHI_pointer_node,
			      V4HI_type_node, intSI_type_node, NULL);
  tree v2si_ftype_const_si_pointer_v2si_si =
    build_function_type_list (V2SI_type_node, const_intSI_pointer_node,
			      V2SI_type_node, intSI_type_node, NULL);
  tree di_ftype_const_di_pointer_di_si =
    build_function_type_list (intDI_type_node, const_intDI_pointer_node,
			      intDI_type_node, intSI_type_node, NULL);
  tree v2sf_ftype_const_sf_pointer_v2sf_si =
    build_function_type_list (V2SF_type_node, const_float_pointer_node,
			      V2SF_type_node, intSI_type_node, NULL);

  /* Load lane operations, quad-word.  */
  tree v16qi_ftype_const_qi_pointer_v16qi_si =
    build_function_type_list (V16QI_type_node, const_intQI_pointer_node,
			      V16QI_type_node, intSI_type_node, NULL);
  tree v8hi_ftype_const_hi_pointer_v8hi_si =
    build_function_type_list (V8HI_type_node, const_intHI_pointer_node,
			      V8HI_type_node, intSI_type_node, NULL);
  tree v4si_ftype_const_si_pointer_v4si_si =
    build_function_type_list (V4SI_type_node, const_intSI_pointer_node,
			      V4SI_type_node, intSI_type_node, NULL);
  tree v2di_ftype_const_di_pointer_v2di_si =
    build_function_type_list (V2DI_type_node, const_intDI_pointer_node,
			      V2DI_type_node, intSI_type_node, NULL);
  tree v4sf_ftype_const_sf_pointer_v4sf_si =
    build_function_type_list (V4SF_type_node, const_float_pointer_node,
			      V4SF_type_node, intSI_type_node, NULL);

  /* Store operations, double-word.  */
  tree void_ftype_qi_pointer_v8qi =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      V8QI_type_node, NULL);
  tree void_ftype_hi_pointer_v4hi =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      V4HI_type_node, NULL);
  tree void_ftype_si_pointer_v2si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      V2SI_type_node, NULL);
  tree void_ftype_di_pointer_di =
    build_function_type_list (void_type_node, intDI_pointer_node,
			      intDI_type_node, NULL);
  tree void_ftype_sf_pointer_v2sf =
    build_function_type_list (void_type_node, float_pointer_node,
			      V2SF_type_node, NULL);

  /* Store operations, quad-word.  */
  tree void_ftype_qi_pointer_v16qi =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      V16QI_type_node, NULL);
  tree void_ftype_hi_pointer_v8hi =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      V8HI_type_node, NULL);
  tree void_ftype_si_pointer_v4si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      V4SI_type_node, NULL);
  tree void_ftype_di_pointer_v2di =
    build_function_type_list (void_type_node, intDI_pointer_node,
			      V2DI_type_node, NULL);
  tree void_ftype_sf_pointer_v4sf =
    build_function_type_list (void_type_node, float_pointer_node,
			      V4SF_type_node, NULL);

  /* Store lane operations, double-word.  */
  tree void_ftype_qi_pointer_v8qi_si =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      V8QI_type_node, intSI_type_node, NULL);
  tree void_ftype_hi_pointer_v4hi_si =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      V4HI_type_node, intSI_type_node, NULL);
  tree void_ftype_si_pointer_v2si_si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      V2SI_type_node, intSI_type_node, NULL);
  tree void_ftype_di_pointer_di_si =
    build_function_type_list (void_type_node, intDI_pointer_node,
			      intDI_type_node, intSI_type_node, NULL);
  tree void_ftype_sf_pointer_v2sf_si =
    build_function_type_list (void_type_node, float_pointer_node,
			      V2SF_type_node, intSI_type_node, NULL);

  /* Store lane operations, quad-word.  */
  tree void_ftype_qi_pointer_v16qi_si =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      V16QI_type_node, intSI_type_node, NULL);
  tree void_ftype_hi_pointer_v8hi_si =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      V8HI_type_node, intSI_type_node, NULL);
  tree void_ftype_si_pointer_v4si_si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      V4SI_type_node, intSI_type_node, NULL);
  tree void_ftype_di_pointer_v2di_si =
    build_function_type_list (void_type_node, intDI_pointer_node,
			      V2DI_type_node, intSI_type_node, NULL);
  tree void_ftype_sf_pointer_v4sf_si =
    build_function_type_list (void_type_node, float_pointer_node,
			      V4SF_type_node, intSI_type_node, NULL);

  /* Load size-2 structure operations, double-word.  */
  tree ti_ftype_const_qi_pointer =
    build_function_type_list (intTI_type_node, const_intQI_pointer_node, NULL);
  tree ti_ftype_const_hi_pointer =
    build_function_type_list (intTI_type_node, const_intHI_pointer_node, NULL);
  tree ti_ftype_const_si_pointer =
    build_function_type_list (intTI_type_node, const_intSI_pointer_node, NULL);
  tree ti_ftype_const_di_pointer =
    build_function_type_list (intTI_type_node, const_intDI_pointer_node, NULL);
  tree ti_ftype_const_sf_pointer =
    build_function_type_list (intTI_type_node, const_float_pointer_node, NULL);

  /* Load size-2 structure operations, quad-word; also load size-4,
     double-word.  */
  tree oi_ftype_const_qi_pointer =
    build_function_type_list (intOI_type_node, const_intQI_pointer_node, NULL);
  tree oi_ftype_const_hi_pointer =
    build_function_type_list (intOI_type_node, const_intHI_pointer_node, NULL);
  tree oi_ftype_const_si_pointer =
    build_function_type_list (intOI_type_node, const_intSI_pointer_node, NULL);
  tree oi_ftype_const_sf_pointer =
    build_function_type_list (intOI_type_node, const_float_pointer_node, NULL);

  /* Load lane size-2 structure operations, double-word.  */
  tree ti_ftype_const_qi_pointer_ti_si =
    build_function_type_list (intTI_type_node, const_intQI_pointer_node,
			      intTI_type_node, intSI_type_node, NULL);
  tree ti_ftype_const_hi_pointer_ti_si =
    build_function_type_list (intTI_type_node, const_intHI_pointer_node,
			      intTI_type_node, intSI_type_node, NULL);
  tree ti_ftype_const_si_pointer_ti_si =
    build_function_type_list (intTI_type_node, const_intSI_pointer_node,
			      intTI_type_node, intSI_type_node, NULL);
  tree ti_ftype_const_sf_pointer_ti_si =
    build_function_type_list (intTI_type_node, const_float_pointer_node,
			      intTI_type_node, intSI_type_node, NULL);

  /* Load lane size-2 structure operations, quad-word; also load lane size-4,
     double-word.  */
  tree oi_ftype_const_hi_pointer_oi_si =
    build_function_type_list (intOI_type_node, const_intHI_pointer_node,
			      intOI_type_node, intSI_type_node, NULL);
  tree oi_ftype_const_si_pointer_oi_si =
    build_function_type_list (intOI_type_node, const_intSI_pointer_node,
			      intOI_type_node, intSI_type_node, NULL);
  tree oi_ftype_const_sf_pointer_oi_si =
    build_function_type_list (intOI_type_node, const_float_pointer_node,
			      intOI_type_node, intSI_type_node, NULL);

  /* Store size-2 structure operations, double-word.  */
  tree void_ftype_qi_pointer_ti =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      intTI_type_node, NULL);
  tree void_ftype_hi_pointer_ti =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intTI_type_node, NULL);
  tree void_ftype_si_pointer_ti =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intTI_type_node, NULL);
  tree void_ftype_di_pointer_ti =
    build_function_type_list (void_type_node, intDI_pointer_node,
			      intTI_type_node, NULL);
  tree void_ftype_sf_pointer_ti =
    build_function_type_list (void_type_node, float_pointer_node,
			      intTI_type_node, NULL);

  /* Store size-2 structure operations, quad-word; also store size-4,
     double-word.  */
  tree void_ftype_qi_pointer_oi =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      intOI_type_node, NULL);
  tree void_ftype_hi_pointer_oi =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intOI_type_node, NULL);
  tree void_ftype_si_pointer_oi =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intOI_type_node, NULL);
  tree void_ftype_sf_pointer_oi =
    build_function_type_list (void_type_node, float_pointer_node,
			      intOI_type_node, NULL);

  /* Store lane size-2 structure operations, double-word.  */
  tree void_ftype_qi_pointer_ti_si =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      intTI_type_node, intSI_type_node, NULL);
  tree void_ftype_hi_pointer_ti_si =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intTI_type_node, intSI_type_node, NULL);
  tree void_ftype_si_pointer_ti_si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intTI_type_node, intSI_type_node, NULL);
  tree void_ftype_sf_pointer_ti_si =
    build_function_type_list (void_type_node, float_pointer_node,
			      intTI_type_node, intSI_type_node, NULL);

  /* Store lane size-2 structure operations, quad-word; also store
     lane size-4, double-word.  */
  tree void_ftype_hi_pointer_oi_si =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intOI_type_node, intSI_type_node, NULL);
  tree void_ftype_si_pointer_oi_si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intOI_type_node, intSI_type_node, NULL);
  tree void_ftype_sf_pointer_oi_si =
    build_function_type_list (void_type_node, float_pointer_node,
			      intOI_type_node, intSI_type_node, NULL);

  /* Load size-3 structure operations, double-word.  */
  tree ei_ftype_const_qi_pointer =
    build_function_type_list (intEI_type_node, const_intQI_pointer_node, NULL);
  tree ei_ftype_const_hi_pointer =
    build_function_type_list (intEI_type_node, const_intHI_pointer_node, NULL);
  tree ei_ftype_const_si_pointer =
    build_function_type_list (intEI_type_node, const_intSI_pointer_node, NULL);
  tree ei_ftype_const_di_pointer =
    build_function_type_list (intEI_type_node, const_intDI_pointer_node, NULL);
  tree ei_ftype_const_sf_pointer =
    build_function_type_list (intEI_type_node, const_float_pointer_node, NULL);

  /* Load size-3 structure operations, quad-word.  */
  tree ci_ftype_const_qi_pointer =
    build_function_type_list (intCI_type_node, const_intQI_pointer_node, NULL);
  tree ci_ftype_const_hi_pointer =
    build_function_type_list (intCI_type_node, const_intHI_pointer_node, NULL);
  tree ci_ftype_const_si_pointer =
    build_function_type_list (intCI_type_node, const_intSI_pointer_node, NULL);
  tree ci_ftype_const_sf_pointer =
    build_function_type_list (intCI_type_node, const_float_pointer_node, NULL);

  /* Load lane size-3 structure operations, double-word.  */
  tree ei_ftype_const_qi_pointer_ei_si =
    build_function_type_list (intEI_type_node, const_intQI_pointer_node,
			      intEI_type_node, intSI_type_node, NULL);
  tree ei_ftype_const_hi_pointer_ei_si =
    build_function_type_list (intEI_type_node, const_intHI_pointer_node,
			      intEI_type_node, intSI_type_node, NULL);
  tree ei_ftype_const_si_pointer_ei_si =
    build_function_type_list (intEI_type_node, const_intSI_pointer_node,
			      intEI_type_node, intSI_type_node, NULL);
  tree ei_ftype_const_sf_pointer_ei_si =
    build_function_type_list (intEI_type_node, const_float_pointer_node,
			      intEI_type_node, intSI_type_node, NULL);

  /* Load lane size-3 structure operations, quad-word.  */
  tree ci_ftype_const_hi_pointer_ci_si =
    build_function_type_list (intCI_type_node, const_intHI_pointer_node,
			      intCI_type_node, intSI_type_node, NULL);
  tree ci_ftype_const_si_pointer_ci_si =
    build_function_type_list (intCI_type_node, const_intSI_pointer_node,
			      intCI_type_node, intSI_type_node, NULL);
  tree ci_ftype_const_sf_pointer_ci_si =
    build_function_type_list (intCI_type_node, const_float_pointer_node,
			      intCI_type_node, intSI_type_node, NULL);

  /* Store size-3 structure operations, double-word.  */
  tree void_ftype_qi_pointer_ei =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      intEI_type_node, NULL);
  tree void_ftype_hi_pointer_ei =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intEI_type_node, NULL);
  tree void_ftype_si_pointer_ei =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intEI_type_node, NULL);
  tree void_ftype_di_pointer_ei =
    build_function_type_list (void_type_node, intDI_pointer_node,
			      intEI_type_node, NULL);
  tree void_ftype_sf_pointer_ei =
    build_function_type_list (void_type_node, float_pointer_node,
			      intEI_type_node, NULL);

  /* Store size-3 structure operations, quad-word.  */
  tree void_ftype_qi_pointer_ci =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      intCI_type_node, NULL);
  tree void_ftype_hi_pointer_ci =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intCI_type_node, NULL);
  tree void_ftype_si_pointer_ci =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intCI_type_node, NULL);
  tree void_ftype_sf_pointer_ci =
    build_function_type_list (void_type_node, float_pointer_node,
			      intCI_type_node, NULL);

  /* Store lane size-3 structure operations, double-word.  */
  tree void_ftype_qi_pointer_ei_si =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      intEI_type_node, intSI_type_node, NULL);
  tree void_ftype_hi_pointer_ei_si =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intEI_type_node, intSI_type_node, NULL);
  tree void_ftype_si_pointer_ei_si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intEI_type_node, intSI_type_node, NULL);
  tree void_ftype_sf_pointer_ei_si =
    build_function_type_list (void_type_node, float_pointer_node,
			      intEI_type_node, intSI_type_node, NULL);

  /* Store lane size-3 structure operations, quad-word.  */
  tree void_ftype_hi_pointer_ci_si =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intCI_type_node, intSI_type_node, NULL);
  tree void_ftype_si_pointer_ci_si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intCI_type_node, intSI_type_node, NULL);
  tree void_ftype_sf_pointer_ci_si =
    build_function_type_list (void_type_node, float_pointer_node,
			      intCI_type_node, intSI_type_node, NULL);

  /* Load size-4 structure operations, double-word.  */
  tree oi_ftype_const_di_pointer =
    build_function_type_list (intOI_type_node, const_intDI_pointer_node, NULL);

  /* Load size-4 structure operations, quad-word.  */
  tree xi_ftype_const_qi_pointer =
    build_function_type_list (intXI_type_node, const_intQI_pointer_node, NULL);
  tree xi_ftype_const_hi_pointer =
    build_function_type_list (intXI_type_node, const_intHI_pointer_node, NULL);
  tree xi_ftype_const_si_pointer =
    build_function_type_list (intXI_type_node, const_intSI_pointer_node, NULL);
  tree xi_ftype_const_sf_pointer =
    build_function_type_list (intXI_type_node, const_float_pointer_node, NULL);

  /* Load lane size-4 structure operations, double-word.  */
  tree oi_ftype_const_qi_pointer_oi_si =
    build_function_type_list (intOI_type_node, const_intQI_pointer_node,
			      intOI_type_node, intSI_type_node, NULL);

  /* Load lane size-4 structure operations, quad-word.  */
  tree xi_ftype_const_hi_pointer_xi_si =
    build_function_type_list (intXI_type_node, const_intHI_pointer_node,
			      intXI_type_node, intSI_type_node, NULL);
  tree xi_ftype_const_si_pointer_xi_si =
    build_function_type_list (intXI_type_node, const_intSI_pointer_node,
			      intXI_type_node, intSI_type_node, NULL);
  tree xi_ftype_const_sf_pointer_xi_si =
    build_function_type_list (intXI_type_node, const_float_pointer_node,
			      intXI_type_node, intSI_type_node, NULL);

  /* Store size-4 structure operations, double-word.  */
  tree void_ftype_di_pointer_oi =
    build_function_type_list (void_type_node, intDI_pointer_node,
			      intOI_type_node, NULL);

  /* Store size-4 structure operations, quad-word.  */
  tree void_ftype_qi_pointer_xi =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      intXI_type_node, NULL);
  tree void_ftype_hi_pointer_xi =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intXI_type_node, NULL);
  tree void_ftype_si_pointer_xi =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intXI_type_node, NULL);
  tree void_ftype_sf_pointer_xi =
    build_function_type_list (void_type_node, float_pointer_node,
			      intXI_type_node, NULL);

  /* Store lane size-4 structure operations, double-word.  */
  tree void_ftype_qi_pointer_oi_si =
    build_function_type_list (void_type_node, intQI_pointer_node,
			      intOI_type_node, intSI_type_node, NULL);

  /* Store lane size-4 structure operations, quad-word.  */
  tree void_ftype_hi_pointer_xi_si =
    build_function_type_list (void_type_node, intHI_pointer_node,
			      intXI_type_node, intSI_type_node, NULL);
  tree void_ftype_si_pointer_xi_si =
    build_function_type_list (void_type_node, intSI_pointer_node,
			      intXI_type_node, intSI_type_node, NULL);
  tree void_ftype_sf_pointer_xi_si =
    build_function_type_list (void_type_node, float_pointer_node,
			      intXI_type_node, intSI_type_node, NULL);

  tree reinterp_ftype_dreg[5][5];
  tree reinterp_ftype_qreg[5][5];
  tree dreg_types[5], qreg_types[5];

  dreg_types[0] = V8QI_type_node;
  dreg_types[1] = V4HI_type_node;
  dreg_types[2] = V2SI_type_node;
  dreg_types[3] = V2SF_type_node;
  dreg_types[4] = neon_intDI_type_node;

  qreg_types[0] = V16QI_type_node;
  qreg_types[1] = V8HI_type_node;
  qreg_types[2] = V4SI_type_node;
  qreg_types[3] = V4SF_type_node;
  qreg_types[4] = V2DI_type_node;
  
  for (i = 0; i < 5; i++)
    {
      int j;
      for (j = 0; j < 5; j++)
        {
          reinterp_ftype_dreg[i][j]
            = build_function_type_list (dreg_types[i], dreg_types[j], NULL);
          reinterp_ftype_qreg[i][j]
            = build_function_type_list (qreg_types[i], qreg_types[j], NULL);
        }
    }

  for (i = 0; i < ARRAY_SIZE (neon_builtin_data); i++)
    {
      neon_builtin_datum *d = &neon_builtin_data[i];
      unsigned int j, codeidx = 0;

      d->base_fcode = fcode;

      for (j = 0; j < T_MAX; j++)
        {
          const char* const modenames[] = {
            "v8qi", "v4hi", "v2si", "v2sf", "di",
            "v16qi", "v8hi", "v4si", "v4sf", "v2di"
          };
          char namebuf[60];
          tree ftype = NULL;
          enum insn_code icode;
          enum machine_mode tmode, mode0, mode1, mode2, mode3;
          
          if ((d->bits & (1 << j)) == 0)
            continue;
          
          icode = d->codes[codeidx++];
          
          tmode = insn_data[icode].operand[0].mode;
          mode0 = insn_data[icode].operand[1].mode;
          mode1 = insn_data[icode].operand[2].mode;
          mode2 = insn_data[icode].operand[3].mode;
          mode3 = insn_data[icode].operand[4].mode;
          
          switch (d->itype)
            {
            case NEON_UNOP:
              /* A unary operation with one vector operand and a vector
                 destination, plus an extra information word.  */
              gcc_assert (valid_neon_mode (tmode) && valid_neon_mode (mode0)
                          && mode1 == SImode);
              switch (tmode)
                {
                case V8QImode:
                  if (mode0 == V8QImode)
                    ftype = v8qi_ftype_v8qi_si;
                  else if (mode0 == V8HImode)
                    ftype = v8qi_ftype_v8hi_si;
                  break;

                case V4HImode:
                  if (mode0 == V4HImode)
                    ftype = v4hi_ftype_v4hi_si;
                  else if (mode0 == V4SImode)
                    ftype = v4hi_ftype_v4si_si;
                  else if (mode0 == V8QImode)
                    ftype = v4hi_ftype_v8qi_si;
                  break;

                case V2SImode:
                  if (mode0 == V2SImode)
                    ftype = v2si_ftype_v2si_si;
                  else if (mode0 == V2DImode)
                    ftype = v2si_ftype_v2di_si;
                  else if (mode0 == V4HImode)
                    ftype = v2si_ftype_v4hi_si;
                  break;

                case V2SFmode:
                  if (mode0 == V2SFmode)
                    ftype = v2sf_ftype_v2sf_si;
                  break;

                case DImode:
                  if (mode0 == DImode)
                    ftype = di_ftype_di_si;
		  else if (mode0 == V2SImode)
		    ftype = di_ftype_v2si_si;
                  break;

                case V16QImode:
                  if (mode0 == V16QImode)
                    ftype = v16qi_ftype_v16qi_si;
                  break;
                
                case V8HImode:
                  if (mode0 == V8HImode)
                    ftype = v8hi_ftype_v8hi_si;
                  else if (mode0 == V8QImode)
                    ftype = v8hi_ftype_v8qi_si;
                  else if (mode0 == V16QImode)
                    ftype = v8hi_ftype_v16qi_si;
                  break;
                
                case V4SImode:
                  if (mode0 == V4SImode)
                    ftype = v4si_ftype_v4si_si;
                  else if (mode0 == V4HImode)
                    ftype = v4si_ftype_v4hi_si;
                  else if (mode0 == V8HImode)
                    ftype = v4si_ftype_v8hi_si;
                  break;
                
                case V4SFmode:
                  if (mode0 == V4SFmode)
                    ftype = v4sf_ftype_v4sf_si;
                  break;
                
                case V2DImode:
                  if (mode0 == V2DImode)
                    ftype = v2di_ftype_v2di_si;
                  else if (mode0 == V2SImode)
                    ftype = v2di_ftype_v2si_si;
                  else if (mode0 == V4SImode)
                    ftype = v2di_ftype_v4si_si;
                  break;

                default:
                  gcc_unreachable ();
                }
              break;

            case NEON_BINOP:
            case NEON_LOGICBINOP:
            case NEON_SHIFTINSERT:
              /* A binary operation with two vector operands and a vector
                 destination, plus an extra information word.  */
              gcc_assert (valid_neon_mode (tmode) && valid_neon_mode (mode0)
                          && valid_neon_mode (mode1) && mode2 == SImode);
              switch (tmode)
                {
                case V8QImode:
                  if (mode0 == V8QImode && mode1 == V8QImode)
                    ftype = v8qi_ftype_v8qi_v8qi_si;
                  else if (mode0 == V8HImode && mode1 == V8HImode)
                    ftype = v8qi_ftype_v8hi_v8hi_si;
                  break;

                case V4HImode:
                  if (mode0 == V4HImode && mode1 == V4HImode)
                    ftype = v4hi_ftype_v4hi_v4hi_si;
                  else if (mode0 == V4SImode && mode1 == V4SImode)
                    ftype = v4hi_ftype_v4si_v4si_si;
                  else if (mode0 == V4HImode && mode1 == V8QImode)
                    ftype = v4hi_ftype_v4hi_v8qi_si;
                  break;

                case V2SImode:
                  if (mode0 == V2SImode && mode1 == V2SImode)
                    ftype = v2si_ftype_v2si_v2si_si;
                  else if (mode0 == V2DImode && mode1 == V2DImode)
                    ftype = v2si_ftype_v2di_v2di_si;
                  else if (mode0 == V2SFmode && mode1 == V2SFmode)
                    ftype = v2si_ftype_v2sf_v2sf_si;
                  else if (mode0 == V2SImode && mode1 == V4HImode)
                    ftype = v2si_ftype_v2si_v4hi_si;
                  break;

                case V2SFmode:
                  if (mode0 == V2SFmode && mode1 == V2SFmode)
                    ftype = v2sf_ftype_v2sf_v2sf_si;
                  break;

                case DImode:
                  if (mode0 == DImode && mode1 == DImode)
                    ftype = di_ftype_di_di_si;
		  else if (mode0 == DImode && mode1 == V2SImode)
		    ftype = di_ftype_di_v2si_si;
                  break;

                case V16QImode:
                  if (mode0 == V16QImode && mode1 == V16QImode)
                    ftype = v16qi_ftype_v16qi_v16qi_si;
                  break;

                case V8HImode:
                  if (mode0 == V8HImode && mode1 == V8HImode)
                    ftype = v8hi_ftype_v8hi_v8hi_si;
                  else if (mode0 == V8QImode && mode1 == V8QImode)
                    ftype = v8hi_ftype_v8qi_v8qi_si;
                  else if (mode0 == V8HImode && mode1 == V8QImode)
                    ftype = v8hi_ftype_v8hi_v8qi_si;
                  else if (mode0 == V8HImode && mode1 == V16QImode)
                    ftype = v8hi_ftype_v8hi_v16qi_si;
                  break;

                case V4SImode:
                  if (mode0 == V4SImode && mode1 == V4SImode)
                    ftype = v4si_ftype_v4si_v4si_si;
                  else if (mode0 == V4HImode && mode1 == V4HImode)
                    ftype = v4si_ftype_v4hi_v4hi_si;
                  else if (mode0 == V4SImode && mode1 == V4HImode)
                    ftype = v4si_ftype_v4si_v4hi_si;
                  else if (mode0 == V4SFmode && mode1 == V4SFmode)
                    ftype = v4si_ftype_v4sf_v4sf_si;
                  else if (mode0 == V4SImode && mode1 == V8HImode)
                    ftype = v4si_ftype_v4si_v8hi_si;
                  break;

                case V4SFmode:
                  if (mode0 == V4SFmode && mode1 == V4SFmode)
                    ftype = v4sf_ftype_v4sf_v4sf_si;
                  break;

                case V2DImode:
                  if (mode0 == V2DImode && mode1 == V2DImode)
                    ftype = v2di_ftype_v2di_v2di_si;
                  else if (mode0 == V2SImode && mode1 == V2SImode)
                    ftype = v2di_ftype_v2si_v2si_si;
                  else if (mode0 == V2DImode && mode1 == V2SImode)
                    ftype = v2di_ftype_v2di_v2si_si;
                  else if (mode0 == V2DImode && mode1 == V4SImode)
                    ftype = v2di_ftype_v2di_v4si_si;
                  break;

                default:
                  gcc_unreachable ();
                }
              break;

            case NEON_TERNOP:
              /* A ternary operation with three vector operands and a
                 vector destination, plus an extra information
                 word.  */
              gcc_assert (valid_neon_mode (tmode) && valid_neon_mode (mode0)
                          && valid_neon_mode (mode1)
			  && valid_neon_mode (mode2)
			  && mode3 == SImode);
              switch (tmode)
                {
                case V8QImode:
                  if (mode0 == V8QImode && mode1 == V8QImode
		      && mode2 == V8QImode)
                    ftype = v8qi_ftype_v8qi_v8qi_v8qi_si;
                  break;

                case V4HImode:
                  if (mode0 == V4HImode && mode1 == V4HImode
		      && mode2 == V4HImode)
                    ftype = v4hi_ftype_v4hi_v4hi_v4hi_si;
                  break;

                case V2SImode:
                  if (mode0 == V2SImode && mode1 == V2SImode
		      && mode2 == V2SImode)
                    ftype = v2si_ftype_v2si_v2si_v2si_si;
                  break;

                case V2SFmode:
                  if (mode0 == V2SFmode && mode1 == V2SFmode
		      && mode2 == V2SFmode)
                    ftype = v2sf_ftype_v2sf_v2sf_v2sf_si;
                  break;

                case V16QImode:
                  if (mode0 == V16QImode && mode1 == V16QImode
		      && mode2 == V16QImode)
                    ftype = v16qi_ftype_v16qi_v16qi_v16qi_si;
                  break;

                case V8HImode:
                  if (mode0 == V8HImode && mode1 == V8HImode
		      && mode2 == V8HImode)
                    ftype = v8hi_ftype_v8hi_v8hi_v8hi_si;
                  else if (mode0 == V8HImode && mode1 == V8QImode
			   && mode2 == V8QImode)
                    ftype = v8hi_ftype_v8hi_v8qi_v8qi_si;
                  break;

                case V4SImode:
                  if (mode0 == V4SImode && mode1 == V4SImode
		      && mode2 == V4SImode)
                    ftype = v4si_ftype_v4si_v4si_v4si_si;
                  else if (mode0 == V4SImode && mode1 == V4HImode
			   && mode2 == V4HImode)
                    ftype = v4si_ftype_v4si_v4hi_v4hi_si;
                  break;

                case V4SFmode:
                  if (mode0 == V4SFmode && mode1 == V4SFmode
		      && mode2 == V4SFmode)
                    ftype = v4sf_ftype_v4sf_v4sf_v4sf_si;
                  break;

                case V2DImode:
                  if (mode0 == V2DImode && mode1 == V2SImode
		      && mode2 == V2SImode)
                    ftype = v2di_ftype_v2di_v2si_v2si_si;
                  break;

                default:
                  gcc_unreachable ();
                }
              break;

	    case NEON_GETLANE:
              /* Vector lane extraction.  */
              gcc_assert (valid_neon_mode (mode0) && mode1 == SImode
			  && mode2 == SImode);
              switch (tmode)
		{
                case QImode:
                  if (mode0 == V8QImode)
                    ftype = qi_ftype_v8qi_si_si;
                  else if (mode0 == V16QImode)
                    ftype = qi_ftype_v16qi_si_si;
                  break;

                case HImode:
                  if (mode0 == V4HImode)
                    ftype = hi_ftype_v4hi_si_si;
                  else if (mode0 == V8HImode)
                    ftype = hi_ftype_v8hi_si_si;
                  break;

                case SImode:
                  if (mode0 == V2SImode)
                    ftype = si_ftype_v2si_si_si;
                  else if (mode0 == V4SImode)
                    ftype = si_ftype_v4si_si_si;
                  break;

                case SFmode:
                  if (mode0 == V2SFmode)
                    ftype = sf_ftype_v2sf_si_si;
                  else if (mode0 == V4SFmode)
                    ftype = sf_ftype_v4sf_si_si;
                  break;

                case DImode:
                  if (mode0 == DImode)
                    ftype = di_ftype_di_si_si;
                  else if (mode0 == V2DImode)
                    ftype = di_ftype_v2di_si_si;
                  break;

                default:
                  gcc_unreachable ();
                }
              break;

	    case NEON_SETLANE:
              {
                /* Set lanes in vector.  */
                gcc_assert (valid_neon_mode (mode1) && mode2 == SImode);
                switch (tmode)
                  {
                  case V8QImode:
                    if (mode0 == QImode && mode1 == V8QImode)
                      ftype = v8qi_ftype_qi_v8qi_si;
                    break;

                  case V4HImode:
                    if (mode0 == HImode && mode1 == V4HImode)
                      ftype = v4hi_ftype_hi_v4hi_si;
                    break;

                  case V2SImode:
                    if (mode0 == SImode && mode1 == V2SImode)
                      ftype = v2si_ftype_si_v2si_si;
                    break;

                  case V2SFmode:
                    if (mode0 == SFmode && mode1 == V2SFmode)
                      ftype = v2sf_ftype_sf_v2sf_si;
                    break;

                  case DImode:
                    if (mode0 == DImode && mode1 == DImode)
                      ftype = di_ftype_di_di_si;
                    break;

                  case V16QImode:
                    if (mode0 == QImode && mode1 == V16QImode)
                      ftype = v16qi_ftype_qi_v16qi_si;
                    break;

                  case V8HImode:
                    if (mode0 == HImode && mode1 == V8HImode)
                      ftype = v8hi_ftype_hi_v8hi_si;
                    break;

                  case V4SImode:
                    if (mode0 == SImode && mode1 == V4SImode)
                      ftype = v4si_ftype_si_v4si_si;
                    break;

                  case V4SFmode:
                    if (mode0 == SFmode && mode1 == V4SFmode)
                      ftype = v4sf_ftype_sf_v4sf_si;
                    break;

                  case V2DImode:
                    if (mode0 == DImode && mode1 == V2DImode)
                      ftype = v2di_ftype_di_v2di_si;
                    break;

                  default:
                    gcc_unreachable ();
                  }
              }
              break;

	    case NEON_CREATE:
              gcc_assert (mode0 == DImode);
              /* Create vector from bit pattern.  */
              switch (tmode)
                {
                case V8QImode: ftype = v8qi_ftype_di; break;
                case V4HImode: ftype = v4hi_ftype_di; break;
                case V2SImode: ftype = v2si_ftype_di; break;
                case V2SFmode: ftype = v2sf_ftype_di; break;
                case DImode: ftype = di_ftype_di; break;
                default: gcc_unreachable ();
                }
              break;

	    case NEON_DUP:
              gcc_assert ((mode0 == DImode && tmode == DImode)
                          || mode0 == GET_MODE_INNER (tmode));
              switch (tmode)
                {
                case V8QImode:  ftype = v8qi_ftype_qi; break;
                case V4HImode:  ftype = v4hi_ftype_hi; break;
                case V2SImode:  ftype = v2si_ftype_si; break;
                case V2SFmode:  ftype = v2sf_ftype_sf; break;
                case DImode:    ftype = di_ftype_di; break;
                case V16QImode: ftype = v16qi_ftype_qi; break;
                case V8HImode:  ftype = v8hi_ftype_hi; break;
                case V4SImode:  ftype = v4si_ftype_si; break;
                case V4SFmode:  ftype = v4sf_ftype_sf; break;
                case V2DImode:  ftype = v2di_ftype_di; break;
                default: gcc_unreachable ();
                }
              break;

	    case NEON_DUPLANE:
              gcc_assert (valid_neon_mode (mode0) && mode1 == SImode);
              switch (tmode)
                {
                case V8QImode:  ftype = v8qi_ftype_v8qi_si; break;
                case V4HImode:  ftype = v4hi_ftype_v4hi_si; break;
                case V2SImode:  ftype = v2si_ftype_v2si_si; break;
                case V2SFmode:  ftype = v2sf_ftype_v2sf_si; break;
                case DImode:    ftype = di_ftype_di_si; break;
                case V16QImode: ftype = v16qi_ftype_v8qi_si; break;
                case V8HImode:  ftype = v8hi_ftype_v4hi_si; break;
                case V4SImode:  ftype = v4si_ftype_v2si_si; break;
                case V4SFmode:  ftype = v4sf_ftype_v2sf_si; break;
                case V2DImode:  ftype = v2di_ftype_di_si; break;
                default: gcc_unreachable ();
                }
              break;

	    case NEON_SHIFTIMM:
              gcc_assert (mode1 == SImode && mode2 == SImode);
              switch (tmode)
                {
                case V8QImode:
                  if (mode0 == V8QImode)
                    ftype = v8qi_ftype_v8qi_si_si;
                  else if (mode0 == V8HImode)
                    ftype = v8qi_ftype_v8hi_si_si;
                  break;

                case V4HImode:
                  if (mode0 == V4HImode)
                    ftype = v4hi_ftype_v4hi_si_si;
                  else if (mode0 == V4SImode)
                    ftype = v4hi_ftype_v4si_si_si;
                  break;
                  
                case V2SImode:
                  if (mode0 == V2SImode)
		    ftype = v2si_ftype_v2si_si_si;
                  else if (mode0 == V2DImode)
                    ftype = v2si_ftype_v2di_si_si;
                  break;

                case DImode:
                  if (mode0 == DImode)
		    ftype = di_ftype_di_si_si;
                  break;

                case V16QImode:
                  if (mode0 == V16QImode)
                    ftype = v16qi_ftype_v16qi_si_si;
                  break;

                case V8HImode:
                  if (mode0 == V8HImode)
                    ftype = v8hi_ftype_v8hi_si_si;
                  else if (mode0 == V8QImode)
                    ftype = v8hi_ftype_v8qi_si_si;
                  break;

                case V4SImode:
                  if (mode0 == V4SImode)
                    ftype = v4si_ftype_v4si_si_si;
                  else if (mode0 == V4HImode)
                    ftype = v4si_ftype_v4hi_si_si;
                  break;

                case V2DImode:
                  if (mode0 == V2DImode)
                    ftype = v2di_ftype_v2di_si_si;
                  else if (mode0 == V2SImode)
                    ftype = v2di_ftype_v2si_si_si;
                  break;

                default: gcc_unreachable ();
                }
              break;

	    case NEON_SHIFTACC:
              gcc_assert (tmode == mode0 && mode0 == mode1 && mode2 == SImode
			  && mode3 == SImode);
	      switch (tmode)
                {
                case V8QImode:  ftype = v8qi_ftype_v8qi_v8qi_si_si; break;
                case V4HImode:  ftype = v4hi_ftype_v4hi_v4hi_si_si; break;
                case V2SImode:  ftype = v2si_ftype_v2si_v2si_si_si; break;
                case V2SFmode:  ftype = v2sf_ftype_v2sf_v2sf_si_si; break;
                case DImode:    ftype = di_ftype_di_di_si_si; break;
                case V16QImode: ftype = v16qi_ftype_v16qi_v16qi_si_si; break;
                case V8HImode:  ftype = v8hi_ftype_v8hi_v8hi_si_si; break;
                case V4SImode:  ftype = v4si_ftype_v4si_v4si_si_si; break;
                case V4SFmode:  ftype = v4sf_ftype_v4sf_v4sf_si_si; break;
                case V2DImode:  ftype = v2di_ftype_v2di_v2di_si_si; break;
                default: gcc_unreachable ();
                }
              break;

	    case NEON_COMBINE:
              gcc_assert (valid_neon_mode (mode0) && valid_neon_mode (mode1));
              switch (tmode)
                {
                case V16QImode:
                  if (mode0 == V8QImode && mode1 == V8QImode)
                    ftype = v16qi_ftype_v8qi_v8qi;
                  break;
                
                case V8HImode:
                  if (mode0 == V4HImode && mode1 == V4HImode)
                    ftype = v8hi_ftype_v4hi_v4hi;
                  break;
                  
                case V4SImode:
                  if (mode0 == V2SImode && mode1 == V2SImode)
                    ftype = v4si_ftype_v2si_v2si;
                  break;
                  
                case V4SFmode:
                  if (mode0 == V2SFmode && mode1 == V2SFmode)
                    ftype = v4sf_ftype_v2sf_v2sf;
                  break;
                  
                case V2DImode:
                  if (mode0 == DImode && mode1 == DImode)
                    ftype = v2di_ftype_di_di;
                  break;

                default:
                  gcc_unreachable ();
                }
              break;

	    case NEON_SPLIT:
              gcc_assert (valid_neon_mode (mode0));
              switch (tmode)
                {
                case V8QImode:
                  if (mode0 == V16QImode)
                    ftype = v8qi_ftype_v16qi;
                  break;

                case V4HImode:
                  if (mode0 == V8HImode)
                    ftype = v4hi_ftype_v8hi;
                  break;

                case V2SImode:
                  if (mode0 == V4SImode)
                    ftype = v2si_ftype_v4si;
                  break;

                case V2SFmode:
                  if (mode0 == V4SFmode)
                    ftype = v2sf_ftype_v4sf;
                  break;

                case DImode:
                  if (mode0 == V2DImode)
                    ftype = di_ftype_v2di;
                  break;

                default:
                  gcc_unreachable ();
		}
              break;

	    case NEON_CONVERT:
              gcc_assert (valid_neon_mode (mode0) && mode1 == SImode);
              switch (tmode)
                {
                case V2SImode:
                  if (mode0 == V2SFmode)
                    ftype = v2si_ftype_v2sf_si;
                  break;

                case V2SFmode:
                  if (mode0 == V2SImode)
                    ftype = v2sf_ftype_v2si_si;
                  break;

                case V4SImode:
                  if (mode0 == V4SFmode)
                    ftype = v4si_ftype_v4sf_si;
                  break;

                case V4SFmode:
                  if (mode0 == V4SImode)
                    ftype = v4sf_ftype_v4si_si;
                  break;

                default: gcc_unreachable ();
                }
              break;

	    case NEON_FIXCONV:
              gcc_assert (valid_neon_mode (mode0) && mode1 == SImode
			  && mode2 == SImode);
              switch (tmode)
		{
                case V2SImode:
                  if (mode0 == V2SFmode)
                    ftype = v2si_ftype_v2sf_si_si;
                  break;

                case V2SFmode:
                  if (mode0 == V2SImode)
                    ftype = v2sf_ftype_v2si_si_si;
                  break;

                case V4SImode:
                  if (mode0 == V4SFmode)
                    ftype = v4si_ftype_v4sf_si_si;
                  break;

                case V4SFmode:
                  if (mode0 == V4SImode)
                    ftype = v4sf_ftype_v4si_si_si;
                  break;
                
                default:
                  gcc_unreachable ();
                }
              break;

	    case NEON_LANEMUL:
              {
                enum machine_mode mode3 = insn_data[icode].operand[4].mode;
                gcc_assert (valid_neon_mode (mode0) && valid_neon_mode (mode1)
			    && mode2 == SImode && mode3 == SImode);
                switch (tmode)
                  {
                  case V4HImode:
                    if (mode0 == V4HImode && mode1 == V4HImode)
                      ftype = v4hi_ftype_v4hi_v4hi_si_si;
                    break;
                  
                  case V2SImode:
                    if (mode0 == V2SImode && mode1 == V2SImode)
                      ftype = v2si_ftype_v2si_v2si_si_si;
                    break;
                  
                  case V2SFmode:
                    if (mode0 == V2SFmode && mode1 == V2SFmode)
                      ftype = v2sf_ftype_v2sf_v2sf_si_si;
                    break;
                  
                  case V8HImode:
                    if (mode0 == V8HImode && mode1 == V4HImode)
                      ftype = v8hi_ftype_v8hi_v4hi_si_si;
                    break;
                  
                  case V4SImode:
                    if (mode0 == V4SImode && mode1 == V2SImode)
                      ftype = v4si_ftype_v4si_v2si_si_si;
                    break;
                  
                  case V4SFmode:
                    if (mode0 == V4SFmode && mode1 == V2SFmode)
                      ftype = v4sf_ftype_v4sf_v2sf_si_si;
                    break;
                  
                  default:
                    gcc_unreachable ();
                  }
              }
              break;

	    case NEON_LANEMULL:
              {
                enum machine_mode mode3 = insn_data[icode].operand[4].mode;
                gcc_assert (valid_neon_mode (mode0) && valid_neon_mode (mode1)
			    && mode2 == SImode && mode3 == SImode);
                switch (tmode)
                  {
                  case V4SImode:
                    if (mode0 == V4HImode && mode1 == V4HImode)
                      ftype = v4si_ftype_v4hi_v4hi_si_si;
                    break;
                  
                  case V2DImode:
                    if (mode0 == V2SImode && mode1 == V2SImode)
                      ftype = v2di_ftype_v2si_v2si_si_si;
                    break;
                  
                  default:
                    gcc_unreachable ();
                  }
              }
              break;

	    case NEON_LANEMULH:
              {
                enum machine_mode mode3 = insn_data[icode].operand[4].mode;
                gcc_assert (valid_neon_mode (mode0) && valid_neon_mode (mode1)
			    && mode2 == SImode && mode3 == SImode);
                switch (tmode)
                  {
                  case V4SImode:
                    if (mode0 == V4SImode && mode1 == V2SImode)
                      ftype = v4si_ftype_v4si_v2si_si_si;
                    break;
                  
                  case V8HImode:
                    if (mode0 == V8HImode && mode1 == V4HImode)
                      ftype = v8hi_ftype_v8hi_v4hi_si_si;
                    break;

                  case V2SImode:
                    if (mode0 == V2SImode && mode1 == V2SImode)
                      ftype = v2si_ftype_v2si_v2si_si_si;
                    break;
                  
                  case V4HImode:
                    if (mode0 == V4HImode && mode1 == V4HImode)
                      ftype = v4hi_ftype_v4hi_v4hi_si_si;
                    break;
                  
                  default:
                    gcc_unreachable ();
                  }
              }
              break;

	    case NEON_LANEMAC:
              {
                enum machine_mode mode3 = insn_data[icode].operand[4].mode;
                enum machine_mode mode4 = insn_data[icode].operand[5].mode;
                gcc_assert (valid_neon_mode (mode0) && valid_neon_mode (mode1)
			    && valid_neon_mode (mode2) && mode3 == SImode
                            && mode4 == SImode);
                switch (tmode)
                  {
                  case V4HImode:
                    if (mode0 == V4HImode && mode1 == V4HImode
			&& mode2 == V4HImode)
                      ftype = v4hi_ftype_v4hi_v4hi_v4hi_si_si;
                    break;
                  
                  case V2SImode:
                    if (mode0 == V2SImode && mode1 == V2SImode
			&& mode2 == V2SImode)
                      ftype = v2si_ftype_v2si_v2si_v2si_si_si;
                    break;
                  
                  case V2SFmode:
                    if (mode0 == V2SFmode && mode1 == V2SFmode
			&& mode2 == V2SFmode)
                      ftype = v2sf_ftype_v2sf_v2sf_v2sf_si_si;
                    break;
                  
                  case V8HImode:
                    if (mode0 == V8HImode && mode1 == V8HImode
			&& mode2 == V4HImode)
                      ftype = v8hi_ftype_v8hi_v8hi_v4hi_si_si;
                    break;
                  
                  case V4SImode:
                    if (mode0 == V4SImode && mode1 == V4SImode
			&& mode2 == V2SImode)
                      ftype = v4si_ftype_v4si_v4si_v2si_si_si;
                    else if (mode0 == V4SImode && mode1 == V4HImode
			&& mode2 == V4HImode)
                      ftype = v4si_ftype_v4si_v4hi_v4hi_si_si;
                    break;
                  
                  case V4SFmode:
                    if (mode0 == V4SFmode && mode1 == V4SFmode
			&& mode2 == V2SFmode)
                      ftype = v4sf_ftype_v4sf_v4sf_v2sf_si_si;
                    break;
                  
                  case V2DImode:
                    if (mode0 == V2DImode && mode1 == V2SImode
			&& mode2 == V2SImode)
                      ftype = v2di_ftype_v2di_v2si_v2si_si_si;
                    break;
                  
                  default:
                    gcc_unreachable ();
                  }
              }
              break;

	    case NEON_SCALARMUL:
              switch (tmode)
                {
                case V4HImode:
                  if (mode0 == V4HImode && mode1 == HImode)
                    ftype = v4hi_ftype_v4hi_hi_si;
                  break;

                case V2SImode:
                  if (mode0 == V2SImode && mode1 == SImode)
                    ftype = v2si_ftype_v2si_si_si;
                  break;

                case V2SFmode:
                  if (mode0 == V2SFmode && mode1 == SFmode)
                    ftype = v2sf_ftype_v2sf_sf_si;
                  break;

                case V8HImode:
                  if (mode0 == V8HImode && mode1 == HImode)
                    ftype = v8hi_ftype_v8hi_hi_si;
                  break;

                case V4SImode:
                  if (mode0 == V4SImode && mode1 == SImode)
                    ftype = v4si_ftype_v4si_si_si;
                  break;

                case V4SFmode:
                  if (mode0 == V4SFmode && mode1 == SFmode)
                    ftype = v4sf_ftype_v4sf_sf_si;
                  break;

                default:
                  gcc_unreachable ();
                }
              break;

	    case NEON_SCALARMULL:
              switch (tmode)
                {
                case V4SImode:
                  if (mode0 == V4HImode && mode1 == HImode)
                    ftype = v4si_ftype_v4hi_hi_si;
                  break;

                case V2DImode:
                  if (mode0 == V2SImode && mode1 == SImode)
                    ftype = v2di_ftype_v2si_si_si;
                  break;

                default:
                  gcc_unreachable ();
                }
              break;

	    case NEON_SCALARMULH:
              {
                switch (tmode)
                  {
                  case V4SImode:
                    if (mode0 == V4SImode && mode1 == SImode)
                      ftype = v4si_ftype_v4si_si_si;
                    break;
                  
                  case V8HImode:
                    if (mode0 == V8HImode && mode1 == HImode)
                      ftype = v8hi_ftype_v8hi_hi_si;
                    break;

                  case V2SImode:
                    if (mode0 == V2SImode && mode1 == SImode)
                      ftype = v2si_ftype_v2si_si_si;
                    break;
                  
                  case V4HImode:
                    if (mode0 == V4HImode && mode1 == HImode)
                      ftype = v4hi_ftype_v4hi_hi_si;
                    break;
                  
                  default:
                    gcc_unreachable ();
                  }
              }
              break;

	    case NEON_SCALARMAC:
              {
                gcc_assert (mode2 == GET_MODE_INNER (mode1));
                switch (tmode)
                  {
                  case V4HImode:
                    if (mode0 == V4HImode && mode1 == V4HImode)
                      ftype = v4hi_ftype_v4hi_v4hi_hi_si;
                    break;

                  case V2SImode:
                    if (mode0 == V2SImode && mode1 == V2SImode)
                      ftype = v2si_ftype_v2si_v2si_si_si;
                    break;

                  case V2SFmode:
                    if (mode0 == V2SFmode && mode1 == V2SFmode)
                      ftype = v2sf_ftype_v2sf_v2sf_sf_si;
                    break;

                  case V8HImode:
                    if (mode0 == V8HImode && mode1 == V8HImode)
                      ftype = v8hi_ftype_v8hi_v8hi_hi_si;
                    break;

                  case V4SImode:
                    if (mode0 == V4SImode && mode1 == V4SImode)
                      ftype = v4si_ftype_v4si_v4si_si_si;
                    else if (mode0 == V4SImode && mode1 == V4HImode)
                      ftype = v4si_ftype_v4si_v4hi_hi_si;
                    break;

                  case V4SFmode:
                    if (mode0 == V4SFmode && mode1 == V4SFmode)
                      ftype = v4sf_ftype_v4sf_v4sf_sf_si;
                    break;

		  case V2DImode:
                    if (mode0 == V2DImode && mode1 == V2SImode)
                      ftype = v2di_ftype_v2di_v2si_si_si;
                    break;

                  default:
                    gcc_unreachable ();
                  }
              }
              break;

	    case NEON_SELECT:
              gcc_assert (mode1 == mode2
                          && (mode0 == mode1
                              || (mode0 == V2SImode && mode1 == V2SFmode)
                              || (mode0 == V4SImode && mode1 == V4SFmode)));
              switch (tmode)
                {
                case V8QImode: ftype = v8qi_ftype_v8qi_v8qi_v8qi; break;
                case V4HImode: ftype = v4hi_ftype_v4hi_v4hi_v4hi; break;
                case V2SImode: ftype = v2si_ftype_v2si_v2si_v2si; break;
                case V2SFmode: ftype = v2sf_ftype_v2si_v2sf_v2sf; break;
                case DImode: ftype = di_ftype_di_di_di; break;
                case V16QImode: ftype = v16qi_ftype_v16qi_v16qi_v16qi; break;
                case V8HImode: ftype = v8hi_ftype_v8hi_v8hi_v8hi; break;
                case V4SImode: ftype = v4si_ftype_v4si_v4si_v4si; break;
                case V4SFmode: ftype = v4sf_ftype_v4si_v4sf_v4sf; break;
                case V2DImode: ftype = v2di_ftype_v2di_v2di_v2di; break;
                default: gcc_unreachable ();
                }
              break;

	    case NEON_VTBL:
              {
                gcc_assert (tmode == V8QImode && mode1 == V8QImode);
                switch (mode0)
                  {
                  case V8QImode: ftype = v8qi_ftype_v8qi_v8qi; break;
                  case TImode: ftype = v8qi_ftype_ti_v8qi; break;
                  case EImode: ftype = v8qi_ftype_ei_v8qi; break;
                  case OImode: ftype = v8qi_ftype_oi_v8qi; break;
                  default: gcc_unreachable ();
                  }
              }
              break;

	    case NEON_VTBX:
              {
                gcc_assert (tmode == V8QImode && mode0 == V8QImode
			    && mode2 == V8QImode);
                switch (mode1)
                  {
                  case V8QImode: ftype = v8qi_ftype_v8qi_v8qi_v8qi; break;
                  case TImode: ftype = v8qi_ftype_v8qi_ti_v8qi; break;
                  case EImode: ftype = v8qi_ftype_v8qi_ei_v8qi; break;
                  case OImode: ftype = v8qi_ftype_v8qi_oi_v8qi; break;
                  default: gcc_unreachable ();
                  }
              }
              break;

	    case NEON_RESULTPAIR:
              {
                switch (mode0)
                  {
		  case V8QImode: ftype = void_ftype_pv8qi_v8qi_v8qi; break;
                  case V4HImode: ftype = void_ftype_pv4hi_v4hi_v4hi; break;
                  case V2SImode: ftype = void_ftype_pv2si_v2si_v2si; break;
                  case V2SFmode: ftype = void_ftype_pv2sf_v2sf_v2sf; break;
                  case DImode: ftype = void_ftype_pdi_di_di; break;
                  case V16QImode: ftype = void_ftype_pv16qi_v16qi_v16qi; break;
                  case V8HImode: ftype = void_ftype_pv8hi_v8hi_v8hi; break;
                  case V4SImode: ftype = void_ftype_pv4si_v4si_v4si; break;
                  case V4SFmode: ftype = void_ftype_pv4sf_v4sf_v4sf; break;
                  case V2DImode: ftype = void_ftype_pv2di_v2di_v2di; break;
                  default: gcc_unreachable ();
                  }
              }
              break;

	    case NEON_REINTERP:
              {
                /* We iterate over 5 doubleword types, then 5 quadword
                   types.  */
                int rhs = j % 5;
                switch (tmode)
                  {
                  case V8QImode: ftype = reinterp_ftype_dreg[0][rhs]; break;
                  case V4HImode: ftype = reinterp_ftype_dreg[1][rhs]; break;
                  case V2SImode: ftype = reinterp_ftype_dreg[2][rhs]; break;
                  case V2SFmode: ftype = reinterp_ftype_dreg[3][rhs]; break;
                  case DImode: ftype = reinterp_ftype_dreg[4][rhs]; break;
                  case V16QImode: ftype = reinterp_ftype_qreg[0][rhs]; break;
                  case V8HImode: ftype = reinterp_ftype_qreg[1][rhs]; break;
                  case V4SImode: ftype = reinterp_ftype_qreg[2][rhs]; break;
		  case V4SFmode: ftype = reinterp_ftype_qreg[3][rhs]; break;
                  case V2DImode: ftype = reinterp_ftype_qreg[4][rhs]; break;
                  default: gcc_unreachable ();
                  }
              }
              break;

	    case NEON_LOAD1:
              switch (tmode)
                {
                case V8QImode: ftype = v8qi_ftype_const_qi_pointer; break;
                case V4HImode: ftype = v4hi_ftype_const_hi_pointer; break;
                case V2SImode: ftype = v2si_ftype_const_si_pointer; break;
                case V2SFmode: ftype = v2sf_ftype_const_sf_pointer; break;
                case DImode: ftype = di_ftype_const_di_pointer; break;
                case V16QImode: ftype = v16qi_ftype_const_qi_pointer; break;
                case V8HImode: ftype = v8hi_ftype_const_hi_pointer; break;
                case V4SImode: ftype = v4si_ftype_const_si_pointer; break;
                case V4SFmode: ftype = v4sf_ftype_const_sf_pointer; break;
                case V2DImode: ftype = v2di_ftype_const_di_pointer; break;
                default: gcc_unreachable ();
                }
              break;

	    case NEON_LOAD1LANE:
              switch (tmode)
                {
                case V8QImode:
		  ftype = v8qi_ftype_const_qi_pointer_v8qi_si;
		  break;
                case V4HImode:
		  ftype = v4hi_ftype_const_hi_pointer_v4hi_si;
		  break;
                case V2SImode:
		  ftype = v2si_ftype_const_si_pointer_v2si_si;
		  break;
                case V2SFmode:
		  ftype = v2sf_ftype_const_sf_pointer_v2sf_si;
		  break;
                case DImode:
		  ftype = di_ftype_const_di_pointer_di_si;
		  break;
                case V16QImode:
		  ftype = v16qi_ftype_const_qi_pointer_v16qi_si;
		  break;
                case V8HImode:
		  ftype = v8hi_ftype_const_hi_pointer_v8hi_si;
		  break;
                case V4SImode:
		  ftype = v4si_ftype_const_si_pointer_v4si_si;
		  break;
                case V4SFmode:
		  ftype = v4sf_ftype_const_sf_pointer_v4sf_si;
		  break;
                case V2DImode:
		  ftype = v2di_ftype_const_di_pointer_v2di_si;
		  break;
                default:
		  gcc_unreachable ();
                }
              break;

	    case NEON_STORE1:
              switch (mode0)
                {
                case V8QImode: ftype = void_ftype_qi_pointer_v8qi; break;
                case V4HImode: ftype = void_ftype_hi_pointer_v4hi; break;
                case V2SImode: ftype = void_ftype_si_pointer_v2si; break;
                case V2SFmode: ftype = void_ftype_sf_pointer_v2sf; break;
                case DImode: ftype = void_ftype_di_pointer_di; break;
                case V16QImode: ftype = void_ftype_qi_pointer_v16qi; break;
                case V8HImode: ftype = void_ftype_hi_pointer_v8hi; break;
                case V4SImode: ftype = void_ftype_si_pointer_v4si; break;
                case V4SFmode: ftype = void_ftype_sf_pointer_v4sf; break;
                case V2DImode: ftype = void_ftype_di_pointer_v2di; break;
                default: gcc_unreachable ();
                }
              break;

	    case NEON_STORE1LANE:
              switch (mode0)
                {
                case V8QImode: ftype = void_ftype_qi_pointer_v8qi_si; break;
                case V4HImode: ftype = void_ftype_hi_pointer_v4hi_si; break;
                case V2SImode: ftype = void_ftype_si_pointer_v2si_si; break;
                case V2SFmode: ftype = void_ftype_sf_pointer_v2sf_si; break;
                case DImode: ftype = void_ftype_di_pointer_di_si; break;
                case V16QImode: ftype = void_ftype_qi_pointer_v16qi_si; break;
                case V8HImode: ftype = void_ftype_hi_pointer_v8hi_si; break;
                case V4SImode: ftype = void_ftype_si_pointer_v4si_si; break;
                case V4SFmode: ftype = void_ftype_sf_pointer_v4sf_si; break;
                case V2DImode: ftype = void_ftype_di_pointer_v2di_si; break;
                default: gcc_unreachable ();
                }
              break;

	    case NEON_LOADSTRUCT:
	      switch (tmode)
		{
		case TImode:
		  /* vld2 cases.  */
		  switch (1 << j)
		    {
		    case T_V8QI: ftype = ti_ftype_const_qi_pointer; break;
		    case T_V4HI: ftype = ti_ftype_const_hi_pointer; break;
		    case T_V2SI: ftype = ti_ftype_const_si_pointer; break;
		    case T_V2SF: ftype = ti_ftype_const_sf_pointer; break;
		    case T_DI: ftype = ti_ftype_const_di_pointer; break;
		    default: gcc_unreachable ();
		    }
		  break;

		case EImode:
		  /* vld3 cases.  */
		  switch (1 << j)
		    {
		    case T_V8QI: ftype = ei_ftype_const_qi_pointer; break;
		    case T_V4HI: ftype = ei_ftype_const_hi_pointer; break;
		    case T_V2SI: ftype = ei_ftype_const_si_pointer; break;
		    case T_V2SF: ftype = ei_ftype_const_sf_pointer; break;
		    case T_DI: ftype = ei_ftype_const_di_pointer; break;
		    default: gcc_unreachable ();
		    }
		  break;

		case OImode:
		  /* vld2q and vld4 cases.  */
		  switch (1 << j)
		    {
		      /* vld2q cases.  */
		    case T_V16QI: ftype = oi_ftype_const_qi_pointer; break;
		    case T_V8HI: ftype = oi_ftype_const_hi_pointer; break;
		    case T_V4SI: ftype = oi_ftype_const_si_pointer; break;
		    case T_V4SF: ftype = oi_ftype_const_sf_pointer; break;
		      /* vld4 cases.  */
		    case T_V8QI: ftype = oi_ftype_const_qi_pointer; break;
		    case T_V4HI: ftype = oi_ftype_const_hi_pointer; break;
		    case T_V2SI: ftype = oi_ftype_const_si_pointer; break;
		    case T_V2SF: ftype = oi_ftype_const_sf_pointer; break;
		    case T_DI: ftype = oi_ftype_const_di_pointer; break;
		    default: gcc_unreachable ();
		    }
		  break;

		case CImode:
		  /* vld3q cases.  */
		  switch (1 << j)
		    {
		    case T_V16QI: ftype = ci_ftype_const_qi_pointer; break;
		    case T_V8HI: ftype = ci_ftype_const_hi_pointer; break;
		    case T_V4SI: ftype = ci_ftype_const_si_pointer; break;
		    case T_V4SF: ftype = ci_ftype_const_sf_pointer; break;
		    default: gcc_unreachable ();
		    }
		  break;

		case XImode:
		  /* vld4q cases.  */
		  switch (1 << j)
		    {
		    case T_V16QI: ftype = xi_ftype_const_qi_pointer; break;
		    case T_V8HI: ftype = xi_ftype_const_hi_pointer; break;
		    case T_V4SI: ftype = xi_ftype_const_si_pointer; break;
		    case T_V4SF: ftype = xi_ftype_const_sf_pointer; break;
		    default: gcc_unreachable ();
		    }
		  break;

		default:
		  gcc_unreachable ();
		}
              break;

	    case NEON_LOADSTRUCTLANE:
	      switch (tmode)
		{
		case TImode:
		  /* vld2_lane cases.  */
		  switch (1 << j)
		    {
		    case T_V8QI:
		      ftype = ti_ftype_const_qi_pointer_ti_si;
		      break;
		    case T_V4HI:
		      ftype = ti_ftype_const_hi_pointer_ti_si;
		      break;
		    case T_V2SI:
		      ftype = ti_ftype_const_si_pointer_ti_si;
		      break;
		    case T_V2SF:
		      ftype = ti_ftype_const_sf_pointer_ti_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		case EImode:
		  /* vld3_lane cases.  */
		  switch (1 << j)
		    {
		    case T_V8QI:
		      ftype = ei_ftype_const_qi_pointer_ei_si;
		      break;
		    case T_V4HI:
		      ftype = ei_ftype_const_hi_pointer_ei_si;
		      break;
		    case T_V2SI:
		      ftype = ei_ftype_const_si_pointer_ei_si;
		      break;
		    case T_V2SF:
		      ftype = ei_ftype_const_sf_pointer_ei_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		case OImode:
		  /* vld2q_lane and vld4_lane cases.  */
		  switch (1 << j)
		    {
		      /* vld2q_lane cases.  */
		    case T_V8HI:
		      ftype = oi_ftype_const_hi_pointer_oi_si;
		      break;
		    case T_V4SI:
		      ftype = oi_ftype_const_si_pointer_oi_si;
		      break;
		    case T_V4SF:
		      ftype = oi_ftype_const_sf_pointer_oi_si;
		      break;
		      /* vld4_lane cases.  */
		    case T_V8QI:
		      ftype = oi_ftype_const_qi_pointer_oi_si;
		      break;
		    case T_V4HI:
		      ftype = oi_ftype_const_hi_pointer_oi_si;
		      break;
		    case T_V2SI:
		      ftype = oi_ftype_const_si_pointer_oi_si;
		      break;
		    case T_V2SF:
		      ftype = oi_ftype_const_sf_pointer_oi_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		case CImode:
		  /* vld3q_lane cases.  */
		  switch (1 << j)
		    {
		    case T_V8HI:
		      ftype = ci_ftype_const_hi_pointer_ci_si;
		      break;
		    case T_V4SI:
		      ftype = ci_ftype_const_si_pointer_ci_si;
		      break;
		    case T_V4SF:
		      ftype = ci_ftype_const_sf_pointer_ci_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		case XImode:
		  /* vld4q_lane cases.  */
		  switch (1 << j)
		    {
		    case T_V8HI:
		      ftype = xi_ftype_const_hi_pointer_xi_si;
		      break;
		    case T_V4SI:
		      ftype = xi_ftype_const_si_pointer_xi_si;
		      break;
		    case T_V4SF:
		      ftype = xi_ftype_const_sf_pointer_xi_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		default:
		  gcc_unreachable ();
		}
              break;


	    case NEON_STORESTRUCT:
	      switch (mode0)
		{
		case TImode:
		  /* vst2 cases.  */
		  switch (1 << j)
		    {
		    case T_V8QI: ftype = void_ftype_qi_pointer_ti; break;
		    case T_V4HI: ftype = void_ftype_hi_pointer_ti; break;
		    case T_V2SI: ftype = void_ftype_si_pointer_ti; break;
		    case T_V2SF: ftype = void_ftype_sf_pointer_ti; break;
		    case T_DI: ftype = void_ftype_di_pointer_ti; break;
		    default: gcc_unreachable ();
		    }
		  break;

		case EImode:
		  /* vst3 cases.  */
		  switch (1 << j)
		    {
		    case T_V8QI: ftype = void_ftype_qi_pointer_ei; break;
		    case T_V4HI: ftype = void_ftype_hi_pointer_ei; break;
		    case T_V2SI: ftype = void_ftype_si_pointer_ei; break;
		    case T_V2SF: ftype = void_ftype_sf_pointer_ei; break;
		    case T_DI: ftype = void_ftype_di_pointer_ei; break;
		    default: gcc_unreachable ();
		    }
		  break;

		case OImode:
		  /* vst2q and vst4 cases.  */
		  switch (1 << j)
		    {
		      /* vst2q cases.  */
		    case T_V16QI: ftype = void_ftype_qi_pointer_oi; break;
		    case T_V8HI: ftype = void_ftype_hi_pointer_oi; break;
		    case T_V4SI: ftype = void_ftype_si_pointer_oi; break;
		    case T_V4SF: ftype = void_ftype_sf_pointer_oi; break;
		      /* vst4 cases.  */
		    case T_V8QI: ftype = void_ftype_qi_pointer_oi; break;
		    case T_V4HI: ftype = void_ftype_hi_pointer_oi; break;
		    case T_V2SI: ftype = void_ftype_si_pointer_oi; break;
		    case T_V2SF: ftype = void_ftype_sf_pointer_oi; break;
		    case T_DI: ftype = void_ftype_di_pointer_oi; break;
		    default: gcc_unreachable ();
		    }
		  break;

		case CImode:
		  /* vst3q cases.  */
		  switch (1 << j)
		    {
		    case T_V16QI: ftype = void_ftype_qi_pointer_ci; break;
		    case T_V8HI: ftype = void_ftype_hi_pointer_ci; break;
		    case T_V4SI: ftype = void_ftype_si_pointer_ci; break;
		    case T_V4SF: ftype = void_ftype_sf_pointer_ci; break;
		    default: gcc_unreachable ();
		    }
		  break;

		case XImode:
		  /* vst4q cases.  */
		  switch (1 << j)
		    {
		    case T_V16QI: ftype = void_ftype_qi_pointer_xi; break;
		    case T_V8HI: ftype = void_ftype_hi_pointer_xi; break;
		    case T_V4SI: ftype = void_ftype_si_pointer_xi; break;
		    case T_V4SF: ftype = void_ftype_sf_pointer_xi; break;
		    default: gcc_unreachable ();
		    }
		  break;

		default:
		  gcc_unreachable ();
		}
              break;

	    case NEON_STORESTRUCTLANE:
	      switch (mode0)
		{
		case TImode:
		  /* vst2_lane cases.  */
		  switch (1 << j)
		    {
		    case T_V8QI:
		      ftype = void_ftype_qi_pointer_ti_si;
		      break;
		    case T_V4HI:
		      ftype = void_ftype_hi_pointer_ti_si;
		      break;
		    case T_V2SI:
		      ftype = void_ftype_si_pointer_ti_si;
		      break;
		    case T_V2SF:
		      ftype = void_ftype_sf_pointer_ti_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		case EImode:
		  /* vst3_lane cases.  */
		  switch (1 << j)
		    {
		    case T_V8QI:
		      ftype = void_ftype_qi_pointer_ei_si;
		      break;
		    case T_V4HI:
		      ftype = void_ftype_hi_pointer_ei_si;
		      break;
		    case T_V2SI:
		      ftype = void_ftype_si_pointer_ei_si;
		      break;
		    case T_V2SF:
		      ftype = void_ftype_sf_pointer_ei_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		case OImode:
		  /* vst2q_lane and vst4_lane cases.  */
		  switch (1 << j)
		    {
		      /* vst2q_lane cases.  */
		    case T_V8HI:
		      ftype = void_ftype_hi_pointer_oi_si;
		      break;
		    case T_V4SI:
		      ftype = void_ftype_si_pointer_oi_si;
		      break;
		    case T_V4SF:
		      ftype = void_ftype_sf_pointer_oi_si;
		      break;
		      /* vst4_lane cases.  */
		    case T_V8QI:
		      ftype = void_ftype_qi_pointer_oi_si;
		      break;
		    case T_V4HI:
		      ftype = void_ftype_hi_pointer_oi_si;
		      break;
		    case T_V2SI:
		      ftype = void_ftype_si_pointer_oi_si;
		      break;
		    case T_V2SF:
		      ftype = void_ftype_sf_pointer_oi_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		case CImode:
		  /* vst3q_lane cases.  */
		  switch (1 << j)
		    {
		    case T_V8HI:
		      ftype = void_ftype_hi_pointer_ci_si;
		      break;
		    case T_V4SI:
		      ftype = void_ftype_si_pointer_ci_si;
		      break;
		    case T_V4SF:
		      ftype = void_ftype_sf_pointer_ci_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		case XImode:
		  /* vst4q_lane cases.  */
		  switch (1 << j)
		    {
		    case T_V8HI:
		      ftype = void_ftype_hi_pointer_xi_si;
		      break;
		    case T_V4SI:
		      ftype = void_ftype_si_pointer_xi_si;
		      break;
		    case T_V4SF:
		      ftype = void_ftype_sf_pointer_xi_si;
		      break;
		    default:
		      gcc_unreachable ();
		    }
		  break;

		default:
		  gcc_unreachable ();
		}
              break;

            default:
              gcc_unreachable ();
            }
            
          gcc_assert (ftype != NULL);
          
          sprintf (namebuf, "__builtin_neon_%s%s", d->name, modenames[j]);
          
          lang_hooks.builtin_function (namebuf, ftype, fcode++, BUILT_IN_MD,
				       NULL, NULL_TREE);
        }
    }
#undef qi_TN
#undef hi_TN
#undef si_TN
#undef di_TN
#undef ti_TN
#undef ei_TN
#undef oi_TN
#undef ci_TN
#undef xi_TN

#undef sf_TN

#undef v8qi_TN
#undef v4hi_TN
#undef v2si_TN
#undef v2sf_TN

#undef v16qi_TN
#undef v8hi_TN
#undef v4si_TN
#undef v4sf_TN
#undef v2di_TN

#undef pv8qi_TN
#undef pv4hi_TN
#undef pv2si_TN
#undef pv2sf_TN
#undef pdi_TN

#undef pv16qi_TN
#undef pv8hi_TN
#undef pv4si_TN
#undef pv4sf_TN
#undef pv2di_TN

#undef void_TN

#undef TYPE2
#undef TYPE3
#undef TYPE4
#undef TYPE5
#undef TYPE6
}

static void
arm_init_builtins (void)
{
  arm_init_tls_builtins ();

  if (TARGET_REALLY_IWMMXT)
    arm_init_iwmmxt_builtins ();
  
  if (TARGET_NEON)
    arm_init_neon_builtins ();
/* APPLE LOCAL begin ARM darwin builtins */
#ifdef SUBTARGET_INIT_BUILTINS
  SUBTARGET_INIT_BUILTINS;
#endif
/* APPLE LOCAL end ARM darwin builtins */
}
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

/* Errors in the source file can cause expand_expr to return const0_rtx
   where we expect a vector.  To avoid crashing, use one of the vector
   clear instructions.  */

static rtx
safe_vector_operand (rtx x, enum machine_mode mode)
{
  if (x != const0_rtx)
    return x;
  x = gen_reg_rtx (mode);

  emit_insn (gen_iwmmxt_clrdi (mode == DImode ? x
			       : gen_rtx_SUBREG (DImode, x, 0)));
  return x;
}

/* Subroutine of arm_expand_builtin to take care of binop insns.  */

static rtx
arm_expand_binop_builtin (enum insn_code icode,
			  tree arglist, rtx target)
{
  rtx pat;
  tree arg0 = TREE_VALUE (arglist);
  tree arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  rtx op0 = expand_normal (arg0);
  rtx op1 = expand_normal (arg1);
  enum machine_mode tmode = insn_data[icode].operand[0].mode;
  enum machine_mode mode0 = insn_data[icode].operand[1].mode;
  enum machine_mode mode1 = insn_data[icode].operand[2].mode;

  if (VECTOR_MODE_P (mode0))
    op0 = safe_vector_operand (op0, mode0);
  if (VECTOR_MODE_P (mode1))
    op1 = safe_vector_operand (op1, mode1);

  if (! target
      || GET_MODE (target) != tmode
      || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
    target = gen_reg_rtx (tmode);

  gcc_assert (GET_MODE (op0) == mode0 && GET_MODE (op1) == mode1);

  if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
    op0 = copy_to_mode_reg (mode0, op0);
  if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
    op1 = copy_to_mode_reg (mode1, op1);

  pat = GEN_FCN (icode) (target, op0, op1);
  if (! pat)
    return 0;
  emit_insn (pat);
  return target;
}

/* Subroutine of arm_expand_builtin to take care of unop insns.  */

static rtx
arm_expand_unop_builtin (enum insn_code icode,
			 tree arglist, rtx target, int do_load)
{
  rtx pat;
  tree arg0 = TREE_VALUE (arglist);
  rtx op0 = expand_normal (arg0);
  enum machine_mode tmode = insn_data[icode].operand[0].mode;
  enum machine_mode mode0 = insn_data[icode].operand[1].mode;

  if (! target
      || GET_MODE (target) != tmode
      || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
    target = gen_reg_rtx (tmode);
  if (do_load)
    op0 = gen_rtx_MEM (mode0, copy_to_mode_reg (Pmode, op0));
  else
    {
      if (VECTOR_MODE_P (mode0))
	op0 = safe_vector_operand (op0, mode0);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
	op0 = copy_to_mode_reg (mode0, op0);
    }

  pat = GEN_FCN (icode) (target, op0);
  if (! pat)
    return 0;
  emit_insn (pat);
  return target;
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
static int
neon_builtin_compare (const void *a, const void *b)
{
  const neon_builtin_datum *key = a;
  const neon_builtin_datum *memb = b;
  unsigned int soughtcode = key->base_fcode;
  
  if (soughtcode >= memb->base_fcode
      && soughtcode < memb->base_fcode + memb->num_vars)
    return 0;
  else if (soughtcode < memb->base_fcode)
    return -1;
  else
    return 1;
}

static enum insn_code
locate_neon_builtin_icode (int fcode, neon_itype *itype)
{
  neon_builtin_datum key, *found;
  int idx;
  
  key.base_fcode = fcode;
  found = bsearch (&key, &neon_builtin_data[0], ARRAY_SIZE (neon_builtin_data),
		   sizeof (neon_builtin_data[0]), neon_builtin_compare);
  gcc_assert (found);
  idx = fcode - (int) found->base_fcode;
  gcc_assert (idx >= 0 && idx < T_MAX && idx < (int)found->num_vars);

  if (itype)
    *itype = found->itype;

  return found->codes[idx];
}

typedef enum {
  NEON_ARG_COPY_TO_REG,
  NEON_ARG_CONSTANT,
  NEON_ARG_STOP
} builtin_arg;

#define NEON_MAX_BUILTIN_ARGS 5

/* Expand a Neon builtin.  */
static rtx
arm_expand_neon_args (rtx target, int icode, int have_retval,
		      tree arglist, ...)
{
  va_list ap;
  rtx pat;
  tree arg[NEON_MAX_BUILTIN_ARGS];
  rtx op[NEON_MAX_BUILTIN_ARGS];
  enum machine_mode tmode = insn_data[icode].operand[0].mode;
  enum machine_mode mode[NEON_MAX_BUILTIN_ARGS];
  int argc = 0;
  
  if (have_retval
      && (!target
	  || GET_MODE (target) != tmode
	  || !(*insn_data[icode].operand[0].predicate) (target, tmode)))
    target = gen_reg_rtx (tmode);
  
  va_start (ap, arglist);
  
  for (;;)
    {
      builtin_arg thisarg = va_arg (ap, int);
      
      if (thisarg == NEON_ARG_STOP)
        break;
      else
        {
          arg[argc] = TREE_VALUE (arglist);
          op[argc] = expand_expr (arg[argc], NULL_RTX, VOIDmode, 0);
          mode[argc] = insn_data[icode].operand[argc + have_retval].mode;

          arglist = TREE_CHAIN (arglist);

          switch (thisarg)
            {
            case NEON_ARG_COPY_TO_REG:
              /*gcc_assert (GET_MODE (op[argc]) == mode[argc]);*/
              if (!(*insn_data[icode].operand[argc + have_retval].predicate)
                     (op[argc], mode[argc]))
                op[argc] = copy_to_mode_reg (mode[argc], op[argc]);
              break;

            case NEON_ARG_CONSTANT:
              /* FIXME: This error message is somewhat unhelpful.  */
              if (!(*insn_data[icode].operand[argc + have_retval].predicate)
                    (op[argc], mode[argc]))
		error ("argument must be a constant");
              break;

            case NEON_ARG_STOP:
              gcc_unreachable ();
            }
          
          argc++;
        }
    }

  va_end (ap);

  if (have_retval)
    switch (argc)
      {
      case 1:
	pat = GEN_FCN (icode) (target, op[0]);
	break;

      case 2:
	pat = GEN_FCN (icode) (target, op[0], op[1]);
	break;

      case 3:
	pat = GEN_FCN (icode) (target, op[0], op[1], op[2]);
	break;

      case 4:
	pat = GEN_FCN (icode) (target, op[0], op[1], op[2], op[3]);
	break;

      case 5:
	pat = GEN_FCN (icode) (target, op[0], op[1], op[2], op[3], op[4]);
	break;
    
      default:
	gcc_unreachable ();
      }
  else
    switch (argc)
      {
      case 1:
	pat = GEN_FCN (icode) (op[0]);
	break;

      case 2:
	pat = GEN_FCN (icode) (op[0], op[1]);
	break;

      case 3:
	pat = GEN_FCN (icode) (op[0], op[1], op[2]);
	break;

      case 4:
	pat = GEN_FCN (icode) (op[0], op[1], op[2], op[3]);
	break;

      case 5:
	pat = GEN_FCN (icode) (op[0], op[1], op[2], op[3], op[4]);
        break;

      default:
	gcc_unreachable ();
      }

  if (!pat)
    return 0;

  emit_insn (pat);

  return target;
}

/* Expand a Neon builtin. These are "special" because they don't have symbolic
   constants defined per-instruction or per instruction-variant. Instead, the
   required info is looked up in the table neon_builtin_data.  */
static rtx
arm_expand_neon_builtin (rtx target, int fcode, tree arglist)
{
  neon_itype itype;
  enum insn_code icode = locate_neon_builtin_icode (fcode, &itype);
  
  switch (itype)
    {
    case NEON_UNOP:
    case NEON_CONVERT:
    case NEON_DUPLANE:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT, NEON_ARG_STOP);
    
    case NEON_BINOP:
    case NEON_SETLANE:
    case NEON_SCALARMUL:
    case NEON_SCALARMULL:
    case NEON_SCALARMULH:
    case NEON_SHIFTINSERT:
    case NEON_LOGICBINOP:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
        NEON_ARG_STOP);
        
    case NEON_TERNOP:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_CONSTANT, NEON_ARG_STOP);
    
    case NEON_GETLANE:
    case NEON_FIXCONV:
    case NEON_SHIFTIMM:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT, NEON_ARG_CONSTANT,
        NEON_ARG_STOP);
        
    case NEON_CREATE:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_DUP:
    case NEON_SPLIT:
    case NEON_REINTERP:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);
    
    case NEON_COMBINE:
    case NEON_VTBL:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_RESULTPAIR:
      return arm_expand_neon_args (target, icode, 0, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_STOP);
    
    case NEON_LANEMUL:
    case NEON_LANEMULL:
    case NEON_LANEMULH:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
        NEON_ARG_CONSTANT, NEON_ARG_STOP);
    
    case NEON_LANEMAC:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_CONSTANT, NEON_ARG_CONSTANT, NEON_ARG_STOP);

    case NEON_SHIFTACC:
      return arm_expand_neon_args (target, icode, 1, arglist,
        NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
        NEON_ARG_CONSTANT, NEON_ARG_STOP);

    case NEON_SCALARMAC:
      return arm_expand_neon_args (target, icode, 1, arglist,
	NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_CONSTANT, NEON_ARG_STOP);

    case NEON_SELECT:
    case NEON_VTBX:
      return arm_expand_neon_args (target, icode, 1, arglist,
	NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG,
        NEON_ARG_STOP);

    case NEON_LOAD1:
    case NEON_LOADSTRUCT:
      return arm_expand_neon_args (target, icode, 1, arglist,
	NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_LOAD1LANE:
    case NEON_LOADSTRUCTLANE:
      return arm_expand_neon_args (target, icode, 1, arglist,
	NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
	NEON_ARG_STOP);

    case NEON_STORE1:
    case NEON_STORESTRUCT:
      return arm_expand_neon_args (target, icode, 0, arglist,
	NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_STOP);

    case NEON_STORE1LANE:
    case NEON_STORESTRUCTLANE:
      return arm_expand_neon_args (target, icode, 0, arglist,
	NEON_ARG_COPY_TO_REG, NEON_ARG_COPY_TO_REG, NEON_ARG_CONSTANT,
	NEON_ARG_STOP);
    }
  
  gcc_unreachable ();
}

/* Emit code to reinterpret one Neon type as another, without altering bits.  */
void
neon_reinterpret (rtx dest, rtx src)
{
  emit_move_insn (dest, gen_lowpart (GET_MODE (dest), src));
}

/* Emit code to place a Neon pair result in memory locations (with equal
   registers).  */
void
neon_emit_pair_result_insn (enum machine_mode mode,
			    rtx (*intfn) (rtx, rtx, rtx, rtx), rtx destaddr,
                            rtx op1, rtx op2)
{
  rtx mem = gen_rtx_MEM (mode, destaddr);
  rtx tmp1 = gen_reg_rtx (mode);
  rtx tmp2 = gen_reg_rtx (mode);
  
  emit_insn (intfn (tmp1, op1, tmp2, op2));
  
  emit_move_insn (mem, tmp1);
  mem = adjust_address (mem, mode, GET_MODE_SIZE (mode));
  emit_move_insn (mem, tmp2);
}

/* Set up operands for a register copy from src to dest, taking care not to
   clobber registers in the process.
   FIXME: This has rather high polynomial complexity (O(n^3)?) but shouldn't
   be called with a large N, so that should be OK.  */

void
neon_disambiguate_copy (rtx *operands, rtx *dest, rtx *src, unsigned int count)
{
  unsigned int copied = 0, opctr = 0;
  unsigned int done = (1 << count) - 1;
  unsigned int i, j;
  
  while (copied != done)
    {
      for (i = 0; i < count; i++)
        {
          int good = 1;

          for (j = 0; good && j < count; j++)
            if (i != j && (copied & (1 << j)) == 0
                && reg_overlap_mentioned_p (src[j], dest[i]))
              good = 0;

          if (good)
            {
              operands[opctr++] = dest[i];
              operands[opctr++] = src[i];
              copied |= 1 << i;
            }
        }
    }

  gcc_assert (opctr == count * 2);
}

/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
/* Expand an expression EXP that calls a built-in function,
   with result going to TARGET if that's convenient
   (and in mode MODE if that's convenient).
   SUBTARGET may be used as the target for computing one of EXP's operands.
   IGNORE is nonzero if the value is to be ignored.  */

static rtx
arm_expand_builtin (tree exp,
		    rtx target,
		    rtx subtarget ATTRIBUTE_UNUSED,
		    enum machine_mode mode ATTRIBUTE_UNUSED,
		    int ignore ATTRIBUTE_UNUSED)
{
  const struct builtin_description * d;
  enum insn_code    icode;
  tree              fndecl = TREE_OPERAND (TREE_OPERAND (exp, 0), 0);
  tree              arglist = TREE_OPERAND (exp, 1);
  tree              arg0;
  tree              arg1;
  tree              arg2;
  rtx               op0;
  rtx               op1;
  rtx               op2;
  rtx               pat;
  int               fcode = DECL_FUNCTION_CODE (fndecl);
  size_t            i;
  enum machine_mode tmode;
  enum machine_mode mode0;
  enum machine_mode mode1;
  enum machine_mode mode2;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  if (fcode >= ARM_BUILTIN_NEON_BASE)
    return arm_expand_neon_builtin (target, fcode, arglist);

  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  switch (fcode)
    {
    case ARM_BUILTIN_TEXTRMSB:
    case ARM_BUILTIN_TEXTRMUB:
    case ARM_BUILTIN_TEXTRMSH:
    case ARM_BUILTIN_TEXTRMUH:
    case ARM_BUILTIN_TEXTRMSW:
    case ARM_BUILTIN_TEXTRMUW:
      icode = (fcode == ARM_BUILTIN_TEXTRMSB ? CODE_FOR_iwmmxt_textrmsb
	       : fcode == ARM_BUILTIN_TEXTRMUB ? CODE_FOR_iwmmxt_textrmub
	       : fcode == ARM_BUILTIN_TEXTRMSH ? CODE_FOR_iwmmxt_textrmsh
	       : fcode == ARM_BUILTIN_TEXTRMUH ? CODE_FOR_iwmmxt_textrmuh
	       : CODE_FOR_iwmmxt_textrmw);

      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
	op0 = copy_to_mode_reg (mode0, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
	{
	  /* @@@ better error message */
	  error ("selector must be an immediate");
	  return gen_reg_rtx (tmode);
	}
      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1);
      if (! pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_TINSRB:
    case ARM_BUILTIN_TINSRH:
    case ARM_BUILTIN_TINSRW:
      icode = (fcode == ARM_BUILTIN_TINSRB ? CODE_FOR_iwmmxt_tinsrb
	       : fcode == ARM_BUILTIN_TINSRH ? CODE_FOR_iwmmxt_tinsrh
	       : CODE_FOR_iwmmxt_tinsrw);
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      arg2 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      op2 = expand_normal (arg2);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;
      mode2 = insn_data[icode].operand[3].mode;

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
	op0 = copy_to_mode_reg (mode0, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
	op1 = copy_to_mode_reg (mode1, op1);
      if (! (*insn_data[icode].operand[3].predicate) (op2, mode2))
	{
	  /* @@@ better error message */
	  error ("selector must be an immediate");
	  return const0_rtx;
	}
      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1, op2);
      if (! pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_SETWCX:
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      op0 = force_reg (SImode, expand_normal (arg0));
      op1 = expand_normal (arg1);
      emit_insn (gen_iwmmxt_tmcr (op1, op0));
      return 0;

    case ARM_BUILTIN_GETWCX:
      arg0 = TREE_VALUE (arglist);
      op0 = expand_normal (arg0);
      target = gen_reg_rtx (SImode);
      emit_insn (gen_iwmmxt_tmrc (target, op0));
      return target;

    case ARM_BUILTIN_WSHUFH:
      icode = CODE_FOR_iwmmxt_wshufh;
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      tmode = insn_data[icode].operand[0].mode;
      mode1 = insn_data[icode].operand[1].mode;
      mode2 = insn_data[icode].operand[2].mode;

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode1))
	op0 = copy_to_mode_reg (mode1, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode2))
	{
	  /* @@@ better error message */
	  error ("mask must be an immediate");
	  return const0_rtx;
	}
      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1);
      if (! pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_WSADB:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wsadb, arglist, target);
    case ARM_BUILTIN_WSADH:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wsadh, arglist, target);
    case ARM_BUILTIN_WSADBZ:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wsadbz, arglist, target);
    case ARM_BUILTIN_WSADHZ:
      return arm_expand_binop_builtin (CODE_FOR_iwmmxt_wsadhz, arglist, target);

      /* Several three-argument builtins.  */
    case ARM_BUILTIN_WMACS:
    case ARM_BUILTIN_WMACU:
    case ARM_BUILTIN_WALIGN:
    case ARM_BUILTIN_TMIA:
    case ARM_BUILTIN_TMIAPH:
    case ARM_BUILTIN_TMIATT:
    case ARM_BUILTIN_TMIATB:
    case ARM_BUILTIN_TMIABT:
    case ARM_BUILTIN_TMIABB:
      icode = (fcode == ARM_BUILTIN_WMACS ? CODE_FOR_iwmmxt_wmacs
	       : fcode == ARM_BUILTIN_WMACU ? CODE_FOR_iwmmxt_wmacu
	       : fcode == ARM_BUILTIN_TMIA ? CODE_FOR_iwmmxt_tmia
	       : fcode == ARM_BUILTIN_TMIAPH ? CODE_FOR_iwmmxt_tmiaph
	       : fcode == ARM_BUILTIN_TMIABB ? CODE_FOR_iwmmxt_tmiabb
	       : fcode == ARM_BUILTIN_TMIABT ? CODE_FOR_iwmmxt_tmiabt
	       : fcode == ARM_BUILTIN_TMIATB ? CODE_FOR_iwmmxt_tmiatb
	       : fcode == ARM_BUILTIN_TMIATT ? CODE_FOR_iwmmxt_tmiatt
	       : CODE_FOR_iwmmxt_walign);
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      arg2 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      op0 = expand_normal (arg0);
      op1 = expand_normal (arg1);
      op2 = expand_normal (arg2);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;
      mode2 = insn_data[icode].operand[3].mode;

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
	op0 = copy_to_mode_reg (mode0, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
	op1 = copy_to_mode_reg (mode1, op1);
      if (! (*insn_data[icode].operand[3].predicate) (op2, mode2))
	op2 = copy_to_mode_reg (mode2, op2);
      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
	target = gen_reg_rtx (tmode);
      pat = GEN_FCN (icode) (target, op0, op1, op2);
      if (! pat)
	return 0;
      emit_insn (pat);
      return target;

    case ARM_BUILTIN_WZERO:
      target = gen_reg_rtx (DImode);
      emit_insn (gen_iwmmxt_clrdi (target));
      return target;

    case ARM_BUILTIN_THREAD_POINTER:
      return arm_load_tp (target);

    default:
      break;
    }

  for (i = 0, d = bdesc_2arg; i < ARRAY_SIZE (bdesc_2arg); i++, d++)
    if (d->code == (const enum arm_builtins) fcode)
      return arm_expand_binop_builtin (d->icode, arglist, target);

  for (i = 0, d = bdesc_1arg; i < ARRAY_SIZE (bdesc_1arg); i++, d++)
    if (d->code == (const enum arm_builtins) fcode)
      return arm_expand_unop_builtin (d->icode, arglist, target, 0);

  /* @@@ Should really do something sensible here.  */
  return NULL_RTX;
}

/* Return the number (counting from 0) of
   the least significant set bit in MASK.  */

inline static int
number_of_first_bit_set (unsigned mask)
{
  int bit;

  for (bit = 0;
       (mask & (1 << bit)) == 0;
       ++bit)
    continue;

  return bit;
}

/* APPLE LOCAL begin ARM compact switch tables */
/* Handle push or pop of registers from the stack.
   If EMIT is true, generate the code.
   If EMIT is false, compute and return the number of bytes that
   would result from a call with EMIT true.  In this case F is
   not necessarily valid and should not be referenced.

   F is the assembly file.  MASK is the registers to push or pop.  PUSH is
   nonzero if we should push, and zero if we should pop.  For debugging
   output, if pushing, adjust CFA_OFFSET by the amount of space added
   to the stack.  REAL_REGS should have the same number of bits set as
   MASK, and will be used instead (in the same order) to describe which
   registers were saved - this is used to mark the save slots when we
   push high registers after moving them to low registers.
*/
static int
handle_thumb_pushpop (FILE *f, unsigned long mask, int push, int *cfa_offset,
		      unsigned long real_regs, bool emit)
{
  int regno;
  int lo_mask = mask & 0xFF;
  int pushed_words = 0;
  int bytes = 0;

  gcc_assert (mask);

  if (lo_mask == 0 && !push && (mask & (1 << PC_REGNUM)))
    {
      /* Special case.  Do not generate a POP PC statement here, do it in
	 thumb_exit() */
      return handle_thumb_exit (f, -1, emit);
    }

  if (ARM_EABI_UNWIND_TABLES && push && emit)
    {
      fprintf (f, "\t.save\t{");
      for (regno = 0; regno < 15; regno++)
	{
	  if (real_regs & (1 << regno))
	    {
	      if (real_regs & ((1 << regno) -1))
		fprintf (f, ", ");
	      asm_fprintf (f, "%r", regno);
	    }
	}
      fprintf (f, "}\n");
    }

  bytes += 2;
  if (emit)
    fprintf (f, "\t%s\t{", push ? "push" : "pop");

  /* Look at the low registers first.  */
  for (regno = 0; regno <= LAST_LO_REGNUM; regno++, lo_mask >>= 1)
    {
      if (lo_mask & 1)
	{
	  if (emit)
	    {
	      asm_fprintf (f, "%r", regno);

	      if ((lo_mask & ~1) != 0)
		fprintf (f, ", ");
	    }

	  pushed_words++;
	}
    }

  if (push && (mask & (1 << LR_REGNUM)))
    {
      /* Catch pushing the LR.  */
      if (emit)
	{
	  if (mask & 0xFF)
	    fprintf (f, ", ");

	  asm_fprintf (f, "%r", LR_REGNUM);
	}

      pushed_words++;
    }
  else if (!push && (mask & (1 << PC_REGNUM)))
    {
      /* Catch popping the PC.  */
      /* APPLE LOCAL begin ARM interworking */
      if ((TARGET_INTERWORK && !arm_arch5)
	  || TARGET_BACKTRACE
	  || current_function_calls_eh_return)
      /* APPLE LOCAL end ARM interworking */
	{
	  /* The PC is never poped directly, instead
	     it is popped into r3 and then BX is used.  */
	  if (emit)
	    fprintf (f, "}\n");

	  bytes += handle_thumb_exit (f, -1, emit);

	  return bytes;
	}
      else if (emit)
	{
	  if (mask & 0xFF)
	    fprintf (f, ", ");

	  asm_fprintf (f, "%r", PC_REGNUM);
	}
    }

  if (emit)
    fprintf (f, "}\n");

  if (emit && push && pushed_words && dwarf2out_do_frame ())
    {
      char *l = dwarf2out_cfi_label ();
      int pushed_mask = real_regs;

      *cfa_offset += pushed_words * 4;
      dwarf2out_def_cfa (l, SP_REGNUM, *cfa_offset);

      pushed_words = 0;
      pushed_mask = real_regs;
      for (regno = 0; regno <= 14; regno++, pushed_mask >>= 1)
	{
	  if (pushed_mask & 1)
	    dwarf2out_reg_save (l, regno, 4 * pushed_words++ - *cfa_offset);
	}
    }
  return bytes;
}

/* Handle return from a thumb function.
   If EMIT is true, generate the code.
   If EMIT is false, compute and return the number of bytes that
   would result from a call with EMIT true.  In this case F is
   not necessarily valid and should not be referenced.
   If 'reg_containing_return_addr' is -1, then the return address is
   actually on the stack, at the stack pointer.
*/
static int
handle_thumb_exit (FILE *f, int reg_containing_return_addr, bool emit)
{
  unsigned regs_available_for_popping;
  unsigned regs_to_pop;
  int pops_needed;
  unsigned available;
  unsigned required;
  int mode;
  int size;
  int restore_a4 = FALSE;
  int bytes = 0;

  /* Compute the registers we need to pop.  */
  regs_to_pop = 0;
  pops_needed = 0;

  if (reg_containing_return_addr == -1)
    {
      regs_to_pop |= 1 << LR_REGNUM;
      ++pops_needed;
    }

  if (TARGET_BACKTRACE)
    {
      /* Restore the (ARM) frame pointer and stack pointer.  */
      regs_to_pop |= (1 << ARM_HARD_FRAME_POINTER_REGNUM) | (1 << SP_REGNUM);
      pops_needed += 2;
    }

  /* If there is nothing to pop then just emit the BX instruction and
     return.  */
  if (pops_needed == 0)
    {
      if (current_function_calls_eh_return)
	{
	  bytes += 2;
	  if (emit)
	    asm_fprintf (f, "\tadd\t%r, %r\n", SP_REGNUM, ARM_EH_STACKADJ_REGNUM);
	}

      bytes += 2;
      if (emit)
	asm_fprintf (f, "\tbx\t%r\n", reg_containing_return_addr);

      return bytes;
    }
  /* Otherwise if we are not supporting interworking and we have not created
     a backtrace structure and the function was not entered in ARM mode then
     just pop the return address straight into the PC.  */
  /* APPLE LOCAL ARM interworking */
  else if ((!TARGET_INTERWORK || arm_arch5)
	   && !TARGET_BACKTRACE
	   && !is_called_in_ARM_mode (current_function_decl)
	   && !current_function_calls_eh_return)
    {
      bytes += 2;
      if (emit)
	asm_fprintf (f, "\tpop\t{%r}\n", PC_REGNUM);
      return bytes;
    }

  /* Find out how many of the (return) argument registers we can corrupt.  */
  regs_available_for_popping = 0;

  /* If returning via __builtin_eh_return, the bottom three registers
     all contain information needed for the return.  */
  if (current_function_calls_eh_return)
    size = 12;
  else
    {
      /* If we can deduce the registers used from the function's
	 return value.  This is more reliable that examining
	 regs_ever_live[] because that will be set if the register is
	 ever used in the function, not just if the register is used
	 to hold a return value.  */

      if (current_function_return_rtx != 0)
	mode = GET_MODE (current_function_return_rtx);
      else
	mode = DECL_MODE (DECL_RESULT (current_function_decl));

      size = GET_MODE_SIZE (mode);

      if (size == 0)
	{
	  /* In a void function we can use any argument register.
	     In a function that returns a structure on the stack
	     we can use the second and third argument registers.  */
	  if (mode == VOIDmode)
	    regs_available_for_popping =
	      (1 << ARG_REGISTER (1))
	      | (1 << ARG_REGISTER (2))
	      | (1 << ARG_REGISTER (3));
	  else
	    regs_available_for_popping =
	      (1 << ARG_REGISTER (2))
	      | (1 << ARG_REGISTER (3));
	}
      else if (size <= 4)
	regs_available_for_popping =
	  (1 << ARG_REGISTER (2))
	  | (1 << ARG_REGISTER (3));
      else if (size <= 8)
	regs_available_for_popping =
	  (1 << ARG_REGISTER (3));
    }

  /* Match registers to be popped with registers into which we pop them.  */
  for (available = regs_available_for_popping,
       required  = regs_to_pop;
       required != 0 && available != 0;
       available &= ~(available & - available),
       required  &= ~(required  & - required))
    -- pops_needed;

  /* If we have any popping registers left over, remove them.  */
  if (available > 0)
    regs_available_for_popping &= ~available;

  /* Otherwise if we need another popping register we can use
     the fourth argument register.  */
  else if (pops_needed)
    {
      /* If we have not found any free argument registers and
	 reg a4 contains the return address, we must move it.  */
      if (regs_available_for_popping == 0
	  && reg_containing_return_addr == LAST_ARG_REGNUM)
	{
	  bytes += 2;
	  if (emit)
	    asm_fprintf (f, "\tmov\t%r, %r\n", LR_REGNUM, LAST_ARG_REGNUM);
	  reg_containing_return_addr = LR_REGNUM;
	}
      else if (size > 12)
	{
	  /* Register a4 is being used to hold part of the return value,
	     but we have dire need of a free, low register.  */
	  restore_a4 = TRUE;

	  bytes += 2;
	  if (emit)
	    asm_fprintf (f, "\tmov\t%r, %r\n",IP_REGNUM, LAST_ARG_REGNUM);
	}

      if (reg_containing_return_addr != LAST_ARG_REGNUM)
	{
	  /* The fourth argument register is available.  */
	  regs_available_for_popping |= 1 << LAST_ARG_REGNUM;

	  --pops_needed;
	}
    }

  /* Pop as many registers as we can.  */
  bytes += handle_thumb_pushpop (f, regs_available_for_popping, FALSE, NULL,
				 regs_available_for_popping, emit);

  /* Process the registers we popped.  */
  if (reg_containing_return_addr == -1)
    {
      /* The return address was popped into the lowest numbered register.  */
      regs_to_pop &= ~(1 << LR_REGNUM);

      reg_containing_return_addr =
	number_of_first_bit_set (regs_available_for_popping);

      /* Remove this register for the mask of available registers, so that
         the return address will not be corrupted by further pops.  */
      regs_available_for_popping &= ~(1 << reg_containing_return_addr);
    }

  /* If we popped other registers then handle them here.  */
  if (regs_available_for_popping)
    {
      int frame_pointer;

      /* Work out which register currently contains the frame pointer.  */
      frame_pointer = number_of_first_bit_set (regs_available_for_popping);

      /* Move it into the correct place.  */
      bytes += 2;
      if (emit)
	asm_fprintf (f, "\tmov\t%r, %r\n",
		     ARM_HARD_FRAME_POINTER_REGNUM, frame_pointer);

      /* (Temporarily) remove it from the mask of popped registers.  */
      regs_available_for_popping &= ~(1 << frame_pointer);
      regs_to_pop &= ~(1 << ARM_HARD_FRAME_POINTER_REGNUM);

      if (regs_available_for_popping)
	{
	  int stack_pointer;

	  /* We popped the stack pointer as well,
	     find the register that contains it.  */
	  stack_pointer = number_of_first_bit_set (regs_available_for_popping);

	  /* Move it into the stack register.  */
	  bytes += 2;
	  if (emit)
	    asm_fprintf (f, "\tmov\t%r, %r\n", SP_REGNUM, stack_pointer);

	  /* At this point we have popped all necessary registers, so
	     do not worry about restoring regs_available_for_popping
	     to its correct value:

	     assert (pops_needed == 0)
	     assert (regs_available_for_popping == (1 << frame_pointer))
	     assert (regs_to_pop == (1 << STACK_POINTER))  */
	}
      else
	{
	  /* Since we have just move the popped value into the frame
	     pointer, the popping register is available for reuse, and
	     we know that we still have the stack pointer left to pop.  */
	  regs_available_for_popping |= (1 << frame_pointer);
	}
    }

  /* If we still have registers left on the stack, but we no longer have
     any registers into which we can pop them, then we must move the return
     address into the link register and make available the register that
     contained it.  */
  if (regs_available_for_popping == 0 && pops_needed > 0)
    {
      regs_available_for_popping |= 1 << reg_containing_return_addr;

      bytes += 2;
      if (emit)
	asm_fprintf (f, "\tmov\t%r, %r\n", LR_REGNUM,
		     reg_containing_return_addr);

      reg_containing_return_addr = LR_REGNUM;
    }

  /* If we have registers left on the stack then pop some more.
     We know that at most we will want to pop FP and SP.  */
  if (pops_needed > 0)
    {
      int  popped_into;
      int  move_to;

      bytes += handle_thumb_pushpop (f, regs_available_for_popping, FALSE, NULL,
				     regs_available_for_popping, emit);

      /* We have popped either FP or SP.
	 Move whichever one it is into the correct register.  */
      popped_into = number_of_first_bit_set (regs_available_for_popping);
      move_to     = number_of_first_bit_set (regs_to_pop);

      bytes += 2;
      if (emit)
	asm_fprintf (f, "\tmov\t%r, %r\n", move_to, popped_into);

      regs_to_pop &= ~(1 << move_to);

      --pops_needed;
    }

  /* If we still have not popped everything then we must have only
     had one register available to us and we are now popping the SP.  */
  if (pops_needed > 0)
    {
      int  popped_into;

      bytes += handle_thumb_pushpop (f, regs_available_for_popping, FALSE, NULL,
				     regs_available_for_popping, emit);

      popped_into = number_of_first_bit_set (regs_available_for_popping);

      bytes += 2;
      if (emit)
	asm_fprintf (f, "\tmov\t%r, %r\n", SP_REGNUM, popped_into);
      /*
	assert (regs_to_pop == (1 << STACK_POINTER))
	assert (pops_needed == 1)
      */
    }

  /* If necessary restore the a4 register.  */
  if (restore_a4)
    {
      if (reg_containing_return_addr != LR_REGNUM)
	{
	  bytes += 2;
	  if (emit)
	    asm_fprintf (f, "\tmov\t%r, %r\n", LR_REGNUM, LAST_ARG_REGNUM);
	  reg_containing_return_addr = LR_REGNUM;
	}

      bytes += 2;
      if (emit)
	asm_fprintf (f, "\tmov\t%r, %r\n", LAST_ARG_REGNUM, IP_REGNUM);
    }

  if (current_function_calls_eh_return)
    {
      bytes += 2;
      if (emit)
	asm_fprintf (f, "\tadd\t%r, %r\n", SP_REGNUM, ARM_EH_STACKADJ_REGNUM);
    }

  /* Return to caller.  */
  bytes += 2;
  if (emit)
    asm_fprintf (f, "\tbx\t%r\n", reg_containing_return_addr);
  return bytes;
}
/* APPLE LOCAL end ARM compact switch tables */


void
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
thumb1_final_prescan_insn (rtx insn)
{
  if (flag_print_asm_name)
    asm_fprintf (asm_out_file, "%@ 0x%04x\n",
		 INSN_ADDRESSES (INSN_UID (insn)));
}

int
thumb_shiftable_const (unsigned HOST_WIDE_INT val)
{
  unsigned HOST_WIDE_INT mask = 0xff;
  int i;

  if (val == 0) /* XXX */
    return 0;

  for (i = 0; i < 25; i++)
    if ((val & (mask << i)) == val)
      return 1;

  return 0;
}

/* Returns nonzero if the current function contains,
   or might contain a far jump.  */
static int
thumb_far_jump_used_p (void)
{
  rtx insn;

  /* This test is only important for leaf functions.  */
  /* assert (!leaf_function_p ()); */

  /* If we have already decided that far jumps may be used,
     do not bother checking again, and always return true even if
     it turns out that they are not being used.  Once we have made
     the decision that far jumps are present (and that hence the link
     register will be pushed onto the stack) we cannot go back on it.  */
  if (cfun->machine->far_jump_used)
    return 1;

  /* If this function is not being called from the prologue/epilogue
     generation code then it must be being called from the
     INITIAL_ELIMINATION_OFFSET macro.  */
  if (!(ARM_DOUBLEWORD_ALIGN || reload_completed))
    {
      /* In this case we know that we are being asked about the elimination
	 of the arg pointer register.  If that register is not being used,
	 then there are no arguments on the stack, and we do not have to
	 worry that a far jump might force the prologue to push the link
	 register, changing the stack offsets.  In this case we can just
	 return false, since the presence of far jumps in the function will
	 not affect stack offsets.

	 If the arg pointer is live (or if it was live, but has now been
	 eliminated and so set to dead) then we do have to test to see if
	 the function might contain a far jump.  This test can lead to some
	 false negatives, since before reload is completed, then length of
	 branch instructions is not known, so gcc defaults to returning their
	 longest length, which in turn sets the far jump attribute to true.

	 A false negative will not result in bad code being generated, but it
	 will result in a needless push and pop of the link register.  We
	 hope that this does not occur too often.

	 If we need doubleword stack alignment this could affect the other
	 elimination offsets so we can't risk getting it wrong.  */
      if (regs_ever_live [ARG_POINTER_REGNUM])
	cfun->machine->arg_pointer_live = 1;
      else if (!cfun->machine->arg_pointer_live)
	return 0;
    }

  /* Check to see if the function contains a branch
     insn with the far jump attribute set.  */
  for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
    {
      if (GET_CODE (insn) == JUMP_INSN
	  /* Ignore tablejump patterns.  */
	  && GET_CODE (PATTERN (insn)) != ADDR_VEC
	  && GET_CODE (PATTERN (insn)) != ADDR_DIFF_VEC
	  && get_attr_far_jump (insn) == FAR_JUMP_YES
	  )
	{
	  /* Record the fact that we have decided that
	     the function does use far jumps.  */
	  cfun->machine->far_jump_used = 1;
	  return 1;
	}
    }

  return 0;
}

/* Return nonzero if FUNC must be entered in ARM mode.  */
int
is_called_in_ARM_mode (tree func)
{
  gcc_assert (TREE_CODE (func) == FUNCTION_DECL);

  /* Ignore the problem about functions whose address is taken.  */
  if (TARGET_CALLEE_INTERWORKING && TREE_PUBLIC (func))
    return TRUE;

#ifdef ARM_PE
  return lookup_attribute ("interfacearm", DECL_ATTRIBUTES (func)) != NULL_TREE;
#else
  return FALSE;
#endif
}

/* APPLE LOCAL begin ARM compact switch tables */
/* This handles the part of the epilogue that is not expressed as RTL.
   It computes and returns the number of bytes in this part of the epilogue.
   When EMIT is true, it additionally outputs this part of the epilogue.
   When !EMIT, this function does not output anything; in this case
   F need not be valid and should not be referenced.
*/
static int
handle_thumb_unexpanded_epilogue (bool emit)
{
  int regno;
  unsigned long live_regs_mask = 0;
  int high_regs_pushed = 0;
  int had_to_push_lr;
  int size;
  int bytes = 0;

  if (return_used_this_function)
    return bytes;

  if (IS_NAKED (arm_current_func_type ()))
    return bytes;

  /* APPLE LOCAL begin 6465387 exception handling interworking VFP save */
  if (current_function_has_nonlocal_label && arm_arch6)
    {
      bytes += 4;
      if (emit)
        asm_fprintf (asm_out_file, "\tblx ___restore_vfp_d8_d15_regs\n");
    }
  /* APPLE LOCAL end 6465387 exception handling interworking VFP save */

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  live_regs_mask = thumb1_compute_save_reg_mask ();
  high_regs_pushed = bit_count (live_regs_mask & 0x0f00);

  /* If we can deduce the registers used from the function's return value.
     This is more reliable that examining regs_ever_live[] because that
     will be set if the register is ever used in the function, not just if
     the register is used to hold a return value.  */
  size = arm_size_return_regs ();

  /* The prolog may have pushed some high registers to use as
     work registers.  e.g. the testsuite file:
     gcc/testsuite/gcc/gcc.c-torture/execute/complex-2.c
     compiles to produce:
	push	{r4, r5, r6, r7, lr}
	mov	r7, r9
	mov	r6, r8
	push	{r6, r7}
     as part of the prolog.  We have to undo that pushing here.  */

  if (high_regs_pushed)
    {
      unsigned long mask = live_regs_mask & 0xff;
      int next_hi_reg;

      /* The available low registers depend on the size of the value we are
         returning.  */
      if (size <= 12)
	mask |=  1 << 3;
      if (size <= 8)
	mask |= 1 << 2;

      if (mask == 0)
	/* Oh dear!  We have no low registers into which we can pop
           high registers!  */
	internal_error
	  ("no low registers available for popping high registers");

      for (next_hi_reg = 8; next_hi_reg < 13; next_hi_reg++)
	if (live_regs_mask & (1 << next_hi_reg))
	  break;

      while (high_regs_pushed)
	{
	  /* Find lo register(s) into which the high register(s) can
             be popped.  */
	  for (regno = 0; regno <= LAST_LO_REGNUM; regno++)
	    {
	      if (mask & (1 << regno))
		high_regs_pushed--;
	      if (high_regs_pushed == 0)
		break;
	    }

	  mask &= (2 << regno) - 1;	/* A noop if regno == 8 */

	  /* Pop the values into the low register(s).  */
	  bytes += handle_thumb_pushpop (asm_out_file, mask, 0, NULL, mask, emit);

	  /* Move the value(s) into the high registers.  */
	  for (regno = 0; regno <= LAST_LO_REGNUM; regno++)
	    {
	      if (mask & (1 << regno))
		{
		  bytes += 2;
		  if (emit)
		    asm_fprintf (asm_out_file, "\tmov\t%r, %r\n", next_hi_reg,
				 regno);

		  for (next_hi_reg++; next_hi_reg < 13; next_hi_reg++)
		    if (live_regs_mask & (1 << next_hi_reg))
		      break;
		}
	    }
	}
      live_regs_mask &= ~0x0f00;
    }

  had_to_push_lr = (live_regs_mask & (1 << LR_REGNUM)) != 0;
  live_regs_mask &= 0xff;

  if (current_function_pretend_args_size == 0 || TARGET_BACKTRACE)
    {
      /* Pop the return address into the PC.  */
      if (had_to_push_lr)
	live_regs_mask |= 1 << PC_REGNUM;

      /* Either no argument registers were pushed or a backtrace
	 structure was created which includes an adjusted stack
	 pointer, so just pop everything.  */
      if (live_regs_mask)
	bytes += handle_thumb_pushpop (asm_out_file, live_regs_mask, FALSE, NULL,
				       live_regs_mask, emit);

      /* We have either just popped the return address into the
	 PC or it is was kept in LR for the entire function.  */
      if (!had_to_push_lr)
	bytes += handle_thumb_exit (asm_out_file, LR_REGNUM, emit);
    }
  else
    {
      /* Pop everything but the return address.  */
      if (live_regs_mask)
	bytes += handle_thumb_pushpop (asm_out_file, live_regs_mask, FALSE, NULL,
				       live_regs_mask, emit);

      if (had_to_push_lr)
	{
	  if (size > 12)
	    {
	      /* We have no free low regs, so save one.  */
	      bytes += 2;
	      if (emit)
		asm_fprintf (asm_out_file, "\tmov\t%r, %r\n", IP_REGNUM,
			     LAST_ARG_REGNUM);
	    }

	  /* Get the return address into a temporary register.  */
	  bytes += handle_thumb_pushpop (asm_out_file, 1 << LAST_ARG_REGNUM, 0, NULL,
					 1 << LAST_ARG_REGNUM, emit);

	  if (size > 12)
	    {
	      bytes += 4;
	      if (emit)
		{
		  /* Move the return address to lr.  */
		  asm_fprintf (asm_out_file, "\tmov\t%r, %r\n", LR_REGNUM,
			       LAST_ARG_REGNUM);
		  /* Restore the low register.  */
		  asm_fprintf (asm_out_file, "\tmov\t%r, %r\n", LAST_ARG_REGNUM,
			       IP_REGNUM);
		}
	      regno = LR_REGNUM;
	    }
	  else
	    regno = LAST_ARG_REGNUM;
	}
      else
	regno = LR_REGNUM;

      /* Remove the argument registers that were pushed onto the stack.  */
      bytes += 2;
      if (emit)
	asm_fprintf (asm_out_file, "\tadd\t%r, %r, #%d\n",
		     SP_REGNUM, SP_REGNUM,
		     current_function_pretend_args_size);

      bytes += handle_thumb_exit (asm_out_file, regno, emit);
    }

  return bytes;
}

/* This is the externally visible entry point for generating code for the
   part of the epilogue that is not stored as RTL.  This is just a wrapper
   around the previous, with the correct externally imposed interface. */

const char * thumb_unexpanded_epilogue (void)
{
  (void) handle_thumb_unexpanded_epilogue (true);
  return "";
}
/* APPLE LOCAL end ARM compact switch tables */

/* Functions to save and restore machine-specific function data.  */
static struct machine_function *
arm_init_machine_status (void)
{
  struct machine_function *machine;
  machine = (machine_function *) ggc_alloc_cleared (sizeof (machine_function));

#if ARM_FT_UNKNOWN != 0
  machine->func_type = ARM_FT_UNKNOWN;
#endif
  return machine;
}

/* Return an RTX indicating where the return address to the
   calling function can be found.  */
/* APPLE LOCAL begin ARM reliable backtraces */
rtx
arm_return_addr (int count, rtx frame)
{
  if (count != 0)
    return gen_rtx_MEM (Pmode, plus_constant (frame, 4));

  return get_hard_reg_initial_val (Pmode, LR_REGNUM);
}
/* APPLE LOCAL end ARM reliable backtraces */

/* Do anything needed before RTL is emitted for each function.  */
void
arm_init_expanders (void)
{
  /* Arrange to initialize and mark the machine per-function status.  */
  init_machine_status = arm_init_machine_status;

  /* This is to stop the combine pass optimizing away the alignment
     adjustment of va_arg.  */
  /* ??? It is claimed that this should not be necessary.  */
  if (cfun)
    mark_reg_pointer (arg_pointer_rtx, PARM_BOUNDARY);
}


/* Like arm_compute_initial_elimination offset.  Simpler because there
   isn't an ABI specified frame pointer for Thumb.  Instead, we set it
   to point at the base of the local variables after static stack
   space for a function has been allocated.  */

HOST_WIDE_INT
thumb_compute_initial_elimination_offset (unsigned int from, unsigned int to)
{
  arm_stack_offsets *offsets;

  offsets = arm_get_frame_offsets ();

  switch (from)
    {
    case ARG_POINTER_REGNUM:
      switch (to)
	{
	case STACK_POINTER_REGNUM:
	  return offsets->outgoing_args - offsets->saved_args;

	case FRAME_POINTER_REGNUM:
	  return offsets->soft_frame - offsets->saved_args;

	/* APPLE LOCAL begin ARM custom frame layout */
	case HARD_FRAME_POINTER_REGNUM:
	  return offsets->frame - offsets->saved_args;
	/* APPLE LOCAL end ARM custom frame layout */

	default:
	  gcc_unreachable ();
	}
      break;

    case FRAME_POINTER_REGNUM:
      switch (to)
	{
	case STACK_POINTER_REGNUM:
	  return offsets->outgoing_args - offsets->soft_frame;

	/* APPLE LOCAL begin ARM custom frame layout */
	case HARD_FRAME_POINTER_REGNUM:
	  return offsets->frame - offsets->soft_frame;
	/* APPLE LOCAL end ARM custom frame layout */

	default:
	  gcc_unreachable ();
	}
      break;

    default:
      gcc_unreachable ();
    }
}

/* Generate the rest of a function's prologue.  */
void
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
thumb1_expand_prologue (void)
{
  rtx insn, dwarf;

  HOST_WIDE_INT amount;
  arm_stack_offsets *offsets;
  unsigned long func_type;
  int regno;
  unsigned long live_regs_mask;

  func_type = arm_current_func_type ();

  /* Naked functions don't have prologues.  */
  if (IS_NAKED (func_type))
    return;

  if (IS_INTERRUPT (func_type))
    {
      error ("interrupt Service Routines cannot be coded in Thumb mode");
      return;
    }

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  live_regs_mask = thumb1_compute_save_reg_mask ();
  /* Load the pic register before setting the frame pointer,
     so we can use r7 as a temporary work register.  */
  if (flag_pic && arm_pic_register != INVALID_REGNUM)
    arm_load_pic_register (live_regs_mask);

  /* APPLE LOCAL begin ARM custom frame layout */
  offsets = arm_get_frame_offsets ();

  if (frame_pointer_needed)
    {
      insn = emit_insn (gen_addsi3 (hard_frame_pointer_rtx,
				    stack_pointer_rtx,
				    GEN_INT (offsets->saved_regs
					     - offsets->frame)));
      RTX_FRAME_RELATED_P (insn) = 1;
    }
  else if (CALLER_INTERWORKING_SLOT_SIZE > 0)
    {
      emit_move_insn (gen_rtx_REG (Pmode, ARM_HARD_FRAME_POINTER_REGNUM),
		      stack_pointer_rtx);
    }
  /* APPLE LOCAL end ARM custom frame layout */

  amount = offsets->outgoing_args - offsets->saved_regs;
  if (amount)
    {
      if (amount < 512)
	{
	  insn = emit_insn (gen_addsi3 (stack_pointer_rtx, stack_pointer_rtx,
					GEN_INT (- amount)));
	  RTX_FRAME_RELATED_P (insn) = 1;
	}
      else
	{
	  rtx reg;

	  /* The stack decrement is too big for an immediate value in a single
	     insn.  In theory we could issue multiple subtracts, but after
	     three of them it becomes more space efficient to place the full
	     value in the constant pool and load into a register.  (Also the
	     ARM debugger really likes to see only one stack decrement per
	     function).  So instead we look for a scratch register into which
	     we can load the decrement, and then we subtract this from the
	     stack pointer.  Unfortunately on the thumb the only available
	     scratch registers are the argument registers, and we cannot use
	     these as they may hold arguments to the function.  Instead we
	     attempt to locate a call preserved register which is used by this
	     function.  If we can find one, then we know that it will have
	     been pushed at the start of the prologue and so we can corrupt
	     it now.  */
	  for (regno = LAST_ARG_REGNUM + 1; regno <= LAST_LO_REGNUM; regno++)
	    if (live_regs_mask & (1 << regno)
		&& !(frame_pointer_needed
		     && (regno == THUMB_HARD_FRAME_POINTER_REGNUM)))
	      break;

	  if (regno > LAST_LO_REGNUM) /* Very unlikely.  */
	    {
	      rtx spare = gen_rtx_REG (SImode, IP_REGNUM);

	      /* Choose an arbitrary, non-argument low register.  */
	      /* APPLE LOCAL ARM custom frame layout */
	      reg = gen_rtx_REG (SImode, LAST_LO_REGNUM - 1);

	      /* Save it by copying it into a high, scratch register.  */
	      emit_insn (gen_movsi (spare, reg));
	      /* Add a USE to stop propagate_one_insn() from barfing.  */
	      emit_insn (gen_prologue_use (spare));

	      /* Decrement the stack.  */
	      emit_insn (gen_movsi (reg, GEN_INT (- amount)));
	      insn = emit_insn (gen_addsi3 (stack_pointer_rtx,
					    stack_pointer_rtx, reg));
	      RTX_FRAME_RELATED_P (insn) = 1;
	      dwarf = gen_rtx_SET (VOIDmode, stack_pointer_rtx,
				   plus_constant (stack_pointer_rtx,
						  -amount));
	      RTX_FRAME_RELATED_P (dwarf) = 1;
	      REG_NOTES (insn)
		= gen_rtx_EXPR_LIST (REG_FRAME_RELATED_EXPR, dwarf,
				     REG_NOTES (insn));

	      /* Restore the low register's original value.  */
	      emit_insn (gen_movsi (reg, spare));

	      /* Emit a USE of the restored scratch register, so that flow
		 analysis will not consider the restore redundant.  The
		 register won't be used again in this function and isn't
		 restored by the epilogue.  */
	      emit_insn (gen_prologue_use (reg));
	    }
	  else
	    {
	      reg = gen_rtx_REG (SImode, regno);

	      emit_insn (gen_movsi (reg, GEN_INT (- amount)));

	      insn = emit_insn (gen_addsi3 (stack_pointer_rtx,
					    stack_pointer_rtx, reg));
	      RTX_FRAME_RELATED_P (insn) = 1;
	      dwarf = gen_rtx_SET (VOIDmode, stack_pointer_rtx,
				   plus_constant (stack_pointer_rtx,
						  -amount));
	      RTX_FRAME_RELATED_P (dwarf) = 1;
	      REG_NOTES (insn)
		= gen_rtx_EXPR_LIST (REG_FRAME_RELATED_EXPR, dwarf,
				     REG_NOTES (insn));
	    }
	}
    }

  /* APPLE LOCAL begin ARM custom frame layout */
  /* Removed lines.  */
  /* APPLE LOCAL end ARM custom frame layout */

  /* If we are profiling, make sure no instructions are scheduled before
     the call to mcount.  Similarly if the user has requested no
     scheduling in the prolog.  Similarly if we want non-call exceptions
     using the EABI unwinder, to prevent faulting instructions from being
     swapped with a stack adjustment.  */
  if (current_function_profile || !TARGET_SCHED_PROLOG
      || (ARM_EABI_UNWIND_TABLES && flag_non_call_exceptions))
    emit_insn (gen_blockage ());

  cfun->machine->lr_save_eliminated = !thumb_force_lr_save ();
  if (live_regs_mask & 0xff)
    cfun->machine->lr_save_eliminated = 0;

  /* If the link register is being kept alive, with the return address in it,
     then make sure that it does not get reused by the ce2 pass.  */
  if (cfun->machine->lr_save_eliminated)
    emit_insn (gen_prologue_use (gen_rtx_REG (SImode, LR_REGNUM)));
}


void
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
thumb1_expand_epilogue (void)
{
  HOST_WIDE_INT amount;
  arm_stack_offsets *offsets;
  int regno;

  /* Naked functions don't have prologues.  */
  if (IS_NAKED (arm_current_func_type ()))
    return;

  offsets = arm_get_frame_offsets ();
  amount = offsets->outgoing_args - offsets->saved_regs;

  /* APPLE LOCAL begin ARM custom frame layout */
  /* Because popping the stack frame using the frame pointer is so much
     more expensive than just popping it from the SP, only use the FP
     when we must -- i.e., when we don't know the SP offset because it
     has changed since the beginning of the function.  */
  if (! current_function_sp_is_unchanging)
    {
      int fp_offset = offsets->frame - offsets->saved_regs;

      if (fp_offset)
	{
	  /* r3 is always free in the epilogue.  */
	  rtx reg = gen_rtx_REG (SImode, LAST_ARG_REGNUM);

	  emit_insn (gen_movsi (reg, hard_frame_pointer_rtx));
	  emit_insn (gen_addsi3 (reg, reg, GEN_INT (fp_offset)));
	  emit_insn (gen_movsi (stack_pointer_rtx, reg));
	}
      else
	{
	  emit_insn (gen_movsi (stack_pointer_rtx,
				hard_frame_pointer_rtx));
	}
    }
  else if (amount)
  /* APPLE LOCAL end ARM custom frame layout */
    {
      if (amount < 512)
	emit_insn (gen_addsi3 (stack_pointer_rtx, stack_pointer_rtx,
			       GEN_INT (amount)));
      else
	{
	  /* r3 is always free in the epilogue.  */
	  rtx reg = gen_rtx_REG (SImode, LAST_ARG_REGNUM);

	  emit_insn (gen_movsi (reg, GEN_INT (amount)));
	  emit_insn (gen_addsi3 (stack_pointer_rtx, stack_pointer_rtx, reg));
	}
    }

  /* Emit a USE (stack_pointer_rtx), so that
     the stack adjustment will not be deleted.  */
  emit_insn (gen_prologue_use (stack_pointer_rtx));

  if (current_function_profile || !TARGET_SCHED_PROLOG)
    emit_insn (gen_blockage ());

  /* Emit a clobber for each insn that will be restored in the epilogue,
     so that flow2 will get register lifetimes correct.  */
  for (regno = 0; regno < 13; regno++)
    if (regs_ever_live[regno] && !call_used_regs[regno])
      emit_insn (gen_rtx_CLOBBER (VOIDmode, gen_rtx_REG (SImode, regno)));

  if (! regs_ever_live[LR_REGNUM])
    emit_insn (gen_rtx_USE (VOIDmode, gen_rtx_REG (SImode, LR_REGNUM)));
}

/* APPLE LOCAL begin ARM 4790140 compact switch tables */
/* This handles the part of the prologue that is not expressed as RTL.
   It computes and returns the number of bytes in this part of the prologue.
   When EMIT is true, it additionally outputs this part of the prologue.
   When !EMIT, this function does not output anything; in this case
   F need not be valid and should not be referenced.
*/
static int
handle_thumb_unexpanded_prologue (FILE *f, bool emit)
{
  unsigned long live_regs_mask = 0;
  unsigned long l_mask;
  unsigned high_regs_pushed = 0;
  int cfa_offset = 0;
  int regno;
  int bytes = 0;

  if (IS_NAKED (arm_current_func_type ()))
    return bytes;

  if (is_called_in_ARM_mode (current_function_decl))
    {
      const char * name;

      gcc_assert (GET_CODE (DECL_RTL (current_function_decl)) == MEM);
      gcc_assert (GET_CODE (XEXP (DECL_RTL (current_function_decl), 0))
		  == SYMBOL_REF);

      bytes += 8;

      if (emit)
	{
	  name = XSTR  (XEXP (DECL_RTL (current_function_decl), 0), 0);

	  /* Generate code sequence to switch us into Thumb mode.  */
	  /* The .code 32 directive has already been emitted by
	     ASM_DECLARE_FUNCTION_NAME.  */
	  asm_fprintf (f, "\torr\t%r, %r, #1\n", IP_REGNUM, PC_REGNUM);
	  asm_fprintf (f, "\tbx\t%r\n", IP_REGNUM);

	  /* Generate a label, so that the debugger will notice the
	     change in instruction sets.  This label is also used by
	     the assembler to bypass the ARM code when this function
	     is called from a Thumb encoded function elsewhere in the
	     same file.  Hence the definition of STUB_NAME here must
	     agree with the definition in gas/config/tc-arm.c.  */

#define STUB_NAME ".real_start_of"

	  fprintf (f, "\t.code\t16\n");
#ifdef ARM_PE
	  if (arm_dllexport_name_p (name))
	    name = arm_strip_name_encoding (name);
#endif
	  asm_fprintf (f, "\t.globl %s%U%s\n", STUB_NAME, name);
	  /* APPLE LOCAL begin ARM thumb_func <symbol_name> */
	  if (TARGET_MACHO)
	    asm_fprintf (f, "\t.thumb_func %s%U%s\n", STUB_NAME, name);
	  else
	    fprintf (f, "\t.thumb_func\n");
	  /* APPLE LOCAL end ARM thumb_func <symbol_name> */

	  asm_fprintf (f, "%s%U%s:\n", STUB_NAME, name);
	}
    }

  if (current_function_pretend_args_size)
    {
      /* Output unwind directive for the stack adjustment.  */
      if (emit && ARM_EABI_UNWIND_TABLES)
	fprintf (f, "\t.pad #%d\n",
		 current_function_pretend_args_size);

      if (emit)
	{
	  if (cfun->machine->uses_anonymous_args)
	    {
	      int num_pushes;

	      fprintf (f, "\tpush\t{");

	      num_pushes = ARM_NUM_INTS (current_function_pretend_args_size);

	      for (regno = LAST_ARG_REGNUM + 1 - num_pushes;
		   regno <= LAST_ARG_REGNUM;
		   regno++)
		asm_fprintf (f, "%r%s", regno,
			     regno == LAST_ARG_REGNUM ? "" : ", ");

	      fprintf (f, "}\n");
	    }
	  else
	    asm_fprintf (f, "\tsub\t%r, %r, #%d\n",
			 SP_REGNUM, SP_REGNUM,
			 current_function_pretend_args_size);
	}

      /* We don't need to record the stores for unwinding (would it
	 help the debugger any if we did?), but record the change in
	 the stack pointer.  */
      if (emit && dwarf2out_do_frame ())
	{
	  char *l = dwarf2out_cfi_label ();

	  cfa_offset = cfa_offset + current_function_pretend_args_size;
	  dwarf2out_def_cfa (l, SP_REGNUM, cfa_offset);
	}
    }

  /* Get the registers we are going to push.  */
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  live_regs_mask = thumb1_compute_save_reg_mask ();
  /* Extract a mask of the ones we can give to the Thumb's push instruction.  */
  l_mask = live_regs_mask & 0x40ff;
  /* Then count how many other high registers will need to be pushed.  */
  high_regs_pushed = bit_count (live_regs_mask & 0x0f00);

  if (TARGET_BACKTRACE)
    {
      unsigned offset;
      unsigned work_register;

      /* We have been asked to create a stack backtrace structure.
         The code looks like this:

	 0   .align 2
	 0   func:
         0     sub   SP, #16         Reserve space for 4 registers.
	 2     push  {R7}            Push low registers.
         4     add   R7, SP, #20     Get the stack pointer before the push.
         6     str   R7, [SP, #8]    Store the stack pointer (before reserving the space).
         8     mov   R7, PC          Get hold of the start of this code plus 12.
        10     str   R7, [SP, #16]   Store it.
        12     mov   R7, FP          Get hold of the current frame pointer.
        14     str   R7, [SP, #4]    Store it.
        16     mov   R7, LR          Get hold of the current return address.
        18     str   R7, [SP, #12]   Store it.
        20     add   R7, SP, #16     Point at the start of the backtrace structure.
        22     mov   FP, R7          Put this value into the frame pointer.  */

      work_register = thumb_find_work_register (live_regs_mask);

      if (emit && ARM_EABI_UNWIND_TABLES)
	asm_fprintf (f, "\t.pad #16\n");

      bytes += 2;
      if (emit)
	asm_fprintf
	  (f, "\tsub\t%r, %r, #16\t%@ Create stack backtrace structure\n",
	   SP_REGNUM, SP_REGNUM);

      if (emit && dwarf2out_do_frame ())
	{
	  char *l = dwarf2out_cfi_label ();

	  cfa_offset = cfa_offset + 16;
	  dwarf2out_def_cfa (l, SP_REGNUM, cfa_offset);
	}

      if (l_mask)
	{
	  bytes += handle_thumb_pushpop (f, l_mask, 1, &cfa_offset, l_mask, emit);
	  offset = bit_count (l_mask) * UNITS_PER_WORD;
	}
      else
	offset = 0;

      bytes += 4;
      if (emit)
	{
	  asm_fprintf (f, "\tadd\t%r, %r, #%d\n", work_register, SP_REGNUM,
		       offset + 16 + current_function_pretend_args_size);

	  asm_fprintf (f, "\tstr\t%r, [%r, #%d]\n", work_register, SP_REGNUM,
		       offset + 4);
	}

      bytes += 8;
      if (emit)
	{
	  /* Make sure that the instruction fetching the PC is in the right place
	     to calculate "start of backtrace creation code + 12".  */
	  if (l_mask)
	    {
	      asm_fprintf (f, "\tmov\t%r, %r\n", work_register, PC_REGNUM);
	      asm_fprintf (f, "\tstr\t%r, [%r, #%d]\n", work_register, SP_REGNUM,
			   offset + 12);
	      asm_fprintf (f, "\tmov\t%r, %r\n", work_register,
			   ARM_HARD_FRAME_POINTER_REGNUM);
	      asm_fprintf (f, "\tstr\t%r, [%r, #%d]\n", work_register, SP_REGNUM,
			   offset);
	    }
	  else
	    {
	      asm_fprintf (f, "\tmov\t%r, %r\n", work_register,
			   ARM_HARD_FRAME_POINTER_REGNUM);
	      asm_fprintf (f, "\tstr\t%r, [%r, #%d]\n", work_register, SP_REGNUM,
			   offset);
	      asm_fprintf (f, "\tmov\t%r, %r\n", work_register, PC_REGNUM);
	      asm_fprintf (f, "\tstr\t%r, [%r, #%d]\n", work_register, SP_REGNUM,
			   offset + 12);
	    }
	}

      bytes += 8;
      if (emit)
	{
	  asm_fprintf (f, "\tmov\t%r, %r\n", work_register, LR_REGNUM);
	  asm_fprintf (f, "\tstr\t%r, [%r, #%d]\n", work_register, SP_REGNUM,
		       offset + 8);
	  asm_fprintf (f, "\tadd\t%r, %r, #%d\n", work_register, SP_REGNUM,
		       offset + 12);
	  asm_fprintf (f, "\tmov\t%r, %r\t\t%@ Backtrace structure created\n",
		       ARM_HARD_FRAME_POINTER_REGNUM, work_register);
	}
    }
  /* Optimization:  If we are not pushing any low registers but we are going
     to push some high registers then delay our first push.  This will just
     be a push of LR and we can combine it with the push of the first high
     register.  */
  else if ((l_mask & 0xff) != 0
	   || (high_regs_pushed == 0 && l_mask))
    bytes += handle_thumb_pushpop (f, l_mask, 1, &cfa_offset, l_mask, emit);

  if (high_regs_pushed)
    {
      unsigned pushable_regs;
      unsigned next_hi_reg;

      for (next_hi_reg = 12; next_hi_reg > LAST_LO_REGNUM; next_hi_reg--)
	if (live_regs_mask & (1 << next_hi_reg))
	  break;

      /* APPLE LOCAL ARM thumb requires FP */
      pushable_regs = l_mask & 0x7f;

      if (pushable_regs == 0)
	pushable_regs = 1 << thumb_find_work_register (live_regs_mask);

      while (high_regs_pushed > 0)
	{
	  unsigned long real_regs_mask = 0;

	  for (regno = LAST_LO_REGNUM; regno >= 0; regno --)
	    {
	      if (pushable_regs & (1 << regno))
		{
		  bytes += 2;
		  if (emit)
		    asm_fprintf (f, "\tmov\t%r, %r\n", regno, next_hi_reg);

		  high_regs_pushed --;
		  real_regs_mask |= (1 << next_hi_reg);

		  if (high_regs_pushed)
		    {
		      for (next_hi_reg --; next_hi_reg > LAST_LO_REGNUM;
			   next_hi_reg --)
			if (live_regs_mask & (1 << next_hi_reg))
			  break;
		    }
		  else
		    {
		      pushable_regs &= ~((1 << regno) - 1);
		      break;
		    }
		}
	    }

	  /* If we had to find a work register and we have not yet
	     saved the LR then add it to the list of regs to push.  */
	  if (l_mask == (1 << LR_REGNUM))
	    {
	      bytes += handle_thumb_pushpop 
			 (f, pushable_regs | (1 << LR_REGNUM),
			  1, &cfa_offset,
			  real_regs_mask | (1 << LR_REGNUM), emit);
	      l_mask = 0;
	    }
	  else
	    bytes += handle_thumb_pushpop (f, pushable_regs, 1, &cfa_offset, real_regs_mask, emit);
	}
    }
  /* APPLE LOCAL begin 6465387 exception handling interworking VFP save */
  if (current_function_has_nonlocal_label && arm_arch6)
    {
      bytes += 4;
      if (emit)
        asm_fprintf (f, "\tblx ___save_vfp_d8_d15_regs\n");
    }
  /* APPLE LOCAL end 6465387 exception handling interworking VFP save */
  return bytes;
}

static void
/* ALQAAHIRA LOCAL v7 support. Merge from mainline */
thumb1_output_function_prologue (FILE *f, HOST_WIDE_INT size ATTRIBUTE_UNUSED)
{
  (void) handle_thumb_unexpanded_prologue (f, true);
}

int count_thumb_unexpanded_prologue (void)
{
  return handle_thumb_unexpanded_prologue (NULL, false);
}
/* APPLE LOCAL end ARM compact switch tables */

/* Handle the case of a double word load into a low register from
   a computed memory address.  The computed address may involve a
   register which is overwritten by the load.  */
const char *
thumb_load_double_from_address (rtx *operands)
{
  rtx addr;
  rtx base;
  rtx offset;
  rtx arg1;
  rtx arg2;

  gcc_assert (GET_CODE (operands[0]) == REG);
  gcc_assert (GET_CODE (operands[1]) == MEM);

  /* Get the memory address.  */
  addr = XEXP (operands[1], 0);

  /* Work out how the memory address is computed.  */
  switch (GET_CODE (addr))
    {
    case REG:
      operands[2] = adjust_address (operands[1], SImode, 4);

      if (REGNO (operands[0]) == REGNO (addr))
	{
	  output_asm_insn ("ldr\t%H0, %2", operands);
	  output_asm_insn ("ldr\t%0, %1", operands);
	}
      else
	{
	  output_asm_insn ("ldr\t%0, %1", operands);
	  output_asm_insn ("ldr\t%H0, %2", operands);
	}
      break;

    case CONST:
      /* Compute <address> + 4 for the high order load.  */
      operands[2] = adjust_address (operands[1], SImode, 4);

      output_asm_insn ("ldr\t%0, %1", operands);
      output_asm_insn ("ldr\t%H0, %2", operands);
      break;

    case PLUS:
      arg1   = XEXP (addr, 0);
      arg2   = XEXP (addr, 1);

      if (CONSTANT_P (arg1))
	base = arg2, offset = arg1;
      else
	base = arg1, offset = arg2;

      gcc_assert (GET_CODE (base) == REG);

      /* Catch the case of <address> = <reg> + <reg> */
      if (GET_CODE (offset) == REG)
	{
	  /* APPLE LOCAL begin ARM compact switch tables */
	  /* thumb_legitimate_address_p won't allow this form,
	     and allowing a 3-instruction variant confuses
	     our instruction length counts, so remove it.
	     Details in rdar://5435967.  */
	  gcc_unreachable();
	  /* APPLE LOCAL end ARM compact switch tables */
	}
      else
	{
	  /* Compute <address> + 4 for the high order load.  */
	  operands[2] = adjust_address (operands[1], SImode, 4);

	  /* If the computed address is held in the low order register
	     then load the high order register first, otherwise always
	     load the low order register first.  */
	  if (REGNO (operands[0]) == REGNO (base))
	    {
	      output_asm_insn ("ldr\t%H0, %2", operands);
	      output_asm_insn ("ldr\t%0, %1", operands);
	    }
	  else
	    {
	      output_asm_insn ("ldr\t%0, %1", operands);
	      output_asm_insn ("ldr\t%H0, %2", operands);
	    }
	}
      break;

    case LABEL_REF:
      /* With no registers to worry about we can just load the value
         directly.  */
      operands[2] = adjust_address (operands[1], SImode, 4);

      output_asm_insn ("ldr\t%H0, %2", operands);
      output_asm_insn ("ldr\t%0, %1", operands);
      break;

    default:
      gcc_unreachable ();
    }

  return "";
}

const char *
thumb_output_move_mem_multiple (int n, rtx *operands)
{
  rtx tmp;

  switch (n)
    {
    case 2:
      if (REGNO (operands[4]) > REGNO (operands[5]))
	{
	  tmp = operands[4];
	  operands[4] = operands[5];
	  operands[5] = tmp;
	}
      output_asm_insn ("ldmia\t%1!, {%4, %5}", operands);
      output_asm_insn ("stmia\t%0!, {%4, %5}", operands);
      break;

    case 3:
      if (REGNO (operands[4]) > REGNO (operands[5]))
	{
	  tmp = operands[4];
	  operands[4] = operands[5];
	  operands[5] = tmp;
	}
      if (REGNO (operands[5]) > REGNO (operands[6]))
	{
	  tmp = operands[5];
	  operands[5] = operands[6];
	  operands[6] = tmp;
	}
      if (REGNO (operands[4]) > REGNO (operands[5]))
	{
	  tmp = operands[4];
	  operands[4] = operands[5];
	  operands[5] = tmp;
	}

      output_asm_insn ("ldmia\t%1!, {%4, %5, %6}", operands);
      output_asm_insn ("stmia\t%0!, {%4, %5, %6}", operands);
      break;

    default:
      gcc_unreachable ();
    }

  return "";
}

/* Output a call-via instruction for thumb state.  */
const char *
thumb_call_via_reg (rtx reg)
{
  int regno = REGNO (reg);
  rtx *labelp;

  gcc_assert (regno < LR_REGNUM);

  /* If we are in the normal text section we can use a single instance
     per compilation unit.  If we are doing function sections, then we need
     an entry per section, since we can't rely on reachability.  */
  if (in_section == text_section)
    {
      thumb_call_reg_needed = 1;

      if (thumb_call_via_label[regno] == NULL)
	thumb_call_via_label[regno] = gen_label_rtx ();
      labelp = thumb_call_via_label + regno;
    }
  else
    {
      if (cfun->machine->call_via[regno] == NULL)
	cfun->machine->call_via[regno] = gen_label_rtx ();
      labelp = cfun->machine->call_via + regno;
    }

  output_asm_insn ("bl\t%a0", labelp);
  return "";
}

/* Routines for generating rtl.  */
void
thumb_expand_movmemqi (rtx *operands)
{
  rtx out = copy_to_mode_reg (SImode, XEXP (operands[0], 0));
  rtx in  = copy_to_mode_reg (SImode, XEXP (operands[1], 0));
  HOST_WIDE_INT len = INTVAL (operands[2]);
  HOST_WIDE_INT offset = 0;

  while (len >= 12)
    {
      emit_insn (gen_movmem12b (out, in, out, in));
      len -= 12;
    }

  if (len >= 8)
    {
      emit_insn (gen_movmem8b (out, in, out, in));
      len -= 8;
    }

  if (len >= 4)
    {
      rtx reg = gen_reg_rtx (SImode);
      emit_insn (gen_movsi (reg, gen_rtx_MEM (SImode, in)));
      emit_insn (gen_movsi (gen_rtx_MEM (SImode, out), reg));
      len -= 4;
      offset += 4;
    }

  if (len >= 2)
    {
      rtx reg = gen_reg_rtx (HImode);
      emit_insn (gen_movhi (reg, gen_rtx_MEM (HImode,
					      plus_constant (in, offset))));
      emit_insn (gen_movhi (gen_rtx_MEM (HImode, plus_constant (out, offset)),
			    reg));
      len -= 2;
      offset += 2;
    }

  if (len)
    {
      rtx reg = gen_reg_rtx (QImode);
      emit_insn (gen_movqi (reg, gen_rtx_MEM (QImode,
					      plus_constant (in, offset))));
      emit_insn (gen_movqi (gen_rtx_MEM (QImode, plus_constant (out, offset)),
			    reg));
    }
}

void
thumb_reload_out_hi (rtx *operands)
{
  emit_insn (gen_thumb_movhi_clobber (operands[0], operands[1], operands[2]));
}

/* Handle reading a half-word from memory during reload.  */
void
thumb_reload_in_hi (rtx *operands ATTRIBUTE_UNUSED)
{
  gcc_unreachable ();
}

/* Return the length of a function name prefix
    that starts with the character 'c'.  */
static int
arm_get_strip_length (int c)
{
  switch (c)
    {
    ARM_NAME_ENCODING_LENGTHS
      default: return 0;
    }
}

/* Return a pointer to a function's name with any
   and all prefix encodings stripped from it.  */
const char *
arm_strip_name_encoding (const char *name)
{
  int skip;

  while ((skip = arm_get_strip_length (* name)))
    name += skip;

  return name;
}

/* If there is a '*' anywhere in the name's prefix, then
   emit the stripped name verbatim, otherwise prepend an
   underscore if leading underscores are being used.  */
void
arm_asm_output_labelref (FILE *stream, const char *name)
{
  int skip;
  int verbatim = 0;

  while ((skip = arm_get_strip_length (* name)))
    {
      verbatim |= (*name == '*');
      name += skip;
    }

  if (verbatim)
    fputs (name, stream);
  else
    asm_fprintf (stream, "%U%s", name);
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
static void
arm_file_start (void)
{
  int val;

  if (TARGET_UNIFIED_ASM)
    asm_fprintf (asm_out_file, "\t.syntax unified\n");

  if (TARGET_BPABI)
    {
      const char *fpu_name;
      if (arm_select[0].string)
	asm_fprintf (asm_out_file, "\t.cpu %s\n", arm_select[0].string);
      else if (arm_select[1].string)
	asm_fprintf (asm_out_file, "\t.arch %s\n", arm_select[1].string);
      else
	asm_fprintf (asm_out_file, "\t.cpu %s\n",
		     all_cores[arm_default_cpu].name);

      if (TARGET_SOFT_FLOAT)
	{
	  if (TARGET_VFP)
	    fpu_name = "softvfp";
	  else
	    fpu_name = "softfpa";
	}
      else
	{
	  int set_float_abi_attributes = 0;
	  switch (arm_fpu_arch)
	    {
	    case FPUTYPE_FPA:
	      fpu_name = "fpa";
	      break;
	    case FPUTYPE_FPA_EMU2:
	      fpu_name = "fpe2";
	      break;
	    case FPUTYPE_FPA_EMU3:
	      fpu_name = "fpe3";
	      break;
	    case FPUTYPE_MAVERICK:
	      fpu_name = "maverick";
	      break;
	    case FPUTYPE_VFP:
	      fpu_name = "vfp";
	      set_float_abi_attributes = 1;
	      break;
	    case FPUTYPE_VFP3:
	      fpu_name = "vfp3";
	      set_float_abi_attributes = 1;
	      break;
	    case FPUTYPE_NEON:
	      fpu_name = "neon";
	      set_float_abi_attributes = 1;
	      break;
	    default:
	      abort();
	    }
	  if (set_float_abi_attributes)
	    {
	      if (TARGET_HARD_FLOAT)
		asm_fprintf (asm_out_file, "\t.eabi_attribute 27, 3\n");
	      if (TARGET_HARD_FLOAT_ABI)
		asm_fprintf (asm_out_file, "\t.eabi_attribute 28, 1\n");
	    }
	}
      asm_fprintf (asm_out_file, "\t.fpu %s\n", fpu_name);

      /* Some of these attributes only apply when the corresponding features
         are used.  However we don't have any easy way of figuring this out.
	 Conservatively record the setting that would have been used.  */

      /* Tag_ABI_PCS_wchar_t.  */
      asm_fprintf (asm_out_file, "\t.eabi_attribute 18, %d\n",
		   (int)WCHAR_TYPE_SIZE / BITS_PER_UNIT);

      /* Tag_ABI_FP_rounding.  */
      if (flag_rounding_math)
	asm_fprintf (asm_out_file, "\t.eabi_attribute 19, 1\n");
      if (!flag_unsafe_math_optimizations)
	{
	  /* Tag_ABI_FP_denomal.  */
	  asm_fprintf (asm_out_file, "\t.eabi_attribute 20, 1\n");
	  /* Tag_ABI_FP_exceptions.  */
	  asm_fprintf (asm_out_file, "\t.eabi_attribute 21, 1\n");
	}
      /* Tag_ABI_FP_user_exceptions.  */
      if (flag_signaling_nans)
	asm_fprintf (asm_out_file, "\t.eabi_attribute 22, 1\n");
      /* Tag_ABI_FP_number_model.  */
      asm_fprintf (asm_out_file, "\t.eabi_attribute 23, %d\n", 
		   flag_finite_math_only ? 1 : 3);

      /* Tag_ABI_align8_needed.  */
      asm_fprintf (asm_out_file, "\t.eabi_attribute 24, 1\n");
      /* Tag_ABI_align8_preserved.  */
      asm_fprintf (asm_out_file, "\t.eabi_attribute 25, 1\n");
      /* Tag_ABI_enum_size.  */
      asm_fprintf (asm_out_file, "\t.eabi_attribute 26, %d\n",
		   flag_short_enums ? 1 : 2);

      /* Tag_ABI_optimization_goals.  */
      if (optimize_size)
	val = 4;
      else if (optimize >= 2)
	val = 2;
      else if (optimize)
	val = 1;
      else
	val = 6;
      asm_fprintf (asm_out_file, "\t.eabi_attribute 30, %d\n", val);
    }
  /* ALQAAHIRA LOCAL 6345234 begin place text sections together */
#if TARGET_MACHO
  /* Emit declarations for all code sections at the beginning of the file; 
     this keeps them from being separated by data sections, which can 
     lead to out-of-range branches. */
  if (flag_pic || MACHO_DYNAMIC_NO_PIC_P)
    {
      fprintf (asm_out_file, "\t.section __TEXT,__text,regular\n");
      fprintf (asm_out_file, "\t.section __TEXT,__textcoal_nt,coalesced\n");
      fprintf (asm_out_file, "\t.section __TEXT,__const_coal,coalesced\n");
      if (MACHO_DYNAMIC_NO_PIC_P )
        fprintf (asm_out_file, 
                 "\t.section __TEXT,__symbol_stub4,symbol_stubs,none,12\n");
      else
        fprintf (asm_out_file, 
                 "\t.section __TEXT,__picsymbolstub4,symbol_stubs,none,16\n");
    }
#endif
  /* ALQAAHIRA LOCAL 6345234 end place text sections together */
  default_file_start();
}

/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
static void
arm_file_end (void)
{
  int regno;

  if (! thumb_call_reg_needed)
    return;

  switch_to_section (text_section);
  asm_fprintf (asm_out_file, "\t.code 16\n");
  ASM_OUTPUT_ALIGN (asm_out_file, 1);

  for (regno = 0; regno < LR_REGNUM; regno++)
    {
      rtx label = thumb_call_via_label[regno];

      if (label != 0)
	{
	  targetm.asm_out.internal_label (asm_out_file, "L",
					  CODE_LABEL_NUMBER (label));
	  asm_fprintf (asm_out_file, "\tbx\t%r\n", regno);
	}
    }
}

/* APPLE LOCAL begin ARM asm file hooks */
#if TARGET_MACHO
static void
arm_darwin_file_start (void)
{
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  arm_file_start();
  darwin_file_start();
}

static void
arm_darwin_file_end (void)
{
  darwin_file_end ();
  arm_file_end ();
}
#endif
/* APPLE LOCAL end ARM asm file hooks */

rtx aof_pic_label;

#ifdef AOF_ASSEMBLER
/* Special functions only needed when producing AOF syntax assembler.  */

struct pic_chain
{
  struct pic_chain * next;
  const char * symname;
};

static struct pic_chain * aof_pic_chain = NULL;

rtx
aof_pic_entry (rtx x)
{
  struct pic_chain ** chainp;
  int offset;

  if (aof_pic_label == NULL_RTX)
    {
      aof_pic_label = gen_rtx_SYMBOL_REF (Pmode, "x$adcons");
    }

  for (offset = 0, chainp = &aof_pic_chain; *chainp;
       offset += 4, chainp = &(*chainp)->next)
    if ((*chainp)->symname == XSTR (x, 0))
      return plus_constant (aof_pic_label, offset);

  *chainp = (struct pic_chain *) xmalloc (sizeof (struct pic_chain));
  (*chainp)->next = NULL;
  (*chainp)->symname = XSTR (x, 0);
  return plus_constant (aof_pic_label, offset);
}

void
aof_dump_pic_table (FILE *f)
{
  struct pic_chain * chain;

  if (aof_pic_chain == NULL)
    return;

  asm_fprintf (f, "\tAREA |%r$$adcons|, BASED %r\n",
	       PIC_OFFSET_TABLE_REGNUM,
	       PIC_OFFSET_TABLE_REGNUM);
  fputs ("|x$adcons|\n", f);

  for (chain = aof_pic_chain; chain; chain = chain->next)
    {
      fputs ("\tDCD\t", f);
      assemble_name (f, chain->symname);
      fputs ("\n", f);
    }
}

int arm_text_section_count = 1;

/* A get_unnamed_section callback for switching to the text section.  */

static void
aof_output_text_section_asm_op (const void *data ATTRIBUTE_UNUSED)
{
  fprintf (asm_out_file, "\tAREA |C$$code%d|, CODE, READONLY",
	   arm_text_section_count++);
  if (flag_pic)
    fprintf (asm_out_file, ", PIC, REENTRANT");
  fprintf (asm_out_file, "\n");
}

static int arm_data_section_count = 1;

/* A get_unnamed_section callback for switching to the data section.  */

static void
aof_output_data_section_asm_op (const void *data ATTRIBUTE_UNUSED)
{
  fprintf (asm_out_file, "\tAREA |C$$data%d|, DATA\n",
	   arm_data_section_count++);
}

/* Implement TARGET_ASM_INIT_SECTIONS.

   AOF Assembler syntax is a nightmare when it comes to areas, since once
   we change from one area to another, we can't go back again.  Instead,
   we must create a new area with the same attributes and add the new output
   to that.  Unfortunately, there is nothing we can do here to guarantee that
   two areas with the same attributes will be linked adjacently in the
   resulting executable, so we have to be careful not to do pc-relative
   addressing across such boundaries.  */

static void
aof_asm_init_sections (void)
{
  text_section = get_unnamed_section (SECTION_CODE,
				      aof_output_text_section_asm_op, NULL);
  data_section = get_unnamed_section (SECTION_WRITE,
				      aof_output_data_section_asm_op, NULL);
  readonly_data_section = text_section;
}

void
zero_init_section (void)
{
  static int zero_init_count = 1;

  fprintf (asm_out_file, "\tAREA |C$$zidata%d|,NOINIT\n", zero_init_count++);
  in_section = NULL;
}

/* The AOF assembler is religiously strict about declarations of
   imported and exported symbols, so that it is impossible to declare
   a function as imported near the beginning of the file, and then to
   export it later on.  It is, however, possible to delay the decision
   until all the functions in the file have been compiled.  To get
   around this, we maintain a list of the imports and exports, and
   delete from it any that are subsequently defined.  At the end of
   compilation we spit the remainder of the list out before the END
   directive.  */

struct import
{
  struct import * next;
  const char * name;
};

static struct import * imports_list = NULL;

void
aof_add_import (const char *name)
{
  struct import * new;

  for (new = imports_list; new; new = new->next)
    if (new->name == name)
      return;

  new = (struct import *) xmalloc (sizeof (struct import));
  new->next = imports_list;
  imports_list = new;
  new->name = name;
}

void
aof_delete_import (const char *name)
{
  struct import ** old;

  for (old = &imports_list; *old; old = & (*old)->next)
    {
      if ((*old)->name == name)
	{
	  *old = (*old)->next;
	  return;
	}
    }
}

int arm_main_function = 0;

static void
aof_dump_imports (FILE *f)
{
  /* The AOF assembler needs this to cause the startup code to be extracted
     from the library.  Brining in __main causes the whole thing to work
     automagically.  */
  if (arm_main_function)
    {
      switch_to_section (text_section);
      fputs ("\tIMPORT __main\n", f);
      fputs ("\tDCD __main\n", f);
    }

  /* Now dump the remaining imports.  */
  while (imports_list)
    {
      fprintf (f, "\tIMPORT\t");
      assemble_name (f, imports_list->name);
      fputc ('\n', f);
      imports_list = imports_list->next;
    }
}

static void
aof_globalize_label (FILE *stream, const char *name)
{
  default_globalize_label (stream, name);
  if (! strcmp (name, "main"))
    arm_main_function = 1;
}

static void
aof_file_start (void)
{
  fputs ("__r0\tRN\t0\n", asm_out_file);
  fputs ("__a1\tRN\t0\n", asm_out_file);
  fputs ("__a2\tRN\t1\n", asm_out_file);
  fputs ("__a3\tRN\t2\n", asm_out_file);
  fputs ("__a4\tRN\t3\n", asm_out_file);
  fputs ("__v1\tRN\t4\n", asm_out_file);
  fputs ("__v2\tRN\t5\n", asm_out_file);
  fputs ("__v3\tRN\t6\n", asm_out_file);
  fputs ("__v4\tRN\t7\n", asm_out_file);
  fputs ("__v5\tRN\t8\n", asm_out_file);
  fputs ("__v6\tRN\t9\n", asm_out_file);
  fputs ("__sl\tRN\t10\n", asm_out_file);
  fputs ("__fp\tRN\t11\n", asm_out_file);
  fputs ("__ip\tRN\t12\n", asm_out_file);
  fputs ("__sp\tRN\t13\n", asm_out_file);
  fputs ("__lr\tRN\t14\n", asm_out_file);
  fputs ("__pc\tRN\t15\n", asm_out_file);
  fputs ("__f0\tFN\t0\n", asm_out_file);
  fputs ("__f1\tFN\t1\n", asm_out_file);
  fputs ("__f2\tFN\t2\n", asm_out_file);
  fputs ("__f3\tFN\t3\n", asm_out_file);
  fputs ("__f4\tFN\t4\n", asm_out_file);
  fputs ("__f5\tFN\t5\n", asm_out_file);
  fputs ("__f6\tFN\t6\n", asm_out_file);
  fputs ("__f7\tFN\t7\n", asm_out_file);
  switch_to_section (text_section);
}

static void
aof_file_end (void)
{
  if (flag_pic)
    aof_dump_pic_table (asm_out_file);
  arm_file_end ();
  aof_dump_imports (asm_out_file);
  fputs ("\tEND\n", asm_out_file);
}
#endif /* AOF_ASSEMBLER */

/* APPLE LOCAL ARM darwin section_info */
#if !defined(ARM_PE) && !TARGET_MACHO
/* Symbols in the text segment can be accessed without indirecting via the
   constant pool; it may take an extra binary operation, but this is still
   faster than indirecting via memory.  Don't do this when not optimizing,
   since we won't be calculating al of the offsets necessary to do this
   simplification.  */

static void
arm_encode_section_info (tree decl, rtx rtl, int first)
{
  /* This doesn't work with AOF syntax, since the string table may be in
     a different AREA.  */
#ifndef AOF_ASSEMBLER
  if (optimize > 0 && TREE_CONSTANT (decl))
    SYMBOL_REF_FLAG (XEXP (rtl, 0)) = 1;
#endif

  /* If we are referencing a function that is weak then encode a long call
     flag in the function name, otherwise if the function is static or
     or known to be defined in this file then encode a short call flag.  */
  if (first && DECL_P (decl))
    {
      if (TREE_CODE (decl) == FUNCTION_DECL && DECL_WEAK (decl))
        arm_encode_call_attribute (decl, LONG_CALL_FLAG_CHAR);
      else if (! TREE_PUBLIC (decl))
        arm_encode_call_attribute (decl, SHORT_CALL_FLAG_CHAR);
    }

  default_encode_section_info (decl, rtl, first);
}
/* APPLE LOCAL begin ARM darwin section_info */
#endif /* !ARM_PE && !TARGET_MACHO*/

#if TARGET_MACHO
/* Encode the standard darwin attributes, plus the longcall flag.  */

static void
arm_darwin_encode_section_info (tree decl, rtx rtl, int first)
{
  darwin_encode_section_info (decl, rtl, first);

  if (optimize > 0 && TREE_CONSTANT (decl))
    SYMBOL_REF_FLAG (XEXP (rtl, 0)) = 1;

  /* If we are referencing a function with default visibility that is
     weak then encode a long call flag in the function name, otherwise
     if the function is static or or known to be defined in this file
     then encode a short call flag.  */
  if (DECL_P (decl))
    {
      if (TREE_CODE (decl) == FUNCTION_DECL
	  && DECL_WEAK (decl)
	  && DECL_VISIBILITY (decl) == VISIBILITY_DEFAULT)
        arm_encode_call_attribute (decl, SYMBOL_LONG_CALL);
      /* Should this be binds_local_p???  */
      else if (! TREE_PUBLIC (decl))
        arm_encode_call_attribute (decl, SYMBOL_SHORT_CALL);
    }
}
#endif
/* APPLE LOCAL end ARM darwin section_info */

static void
arm_internal_label (FILE *stream, const char *prefix, unsigned long labelno)
{
  if (arm_ccfsm_state == 3 && (unsigned) arm_target_label == labelno
      && !strcmp (prefix, "L"))
    {
      arm_ccfsm_state = 0;
      arm_target_insn = NULL;
    }
  default_internal_label (stream, prefix, labelno);
}

/* Output code to add DELTA to the first argument, and then jump
   to FUNCTION.  Used for C++ multiple inheritance.  */
static void
arm_output_mi_thunk (FILE *file, tree thunk ATTRIBUTE_UNUSED,
		     HOST_WIDE_INT delta,
		     HOST_WIDE_INT vcall_offset ATTRIBUTE_UNUSED,
		     tree function)
{
  /* APPLE LOCAL begin ARM 4620953 4745175 5920116 */
  static int thunk_label = 0;
  char label[256];
  char labelpc[256];
  int mi_delta = delta;
  const char *const mi_op = mi_delta < 0 ? "sub" : "add";
  int shift = 0;
  int this_regno = (aggregate_value_p (TREE_TYPE (TREE_TYPE (function)), function)
                    ? 1 : 0);
  rtx function_rtx = XEXP (DECL_RTL (function), 0);
  const char *function_name;
  bool is_longcall = arm_is_longcall_p (function_rtx,
					SYMBOL_REF_FLAGS (function_rtx),
					1);
  bool is_indirected = false;
    

  /* Darwin/mach-o: use a stub for dynamic references.  */
#if TARGET_MACHO
  if (TARGET_MACHO
      && MACHOPIC_INDIRECT
      && (! machopic_data_defined_p (function_rtx)))
    {
      function_name = machopic_indirection_name (function_rtx, !is_longcall);
      /* ALQAAHIRA LOCAL 6858124 don't indirect if it's just a stub */
      is_indirected = is_longcall;
    }
  else
#endif
    function_name = XSTR (function_rtx, 0);

  if (mi_delta < 0)
    mi_delta = - mi_delta;
  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  /* ALQAAHIRA LOCAL 6361608 begin Thumb-2 longcall thunks */
  /* When generating 16-bit thumb-1 code, thunks are entered in arm mode.
     In thumb-2, thunks can be in thumb mode.  */
  /* ALQAAHIRA LOCAL 6361608 end Thumb-2 longcall thunks */
  if (TARGET_THUMB1 || is_longcall)
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
    {
      int labelno = thunk_label++;
      ASM_GENERATE_INTERNAL_LABEL (label, "LTHUMBFUNC", labelno);
      fputs ("\tldr\tr12, ", file);
      assemble_name (file, label);
      fputc ('\n', file);
      if (flag_pic)
	{
	  /* If we are generating PIC, the ldr instruction below loads
	     "(target - 7) - .LTHUNKPCn" into r12.  The pc reads as
	     the address of the add + 8, so we have:

	     r12 = (target - 7) - .LTHUNKPCn + (.LTHUNKPCn + 8)
	         = target + 1.

	     Note that we have "+ 1" because some versions of GNU ld
	     don't set the low bit of the result for R_ARM_REL32
	     relocations against thumb function symbols.  */
	  ASM_GENERATE_INTERNAL_LABEL (labelpc, "LTHUNKPC", labelno);
	  assemble_name (file, labelpc);
	  fputs (":\n", file);
	  fputs ("\tadd\tr12, pc, r12\n", file);
	}
      if (is_indirected)
	fputs ("\tldr\tr12, [r12]\n", file);
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  /* TODO: Use movw/movt for large constants when available.  */
  while (mi_delta != 0)
    {
      if ((mi_delta & (3 << shift)) == 0)
        shift += 2;
      else
        {
          asm_fprintf (file, "\t%s\t%r, %r, #%d\n",
                       mi_op, this_regno, this_regno,
                       mi_delta & (0xff << shift));
          mi_delta &= ~(0xff << shift);
          shift += 8;
        }
    }
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  if (TARGET_THUMB1 || is_longcall)
    {
      fprintf (file, "\tbx\tr12\n");
      ASM_OUTPUT_ALIGN (file, 2);
      assemble_name (file, label);
      fputs (":\n", file);
      if (flag_pic)
	{
          /* ALQAAHIRA LOCAL 6361608 begin Thumb-2 longcall thunks */
          int pc_offset;
	  /* If we're branching to a local Thumb routine, output:
	       ".word .LTHUNKn-7-.LTHUNKPCn".
	     Otherwise, output:
	       ".word .LTHUNKn-8-.LTHUNKPCn".
	     (inter-module thumbness is fixed up by the linker).  
             If we're in a Thumb2 thunk, it's -4 and -3, respectively.  */
	  rtx tem = gen_rtx_SYMBOL_REF (Pmode, function_name);

          /* Thumb2 add instructions w/ PC source have a +4 bias. ARM
             mode has +8. */
          pc_offset = TARGET_THUMB2 ? -4 : -8;
	  if (TARGET_MACHO && (TARGET_ARM || is_indirected))
	    tem = gen_rtx_PLUS (GET_MODE (tem), tem, GEN_INT (pc_offset));
	  else
	    tem = gen_rtx_PLUS (GET_MODE (tem), tem, GEN_INT (pc_offset+1));
          /* ALQAAHIRA LOCAL 6361608 end Thumb-2 longcall thunks */

	  tem = gen_rtx_MINUS (GET_MODE (tem),
			       tem,
			       gen_rtx_SYMBOL_REF (Pmode,
						   ggc_strdup (labelpc)));
	  assemble_integer (tem, 4, BITS_PER_WORD, 1);
	}
      else
	/* Output ".word .LTHUNKn".  */
	assemble_integer (gen_rtx_SYMBOL_REF (Pmode, function_name),
			  4, BITS_PER_WORD, 1);
    }
  else
    {
      /* ALQAAHIRA LOCAL begin 6297258 */
      if (TARGET_THUMB2)
	fputs ("\tb.w\t", file);
      else
        fputs ("\tb\t", file);
      /* ALQAAHIRA LOCAL end 6297258 */

      assemble_name (file, function_name);
      if (NEED_PLT_RELOC)
        fputs ("(PLT)", file);
      fputc ('\n', file);
    }
  /* APPLE LOCAL end ARM 4620953 4745175 5920116 */
}

int
arm_emit_vector_const (FILE *file, rtx x)
{
  int i;
  const char * pattern;

  gcc_assert (GET_CODE (x) == CONST_VECTOR);

  switch (GET_MODE (x))
    {
    case V2SImode: pattern = "%08x"; break;
    case V4HImode: pattern = "%04x"; break;
    case V8QImode: pattern = "%02x"; break;
    default:       gcc_unreachable ();
    }

  fprintf (file, "0x");
  for (i = CONST_VECTOR_NUNITS (x); i--;)
    {
      rtx element;

      element = CONST_VECTOR_ELT (x, i);
      fprintf (file, pattern, INTVAL (element));
    }

  return 1;
}

const char *
arm_output_load_gr (rtx *operands)
{
  rtx reg;
  rtx offset;
  rtx wcgr;
  rtx sum;

  if (GET_CODE (operands [1]) != MEM
      || GET_CODE (sum = XEXP (operands [1], 0)) != PLUS
      || GET_CODE (reg = XEXP (sum, 0)) != REG
      || GET_CODE (offset = XEXP (sum, 1)) != CONST_INT
      || ((INTVAL (offset) < 1024) && (INTVAL (offset) > -1024)))
    return "wldrw%?\t%0, %1";

  /* Fix up an out-of-range load of a GR register.  */
  output_asm_insn ("str%?\t%0, [sp, #-4]!\t@ Start of GR load expansion", & reg);
  wcgr = operands[0];
  operands[0] = reg;
  output_asm_insn ("ldr%?\t%0, %1", operands);

  operands[0] = wcgr;
  operands[1] = reg;
  output_asm_insn ("tmcr%?\t%0, %1", operands);
  output_asm_insn ("ldr%?\t%0, [sp], #4\t@ End of GR load expansion", & reg);

  return "";
}

/* Worker function for TARGET_SETUP_INCOMING_VARARGS.

   On the ARM, PRETEND_SIZE is set in order to have the prologue push the last
   named arg and all anonymous args onto the stack.
   XXX I know the prologue shouldn't be pushing registers, but it is faster
   that way.  */

static void
arm_setup_incoming_varargs (CUMULATIVE_ARGS *cum,
			    enum machine_mode mode ATTRIBUTE_UNUSED,
			    tree type ATTRIBUTE_UNUSED,
			    int *pretend_size,
			    int second_time ATTRIBUTE_UNUSED)
{
  cfun->machine->uses_anonymous_args = 1;
  if (cum->nregs < NUM_ARG_REGS)
    *pretend_size = (NUM_ARG_REGS - cum->nregs) * UNITS_PER_WORD;
}

/* Return nonzero if the CONSUMER instruction (a store) does not need
   PRODUCER's value to calculate the address.  */

int
arm_no_early_store_addr_dep (rtx producer, rtx consumer)
{
  rtx value = PATTERN (producer);
  rtx addr = PATTERN (consumer);

  if (GET_CODE (value) == COND_EXEC)
    value = COND_EXEC_CODE (value);
  if (GET_CODE (value) == PARALLEL)
    value = XVECEXP (value, 0, 0);
  value = XEXP (value, 0);
  if (GET_CODE (addr) == COND_EXEC)
    addr = COND_EXEC_CODE (addr);
  if (GET_CODE (addr) == PARALLEL)
    addr = XVECEXP (addr, 0, 0);
  addr = XEXP (addr, 0);

  return !reg_overlap_mentioned_p (value, addr);
}

/* Return nonzero if the CONSUMER instruction (an ALU op) does not
   have an early register shift value or amount dependency on the
   result of PRODUCER.  */

int
arm_no_early_alu_shift_dep (rtx producer, rtx consumer)
{
  rtx value = PATTERN (producer);
  rtx op = PATTERN (consumer);
  rtx early_op;

  if (GET_CODE (value) == COND_EXEC)
    value = COND_EXEC_CODE (value);
  if (GET_CODE (value) == PARALLEL)
    value = XVECEXP (value, 0, 0);
  value = XEXP (value, 0);
  if (GET_CODE (op) == COND_EXEC)
    op = COND_EXEC_CODE (op);
  if (GET_CODE (op) == PARALLEL)
    op = XVECEXP (op, 0, 0);
  op = XEXP (op, 1);

  early_op = XEXP (op, 0);
  /* This is either an actual independent shift, or a shift applied to
     the first operand of another operation.  We want the whole shift
     operation.  */
  if (GET_CODE (early_op) == REG)
    early_op = op;

  return !reg_overlap_mentioned_p (value, early_op);
}

/* Return nonzero if the CONSUMER instruction (an ALU op) does not
   have an early register shift value dependency on the result of
   PRODUCER.  */

int
arm_no_early_alu_shift_value_dep (rtx producer, rtx consumer)
{
  rtx value = PATTERN (producer);
  rtx op = PATTERN (consumer);
  rtx early_op;

  if (GET_CODE (value) == COND_EXEC)
    value = COND_EXEC_CODE (value);
  if (GET_CODE (value) == PARALLEL)
    value = XVECEXP (value, 0, 0);
  value = XEXP (value, 0);
  if (GET_CODE (op) == COND_EXEC)
    op = COND_EXEC_CODE (op);
  if (GET_CODE (op) == PARALLEL)
    op = XVECEXP (op, 0, 0);
  op = XEXP (op, 1);

  early_op = XEXP (op, 0);

  /* This is either an actual independent shift, or a shift applied to
     the first operand of another operation.  We want the value being
     shifted, in either case.  */
  if (GET_CODE (early_op) != REG)
    early_op = XEXP (early_op, 0);

  return !reg_overlap_mentioned_p (value, early_op);
}

/* Return nonzero if the CONSUMER (a mul or mac op) does not
   have an early register mult dependency on the result of
   PRODUCER.  */

int
arm_no_early_mul_dep (rtx producer, rtx consumer)
{
  rtx value = PATTERN (producer);
  rtx op = PATTERN (consumer);

  if (GET_CODE (value) == COND_EXEC)
    value = COND_EXEC_CODE (value);
  if (GET_CODE (value) == PARALLEL)
    value = XVECEXP (value, 0, 0);
  value = XEXP (value, 0);
  if (GET_CODE (op) == COND_EXEC)
    op = COND_EXEC_CODE (op);
  if (GET_CODE (op) == PARALLEL)
    op = XVECEXP (op, 0, 0);
  op = XEXP (op, 1);

  return (GET_CODE (op) == PLUS
	  && !reg_overlap_mentioned_p (value, XEXP (op, 0)));
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
/* Return non-zero if the consumer (a multiply-accumulate instruction)
   has an accumulator dependency on the result of the producer (a
   multiplication instruction) and no other dependency on that result.  */
int
arm_mac_accumulator_is_mul_result (rtx producer, rtx consumer)
{
  rtx mul = PATTERN (producer);
  rtx mac = PATTERN (consumer);
  rtx mul_result;
  rtx mac_op0, mac_op1, mac_acc;

  if (GET_CODE (mul) == COND_EXEC)
    mul = COND_EXEC_CODE (mul);
  if (GET_CODE (mac) == COND_EXEC)
    mac = COND_EXEC_CODE (mac);

  /* Check that mul is of the form (set (...) (mult ...))
     and mla is of the form (set (...) (plus (mult ...) (...))).  */
  if ((GET_CODE (mul) != SET || GET_CODE (XEXP (mul, 1)) != MULT)
      || (GET_CODE (mac) != SET || GET_CODE (XEXP (mac, 1)) != PLUS
          || GET_CODE (XEXP (XEXP (mac, 1), 0)) != MULT))
    return 0;

  mul_result = XEXP (mul, 0);
  mac_op0 = XEXP (XEXP (XEXP (mac, 1), 0), 0);
  mac_op1 = XEXP (XEXP (XEXP (mac, 1), 0), 1);
  mac_acc = XEXP (XEXP (mac, 1), 1);

  return (reg_overlap_mentioned_p (mul_result, mac_acc)
          && !reg_overlap_mentioned_p (mul_result, mac_op0)
          && !reg_overlap_mentioned_p (mul_result, mac_op1));
}


/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
/* We can't rely on the caller doing the proper promotion when
   using APCS or ATPCS.  */

static bool
arm_promote_prototypes (tree t ATTRIBUTE_UNUSED)
{
    return !TARGET_AAPCS_BASED;
}


/* AAPCS based ABIs use short enums by default.  */

static bool
arm_default_short_enums (void)
{
  return TARGET_AAPCS_BASED && arm_abi != ARM_ABI_AAPCS_LINUX;
}


/* AAPCS requires that anonymous bitfields affect structure alignment.  */

static bool
arm_align_anon_bitfield (void)
{
  return TARGET_AAPCS_BASED;
}


/* The generic C++ ABI says 64-bit (long long).  The EABI says 32-bit.  */

static tree
arm_cxx_guard_type (void)
{
  return TARGET_AAPCS_BASED ? integer_type_node : long_long_integer_type_node;
}


/* The EABI says test the least significant bit of a guard variable.  */

static bool
arm_cxx_guard_mask_bit (void)
{
  return TARGET_AAPCS_BASED;
}


/* The EABI specifies that all array cookies are 8 bytes long.  */

static tree
arm_get_cookie_size (tree type)
{
  tree size;

  if (!TARGET_AAPCS_BASED)
    return default_cxx_get_cookie_size (type);

  size = build_int_cst (sizetype, 8);
  return size;
}


/* The EABI says that array cookies should also contain the element size.  */

static bool
arm_cookie_has_size (void)
{
  return TARGET_AAPCS_BASED;
}


/* The EABI says constructors and destructors should return a pointer to
   the object constructed/destroyed.  */

static bool
arm_cxx_cdtor_returns_this (void)
{
  return TARGET_AAPCS_BASED;
}

/* The EABI says that an inline function may never be the key
   method.  */

static bool
arm_cxx_key_method_may_be_inline (void)
{
  return !TARGET_AAPCS_BASED;
}

static void
arm_cxx_determine_class_data_visibility (tree decl)
{
  if (!TARGET_AAPCS_BASED)
    return;

  /* In general, \S 3.2.5.5 of the ARM EABI requires that class data
     is exported.  However, on systems without dynamic vague linkage,
     \S 3.2.5.6 says that COMDAT class data has hidden linkage.  */
  if (!TARGET_ARM_DYNAMIC_VAGUE_LINKAGE_P && DECL_COMDAT (decl))
    DECL_VISIBILITY (decl) = VISIBILITY_HIDDEN;
  else
    DECL_VISIBILITY (decl) = VISIBILITY_DEFAULT;
  DECL_VISIBILITY_SPECIFIED (decl) = 1;
}

static bool
arm_cxx_class_data_always_comdat (void)
{
/* APPLE LOCAL begin ARM follow Darwin semantics on Darwin */
#if TARGET_MACHO
  return false;
#endif
/* APPLE LOCAL end ARM follow Darwin semantics on Darwin */

  /* \S 3.2.5.4 of the ARM C++ ABI says that class data only have
     vague linkage if the class has no key function.  */
  return !TARGET_AAPCS_BASED;
}


/* The EABI says __aeabi_atexit should be used to register static
   destructors.  */

static bool
arm_cxx_use_aeabi_atexit (void)
{
  return TARGET_AAPCS_BASED;
}


void
arm_set_return_address (rtx source, rtx scratch)
{
  arm_stack_offsets *offsets;
  HOST_WIDE_INT delta;
  rtx addr;
  unsigned long saved_regs;

  saved_regs = arm_compute_save_reg_mask ();

  if ((saved_regs & (1 << LR_REGNUM)) == 0)
    emit_move_insn (gen_rtx_REG (Pmode, LR_REGNUM), source);
  else
    {
      if (frame_pointer_needed)
	/* APPLE LOCAL ARM custom frame layout */
	addr = plus_constant(hard_frame_pointer_rtx, 4);
      else
	{
	  /* LR will be the first saved register.  */
	  offsets = arm_get_frame_offsets ();
	  /* APPLE LOCAL ARM custom frame layout */
	  delta = offsets->outgoing_args - (offsets->frame - 4);


	  if (delta >= 4096)
	    {
	      emit_insn (gen_addsi3 (scratch, stack_pointer_rtx,
				     GEN_INT (delta & ~4095)));
	      addr = scratch;
	      delta &= 4095;
	    }
	  else
	    addr = stack_pointer_rtx;

	  addr = plus_constant (addr, delta);
	}
      emit_move_insn (gen_frame_mem (Pmode, addr), source);
    }
}


void
thumb_set_return_address (rtx source, rtx scratch)
{
  arm_stack_offsets *offsets;
  HOST_WIDE_INT delta;
  int reg;
  rtx addr;
  unsigned long mask;

  emit_insn (gen_rtx_USE (VOIDmode, source));

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  mask = thumb1_compute_save_reg_mask ();
  if (mask & (1 << LR_REGNUM))
    {
      offsets = arm_get_frame_offsets ();

      /* Find the saved regs.  */
      if (frame_pointer_needed)
	{
	  /* APPLE LOCAL ARM custom frame layout */
	  delta = 4;
	  reg = THUMB_HARD_FRAME_POINTER_REGNUM;
	}
      else
	{
	  /* APPLE LOCAL ARM custom frame layout */
	  delta = offsets->outgoing_args - (offsets->saved_args + 4);
	  reg = SP_REGNUM;
	}
      /* Allow for the stack frame.  */
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      if (TARGET_THUMB1 && TARGET_BACKTRACE)
	delta -= 16;
      /* APPLE LOCAL ARM custom frame layout */
      /* Removed lines.  */

      /* Construct the address.  */
      addr = gen_rtx_REG (SImode, reg);
      if ((reg != SP_REGNUM && delta >= 128)
	  || delta >= 1024)
	{
	  emit_insn (gen_movsi (scratch, GEN_INT (delta)));
	  emit_insn (gen_addsi3 (scratch, scratch, stack_pointer_rtx));
	  addr = scratch;
	}
      else
	addr = plus_constant (addr, delta);

      emit_move_insn (gen_frame_mem (Pmode, addr), source);
    }
  else
    emit_move_insn (gen_rtx_REG (Pmode, LR_REGNUM), source);
}

/* Implements target hook vector_mode_supported_p.  */
bool
arm_vector_mode_supported_p (enum machine_mode mode)
{
  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  /* Neon also supports V2SImode, etc. listed in the clause below.  */
  if (TARGET_NEON && (mode == V2SFmode || mode == V4SImode || mode == V8HImode
      || mode == V16QImode || mode == V4SFmode || mode == V2DImode))
    return true;

  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */
  if ((mode == V2SImode)
      || (mode == V4HImode)
      || (mode == V8QImode))
    return true;

  return false;
}

/* Implement TARGET_SHIFT_TRUNCATION_MASK.  SImode shifts use normal
   ARM insns and therefore guarantee that the shift count is modulo 256.
   DImode shifts (those implemented by lib1funcs.asm or by optabs.c)
   guarantee no particular behavior for out-of-range counts.  */

static unsigned HOST_WIDE_INT
arm_shift_truncation_mask (enum machine_mode mode)
{
  return mode == SImode ? 255 : 0;
}


/* Map internal gcc register numbers to DWARF2 register numbers.  */

unsigned int
arm_dbx_register_number (unsigned int regno)
{
  if (regno < 16)
    return regno;

  /* TODO: Legacy targets output FPA regs as registers 16-23 for backwards
     compatibility.  The EABI defines them as registers 96-103.  */
  if (IS_FPA_REGNUM (regno))
    return (TARGET_AAPCS_BASED ? 96 : 16) + regno - FIRST_FPA_REGNUM;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  if (IS_VFP_REGNUM (regno))
    {
      /* See comment in arm_dwarf_register_span.  */
      if (VFP_REGNO_OK_FOR_SINGLE (regno))
        /* APPLE LOCAL ARM 5757769 */
	return 256 + regno - FIRST_VFP_REGNUM;
      else
	  return 256 + (regno - FIRST_VFP_REGNUM) / 2;
    }
  /* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

  if (IS_IWMMXT_GR_REGNUM (regno))
    return 104 + regno - FIRST_IWMMXT_GR_REGNUM;

  if (IS_IWMMXT_REGNUM (regno))
    return 112 + regno - FIRST_IWMMXT_REGNUM;

  gcc_unreachable ();
}

/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
/* Dwarf models VFPv3 registers as 32 64-bit registers.
   GCC models tham as 64 32-bit registers, so we need to describe this to
   the DWARF generation code.  Other registers can use the default.  */
static rtx
arm_dwarf_register_span(rtx rtl)
{
    unsigned regno;
    int nregs;
    int i;
    rtx p;

    regno = REGNO (rtl);
    if (!IS_VFP_REGNUM (regno))
	return NULL_RTX;

    /* The EABI defines two VFP register ranges:
	  64-95: Legacy VFPv2 numbering for S0-S31 (obsolescent)
	  256-287: D0-D31
       The recommended encodings for s0-s31 is a DW_OP_bit_piece of the
       corresponding D register.  However gdb6.6 does not support this, so
       we use the legacy encodings.  We also use these encodings for D0-D15
       for compatibility with older debuggers.  */
    if (VFP_REGNO_OK_FOR_SINGLE (regno))
	return NULL_RTX;

    nregs = GET_MODE_SIZE (GET_MODE (rtl)) / 8;
    p = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc(nregs));
    regno = (regno - FIRST_VFP_REGNUM) / 2;
    for (i = 0; i < nregs; i++)
      XVECEXP (p, 0, i) = gen_rtx_REG (DImode, 256 + regno + i);

    return p;
}
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

#ifdef TARGET_UNWIND_INFO
/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Emit unwind directives for a store-multiple instruction or stack pointer
   push during alignment.
   These should only ever be generated by the function prologue code, so
   expect them to have a particular form.  */

static void
arm_unwind_emit_sequence (FILE * asm_out_file, rtx p)
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
{
  int i;
  HOST_WIDE_INT offset;
  HOST_WIDE_INT nregs;
  int reg_size;
  unsigned reg;
  unsigned lastreg;
  rtx e;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  e = XVECEXP (p, 0, 0);
  if (GET_CODE (e) != SET)
    abort ();

  /* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
  /* First insn will adjust the stack pointer.  */
  if (GET_CODE (e) != SET
      || GET_CODE (XEXP (e, 0)) != REG
      || REGNO (XEXP (e, 0)) != SP_REGNUM
      || GET_CODE (XEXP (e, 1)) != PLUS)
    abort ();

  offset = -INTVAL (XEXP (XEXP (e, 1), 1));
  nregs = XVECLEN (p, 0) - 1;

  /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
  reg = REGNO (XEXP (XVECEXP (p, 0, 1), 1));
  if (reg < 16)
    {
      /* The function prologue may also push pc, but not annotate it as it is
	 never restored.  We turn this into a stack pointer adjustment.  */
      if (nregs * 4 == offset - 4)
	{
	  fprintf (asm_out_file, "\t.pad #4\n");
	  offset -= 4;
	}
      reg_size = 4;
      fprintf (asm_out_file, "\t.save {");
    }
  else if (IS_VFP_REGNUM (reg))
    {
      reg_size = 8;
      fprintf (asm_out_file, "\t.vsave {");
    }
  /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
  else if (reg >= FIRST_FPA_REGNUM && reg <= LAST_FPA_REGNUM)
    {
      /* FPA registers are done differently.  */
      asm_fprintf (asm_out_file, "\t.save %r, %wd\n", reg, nregs);
      return;
    }
  else
    /* Unknown register type.  */
    abort ();

  /* If the stack increment doesn't match the size of the saved registers,
     something has gone horribly wrong.  */
  if (offset != nregs * reg_size)
    abort ();

  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  /* Removed lines */
  offset = 0;
  lastreg = 0;
  /* The remaining insns will describe the stores.  */
  for (i = 1; i <= nregs; i++)
    {
      /* Expect (set (mem <addr>) (reg)).
         Where <addr> is (reg:SP) or (plus (reg:SP) (const_int)).  */
      e = XVECEXP (p, 0, i);
      if (GET_CODE (e) != SET
	  || GET_CODE (XEXP (e, 0)) != MEM
	  || GET_CODE (XEXP (e, 1)) != REG)
	abort ();

      reg = REGNO (XEXP (e, 1));
      if (reg < lastreg)
	abort ();

      if (i != 1)
	fprintf (asm_out_file, ", ");
      /* We can't use %r for vfp because we need to use the
	 double precision register names.  */
      if (IS_VFP_REGNUM (reg))
	asm_fprintf (asm_out_file, "d%d", (reg - FIRST_VFP_REGNUM) / 2);
      else
	asm_fprintf (asm_out_file, "%r", reg);

#ifdef ENABLE_CHECKING
      /* Check that the addresses are consecutive.  */
      e = XEXP (XEXP (e, 0), 0);
      if (GET_CODE (e) == PLUS)
	{
	  offset += reg_size;
	  if (GET_CODE (XEXP (e, 0)) != REG
	      || REGNO (XEXP (e, 0)) != SP_REGNUM
	      || GET_CODE (XEXP (e, 1)) != CONST_INT
	      || offset != INTVAL (XEXP (e, 1)))
	    abort ();
	}
      else if (i != 1
	       || GET_CODE (e) != REG
	       || REGNO (e) != SP_REGNUM)
	abort ();
#endif
    }
  fprintf (asm_out_file, "}\n");
}

/*  Emit unwind directives for a SET.  */

static void
arm_unwind_emit_set (FILE * asm_out_file, rtx p)
{
  rtx e0;
  rtx e1;
  /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
  unsigned reg;

  e0 = XEXP (p, 0);
  e1 = XEXP (p, 1);
  switch (GET_CODE (e0))
    {
    case MEM:
      /* Pushing a single register.  */
      if (GET_CODE (XEXP (e0, 0)) != PRE_DEC
	  || GET_CODE (XEXP (XEXP (e0, 0), 0)) != REG
	  || REGNO (XEXP (XEXP (e0, 0), 0)) != SP_REGNUM)
	abort ();

      asm_fprintf (asm_out_file, "\t.save ");
      if (IS_VFP_REGNUM (REGNO (e1)))
	asm_fprintf(asm_out_file, "{d%d}\n",
		    (REGNO (e1) - FIRST_VFP_REGNUM) / 2);
      else
	asm_fprintf(asm_out_file, "{%r}\n", REGNO (e1));
      break;

    case REG:
      if (REGNO (e0) == SP_REGNUM)
	{
	  /* A stack increment.  */
	  if (GET_CODE (e1) != PLUS
	      || GET_CODE (XEXP (e1, 0)) != REG
	      || REGNO (XEXP (e1, 0)) != SP_REGNUM
	      || GET_CODE (XEXP (e1, 1)) != CONST_INT)
	    abort ();

	  asm_fprintf (asm_out_file, "\t.pad #%wd\n",
		       -INTVAL (XEXP (e1, 1)));
	}
      else if (REGNO (e0) == HARD_FRAME_POINTER_REGNUM)
	{
	  HOST_WIDE_INT offset;
          /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
          /* moved 'reg' to function level scope */

	  if (GET_CODE (e1) == PLUS)
	    {
	      if (GET_CODE (XEXP (e1, 0)) != REG
		  || GET_CODE (XEXP (e1, 1)) != CONST_INT)
		abort ();
	      reg = REGNO (XEXP (e1, 0));
	      offset = INTVAL (XEXP (e1, 1));
	      asm_fprintf (asm_out_file, "\t.setfp %r, %r, #%wd\n",
			   HARD_FRAME_POINTER_REGNUM, reg,
			   INTVAL (XEXP (e1, 1)));
	    }
	  else if (GET_CODE (e1) == REG)
	    {
	      reg = REGNO (e1);
	      asm_fprintf (asm_out_file, "\t.setfp %r, %r\n",
			   HARD_FRAME_POINTER_REGNUM, reg);
	    }
	  else
	    abort ();
	}
      else if (GET_CODE (e1) == REG && REGNO (e1) == SP_REGNUM)
	{
	  /* Move from sp to reg.  */
	  asm_fprintf (asm_out_file, "\t.movsp %r\n", REGNO (e0));
	}
     else if (GET_CODE (e1) == PLUS
	      && GET_CODE (XEXP (e1, 0)) == REG
	      && REGNO (XEXP (e1, 0)) == SP_REGNUM
	      && GET_CODE (XEXP (e1, 1)) == CONST_INT)
	{
	  /* Set reg to offset from sp.  */
	  asm_fprintf (asm_out_file, "\t.movsp %r, #%d\n",
		       REGNO (e0), (int)INTVAL(XEXP (e1, 1)));
	}
      /* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
      else if (GET_CODE (e1) == UNSPEC && XINT (e1, 1) == UNSPEC_STACK_ALIGN)
	{
	  /* Stack pointer save before alignment.  */
	  reg = REGNO (e0);
	  asm_fprintf (asm_out_file, "\t.unwind_raw 0, 0x%x @ vsp = r%d\n",
		       reg + 0x90, reg);
	}
      /* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
      else
	abort ();
      break;

    default:
      abort ();
    }
}


/* Emit unwind directives for the given insn.  */

static void
arm_unwind_emit (FILE * asm_out_file, rtx insn)
{
  rtx pat;

  if (!ARM_EABI_UNWIND_TABLES)
    return;

  if (GET_CODE (insn) == NOTE || !RTX_FRAME_RELATED_P (insn))
    return;

  pat = find_reg_note (insn, REG_FRAME_RELATED_EXPR, NULL_RTX);
  if (pat)
    pat = XEXP (pat, 0);
  else
    pat = PATTERN (insn);

  switch (GET_CODE (pat))
    {
    case SET:
      arm_unwind_emit_set (asm_out_file, pat);
      break;

    case SEQUENCE:
      /* Store multiple.  */
      /* ALQAAHIRA LOCAL v7 support. Merge from mainline */
      arm_unwind_emit_sequence (asm_out_file, pat);
      break;

    default:
      abort();
    }
}


/* Output a reference from a function exception table to the type_info
   object X.  The EABI specifies that the symbol should be relocated by
   an R_ARM_TARGET2 relocation.  */

static bool
arm_output_ttype (rtx x)
{
  fputs ("\t.word\t", asm_out_file);
  output_addr_const (asm_out_file, x);
  /* Use special relocations for symbol references.  */
  if (GET_CODE (x) != CONST_INT)
    fputs ("(TARGET2)", asm_out_file);
  fputc ('\n', asm_out_file);

  return TRUE;
}
#endif /* TARGET_UNWIND_INFO */


/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Handle UNSPEC DWARF call frame instructions.  These are needed for dynamic
   stack alignment.  */

static void
arm_dwarf_handle_frame_unspec (const char *label, rtx pattern, int index)
{
  rtx unspec = SET_SRC (pattern);
  gcc_assert (GET_CODE (unspec) == UNSPEC);

  switch (index)
    {
    case UNSPEC_STACK_ALIGN:
      /* ??? We should set the CFA = (SP & ~7).  At this point we haven't
         put anything on the stack, so hopefully it won't matter.
         CFA = SP will be correct after alignment.  */
      dwarf2out_reg_save_reg (label, stack_pointer_rtx,
                              SET_DEST (pattern));
      break;
    default:
      gcc_unreachable ();
    }
}


/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* Output unwind directives for the start/end of a function.  */

void
arm_output_fn_unwind (FILE * f, bool prologue)
{
  if (!ARM_EABI_UNWIND_TABLES)
    return;

  if (prologue)
    fputs ("\t.fnstart\n", f);
  else
    fputs ("\t.fnend\n", f);
}

static bool
arm_emit_tls_decoration (FILE *fp, rtx x)
{
  enum tls_reloc reloc;
  rtx val;

  val = XVECEXP (x, 0, 0);
  reloc = INTVAL (XVECEXP (x, 0, 1));

  output_addr_const (fp, val);

  switch (reloc)
    {
    case TLS_GD32:
      fputs ("(tlsgd)", fp);
      break;
    case TLS_LDM32:
      fputs ("(tlsldm)", fp);
      break;
    case TLS_LDO32:
      fputs ("(tlsldo)", fp);
      break;
    case TLS_IE32:
      fputs ("(gottpoff)", fp);
      break;
    case TLS_LE32:
      fputs ("(tpoff)", fp);
      break;
    default:
      gcc_unreachable ();
    }

  switch (reloc)
    {
    case TLS_GD32:
    case TLS_LDM32:
    case TLS_IE32:
      fputs (" + (. - ", fp);
      output_addr_const (fp, XVECEXP (x, 0, 2));
      fputs (" - ", fp);
      output_addr_const (fp, XVECEXP (x, 0, 3));
      fputc (')', fp);
      break;
    default:
      break;
    }

  return TRUE;
}

bool
arm_output_addr_const_extra (FILE *fp, rtx x)
{
  if (GET_CODE (x) == UNSPEC && XINT (x, 1) == UNSPEC_TLS)
    return arm_emit_tls_decoration (fp, x);
  else if (GET_CODE (x) == UNSPEC && XINT (x, 1) == UNSPEC_PIC_LABEL)
    {
      char label[256];
      int labelno = INTVAL (XVECEXP (x, 0, 0));

      ASM_GENERATE_INTERNAL_LABEL (label, "LPIC", labelno);
      assemble_name_raw (fp, label);

      return TRUE;
    }
  else if (GET_CODE (x) == CONST_VECTOR)
    return arm_emit_vector_const (fp, x);

  return FALSE;
}

/* APPLE LOCAL begin ARM darwin local binding */
#if TARGET_MACHO
/* Cross-module name binding.  Darwin does not support overriding
   functions at dynamic-link time.  */

static bool
arm_binds_local_p (tree decl)
{
  return default_binds_local_p_1 (decl,
        flag_apple_kext && lang_hooks.vtable_p (decl));
}
#endif
/* APPLE LOCAL end ARM darwin local binding */

/* APPLE LOCAL begin ARM setjmp/longjmp interworking */
static rtx
arm_builtin_setjmp_frame_value (void)
{
  static rtx arm_hard_frame_pointer_rtx;
  if (! arm_hard_frame_pointer_rtx)
    arm_hard_frame_pointer_rtx =
      gen_rtx_REG (Pmode, ARM_HARD_FRAME_POINTER_REGNUM);
  return arm_hard_frame_pointer_rtx;
}
/* APPLE LOCAL end ARM setjmp/longjmp interworking */

/* APPLE LOCAL begin ARM optimization pragmas */
/* Version of the above for use from #pragma optimization_level.  Only
   per-function flags are reset.  */
#if TARGET_MACHO
void
reset_optimization_options (int level ATTRIBUTE_UNUSED, int size ATTRIBUTE_UNUSED)
{
}
#endif
/* APPLE LOCAL end ARM optimization pragmas */

/* APPLE LOCAL begin ARM pic support */
#ifdef OBJECT_FORMAT_MACHO

/* Generate PIC and indirect symbol stubs.  */

void
machopic_output_stub (FILE *file, const char *symb, const char *stub)
{
  unsigned int length;
  char *symbol_name, *lazy_ptr_name, *slp_label_name;
  static int label = 0;

  /* Lose our funky encoding stuff so it doesn't contaminate the stub.  */
  symb = (*targetm.strip_name_encoding) (symb);

  length = strlen (symb);
  symbol_name = alloca (length + 32);
  GEN_SYMBOL_NAME_FOR_SYMBOL (symbol_name, symb, length);

  lazy_ptr_name = alloca (length + 32);
  GEN_LAZY_PTR_NAME_FOR_SYMBOL (lazy_ptr_name, symb, length);

  slp_label_name = alloca (length + 32);
  GEN_SUFFIXED_NAME_FOR_SYMBOL (slp_label_name, symb, length, "$slp");

  if (flag_pic == 2)
    switch_to_section (darwin_sections[machopic_picsymbol_stub4_section]);
  else
    switch_to_section (darwin_sections[machopic_symbol_stub4_section]);

  fprintf (file, "\t.align 2\n");

  if (TARGET_THUMB)
    fprintf (file, "\t.code 32\n");

  fprintf (file, "%s:\n", stub);
  fprintf (file, "\t.indirect_symbol %s\n", symbol_name);
  fprintf (file, "\tldr\tip, %s\n", slp_label_name);

  label++;

  if (flag_pic == 2)
    fprintf (file, "L%d$scv:\tadd\tip, pc, ip\n", label);

  fprintf (file, "\tldr\tpc, [ip, #0]\n");

  if (flag_pic == 2)
    fprintf (file, "%s:\n\t.long\t%s - (L%d$scv + 8)\n",
	     slp_label_name, lazy_ptr_name, label);
  else
    fprintf (file, "%s:\n\t.long\t%s\n",
	     slp_label_name, lazy_ptr_name);
      
  switch_to_section (darwin_sections[machopic_lazy_symbol_ptr_section]);
  fprintf (file, "%s:\n", lazy_ptr_name);
  fprintf (file, "\t.indirect_symbol\t%s\n", symbol_name);
  fprintf (file, "\t.long\tdyld_stub_binding_helper\n");
}

#endif
/* APPLE LOCAL end ARM pic support */

/* APPLE LOCAL begin ARM MACH assembler */
extern bool iasm_memory_clobber (const char *);

/* FIXME: we can be more specific here.  */
bool iasm_memory_clobber (const char *ARG_UNUSED (opcode))
{
  return true;
}
/* APPLE LOCAL end ARM MACH assembler */

/* APPLE LOCAL begin ARM darwin optimization defaults */
void
optimization_options (int level, int size ATTRIBUTE_UNUSED)
{
  /* disable strict aliasing; breaks too much existing code.  */
#if TARGET_MACHO
  flag_strict_aliasing = 0;

  /* Trapping math is not needed by many users, and is expensive.
     C99 permits us to default it off and we do that.  It is
     turned on when <fenv.h> is included (see darwin_pragma_fenv
     in darwin-c.c).  */
  flag_trapping_math = 0;

  /* APPLE LOCAL conditionally disable local RA */
  flag_local_alloc = 0;
  /* APPLE LOCAL rerun cse after combine */
  /* flag_rerun_cse_after_combine = 1; */
  
  /* For -O2 and beyond, turn off -fschedule-insns by default.  It tends to
     make the problem with not enough registers even worse.  */
#ifdef INSN_SCHEDULING
  if (level > 1)
    flag_schedule_insns = 0;
#endif

  /* radar 4094534. */
  /* The Darwin libraries never set errno, so we might as well
     avoid calling them when that's the only reason we would.  */
  flag_errno_math = 0;
#endif

#ifdef SUBTARGET_OPTIMIZATION_OPTIONS
  SUBTARGET_OPTIMIZATION_OPTIONS;
#endif
}
/* APPLE LOCAL end ARM darwin optimization defaults */

/* APPLE LOCAL begin ARM prefer SP to FP */
/* Stabs is so much fun.  Stabs doesn't distinguish between a SP and a
   FP offset -- if your function has a frame pointer, it is assumed
   that is what offsets to locals are from.  So, in the cases where we
   have a FP, but are using a SP anyway, we have to adjust the values
   to be FP-based.  */
HOST_WIDE_INT
arm_local_debug_offset (rtx var)
{
  int offset;
  int reg;

  if (GET_CODE (var) == PLUS)
    {
      reg = REGNO (XEXP (var, 0));
      offset = INTVAL (XEXP (var, 1));
    }
  else if (GET_CODE (var) == REG)
    {
      reg = REGNO (var);
      offset = 0;
    }
  else
    {
      return 0;
    }

  if (frame_pointer_needed && reg == SP_REGNUM)
    {
      arm_stack_offsets *offsets = arm_get_frame_offsets();

      return offset + (offsets->frame - offsets->outgoing_args);
    }
  else
    return offset;
}
/* APPLE LOCAL end ARM prefer SP to FP */

/* APPLE LOCAL begin ARM compact switch tables */
int arm_label_align (rtx label)
{
  rtx insn = NEXT_INSN (label);
  if (insn
      && GET_CODE (insn) == INSN
      && GET_CODE (PATTERN (insn)) == UNSPEC_VOLATILE)
    {
      if ((int) XEXP (PATTERN (insn), 1) == VUNSPEC_ALIGN)
	return 2;
      if ((int) XEXP (PATTERN (insn), 1) == VUNSPEC_ALIGN8)
	return 3;
    }
  return align_labels_log;
}
/* APPLE LOCAL end ARM compact switch tables */

/* ALQAAHIRA LOCAL begin v7 support. Merge from mainline */
/* Output assembly for a shift instruction.
   SET_FLAGS determines how the instruction modifies the condition codes.
   0 - Do not set conditiona codes.
   1 - Set condition codes.
   2 - Use smallest instruction.  */
const char *
arm_output_shift(rtx * operands, int set_flags)
{
  char pattern[100];
  static const char flag_chars[3] = {'?', '.', '!'};
  const char *shift;
  HOST_WIDE_INT val;
  char c;
  
  c = flag_chars[set_flags];
  if (TARGET_UNIFIED_ASM)
    {
      shift = shift_op(operands[3], &val);
      if (shift)
	{
	  if (val != -1)
	    operands[2] = GEN_INT(val);
	  sprintf (pattern, "%s%%%c\t%%0, %%1, %%2", shift, c);
	}
      else
	sprintf (pattern, "mov%%%c\t%%0, %%1", c);
    }
  else
    sprintf (pattern, "mov%%%c\t%%0, %%1%%S3", c);
  output_asm_insn (pattern, operands);
  return "";
}

/* Output a Thumb-2 casesi instruction.  */
const char *
thumb2_output_casesi (rtx *operands)
{
  rtx diff_vec = PATTERN (next_real_insn (operands[2]));

  gcc_assert (GET_CODE (diff_vec) == ADDR_DIFF_VEC);

  output_asm_insn ("cmp\t%0, %1", operands);
  output_asm_insn ("bhi\t%l3", operands);
  switch (GET_MODE(diff_vec))
    {
    case QImode:
      return "tbb\t[%|pc, %0]";
    case HImode:
      return "tbh\t[%|pc, %0, lsl #1]";
    case SImode:
      /* ALQAAHIRA LOCAL begin 6152801 SImode thumb2 switch table dispatch */
      output_asm_insn ("adr\t%4, %l2", operands);
      output_asm_insn ("add\t%4, %4, %0, lsl #2", operands);
      return "mov\t%|pc, %4";
      /* ALQAAHIRA LOCAL end 6152801 SImode thumb2 switch table dispatch */
    default:
      gcc_unreachable ();
    }
}
/* ALQAAHIRA LOCAL end v7 support. Merge from mainline */
/* ALQAAHIRA LOCAL begin v7 support. Merge from Codesourcery */
 
/* A table and a function to perform ARM-specific name mangling for
   NEON vector types in order to conform to the AAPCS (see "Procedure
   Call Standard for the ARM Architecture", Appendix A).  To qualify
   for emission with the mangled names defined in that document, a
   vector type must not only be of the correct mode but also be
   composed of NEON vector element types (e.g. __builtin_neon_qi).  */
typedef struct
{
  enum machine_mode mode;
  const char *element_type_name;
  const char *aapcs_name;
} arm_mangle_map_entry;

static arm_mangle_map_entry arm_mangle_map[] = {
  /* 64-bit containerized types.  */
  { V8QImode,  "__builtin_neon_qi",     "15__simd64_int8_t" },
  { V8QImode,  "__builtin_neon_uqi",    "16__simd64_uint8_t" },
  { V4HImode,  "__builtin_neon_hi",     "16__simd64_int16_t" },
  { V4HImode,  "__builtin_neon_uhi",    "17__simd64_uint16_t" },
  { V2SImode,  "__builtin_neon_si",     "16__simd64_int32_t" },
  { V2SImode,  "__builtin_neon_usi",    "17__simd64_uint32_t" },
  { V2SFmode,  "__builtin_neon_sf",     "18__simd64_float32_t" },
  { V8QImode,  "__builtin_neon_poly8",  "16__simd64_poly8_t" },
  { V4HImode,  "__builtin_neon_poly16", "17__simd64_poly16_t" },
  /* 128-bit containerized types.  */
  { V16QImode, "__builtin_neon_qi",     "16__simd128_int8_t" },
  { V16QImode, "__builtin_neon_uqi",    "17__simd128_uint8_t" },
  { V8HImode,  "__builtin_neon_hi",     "17__simd128_int16_t" },
  { V8HImode,  "__builtin_neon_uhi",    "18__simd128_uint16_t" },
  { V4SImode,  "__builtin_neon_si",     "17__simd128_int32_t" },
  { V4SImode,  "__builtin_neon_usi",    "18__simd128_uint32_t" },
  { V4SFmode,  "__builtin_neon_sf",     "19__simd128_float32_t" },
  { V16QImode, "__builtin_neon_poly8",  "17__simd128_poly8_t" },
  { V8HImode,  "__builtin_neon_poly16", "18__simd128_poly16_t" },
  { VOIDmode, NULL, NULL }
};

const char *
arm_mangle_vector_type (tree type)
{
  arm_mangle_map_entry *pos = arm_mangle_map;

  gcc_assert (TREE_CODE (type) == VECTOR_TYPE);

  /* Check the mode of the vector type, and the name of the vector
     element type, against the table.  */
  while (pos->mode != VOIDmode)
    {
      tree elt_type = TREE_TYPE (type);

      if (pos->mode == TYPE_MODE (type)
          && TREE_CODE (TYPE_NAME (elt_type)) == TYPE_DECL
          && !strcmp (IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (elt_type))),
                      pos->element_type_name))
	return pos->aapcs_name;

      pos++;
    }

  /* Use the default mangling for unrecognized (possibly user-defined)
     vector types.  */
  return NULL;
}

void
arm_asm_output_addr_diff_vec (FILE *file, rtx label, rtx body)
{
  int idx, size = GET_MODE_SIZE (GET_MODE (body));
  int pack = (TARGET_THUMB) ? 2 : 4;
  /* APPLE LOCAL 5837498 assembler expr for (L1-L2)/2 */
  /* removed unused variable "base_addr" */
  int base_label_no = CODE_LABEL_NUMBER (label);
  int vlen = XVECLEN (body, 1); /*includes trailing default */
  const char* directive;
  if (GET_MODE (body) == QImode)
      directive = ".byte";
  else if (GET_MODE (body) == HImode)
      directive = ".short";
  else
    {
      pack = 1;		    
      directive = ".long";
    }
  /* Alignment of table was handled by aligning its label,
     in final_scan_insn. */
  targetm.asm_out.internal_label (file, "L", base_label_no);
  /* Default is not included in output count */
  if (TARGET_COMPACT_SWITCH_TABLES)
    asm_fprintf (file, "\t%s\t%d @ size\n", directive, vlen - 1);
  for (idx = 0; idx < vlen; idx++)
    {
      rtx target_label = XEXP (XVECEXP (body, 1, idx), 0);
      /* APPLE LOCAL begin 5837498 assembler expr for (L1-L2)/2 */
      if (GET_MODE (body) != SImode)
        {
	  /* ARM mode is always SImode bodies */
	  asm_fprintf (file, "\t%s\t(L%d-L%d)/%d\n",
	    directive,
	  CODE_LABEL_NUMBER (target_label), base_label_no, pack);
        }    
      /* APPLE LOCAL end 5837498 assembler expr for (L1-L2)/2 */
      /* ALQAAHIRA LOCAL begin 6152801 SImode thumb2 switch table dispatch */
      else if (TARGET_ARM)
	asm_fprintf (file, "\tb\tL%d\n",
			CODE_LABEL_NUMBER (target_label));
      else if (TARGET_THUMB2)
	asm_fprintf (file, "\tb.w\tL%d\n",
			CODE_LABEL_NUMBER (target_label));
      /* ALQAAHIRA LOCAL end 6152801 SImode thumb2 switch table dispatch */
      else if (TARGET_COMPACT_SWITCH_TABLES || flag_pic)
	/* Let the assembler do the computation here; one case that
	   uses is this is when there are asm's, which makes
	   compile time computations unreliable. */
	asm_fprintf (file, "\t%s\tL%d-L%d\n",
	  directive,
	  CODE_LABEL_NUMBER (target_label), base_label_no);
      else
	asm_fprintf (file, "\t%s\tL%d\n", directive,
		     CODE_LABEL_NUMBER (target_label));
    }
  /* Pad to instruction boundary. */
  if (TARGET_COMPACT_SWITCH_TABLES)
    vlen = (vlen + 1/*count*/) * size;
  else
    vlen = vlen * size;
  while (vlen % pack != 0)
    {
      asm_fprintf (file, "\t%s\t0 @ pad\n", directive);
      vlen += size;
    }
}
/* ALQAAHIRA LOCAL end v7 support. Merge from Codesourcery */

/* APPLE LOCAL begin ARM enhance conditional insn generation */
/* A C expression to modify the code described by the conditional if
   information CE_INFO, for the basic block BB, possibly updating the tests in
   TRUE_EXPR, and FALSE_EXPR for converting the && and || parts of if-then or
   if-then-else code to conditional instructions.  Set either TRUE_EXPR or
   FALSE_EXPR to a null pointer if the tests cannot be converted.  */

/* p_true and p_false are given expressions of the form:

	(and (relop:CC (reg:CC) (const_int 0))
	     (relop:CC (reg:CC) (const_int 0)))

  We try to simplify them to something that will work in a branch instruction.
  If we can't do anything useful, return; the caller will try to substitute
  the complex expression and will fail.
  Currently the true and false cases are not handled.
  It's surprising that there isn't already a routine somewhere that does this,
  but I couldn't find one. */
 
void
arm_ifcvt_modify_multiple_tests (ce_if_block_t *ce_info ATTRIBUTE_UNUSED,
                                 basic_block bb ATTRIBUTE_UNUSED,
                                 rtx *p_true,
                                 rtx *p_false)
{
  /* There is a dependency here on the order of codes in rtl.def,
     also an assumption that none of the useful enum values will
     collide with 0 or 1.  
     Order is:  NE EQ GE GT LE LT GEU GTU LEU LTU */
  static RTX_CODE and_codes[10][10] =
	{ {  NE,  0, GT, GT, LT, LT, GTU, GTU, LTU, LTU },
	  {   0, EQ, EQ,  0, EQ,  0,  EQ,   0,  EQ,   0 },
	  {  GT, EQ, GE, GT, EQ,  0,   0,   0,   0,   0 },
	  {  GT,  0, GT, GT,  0,  0,   0,   0,   0,   0 },
	  {  LT, EQ, EQ,  0, LE, LT,   0,   0,   0,   0 },
	  {  LT,  0,  0,  0, LT, LT,   0,   0,   0,   0 },
	  { GTU, EQ,  0,  0,  0,  0, GEU, GTU,  EQ,   0 },
	  { GTU,  0,  0,  0,  0,  0, GTU, GTU,   0,   0 },
	  { LTU, EQ,  0,  0,  0,  0,  EQ,   0, LEU, LTU },
	  { LTU,  0,  0,  0,  0,  0,   0,   0, LTU, LTU } };

  static RTX_CODE or_codes[10][10] =
	{ { NE,   1,  1, NE,  1, NE,   1,  NE,   1,  NE },
	  {  1,  EQ, GE, GE, LE, LE, GEU, GEU, LEU, LEU },
	  {  1,  GE, GE, GE,  1,  1,   0,   0,   0,   0 },
	  { NE,  GE, GE, GT,  1, NE,   0,   0,   0,   0 },
	  {  1,  LE,  1,  1, LE, LE,   0,   0,   0,   0 },
	  { NE,  LE,  1, NE, LE, LT,   0,   0,   0,   0 },
	  {  1, GEU,  0,  0,  0,  0, GEU, GEU,   1,   1 },
	  { NE, GEU,  0,  0,  0,  0, GEU, GTU,   1,  NE },
	  {  1, LEU,  0,  0,  0,  0,   1,   1, LEU, LEU },
	  { NE, LEU,  0,  0,  0,  0,   1,  NE, LEU, LTU } };

  rtx true_lhs = XEXP (*p_true, 0);
  rtx false_lhs = XEXP (*p_false, 0);
  rtx true_rhs = XEXP (*p_true, 1);
  rtx false_rhs = XEXP (*p_false, 1);
  int true_and_p, false_and_p;
  RTX_CODE merged_code;

  if (!TARGET_ARM)
    return;

  if (GET_CODE (*p_true) == AND)
    true_and_p = true;
  else if (GET_CODE (*p_true) == IOR)
    true_and_p = false;
  else
    return;

  if (GET_CODE (*p_false) == AND)
    false_and_p = true;
  else if (GET_CODE (*p_false) == IOR)
    false_and_p = false;
  else
    return;

  if (!cc_register (XEXP (true_lhs, 0), CCmode)
      || !cc_register (XEXP (true_lhs, 0), CCmode)
      || !cc_register (XEXP (true_lhs, 0), CCmode)
      || !cc_register (XEXP (true_lhs, 0), CCmode))
    return;

  if (XEXP (true_lhs, 1) != const0_rtx
      || XEXP (true_rhs, 1) != const0_rtx
      || XEXP (false_lhs, 1) != const0_rtx
      || XEXP (false_rhs, 1) != const0_rtx)
    return;

  if (GET_CODE (true_lhs) < NE || GET_CODE (true_lhs) > LTU
      || GET_CODE (true_rhs) < NE || GET_CODE (true_rhs) > LTU)
    *p_true = 0;
  else
    {
      if (true_and_p)
        merged_code = and_codes [GET_CODE (true_lhs) - NE][GET_CODE (true_rhs) - NE];
      else
        merged_code = or_codes [GET_CODE (true_lhs) - NE][GET_CODE (true_rhs) - NE];
      if (merged_code == 0 || merged_code == 1)
	*p_true = 0;
      else
	*p_true = gen_rtx_fmt_ee (merged_code, VOIDmode, gen_rtx_REG (CCmode, CC_REGNUM), const0_rtx);
    }

  if (GET_CODE (false_lhs) < NE || GET_CODE (false_lhs) > LTU
      || GET_CODE (false_rhs) < NE || GET_CODE (false_rhs) > LTU)
    *p_false = 0;
  else
    {
      if (false_and_p)
        merged_code = and_codes [GET_CODE (false_lhs) - NE][GET_CODE (false_rhs) - NE];
      else
        merged_code = or_codes [GET_CODE (false_lhs) - NE][GET_CODE (false_rhs) - NE];
      if (merged_code == 0 || merged_code == 1)
	*p_false = 0;
      else
        *p_false = gen_rtx_fmt_ee (merged_code, VOIDmode, gen_rtx_REG (CCmode, CC_REGNUM), const0_rtx);
    }
}
/* APPLE LOCAL end ARM enhance conditional insn generation */

/* APPLE LOCAL begin ARM 6008578 */
/* Minimum alignment of a function entry point, in bits.  */
int
arm_function_boundary (void)
{
  /* ALQAAHIRA LOCAL 6357106 thumb2 functions should be 4-byte aligned */
  int min_align = TARGET_32BIT ? 32 : 16;

  /* Even in Thumb mode, thunks are output as ARM functions.  */
  if (cfun && current_function_is_thunk)
    min_align = MAX (min_align, 32);

  /* e.g., Thumb functions with jump tables.  */
  if (cfun && cfun->needs_4byte_alignment)
    min_align = MAX (min_align, 32);

  /* If -falign-loops was specified, use that alignment.  This is _not_
     needed to guarantee that loop alignments within the function are
     honored -- that's handled by the assembler and linker.  However,
     if we don't align the function, then our address calculations (in
     arm_reorg) are incorrect, potentially wreaking havoc on the
     constant pool calculations.  */
  min_align = MAX (min_align, align_loops * BITS_PER_UNIT);

  return min_align;
}
/* APPLE LOCAL end ARM 6008578 */

/* ALQAAHIRA LOCAL begin 6160917 */
/* Handle the cases where SECONDARY_INPUT_RELOAD_CLASS said that we
   needed a scratch register.  Currently, we only handle the case
   where there was indexed literal addressing with an out-of-range
   offset.  */
void
neon_reload_in (rtx *operands, enum machine_mode mode)
{
  if (GET_CODE (operands[1]) == MEM)
    {
      rtx mem_addr = XEXP (operands[1], 0);
      if (GET_CODE (mem_addr) == PLUS
	  && GET_CODE (XEXP (mem_addr, 0)) == REG
	  && REG_MODE_OK_FOR_BASE_P (XEXP (mem_addr, 0), VOIDmode)
	  && ! arm_legitimate_index_p (mode, XEXP (mem_addr, 1), SET, 0))
	{
	  rtx scratch;

	  /* Load the address into the scratch register provided,
	     and then indirect it.  */
	  emit_move_insn (operands[2], mem_addr);
	  scratch = gen_rtx_MEM (mode, operands[2]);
	  emit_move_insn (operands[0], scratch);
	  return;
	}
    }
  /* If you reach here, SECONDARY_INPUT_RELOAD_CLASS is indicating that
     a scratch register is needed, but we don't have any code to
     handle it.  Add that code here.  */
  gcc_unreachable ();
}
  
/* Handle the cases where SECONDARY_OUTPUT_RELOAD_CLASS said that we
   needed a scratch register.  Currently, we only handle the case
   where there was indexed literal addressing with an out-of-range
   offset.  */
void
neon_reload_out (rtx *operands, enum machine_mode mode)
{
  if (GET_CODE (operands[0]) == MEM)
    {
      rtx mem_addr = XEXP (operands[0], 0);
      if (GET_CODE (mem_addr) == PLUS
	  && GET_CODE (XEXP (mem_addr, 0)) == REG
	  && REG_MODE_OK_FOR_BASE_P (XEXP (mem_addr, 0), VOIDmode)
	  && ! arm_legitimate_index_p (mode, XEXP (mem_addr, 1), SET, 0))
	{
	  rtx scratch;

	  /* Load the address into the scratch register provided,
	     and then indirect it.  */
	  emit_move_insn (operands[2], mem_addr);
	  scratch = gen_rtx_MEM (mode, operands[2]);
	  emit_move_insn (scratch, operands[1]);
	  return;
	}
    }
  /* If you reach here, SECONDARY_OUTPUT_RELOAD_CLASS is indicating that
     a scratch register is needed, but we don't have any code to
     handle it.  Add that code here.  */
  gcc_unreachable ();
}
/* ALQAAHIRA LOCAL end 6160917 */

/* ALQAAHIRA LOCAL begin 5571707 Allow R9 as caller-saved register */
/* For v4 and v5, we always reserve R9 for thread local data. For v6 and
   v7, we can make it available when the target is iPhoneOS v3.0 or later. */
void
arm_darwin_subtarget_conditional_register_usage (void)
{
  if (!(arm_arch6 && !darwin_reserve_r9_on_v6) && !arm_arch7a)
    fixed_regs[9]   = 1;
  call_used_regs[9] = 1;

  if (TARGET_THUMB)
    {		
      fixed_regs[THUMB_HARD_FRAME_POINTER_REGNUM] = 1;
      call_used_regs[THUMB_HARD_FRAME_POINTER_REGNUM] = 1;
      global_regs[THUMB_HARD_FRAME_POINTER_REGNUM] = 1;	
    }
}
/* ALQAAHIRA LOCAL end 5571707 Allow R9 as caller-saved register */

/* ALQAAHIRA LOCAL begin 6902792 Q register clobbers in inline asm */
/* Worker function for TARGET_MD_ASM_CLOBBERS.
   We do this to translate references to Qn registers into the equivalent
   D(2n)/D(2n+1) register pairs. */
static tree
arm_md_asm_clobbers (tree outputs ATTRIBUTE_UNUSED,
		      tree inputs ATTRIBUTE_UNUSED,
		      tree clobbers)
{
  tree tail;

  for (tail = clobbers; tail; tail = TREE_CHAIN (tail))
    {
      const char *clobber_name;
      clobber_name = TREE_STRING_POINTER (TREE_VALUE (tail));
      if (tolower (clobber_name[0]) == 'q' && isdigit (clobber_name[1])
          && (isdigit (clobber_name[2]) || clobber_name[2] == '\0'))
        {
          char regname[4] = "dXX";
          /* found a Q register in the clobber list, so add the D reference
             to the upper dword of it. The existing clobber for the Q
             register will automatically translate to the low dword. */
          int regno = atoi (clobber_name + 1) * 2 + 1;
          snprintf (regname + 1, 3, "%d", regno);
          clobbers =
            tree_cons (NULL_TREE, build_string (strlen(regname), regname),
                       clobbers);
        }
    }
  return clobbers;
}
/* ALQAAHIRA LOCAL end 6902792 Q register clobbers in inline asm */

#include "gt-arm.h"
