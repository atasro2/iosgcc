/* Output dbx-format symbol table information from GNU compiler.
   Copyright (C) 1987, 1988, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.  */


/* Output dbx-format symbol table data.
   This consists of many symbol table entries, each of them
   a .stabs assembler pseudo-op with four operands:
   a "name" which is really a description of one symbol and its type,
   a "code", which is a symbol defined in stab.h whose name starts with N_,
   an unused operand always 0,
   and a "value" which is an address or an offset.
   The name is enclosed in doublequote characters.

   Each function, variable, typedef, and structure tag
   has a symbol table entry to define it.
   The beginning and end of each level of name scoping within
   a function are also marked by special symbol table entries.

   The "name" consists of the symbol name, a colon, a kind-of-symbol letter,
   and a data type number.  The data type number may be followed by
   "=" and a type definition; normally this will happen the first time
   the type number is mentioned.  The type definition may refer to
   other types by number, and those type numbers may be followed
   by "=" and nested definitions.

   This can make the "name" quite long.
   When a name is more than 80 characters, we split the .stabs pseudo-op
   into two .stabs pseudo-ops, both sharing the same "code" and "value".
   The first one is marked as continued with a double-backslash at the
   end of its "name".

   The kind-of-symbol letter distinguished function names from global
   variables from file-scope variables from parameters from auto
   variables in memory from typedef names from register variables.
   See `dbxout_symbol'.

   The "code" is mostly redundant with the kind-of-symbol letter
   that goes in the "name", but not entirely: for symbols located
   in static storage, the "code" says which segment the address is in,
   which controls how it is relocated.

   The "value" for a symbol in static storage
   is the core address of the symbol (actually, the assembler
   label for the symbol).  For a symbol located in a stack slot
   it is the stack offset; for one in a register, the register number.
   For a typedef symbol, it is zero.

   If DEBUG_SYMS_TEXT is defined, all debugging symbols must be
   output while in the text section.

   For more on data type definitions, see `dbxout_type'.  */

#include "config.h"
#include "system.h"

#include "tree.h"
#include "rtl.h"
#include "flags.h"
#include "regs.h"
#include "insn-config.h"
#include "reload.h"
#include "output.h" /* ASM_OUTPUT_SOURCE_LINE may refer to sdb functions.  */
#include "dbxout.h"
#include "toplev.h"
#include "tm_p.h"
#include "ggc.h"
#include "debug.h"
#include "function.h"
#include "target.h"
#include "langhooks.h"
/* APPLE LOCAL begin Constructors return THIS  turly 20020315  */
/* FIXME: dbxout.c should not need language-specific headers.  */
#include "c-common.h"


#ifdef XCOFF_DEBUGGING_INFO
#include "xcoffout.h"
#endif

/* APPLE LOCAL begin empty BINCL/EINCL optimization */
enum binclstatus {BINCL_NOT_REQUIRED, BINCL_PENDING, BINCL_PROCESSED};
static void emit_bincl_stab             PARAMS ((const char *));
static void emit_pending_bincls         PARAMS ((void));
static inline void emit_pending_bincls_if_required PARAMS ((void));
/* APPLE LOCAL end empty BINCL/EINCL optimization */

/* APPLE LOCAL begin gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
/* In addition to the standard intercepted debug_hooks there are some
   direct calls into this file, i.e., dbxout_symbol, dbxout_parms, and
   dbxout_reg_params.  But those routines may also be called from a
   higher level intercepted routine.  So to prevent recording data
   for an inner call to one of these for an intercept we maintain a
   intercept nesting counter (dbxout_nesting).  We only save the 
   intercepted arguments if the nesting is 1.  */

#define DBXOUT_TRACK_NESTING
static int dbxout_nesting = 0;

/* Nonzero if generating debugger info for used symbols only.  */
extern int flag_debug_only_used_symbols;
/* APPLE LOCAL begin - rarely executed bb optimization */
extern int in_unlikely_text_section  PARAMS ((void));
/* APPLE LOCAL end - rarely executed bb optimization */

static void dbxout_flush_symbol_queue PARAMS ((void));
static void dbxout_queue_symbol       PARAMS ((tree decl));

static tree *symbol_queue;
static int  symbol_queue_index = 0;
static int  symbol_queue_size = 0;

/* The DBXOUT_DECR_... macros together with DBXOUT_TRACK_NESTING are
   used to avoid a bunch of additional #ifdef's sprinkled throughout
   the code.  */

#undef DBXOUT_DECR_NESTING
#define DBXOUT_DECR_NESTING \
  if (--dbxout_nesting == 0 && symbol_queue_index > 0) \
    dbxout_flush_symbol_queue ()
#undef DBXOUT_DECR_NESTING_AND_RETURN
#define DBXOUT_DECR_NESTING_AND_RETURN(x) \
  do {--dbxout_nesting; return (x);} while (0)
#endif /* DBX_ONLY_USED_SYMBOLS */

#ifndef DBXOUT_TRACK_NESTING
#define DBXOUT_DECR_NESTING
#define DBXOUT_DECR_NESTING_AND_RETURN(x) return (x)
#endif
/* APPLE LOCAL end gdb only used symbols */

#ifndef ASM_STABS_OP
#define ASM_STABS_OP "\t.stabs\t"
#endif

#ifndef ASM_STABN_OP
#define ASM_STABN_OP "\t.stabn\t"
#endif

#ifndef DBX_TYPE_DECL_STABS_CODE
#define DBX_TYPE_DECL_STABS_CODE N_LSYM
#endif

#ifndef DBX_STATIC_CONST_VAR_CODE
#define DBX_STATIC_CONST_VAR_CODE N_FUN
#endif

#ifndef DBX_REGPARM_STABS_CODE
#define DBX_REGPARM_STABS_CODE N_RSYM
#endif

#ifndef DBX_REGPARM_STABS_LETTER
#define DBX_REGPARM_STABS_LETTER 'P'
#endif

/* This is used for parameters passed by invisible reference in a register.  */
#ifndef GDB_INV_REF_REGPARM_STABS_LETTER
#define GDB_INV_REF_REGPARM_STABS_LETTER 'a'
#endif

#ifndef DBX_MEMPARM_STABS_LETTER
#define DBX_MEMPARM_STABS_LETTER 'p'
#endif

#ifndef FILE_NAME_JOINER
#define FILE_NAME_JOINER "/"
#endif

/* GDB needs to know that the stabs were generated by GCC.  We emit an
   N_OPT stab at the beginning of the source file to indicate this.
   The string is historical, and different on a very few targets.  */
#ifndef STABS_GCC_MARKER
#define STABS_GCC_MARKER "gcc2_compiled."
#endif

enum typestatus {TYPE_UNSEEN, TYPE_XREF, TYPE_DEFINED};

/* Structure recording information about a C data type.
   The status element says whether we have yet output
   the definition of the type.  TYPE_XREF says we have
   output it as a cross-reference only.
   The file_number and type_number elements are used if DBX_USE_BINCL
   is defined.  */

struct typeinfo GTY(())
{
  enum typestatus status;
  int file_number;
  int type_number;
};

/* Vector recording information about C data types.
   When we first notice a data type (a tree node),
   we assign it a number using next_type_number.
   That is its index in this vector.  */

static GTY ((length ("typevec_len"))) struct typeinfo *typevec;

/* Number of elements of space allocated in `typevec'.  */

static GTY(()) int typevec_len;

/* In dbx output, each type gets a unique number.
   This is the number for the next type output.
   The number, once assigned, is in the TYPE_SYMTAB_ADDRESS field.  */

static GTY(()) int next_type_number;

/* When using N_BINCL in dbx output, each type number is actually a
   pair of the file number and the type number within the file.
   This is a stack of input files.  */

/* APPLE LOCAL 3273065 */
struct dbx_file
{
  struct dbx_file *next;
  int file_number;
  int next_type_number;
  /* APPLE LOCAL begin empty BINC/EINCL optimization */
  enum binclstatus bincl_status;
  const char *pending_bincl_name;
  struct dbx_file *prev;
  /* APPLE LOCAL end empty BINC/EINCL optimization */
};

/* APPLE LOCAL empty BINC/EINCL optimization */
static int pending_bincls = 0;

/* This is the top of the stack.  

BEGIN APPLE LOCAL 3273065
   This is not saved for PCH, because restoring a PCH should not change it.
   next_file_number does have to be saved, because the PCH may use some
   file numbers; however, just before restoring a PCH, next_file_number
   should always be 0 because we should not have needed any file numbers
   yet.  */

static struct dbx_file *current_file;
/* END APPLE LOCAL 3273065 */

/* This is the next file number to use.  */

static GTY(()) int next_file_number;

/* A counter for dbxout_function_end.  */

static GTY(()) int scope_labelno;

/* Nonzero if we have actually used any of the GDB extensions
   to the debugging format.  The idea is that we use them for the
   first time only if there's a strong reason, but once we have done that,
   we use them whenever convenient.  */

static GTY(()) int have_used_extensions = 0;

/* Number for the next N_SOL filename stabs label.  The number 0 is reserved
   for the N_SO filename stabs label.  */

static GTY(()) int source_label_number = 1;

/* Last source file name mentioned in a NOTE insn.  */

static GTY(()) const char *lastfile;

/* Used by PCH machinery to detect if 'lastfile' should be reset to
   base_input_file.  */
static GTY(()) int lastfile_is_base;

/* Typical USG systems don't have stab.h, and they also have
   no use for DBX-format debugging info.  */

#if defined (DBX_DEBUGGING_INFO) || defined (XCOFF_DEBUGGING_INFO)

/* The original input file name.  */
static const char *base_input_file;

/* Current working directory.  */

static const char *cwd;

#ifdef DEBUG_SYMS_TEXT
#define FORCE_TEXT function_section (current_function_decl);
#else
#define FORCE_TEXT
#endif

#include "gstab.h"

#define STAB_CODE_TYPE enum __stab_debug_code

/* 1 if PARM is passed to this function in memory.  */

#define PARM_PASSED_IN_MEMORY(PARM) \
 (GET_CODE (DECL_INCOMING_RTL (PARM)) == MEM)

/* A C expression for the integer offset value of an automatic variable
   (N_LSYM) having address X (an RTX).  */
#ifndef DEBUGGER_AUTO_OFFSET
#define DEBUGGER_AUTO_OFFSET(X) \
  (GET_CODE (X) == PLUS ? INTVAL (XEXP (X, 1)) : 0)
#endif

/* A C expression for the integer offset value of an argument (N_PSYM)
   having address X (an RTX).  The nominal offset is OFFSET.  */
#ifndef DEBUGGER_ARG_OFFSET
#define DEBUGGER_ARG_OFFSET(OFFSET, X) (OFFSET)
#endif

/* Stream for writing to assembler file.  */

static FILE *asmfile;

/* These variables are for dbxout_symbol to communicate to
   dbxout_finish_symbol.
   current_sym_code is the symbol-type-code, a symbol N_... define in stab.h.
   current_sym_value and current_sym_addr are two ways to address the
   value to store in the symtab entry.
   current_sym_addr if nonzero represents the value as an rtx.
   If that is zero, current_sym_value is used.  This is used
   when the value is an offset (such as for auto variables,
   register variables and parms).  */

static STAB_CODE_TYPE current_sym_code;
static int current_sym_value;
static rtx current_sym_addr;

/* Number of chars of symbol-description generated so far for the
   current symbol.  Used by CHARS and CONTIN.  */

static int current_sym_nchars;

/* Report having output N chars of the current symbol-description.  */

#define CHARS(N) (current_sym_nchars += (N))

/* Break the current symbol-description, generating a continuation,
   if it has become long.  */

#ifndef DBX_CONTIN_LENGTH
#define DBX_CONTIN_LENGTH 80
#endif

#if DBX_CONTIN_LENGTH > 0
#define CONTIN  \
  do {if (current_sym_nchars > DBX_CONTIN_LENGTH) dbxout_continue ();} while (0)
#else
#define CONTIN do { } while (0)
#endif

static void dbxout_init			PARAMS ((const char *));
static void dbxout_finish		PARAMS ((const char *));
/* APPLE LOCAL begin Symbol Separtion */
static void dbxout_restore_write_symbols   PARAMS ((void));
static void dbxout_clear_write_symbols     PARAMS ((const char *, unsigned long));
static void dbxout_start_symbol_repository PARAMS ((unsigned int, const char *, unsigned long));
static void dbxout_end_symbol_repository   PARAMS ((unsigned int));
/* APPLE LOCAL end Symbol Separation */
static void dbxout_start_source_file	PARAMS ((unsigned, const char *));
static void dbxout_end_source_file	PARAMS ((unsigned));
static void dbxout_typedefs		PARAMS ((tree));
static void dbxout_fptype_value		PARAMS ((tree));
static void dbxout_type_index		PARAMS ((tree));
#if DBX_CONTIN_LENGTH > 0
static void dbxout_continue		PARAMS ((void));
#endif
static void dbxout_args			PARAMS ((tree));
static void dbxout_type_fields		PARAMS ((tree));
static void dbxout_type_method_1	PARAMS ((tree, const char *));
static void dbxout_type_methods		PARAMS ((tree));
static void dbxout_range_type		PARAMS ((tree));
static void dbxout_type			PARAMS ((tree, int));
static void print_int_cst_octal		PARAMS ((tree));
static void print_octal			PARAMS ((unsigned HOST_WIDE_INT, int));
static void print_wide_int		PARAMS ((HOST_WIDE_INT));
static void dbxout_type_name		PARAMS ((tree));
static void dbxout_class_name_qualifiers PARAMS ((tree));
static int dbxout_symbol_location	PARAMS ((tree, tree, const char *, rtx));
static void dbxout_symbol_name		PARAMS ((tree, const char *, int));
static void dbxout_prepare_symbol	PARAMS ((tree));
static void dbxout_finish_symbol	PARAMS ((tree));
static void dbxout_block		PARAMS ((tree, int, tree));
static void dbxout_global_decl		PARAMS ((tree));
static void dbxout_handle_pch		PARAMS ((unsigned));

/* The debug hooks structure.  */
#if defined (DBX_DEBUGGING_INFO)

static void dbxout_source_line		PARAMS ((unsigned int, const char *));
static void dbxout_source_file		PARAMS ((FILE *, const char *));
static void dbxout_function_end		PARAMS ((void));
static void dbxout_begin_function	PARAMS ((tree));
static void dbxout_begin_block		PARAMS ((unsigned, unsigned));
static void dbxout_end_block		PARAMS ((unsigned, unsigned));
static void dbxout_function_decl	PARAMS ((tree));

const struct gcc_debug_hooks dbx_debug_hooks =
{
  dbxout_init,
  dbxout_finish,
  debug_nothing_int_charstar,
  debug_nothing_int_charstar,
  dbxout_start_source_file,
  dbxout_end_source_file,
  dbxout_begin_block,
  dbxout_end_block,
  debug_true_tree,		/* ignore_block */
  dbxout_source_line,		/* source_line */
  dbxout_source_line,		/* begin_prologue: just output line info */
  debug_nothing_int_charstar,	/* end_prologue */
  debug_nothing_int_charstar,	/* end_epilogue */
#ifdef DBX_FUNCTION_FIRST
  dbxout_begin_function,
#else
  debug_nothing_tree,		/* begin_function */
#endif
  debug_nothing_int,		/* end_function */
  dbxout_function_decl,
  dbxout_global_decl,		/* global_decl */
  debug_nothing_tree,		/* deferred_inline_function */
  debug_nothing_tree,		/* outlining_inline_function */
  debug_nothing_rtx,		/* label */
  dbxout_handle_pch,		/* handle_pch */
  /* APPLE LOCAL begin Symbol Separation */
  dbxout_restore_write_symbols,
  dbxout_clear_write_symbols,
  dbxout_start_symbol_repository,
  dbxout_end_symbol_repository
  /* APPLE LOCAL end Symbol Separation */
};
#endif /* DBX_DEBUGGING_INFO  */

#if defined (XCOFF_DEBUGGING_INFO)
const struct gcc_debug_hooks xcoff_debug_hooks =
{
  dbxout_init,
  dbxout_finish,
  debug_nothing_int_charstar,
  debug_nothing_int_charstar,
  dbxout_start_source_file,
  dbxout_end_source_file,
  xcoffout_begin_block,
  xcoffout_end_block,
  debug_true_tree,		/* ignore_block */
  xcoffout_source_line,
  xcoffout_begin_prologue,	/* begin_prologue */
  debug_nothing_int_charstar,	/* end_prologue */
  xcoffout_end_epilogue,
  debug_nothing_tree,		/* begin_function */
  xcoffout_end_function,
  debug_nothing_tree,		/* function_decl */
  dbxout_global_decl,		/* global_decl */
  debug_nothing_tree,		/* deferred_inline_function */
  debug_nothing_tree,		/* outlining_inline_function */
  debug_nothing_rtx,		/* label */
  dbxout_handle_pch,		/* handle_pch */
  /* APPLE LOCAL begin Symbol Separtion */
  debug_nothing_void,           /* restore write_symbols */
  debug_nothing_void,           /* clear write_symbols */
  debug_nothing_void,           /* start repository */
  debug_nothing_void            /* end repository */
  /* APPLE LOCAL end Symbol Separation */
};
#endif /* XCOFF_DEBUGGING_INFO  */

#if defined (DBX_DEBUGGING_INFO)
static void
dbxout_function_end ()
{
  char lscope_label_name[100];

  /* APPLE LOCAL begin - rarely executed bb optimization */

#ifndef HOT_TEXT_SECTION_NAME
#define HOT_TEXT_SECTION_NAME TEXT_SECTION_ASM_OP
#endif

#ifndef SECTION_FORMAT_STRING
#define SECTION_FORMAT_STRING "%s\n"
#endif

  if (current_function_decl->decl.section_name)
    fprintf (asmfile, SECTION_FORMAT_STRING,
	     TREE_STRING_POINTER (current_function_decl->decl.section_name));
  else
    fprintf (asmfile, SECTION_FORMAT_STRING, HOT_TEXT_SECTION_NAME);

  /* APPLE LOCAL end - rarely executed bb optimization */

  /* Convert Ltext into the appropriate format for local labels in case
     the system doesn't insert underscores in front of user generated
     labels.  */
  ASM_GENERATE_INTERNAL_LABEL (lscope_label_name, "Lscope", scope_labelno);
  ASM_OUTPUT_INTERNAL_LABEL (asmfile, "Lscope", scope_labelno);
  scope_labelno++;

  /* By convention, GCC will mark the end of a function with an N_FUN
     symbol and an empty string.  */
#ifdef DBX_OUTPUT_NFUN
  DBX_OUTPUT_NFUN (asmfile, lscope_label_name, current_function_decl);
#else
  fprintf (asmfile, "%s\"\",%d,0,0,", ASM_STABS_OP, N_FUN);
  assemble_name (asmfile, lscope_label_name);
  putc ('-', asmfile);
  assemble_name (asmfile, XSTR (XEXP (DECL_RTL (current_function_decl), 0), 0));
  fprintf (asmfile, "\n");
#endif
}
#endif /* DBX_DEBUGGING_INFO */

/* At the beginning of compilation, start writing the symbol table.
   Initialize `typevec' and output the standard data types of C.  */

static void
dbxout_init (input_file_name)
     const char *input_file_name;
{
  char ltext_label_name[100];
  tree syms = (*lang_hooks.decls.getdecls) ();

  asmfile = asm_out_file;

  typevec_len = 100;
  typevec = (struct typeinfo *) ggc_calloc (typevec_len, sizeof typevec[0]);

  /* Convert Ltext into the appropriate format for local labels in case
     the system doesn't insert underscores in front of user generated
     labels.  */
  ASM_GENERATE_INTERNAL_LABEL (ltext_label_name, "Ltext", 0);

  /* Put the current working directory in an N_SO symbol.  */
#ifndef DBX_WORKING_DIRECTORY /* Only some versions of DBX want this,
				 but GDB always does.  */
  if (use_gnu_debug_info_extensions)
#endif
    {
      /* APPLE LOCAL - PCH distcc debugging mrs  */
      if (!cwd && (cwd = gcc_getpwd ()) && (!*cwd || cwd[strlen (cwd) - 1] != '/'))
	cwd = concat (cwd, FILE_NAME_JOINER, NULL);
      if (cwd)
	{
#ifdef DBX_OUTPUT_MAIN_SOURCE_DIRECTORY
	  DBX_OUTPUT_MAIN_SOURCE_DIRECTORY (asmfile, cwd);
#else /* no DBX_OUTPUT_MAIN_SOURCE_DIRECTORY */
	  fprintf (asmfile, "%s", ASM_STABS_OP);
	  output_quoted_string (asmfile, cwd);
          fprintf (asmfile, ",%d,0,0,", N_SO);
          assemble_name (asmfile, ltext_label_name);
          fputc ('\n', asmfile);
#endif /* no DBX_OUTPUT_MAIN_SOURCE_DIRECTORY */
	}
    }

#ifdef DBX_OUTPUT_MAIN_SOURCE_FILENAME
  /* This should NOT be DBX_OUTPUT_SOURCE_FILENAME. That
     would give us an N_SOL, and we want an N_SO.  */
  DBX_OUTPUT_MAIN_SOURCE_FILENAME (asmfile, input_file_name);
#else /* no DBX_OUTPUT_MAIN_SOURCE_FILENAME */
  /* We include outputting `Ltext:' here,
     because that gives you a way to override it.  */
  /* Used to put `Ltext:' before the reference, but that loses on sun 4.  */
  fprintf (asmfile, "%s", ASM_STABS_OP);
  output_quoted_string (asmfile, input_file_name);
  fprintf (asmfile, ",%d,0,0,", N_SO);
  assemble_name (asmfile, ltext_label_name);
  fputc ('\n', asmfile);
  text_section ();
  ASM_OUTPUT_INTERNAL_LABEL (asmfile, "Ltext", 0);
#endif /* no DBX_OUTPUT_MAIN_SOURCE_FILENAME */

#ifdef DBX_OUTPUT_GCC_MARKER
  DBX_OUTPUT_GCC_MARKER (asmfile);
#else
  /* Emit an N_OPT stab to indicate that this file was compiled by GCC.  */
  fprintf (asmfile, "%s\"%s\",%d,0,0,0\n",
	   ASM_STABS_OP, STABS_GCC_MARKER, N_OPT);
#endif

  base_input_file = lastfile = input_file_name;

  next_type_number = 1;

#ifdef DBX_USE_BINCL
/* APPLE LOCAL 3273065 */
  current_file = (struct dbx_file *) xmalloc (sizeof *current_file);
  current_file->next = NULL;
  current_file->file_number = 0;
  current_file->next_type_number = 1;
  next_file_number = 1;
  /* APPLE LOCAL begin empty BINCL/EINCL optimization */
  current_file->prev = NULL;
  current_file->bincl_status = BINCL_NOT_REQUIRED;
  current_file->pending_bincl_name = NULL;
  /* APPLE LOCAL end empty BINCL/EINCL optimization */
#endif

  /* Make sure that types `int' and `char' have numbers 1 and 2.
     Definitions of other integer types will refer to those numbers.
     (Actually it should no longer matter what their numbers are.
     Also, if any types with tags have been defined, dbxout_symbol
     will output them first, so the numbers won't be 1 and 2.  That
     happens in C++.  So it's a good thing it should no longer matter).  */

#ifdef DBX_OUTPUT_STANDARD_TYPES
  DBX_OUTPUT_STANDARD_TYPES (syms);
#else
/* APPLE LOCAL gdb only used symbols */
#ifndef DBX_ONLY_USED_SYMBOLS
      dbxout_symbol (TYPE_NAME (integer_type_node), 0);
      dbxout_symbol (TYPE_NAME (char_type_node), 0);
#endif
#endif

  dbxout_typedefs (syms);
}

/* Output any typedef names for types described by TYPE_DECLs in SYMS,
   in the reverse order from that which is found in SYMS.  */

static void
dbxout_typedefs (syms)
     tree syms;
{
  if (syms)
    {
      dbxout_typedefs (TREE_CHAIN (syms));
      if (TREE_CODE (syms) == TYPE_DECL)
	{
	  tree type = TREE_TYPE (syms);
	  if (TYPE_NAME (type)
	      && TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
	      && COMPLETE_TYPE_P (type)
	      && ! TREE_ASM_WRITTEN (TYPE_NAME (type)))
	    dbxout_symbol (TYPE_NAME (type), 0);
	}
    }
}

/* APPLE LOCAL begin Symbol Separation */
/* Restore write_symbols */
static void
dbxout_restore_write_symbols ()
{
  if (flag_grepository)
    write_symbols = orig_write_symbols;
}

/* Clear write_symbols and emit EXCL stab.  */
static void
dbxout_clear_write_symbols (filename, checksum)
     const char *filename;
     unsigned long checksum;
{
  if (flag_grepository)
    {
      write_symbols = NO_DEBUG;
      fprintf (asmfile, "%s", ASM_STABS_OP);
      output_quoted_string (asmfile, filename);
      fprintf (asmfile, ",%d,0,0,%ld\n", N_EXCL, checksum);
    }
}

/* Start symbol repository */
/* Add checksum with BINCL.  */
static void
dbxout_start_symbol_repository (lineno, filename, checksum)
     unsigned int lineno ATTRIBUTE_UNUSED;
     const char *filename ATTRIBUTE_UNUSED; 
     unsigned long checksum ATTRIBUTE_UNUSED;
{
#ifdef DBX_USE_BINCL
/* APPLE LOCAL 3273065 */
  struct dbx_file *n = (struct dbx_file *) xmalloc (sizeof *n);

  n->next = current_file;
  n->file_number = next_file_number++;
  n->next_type_number = 1;
  /* APPLE LOCAL begin empty BINCL/EINCL optimization */
  n->prev = NULL;
  current_file->prev = n;
  n->bincl_status = BINCL_NOT_REQUIRED;
  n->pending_bincl_name = NULL;
  /* APPLE LOCAL end empty BINCL/EINCL optimization */
  current_file = n;

  fprintf (asmfile, "%s", ASM_STABS_OP);
  output_quoted_string (asmfile, filename);
  fprintf (asmfile, ",%d,0,0,%ld\n", N_BINCL, checksum);
#endif
}

/* End symbol repository */
static void
dbxout_end_symbol_repository (lineno)
     unsigned int lineno ATTRIBUTE_UNUSED;
{
#ifdef DBX_USE_BINCL
  fprintf (asmfile, "%s%d,0,0,0\n", ASM_STABN_OP, N_EINCL);
  current_file = current_file->next;
#endif
}
/* APPLE LOCAL end Symbol Separation */

/* APPLE LOCAL begin empty BINCL/EINCL optimization */
static void
emit_bincl_stab (name)
     const char *name;
{
  fprintf (asmfile, "%s", ASM_STABS_OP);
  output_quoted_string (asmfile, name);
  fprintf (asmfile, ",%d,0,0,0\n", N_BINCL);
}

static inline void 
emit_pending_bincls_if_required ()
{
#ifdef DBX_USE_BINCL
  if (pending_bincls)
    emit_pending_bincls ();
#endif
}

static void
emit_pending_bincls ()
{
  struct dbx_file *f = current_file;

  /* Find first pending bincl */
  while (f->bincl_status == BINCL_PENDING)
    f = f->next;

  /* Now emit all bincls */
  f = f->prev;

  while (f)
    {
      /* Do not emit BINCL for main input file */
      if (f->bincl_status == BINCL_PENDING) /* && strcmp (f->pending_bincl_name, main_input_filename)) */
	{
	  emit_bincl_stab (f->pending_bincl_name);
	  f->file_number = next_file_number++;
	  f->bincl_status = BINCL_PROCESSED;
	}

      if (f == current_file)
	break;
      f = f->prev;
    }

  /* All pending bincls have been emitted.  */
  pending_bincls = 0;
}
/* APPLE LOCAL end empty BINCL/EINCL optimization */

/* Change to reading from a new source file.  Generate a N_BINCL stab.  */

static void
dbxout_start_source_file (line, filename)
     unsigned int line ATTRIBUTE_UNUSED;
     const char *filename ATTRIBUTE_UNUSED;
{
#ifdef DBX_USE_BINCL
/* APPLE LOCAL 3273065 */
  struct dbx_file *n = (struct dbx_file *) xmalloc (sizeof *n);
  /* APPLE LOCAL begin Symbol Separation */
  if (write_symbols == NO_DEBUG)
    {
      n = NULL;
      return;
    }
  /* APPLE LOCAL end Symbol Separation */

  /* APPLE LOCAL begin empty BINCL/EINCL optimization */
  n->next = current_file;
  n->next_type_number = 1;
  /* Do not assign file number now. Delay it until we actually emit BINCL.  */
  n->file_number = 0;
  n->prev = NULL;
  current_file->prev = n;
  n->bincl_status = BINCL_PENDING;
  n->pending_bincl_name = filename;
  pending_bincls = 1;
  /* APPLE LOCAL end empty BINCL/EINCL optimization */
  current_file = n;
#endif
}

/* Revert to reading a previous source file.  Generate a N_EINCL stab.  */

static void
dbxout_end_source_file (line)
     unsigned int line ATTRIBUTE_UNUSED;
{
#ifdef DBX_USE_BINCL
  /* APPLE LOCAL begin Symbol Separation */
  if (write_symbols == NO_DEBUG)
    return;
  /* APPLE LOCAL end Symbol Separation */

  /* APPLE LOCAL begin empty BINCl/EINCL optimization */
  /* Emit EINCL stab only if BINCL is not pending.  */
  if (current_file->bincl_status == BINCL_PROCESSED)
    fprintf (asmfile, "%s%d,0,0,0\n", ASM_STABN_OP, N_EINCL);
  current_file->bincl_status = BINCL_NOT_REQUIRED;
  /* APPLE LOCAL end empty BINCL/EINCL optimization */
  current_file = current_file->next;
#endif
}

/* Handle a few odd cases that occur when trying to make PCH files work.  */

static void
dbxout_handle_pch (at_end)
     unsigned at_end;
{
  if (! at_end)
    {
      /* When using the PCH, this file will be included, so we need to output
	 a BINCL.  */
      dbxout_start_source_file (0, lastfile);

      /* The base file when using the PCH won't be the same as
	 the base file when it's being generated.  */
      lastfile = NULL;
    }
  else
    {
      /* ... and an EINCL. */
      dbxout_end_source_file (0);

      /* Deal with cases where 'lastfile' was never actually changed.  */
      lastfile_is_base = lastfile == NULL;
    }
}

#if defined (DBX_DEBUGGING_INFO)
/* Output debugging info to FILE to switch to sourcefile FILENAME.  */

static void
dbxout_source_file (file, filename)
     FILE *file;
     const char *filename;
{
  if (lastfile == 0 && lastfile_is_base)
    {
      lastfile = base_input_file;
      lastfile_is_base = 0;
    }

  if (filename && (lastfile == 0 || strcmp (filename, lastfile)))
    {

#ifdef DBX_OUTPUT_SOURCE_FILENAME
      DBX_OUTPUT_SOURCE_FILENAME (file, filename);
#else
      char ltext_label_name[100];

      ASM_GENERATE_INTERNAL_LABEL (ltext_label_name, "Ltext",
				   source_label_number);
      fprintf (file, "%s", ASM_STABS_OP);
      output_quoted_string (file, filename);
      /* APPLE LOCAL begin 3109828 fix */
      fprintf (asmfile, ",%d,0,0,0\n", N_SOL);
      if (current_function_decl != NULL_TREE
	  && DECL_SECTION_NAME (current_function_decl) != NULL_TREE)
	; /* Don't change section amid function.  */
      else
	{
	  if (!in_unlikely_text_section())
	    text_section ();
	}
      /* APPLE LOCAL end 310928 fix */
      source_label_number++;
#endif
      lastfile = filename;
    }
}

/* Output a line number symbol entry for source file FILENAME and line
   number LINENO.  */

static void
dbxout_source_line (lineno, filename)
     unsigned int lineno;
     const char *filename;
{
  dbxout_source_file (asmfile, filename);

#ifdef ASM_OUTPUT_SOURCE_LINE
  ASM_OUTPUT_SOURCE_LINE (asmfile, lineno);
#else
  fprintf (asmfile, "%s%d,0,%d\n", ASM_STABD_OP, N_SLINE, lineno);
#endif
}

/* Describe the beginning of an internal block within a function.  */

static void
dbxout_begin_block (line, n)
     unsigned int line ATTRIBUTE_UNUSED;
     unsigned int n;
{
  /* APPLE LOCAL empty BINCL/EINCL optimization */
    emit_pending_bincls_if_required ();

  ASM_OUTPUT_INTERNAL_LABEL (asmfile, "LBB", n);
}

/* Describe the end line-number of an internal block within a function.  */

static void
dbxout_end_block (line, n)
     unsigned int line ATTRIBUTE_UNUSED;
     unsigned int n;
{
  /* APPLE LOCAL empty BINCL/EINCL optimization */
  emit_pending_bincls_if_required ();

  ASM_OUTPUT_INTERNAL_LABEL (asmfile, "LBE", n);
}

/* Output dbx data for a function definition.
   This includes a definition of the function name itself (a symbol),
   definitions of the parameters (locating them in the parameter list)
   and then output the block that makes up the function's body
   (including all the auto variables of the function).  */

static void
dbxout_function_decl (decl)
     tree decl;
{
  /* APPLE LOCAL empty BINCL/EINCL optimization */
  emit_pending_bincls_if_required ();

#ifndef DBX_FUNCTION_FIRST
  dbxout_begin_function (decl);
#endif
  dbxout_block (DECL_INITIAL (decl), 0, DECL_ARGUMENTS (decl));
#ifdef DBX_OUTPUT_FUNCTION_END
  DBX_OUTPUT_FUNCTION_END (asmfile, decl);
#endif
  if (use_gnu_debug_info_extensions
#if defined(NO_DBX_FUNCTION_END)
      && ! NO_DBX_FUNCTION_END
#endif
      && targetm.have_named_sections)
    dbxout_function_end ();
}

#endif /* DBX_DEBUGGING_INFO  */

/* Debug information for a global DECL.  Called from toplev.c after
   compilation proper has finished.  */
static void
dbxout_global_decl (decl)
     tree decl;
{
  if (TREE_CODE (decl) == VAR_DECL
      && ! DECL_EXTERNAL (decl)
      && DECL_RTL_SET_P (decl))	/* Not necessary?  */
/* APPLE LOCAL gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
    {
      int saved_tree_used = TREE_USED (decl);
      TREE_USED (decl) = 1;
      dbxout_symbol (decl, 0);
      TREE_USED (decl) = saved_tree_used;
    }
#else
    dbxout_symbol (decl, 0);
#endif
}

/* At the end of compilation, finish writing the symbol table.
   Unless you define DBX_OUTPUT_MAIN_SOURCE_FILE_END, the default is
   to do nothing.  */

static void
dbxout_finish (filename)
     const char *filename ATTRIBUTE_UNUSED;
{
#ifdef DBX_OUTPUT_MAIN_SOURCE_FILE_END
  DBX_OUTPUT_MAIN_SOURCE_FILE_END (asmfile, filename);
#endif /* DBX_OUTPUT_MAIN_SOURCE_FILE_END */
/* APPLE LOCAL begin gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
  if (symbol_queue)
    {
      free (symbol_queue);
      symbol_queue = NULL;
      symbol_queue_size = 0;
    }
#endif
}

/* Output floating point type values used by the 'R' stab letter.
   These numbers come from include/aout/stab_gnu.h in binutils/gdb.

   There are only 3 real/complex types defined, and we need 7/6.
   We use NF_SINGLE as a generic float type, and NF_COMPLEX as a generic
   complex type.  Since we have the type size anyways, we don't really need
   to distinguish between different FP types, we only need to distinguish
   between float and complex.  This works fine with gdb.

   We only use this for complex types, to avoid breaking backwards
   compatibility for real types.  complex types aren't in ISO C90, so it is
   OK if old debuggers don't understand the debug info we emit for them.  */

/* ??? These are supposed to be IEEE types, but we don't check for that.
   We could perhaps add additional numbers for non-IEEE types if we need
   them.  */

static void
dbxout_fptype_value (type)
     tree type;
{
  char value = '0';
  enum machine_mode mode = TYPE_MODE (type);

  if (TREE_CODE (type) == REAL_TYPE)
    {
      if (mode == SFmode)
	value = '1';
      else if (mode == DFmode)
	value = '2';
      else if (mode == TFmode || mode == XFmode)
	value = '6';
      else
	/* Use NF_SINGLE as a generic real type for other sizes.  */
	value = '1';
    }
  else if (TREE_CODE (type) == COMPLEX_TYPE)
    {
      if (mode == SCmode)
	value = '3';
      else if (mode == DCmode)
	value = '4';
      else if (mode == TCmode || mode == XCmode)
	value = '5';
      else
	/* Use NF_COMPLEX as a generic complex type for other sizes.  */
	value = '3';
    }
  else
    abort ();

  putc (value, asmfile);
  CHARS (1);
}

/* Output the index of a type.  */

static void
dbxout_type_index (type)
     tree type;
{
#ifndef DBX_USE_BINCL
  fprintf (asmfile, "%d", TYPE_SYMTAB_ADDRESS (type));
  CHARS (3);
#else
  struct typeinfo *t = &typevec[TYPE_SYMTAB_ADDRESS (type)];
  fprintf (asmfile, "(%d,%d)", t->file_number, t->type_number);
  CHARS (9);
#endif
}

#if DBX_CONTIN_LENGTH > 0
/* Continue a symbol-description that gets too big.
   End one symbol table entry with a double-backslash
   and start a new one, eventually producing something like
   .stabs "start......\\",code,0,value
   .stabs "...rest",code,0,value   */

static void
dbxout_continue ()
{
  /* APPLE LOCAL empty BINCL/EINCL optimization */
  emit_pending_bincls_if_required ();

#ifdef DBX_CONTIN_CHAR
  fprintf (asmfile, "%c", DBX_CONTIN_CHAR);
#else
  fprintf (asmfile, "\\\\");
#endif
  dbxout_finish_symbol (NULL_TREE);
  fprintf (asmfile, "%s\"", ASM_STABS_OP);
  current_sym_nchars = 0;
}
#endif /* DBX_CONTIN_LENGTH > 0 */

/* Subroutine of `dbxout_type'.  Output the type fields of TYPE.
   This must be a separate function because anonymous unions require
   recursive calls.  */

static void
dbxout_type_fields (type)
     tree type;
{
  tree tem;

  /* Output the name, type, position (in bits), size (in bits) of each
     field that we can support.  */
  for (tem = TYPE_FIELDS (type); tem; tem = TREE_CHAIN (tem))
    {
      /* Omit here local type decls until we know how to support them.  */
      if (TREE_CODE (tem) == TYPE_DECL
	  /* Omit fields whose position or size are variable or too large to
	     represent.  */
	  || (TREE_CODE (tem) == FIELD_DECL
	      && (! host_integerp (bit_position (tem), 0)
		  || ! DECL_SIZE (tem)
		  || ! host_integerp (DECL_SIZE (tem), 1)))
	  /* Omit here the nameless fields that are used to skip bits.  */
	   || DECL_IGNORED_P (tem))
	continue;

      else if (TREE_CODE (tem) != CONST_DECL)
	{
	  /* Continue the line if necessary,
	     but not before the first field.  */
	  if (tem != TYPE_FIELDS (type))
	    CONTIN;

	  if (DECL_NAME (tem))
	    {
	      fprintf (asmfile, "%s:", IDENTIFIER_POINTER (DECL_NAME (tem)));
	      CHARS (2 + IDENTIFIER_LENGTH (DECL_NAME (tem)));
	    }
	  else
	    {
	      fprintf (asmfile, ":");
	      CHARS (1);
	    }

	  if (use_gnu_debug_info_extensions
	      && (TREE_PRIVATE (tem) || TREE_PROTECTED (tem)
		  || TREE_CODE (tem) != FIELD_DECL))
	    {
	      have_used_extensions = 1;
	      putc ('/', asmfile);
	      putc ((TREE_PRIVATE (tem) ? '0'
		     : TREE_PROTECTED (tem) ? '1' : '2'),
		    asmfile);
	      CHARS (2);
	    }

	  dbxout_type ((TREE_CODE (tem) == FIELD_DECL
			&& DECL_BIT_FIELD_TYPE (tem))
		       ? DECL_BIT_FIELD_TYPE (tem) : TREE_TYPE (tem), 0);

	  if (TREE_CODE (tem) == VAR_DECL)
	    {
	      if (TREE_STATIC (tem) && use_gnu_debug_info_extensions)
		{
		  tree name = DECL_ASSEMBLER_NAME (tem);

		  have_used_extensions = 1;
		  fprintf (asmfile, ":%s;", IDENTIFIER_POINTER (name));
		  CHARS (IDENTIFIER_LENGTH (name) + 2);
		}
	      else
		{
		  /* If TEM is non-static, GDB won't understand it.  */
		  fprintf (asmfile, ",0,0;");
		  CHARS (5);
		}
	    }
	  else
	    {
	      putc (',', asmfile);
	      print_wide_int (int_bit_position (tem));
	      putc (',', asmfile);
	      print_wide_int (tree_low_cst (DECL_SIZE (tem), 1));
	      putc (';', asmfile);
	      CHARS (3);
	    }
	}
    }
}

/* Subroutine of `dbxout_type_methods'.  Output debug info about the
   method described DECL.  DEBUG_NAME is an encoding of the method's
   type signature.  ??? We may be able to do without DEBUG_NAME altogether
   now.  */

static void
dbxout_type_method_1 (decl, debug_name)
     tree decl;
     const char *debug_name;
{
  char c1 = 'A', c2;

  if (TREE_CODE (TREE_TYPE (decl)) == FUNCTION_TYPE)
    c2 = '?';
  else /* it's a METHOD_TYPE.  */
    {
      tree firstarg = TREE_VALUE (TYPE_ARG_TYPES (TREE_TYPE (decl)));
      /* A for normal functions.
	 B for `const' member functions.
	 C for `volatile' member functions.
	 D for `const volatile' member functions.  */
      if (TYPE_READONLY (TREE_TYPE (firstarg)))
	c1 += 1;
      if (TYPE_VOLATILE (TREE_TYPE (firstarg)))
	c1 += 2;

      if (DECL_VINDEX (decl))
	c2 = '*';
      else
	c2 = '.';
    }

  fprintf (asmfile, ":%s;%c%c%c", debug_name,
	   TREE_PRIVATE (decl) ? '0'
	   : TREE_PROTECTED (decl) ? '1' : '2', c1, c2);
  CHARS (IDENTIFIER_LENGTH (DECL_ASSEMBLER_NAME (decl)) + 6
	 - (debug_name - IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl))));

  if (DECL_VINDEX (decl) && host_integerp (DECL_VINDEX (decl), 0))
    {
      print_wide_int (tree_low_cst (DECL_VINDEX (decl), 0));
      putc (';', asmfile);
      CHARS (1);
      dbxout_type (DECL_CONTEXT (decl), 0);
      fprintf (asmfile, ";");
      CHARS (1);
    }
}

/* Subroutine of `dbxout_type'.  Output debug info about the methods defined
   in TYPE.  */

static void
dbxout_type_methods (type)
     tree type;
{
  /* C++: put out the method names and their parameter lists */
  tree methods = TYPE_METHODS (type);
  tree type_encoding;
  tree fndecl;
  tree last;
  char formatted_type_identifier_length[16];
  int type_identifier_length;

  if (methods == NULL_TREE)
    return;

  type_encoding = DECL_NAME (TYPE_NAME (type));

#if 0
  /* C++: Template classes break some assumptions made by this code about
     the class names, constructor names, and encodings for assembler
     label names.  For now, disable output of dbx info for them.  */
  {
    const char *ptr = IDENTIFIER_POINTER (type_encoding);
    /* This should use index.  (mrs) */
    while (*ptr && *ptr != '<') ptr++;
    if (*ptr != 0)
      {
	static int warned;
	if (!warned)
	    warned = 1;
	return;
      }
  }
#endif

  type_identifier_length = IDENTIFIER_LENGTH (type_encoding);

  sprintf (formatted_type_identifier_length, "%d", type_identifier_length);

  if (TREE_CODE (methods) != TREE_VEC)
    fndecl = methods;
  else if (TREE_VEC_ELT (methods, 0) != NULL_TREE)
    fndecl = TREE_VEC_ELT (methods, 0);
  else
    fndecl = TREE_VEC_ELT (methods, 1);

  while (fndecl)
    {
      int need_prefix = 1;

      /* Group together all the methods for the same operation.
	 These differ in the types of the arguments.  */
      for (last = NULL_TREE;
	   fndecl && (last == NULL_TREE || DECL_NAME (fndecl) == DECL_NAME (last));
	   fndecl = TREE_CHAIN (fndecl))
	/* Output the name of the field (after overloading), as
	   well as the name of the field before overloading, along
	   with its parameter list */
	{
	  /* This is the "mangled" name of the method.
	     It encodes the argument types.  */
	  const char *debug_name;

	  /* Skip methods that aren't FUNCTION_DECLs.  (In C++, these
	     include TEMPLATE_DECLs.)  The debugger doesn't know what
	     to do with such entities anyhow.  */
	  if (TREE_CODE (fndecl) != FUNCTION_DECL)
	    continue;

	  debug_name = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fndecl));

	  CONTIN;

	  last = fndecl;

	  /* Also ignore abstract methods; those are only interesting to
	     the DWARF backends.  */
	  if (DECL_IGNORED_P (fndecl) || DECL_ABSTRACT (fndecl))
	    continue;

	  /* Redundantly output the plain name, since that's what gdb
	     expects.  */
	  if (need_prefix)
	    {
	      tree name = DECL_NAME (fndecl);
	      fprintf (asmfile, "%s::", IDENTIFIER_POINTER (name));
	      CHARS (IDENTIFIER_LENGTH (name) + 2);
	      need_prefix = 0;
	    }

	  dbxout_type (TREE_TYPE (fndecl), 0);

	  dbxout_type_method_1 (fndecl, debug_name);
	}
      if (!need_prefix)
	{
	  putc (';', asmfile);
	  CHARS (1);
	}
    }
}

/* Emit a "range" type specification, which has the form:
   "r<index type>;<lower bound>;<upper bound>;".
   TYPE is an INTEGER_TYPE.  */

static void
dbxout_range_type (type)
     tree type;
{
  fprintf (asmfile, "r");
  if (TREE_TYPE (type))
    dbxout_type (TREE_TYPE (type), 0);
  else if (TREE_CODE (type) != INTEGER_TYPE)
    dbxout_type (type, 0); /* E.g. Pascal's ARRAY [BOOLEAN] of INTEGER */
  else
    {
      /* Traditionally, we made sure 'int' was type 1, and builtin types
	 were defined to be sub-ranges of int.  Unfortunately, this
	 does not allow us to distinguish true sub-ranges from integer
	 types.  So, instead we define integer (non-sub-range) types as
	 sub-ranges of themselves.  This matters for Chill.  If this isn't
	 a subrange type, then we want to define it in terms of itself.
	 However, in C, this may be an anonymous integer type, and we don't
	 want to emit debug info referring to it.  Just calling
	 dbxout_type_index won't work anyways, because the type hasn't been
	 defined yet.  We make this work for both cases by checked to see
	 whether this is a defined type, referring to it if it is, and using
	 'int' otherwise.  */
      if (TYPE_SYMTAB_ADDRESS (type) != 0)
	dbxout_type_index (type);
      else
	dbxout_type_index (integer_type_node);
    }

  if (TYPE_MIN_VALUE (type) != 0
      && host_integerp (TYPE_MIN_VALUE (type), 0))
    {
      putc (';', asmfile);
      CHARS (1);
      print_wide_int (tree_low_cst (TYPE_MIN_VALUE (type), 0));
    }
  else
    {
      fprintf (asmfile, ";0");
      CHARS (2);
    }

  if (TYPE_MAX_VALUE (type) != 0
      && host_integerp (TYPE_MAX_VALUE (type), 0))
    {
      putc (';', asmfile);
      CHARS (1);
      print_wide_int (tree_low_cst (TYPE_MAX_VALUE (type), 0));
      putc (';', asmfile);
      CHARS (1);
    }
  else
    {
      fprintf (asmfile, ";-1;");
      CHARS (4);
    }
}

/* APPLE LOCAL begin gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
/* Generate the symbols for any queued up type symbols we encountered
   while generating the type info for some originally used symbol.
   This might generate additional entries in the queue.  Only when
   the nesting depth goes to 0 is this routine called.  */

static void
dbxout_flush_symbol_queue ()
{
  int i;
  
  /* APPLE LOCAL empty BINCL/EINCL optimization */
  emit_pending_bincls_if_required ();


  /* Make sure that additionally queued items are not flushed
     prematurely.  */
     
  ++dbxout_nesting;
  
  for (i = 0; i < symbol_queue_index; ++i)
    {
      /* If we pushed queued symbols then such symbols are must be
         output no matter what anyone else says.  Specifically,
         we need to make sure dbxout_symbol() thinks the symbol was
         used and also we need to override TYPE_DECL_SUPPRESS_DEBUG
         which may be set for outside reasons.  */
      int saved_tree_used = TREE_USED (symbol_queue[i]);
      int saved_suppress_debug = TYPE_DECL_SUPPRESS_DEBUG (symbol_queue[i]);
      TREE_USED (symbol_queue[i]) = 1;
      TYPE_DECL_SUPPRESS_DEBUG (symbol_queue[i]) = 0;

      dbxout_symbol (symbol_queue[i], 0);

      TREE_USED (symbol_queue[i]) = saved_tree_used;
      TYPE_DECL_SUPPRESS_DEBUG (symbol_queue[i]) = saved_suppress_debug;
    }

  symbol_queue_index = 0;
  --dbxout_nesting;
}

/* Queue a type symbol needed as part of the definition of a decl
   symbol.  These symbols are generated when dbxout_flush_symbol_queue()
   is called.  */
   
static void
dbxout_queue_symbol (decl)
     tree decl;
{
  if (symbol_queue_index >= symbol_queue_size)
    {
      symbol_queue_size += 10;
      symbol_queue = (tree *) xrealloc (symbol_queue,
      				 	symbol_queue_size * sizeof (tree));
    }
    
  symbol_queue[symbol_queue_index++] = decl;
}
#endif /* DBX_ONLY_USED_SYMBOLS */
/* APPLE LOCAL end gdb only used symbols */

/* Output a reference to a type.  If the type has not yet been
   described in the dbx output, output its definition now.
   For a type already defined, just refer to its definition
   using the type number.

   If FULL is nonzero, and the type has been described only with
   a forward-reference, output the definition now.
   If FULL is zero in this case, just refer to the forward-reference
   using the number previously allocated.  */

static void
dbxout_type (type, full)
     tree type;
     int full;
{
  tree tem;
  tree main_variant;
  static int anonymous_type_number = 0;

  if (TREE_CODE (type) == VECTOR_TYPE)
    /* The frontend feeds us a representation for the vector as a struct
       containing an array.  Pull out the array type.  */
    type = TREE_TYPE (TYPE_FIELDS (TYPE_DEBUG_REPRESENTATION_TYPE (type)));

  /* If there was an input error and we don't really have a type,
     avoid crashing and write something that is at least valid
     by assuming `int'.  */
  if (type == error_mark_node)
    type = integer_type_node;
  else
    {
      if (TYPE_NAME (type)
	  && TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
	  && TYPE_DECL_SUPPRESS_DEBUG (TYPE_NAME (type)))
	full = 0;
    }

  /* Try to find the "main variant" with the same name.  */
  if (TYPE_NAME (type) && TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
      && DECL_ORIGINAL_TYPE (TYPE_NAME (type)))
    main_variant = TREE_TYPE (TYPE_NAME (type));
  else
    main_variant = TYPE_MAIN_VARIANT (type);

  /* If we are not using extensions, stabs does not distinguish const and
     volatile, so there is no need to make them separate types.  */
  if (!use_gnu_debug_info_extensions)
    type = main_variant;

  if (TYPE_SYMTAB_ADDRESS (type) == 0)
    {
      /* Type has no dbx number assigned.  Assign next available number.  */
      TYPE_SYMTAB_ADDRESS (type) = next_type_number++;

      /* Make sure type vector is long enough to record about this type.  */

      if (next_type_number == typevec_len)
	{
	  typevec
	    = (struct typeinfo *) ggc_realloc (typevec,
					       (typevec_len * 2 
						* sizeof typevec[0]));
	  memset ((char *) (typevec + typevec_len), 0,
		 typevec_len * sizeof typevec[0]);
	  typevec_len *= 2;
	}

#ifdef DBX_USE_BINCL
      /* APPLE LOCAL empty BINCL/EINCL optimization */
	emit_pending_bincls_if_required ();

      typevec[TYPE_SYMTAB_ADDRESS (type)].file_number
	= current_file->file_number;
      typevec[TYPE_SYMTAB_ADDRESS (type)].type_number
	= current_file->next_type_number++;
#endif

/* APPLE LOCAL gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
      if (flag_debug_only_used_symbols)
        {
	  if ((TREE_CODE (type) == RECORD_TYPE
	       || TREE_CODE (type) == UNION_TYPE
	       || TREE_CODE (type) == QUAL_UNION_TYPE
	       || TREE_CODE (type) == ENUMERAL_TYPE)
	      && TYPE_STUB_DECL (type)
	      && TREE_CODE_CLASS (TREE_CODE (TYPE_STUB_DECL (type))) == 'd'
	      && ! DECL_IGNORED_P (TYPE_STUB_DECL (type)))
	    dbxout_queue_symbol (TYPE_STUB_DECL (type));
	  else if (TYPE_NAME (type)
	    	   && TREE_CODE (TYPE_NAME (type)) == TYPE_DECL)
	    dbxout_queue_symbol (TYPE_NAME (type));
        }
#endif
    }

  /* Output the number of this type, to refer to it.  */
  dbxout_type_index (type);

#ifdef DBX_TYPE_DEFINED
  if (DBX_TYPE_DEFINED (type))
    return;
#endif

  /* If this type's definition has been output or is now being output,
     that is all.  */

  switch (typevec[TYPE_SYMTAB_ADDRESS (type)].status)
    {
    case TYPE_UNSEEN:
      break;
    case TYPE_XREF:
      /* If we have already had a cross reference,
	 and either that's all we want or that's the best we could do,
	 don't repeat the cross reference.
	 Sun dbx crashes if we do.  */
      if (! full || !COMPLETE_TYPE_P (type)
	  /* No way in DBX fmt to describe a variable size.  */
	  || ! host_integerp (TYPE_SIZE (type), 1))
	return;
      break;
    case TYPE_DEFINED:
      return;
    }

#ifdef DBX_NO_XREFS
  /* For systems where dbx output does not allow the `=xsNAME:' syntax,
     leave the type-number completely undefined rather than output
     a cross-reference.  If we have already used GNU debug info extensions,
     then it is OK to output a cross reference.  This is necessary to get
     proper C++ debug output.  */
  if ((TREE_CODE (type) == RECORD_TYPE || TREE_CODE (type) == UNION_TYPE
       || TREE_CODE (type) == QUAL_UNION_TYPE
       || TREE_CODE (type) == ENUMERAL_TYPE)
      && ! use_gnu_debug_info_extensions)
    /* We must use the same test here as we use twice below when deciding
       whether to emit a cross-reference.  */
    if ((TYPE_NAME (type) != 0
	 && ! (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
	       && DECL_IGNORED_P (TYPE_NAME (type)))
	 && !full)
	|| !COMPLETE_TYPE_P (type)
	/* No way in DBX fmt to describe a variable size.  */
	|| ! host_integerp (TYPE_SIZE (type), 1))
      {
	typevec[TYPE_SYMTAB_ADDRESS (type)].status = TYPE_XREF;
	return;
      }
#endif

  /* Output a definition now.  */

  fprintf (asmfile, "=");
  CHARS (1);

  /* Mark it as defined, so that if it is self-referent
     we will not get into an infinite recursion of definitions.  */

  typevec[TYPE_SYMTAB_ADDRESS (type)].status = TYPE_DEFINED;

  /* If this type is a variant of some other, hand off.  Types with
     different names are usefully distinguished.  We only distinguish
     cv-qualified types if we're using extensions.  */
  if (TYPE_READONLY (type) > TYPE_READONLY (main_variant))
    {
      putc ('k', asmfile);
      CHARS (1);
      dbxout_type (build_type_variant (type, 0, TYPE_VOLATILE (type)), 0);
      return;
    }
  else if (TYPE_VOLATILE (type) > TYPE_VOLATILE (main_variant))
    {
      putc ('B', asmfile);
      CHARS (1);
      dbxout_type (build_type_variant (type, TYPE_READONLY (type), 0), 0);
      return;
    }
  else if (main_variant != TYPE_MAIN_VARIANT (type))
    {
      /* APPLE LOCAL begin gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
      if (flag_debug_only_used_symbols)
        {
          tree orig_type = DECL_ORIGINAL_TYPE (TYPE_NAME (type));
          
	  if ((TREE_CODE (orig_type) == RECORD_TYPE
	       || TREE_CODE (orig_type) == UNION_TYPE
	       || TREE_CODE (orig_type) == QUAL_UNION_TYPE
	       || TREE_CODE (orig_type) == ENUMERAL_TYPE)
	      && TYPE_STUB_DECL (orig_type)
	      && ! DECL_IGNORED_P (TYPE_STUB_DECL (orig_type)))
	    dbxout_queue_symbol (TYPE_STUB_DECL (orig_type));
      	}
#endif
      /* APPLE LOCAL end gdb only used symbols */
      /* 'type' is a typedef; output the type it refers to.  */
      dbxout_type (DECL_ORIGINAL_TYPE (TYPE_NAME (type)), 0);
      return;
    }
  /* else continue.  */

  switch (TREE_CODE (type))
    {
    case VOID_TYPE:
    case LANG_TYPE:
      /* For a void type, just define it as itself; ie, "5=5".
	 This makes us consider it defined
	 without saying what it is.  The debugger will make it
	 a void type when the reference is seen, and nothing will
	 ever override that default.  */
      dbxout_type_index (type);
      break;

    case INTEGER_TYPE:
      if (type == char_type_node && ! TREE_UNSIGNED (type))
	{
	  /* Output the type `char' as a subrange of itself!
	     I don't understand this definition, just copied it
	     from the output of pcc.
	     This used to use `r2' explicitly and we used to
	     take care to make sure that `char' was type number 2.  */
	  fprintf (asmfile, "r");
	  CHARS (1);
	  dbxout_type_index (type);
	  fprintf (asmfile, ";0;127;");
	  CHARS (7);
	}

      /* If this is a subtype of another integer type, always prefer to
	 write it as a subtype.  */
      else if (TREE_TYPE (type) != 0
	       && TREE_CODE (TREE_TYPE (type)) == INTEGER_TYPE)
	{
	  /* If the size is non-standard, say what it is if we can use
	     GDB extensions.  */

	  if (use_gnu_debug_info_extensions
	      && TYPE_PRECISION (type) != TYPE_PRECISION (integer_type_node))
	    {
	      have_used_extensions = 1;
	      fprintf (asmfile, "@s%d;", TYPE_PRECISION (type));
	      CHARS (5);
	    }

	  dbxout_range_type (type);
	}

      else
	{
	  /* If the size is non-standard, say what it is if we can use
	     GDB extensions.  */

	  if (use_gnu_debug_info_extensions
	      && TYPE_PRECISION (type) != TYPE_PRECISION (integer_type_node))
	    {
	      have_used_extensions = 1;
	      fprintf (asmfile, "@s%d;", TYPE_PRECISION (type));
	      CHARS (5);
	    }

	  /* If we can use GDB extensions and the size is wider than a
	     long (the size used by GDB to read them) or we may have
	     trouble writing the bounds the usual way, write them in
	     octal.  Note the test is for the *target's* size of "long",
	     not that of the host.  The host test is just to make sure we
	     can write it out in case the host wide int is narrower than the
	     target "long".  */

	  /* For unsigned types, we use octal if they are the same size or
	     larger.  This is because we print the bounds as signed decimal,
	     and hence they can't span same size unsigned types.  */

	  if (use_gnu_debug_info_extensions
	      && TYPE_MIN_VALUE (type) != 0
	      && TREE_CODE (TYPE_MIN_VALUE (type)) == INTEGER_CST
	      && TYPE_MAX_VALUE (type) != 0
	      && TREE_CODE (TYPE_MAX_VALUE (type)) == INTEGER_CST
	      && (TYPE_PRECISION (type) > TYPE_PRECISION (integer_type_node)
		  || ((TYPE_PRECISION (type)
		       == TYPE_PRECISION (integer_type_node))
		      && TREE_UNSIGNED (type))
		  || TYPE_PRECISION (type) > HOST_BITS_PER_WIDE_INT
		  || (TYPE_PRECISION (type) == HOST_BITS_PER_WIDE_INT
		      && TREE_UNSIGNED (type))))
	    {
	      fprintf (asmfile, "r");
	      CHARS (1);
	      dbxout_type_index (type);
	      fprintf (asmfile, ";");
	      CHARS (1);
	      print_int_cst_octal (TYPE_MIN_VALUE (type));
	      fprintf (asmfile, ";");
	      CHARS (1);
	      print_int_cst_octal (TYPE_MAX_VALUE (type));
	      fprintf (asmfile, ";");
	      CHARS (1);
	    }

	  else
	    /* Output other integer types as subranges of `int'.  */
	    dbxout_range_type (type);
	}

      break;

    case REAL_TYPE:
      /* This used to say `r1' and we used to take care
	 to make sure that `int' was type number 1.  */
      fprintf (asmfile, "r");
      CHARS (1);
      dbxout_type_index (integer_type_node);
      putc (';', asmfile);
      CHARS (1);
      print_wide_int (int_size_in_bytes (type));
      fputs (";0;", asmfile);
      CHARS (3);
      break;

    case CHAR_TYPE:
      if (use_gnu_debug_info_extensions)
	{
	  have_used_extensions = 1;
	  fputs ("@s", asmfile);
	  CHARS (2);
	  print_wide_int (BITS_PER_UNIT * int_size_in_bytes (type));
	  fputs (";-20;", asmfile);
	  CHARS (4);
	}
      else
	{
	  /* Output the type `char' as a subrange of itself.
	     That is what pcc seems to do.  */
	  fprintf (asmfile, "r");
	  CHARS (1);
	  dbxout_type_index (char_type_node);
	  fprintf (asmfile, ";0;%d;", TREE_UNSIGNED (type) ? 255 : 127);
	  CHARS (7);
	}
      break;

    case BOOLEAN_TYPE:
      if (use_gnu_debug_info_extensions)
	{
	  have_used_extensions = 1;
	  fputs ("@s", asmfile);
	  CHARS (2);
	  print_wide_int (BITS_PER_UNIT * int_size_in_bytes (type));
	  fputs (";-16;", asmfile);
	  CHARS (4);
	}
      else /* Define as enumeral type (False, True) */
	{
	  fprintf (asmfile, "eFalse:0,True:1,;");
	  CHARS (17);
	}
      break;

    case FILE_TYPE:
      putc ('d', asmfile);
      CHARS (1);
      dbxout_type (TREE_TYPE (type), 0);
      break;

    case COMPLEX_TYPE:
      /* Differs from the REAL_TYPE by its new data type number */

      if (TREE_CODE (TREE_TYPE (type)) == REAL_TYPE)
	{
	  putc ('R', asmfile);
	  CHARS (1);
	  dbxout_fptype_value (type);
	  putc (';', asmfile);
	  CHARS (1);
	  print_wide_int (2 * int_size_in_bytes (TREE_TYPE (type)));
	  fputs (";0;", asmfile);
	  CHARS (3);
	}
      else
	{
	  /* Output a complex integer type as a structure,
	     pending some other way to do it.  */
	  putc ('s', asmfile);
	  CHARS (1);
	  print_wide_int (int_size_in_bytes (type));
	  fprintf (asmfile, "real:");
	  CHARS (5);

	  dbxout_type (TREE_TYPE (type), 0);
	  fprintf (asmfile, ",0,%d;", TYPE_PRECISION (TREE_TYPE (type)));
	  CHARS (7);
	  fprintf (asmfile, "imag:");
	  CHARS (5);
	  dbxout_type (TREE_TYPE (type), 0);
	  fprintf (asmfile, ",%d,%d;;", TYPE_PRECISION (TREE_TYPE (type)),
		   TYPE_PRECISION (TREE_TYPE (type)));
	  CHARS (10);
	}
      break;

    case SET_TYPE:
      if (use_gnu_debug_info_extensions)
	{
	  have_used_extensions = 1;
	  fputs ("@s", asmfile);
	  CHARS (2);
	  print_wide_int (BITS_PER_UNIT * int_size_in_bytes (type));
	  putc (';', asmfile);
	  CHARS (1);

	  /* Check if a bitstring type, which in Chill is
	     different from a [power]set.  */
	  if (TYPE_STRING_FLAG (type))
	    {
	      fprintf (asmfile, "@S;");
	      CHARS (3);
	    }
	}
      putc ('S', asmfile);
      CHARS (1);
      dbxout_type (TYPE_DOMAIN (type), 0);
      break;

    case ARRAY_TYPE:
      /* Make arrays of packed bits look like bitstrings for chill.  */
      if (TYPE_PACKED (type) && use_gnu_debug_info_extensions)
	{
	  have_used_extensions = 1;
	  fputs ("@s", asmfile);
	  CHARS (2);
	  print_wide_int (BITS_PER_UNIT * int_size_in_bytes (type));
	  fprintf (asmfile, ";@S;S");
	  CHARS (5);
	  dbxout_type (TYPE_DOMAIN (type), 0);
	  break;
	}

      /* Output "a" followed by a range type definition
	 for the index type of the array
	 followed by a reference to the target-type.
	 ar1;0;N;M for a C array of type M and size N+1.  */
      /* Check if a character string type, which in Chill is
	 different from an array of characters.  */
      if (TYPE_STRING_FLAG (type) && use_gnu_debug_info_extensions)
	{
	  have_used_extensions = 1;
	  fprintf (asmfile, "@S;");
	  CHARS (3);
	}
      tem = TYPE_DOMAIN (type);
      if (tem == NULL)
	{
	  fprintf (asmfile, "ar");
	  CHARS (2);
	  dbxout_type_index (integer_type_node);
	  fprintf (asmfile, ";0;-1;");
	  CHARS (6);
	}
      else
	{
	  fprintf (asmfile, "a");
	  CHARS (1);
	  dbxout_range_type (tem);
	}

      dbxout_type (TREE_TYPE (type), 0);
      break;

    case RECORD_TYPE:
    case UNION_TYPE:
    case QUAL_UNION_TYPE:
      {
	int i, n_baseclasses = 0;

	if (TYPE_BINFO (type) != 0
	    && TREE_CODE (TYPE_BINFO (type)) == TREE_VEC
	    && TYPE_BINFO_BASETYPES (type) != 0)
	  n_baseclasses = TREE_VEC_LENGTH (TYPE_BINFO_BASETYPES (type));

	/* Output a structure type.  We must use the same test here as we
	   use in the DBX_NO_XREFS case above.  */
	if ((TYPE_NAME (type) != 0
	     && ! (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
		   && DECL_IGNORED_P (TYPE_NAME (type)))
	     && !full)
	    || !COMPLETE_TYPE_P (type)
	    /* No way in DBX fmt to describe a variable size.  */
	    || ! host_integerp (TYPE_SIZE (type), 1))
	  {
	    /* If the type is just a cross reference, output one
	       and mark the type as partially described.
	       If it later becomes defined, we will output
	       its real definition.
	       If the type has a name, don't nest its definition within
	       another type's definition; instead, output an xref
	       and let the definition come when the name is defined.  */
	    fputs ((TREE_CODE (type) == RECORD_TYPE) ? "xs" : "xu", asmfile);
	    CHARS (2);
#if 0 /* This assertion is legitimately false in C++.  */
	    /* We shouldn't be outputting a reference to a type before its
	       definition unless the type has a tag name.
	       A typedef name without a tag name should be impossible.  */
	    if (TREE_CODE (TYPE_NAME (type)) != IDENTIFIER_NODE)
	      abort ();
#endif
	    if (TYPE_NAME (type) != 0)
	      dbxout_type_name (type);
	    else
	      {
		fprintf (asmfile, "$$%d", anonymous_type_number++);
		CHARS (5);
	      }

	    fprintf (asmfile, ":");
	    CHARS (1);
	    typevec[TYPE_SYMTAB_ADDRESS (type)].status = TYPE_XREF;
	    break;
	  }

	/* Identify record or union, and print its size.  */
	putc (((TREE_CODE (type) == RECORD_TYPE) ? 's' : 'u'), asmfile);
	CHARS (1);
	print_wide_int (int_size_in_bytes (type));

	if (use_gnu_debug_info_extensions)
	  {
	    if (n_baseclasses)
	      {
		have_used_extensions = 1;
		fprintf (asmfile, "!%d,", n_baseclasses);
		CHARS (8);
	      }
	  }
	for (i = 0; i < n_baseclasses; i++)
	  {
	    tree child = TREE_VEC_ELT (BINFO_BASETYPES (TYPE_BINFO (type)), i);

	    if (use_gnu_debug_info_extensions)
	      {
		have_used_extensions = 1;
		putc (TREE_VIA_VIRTUAL (child) ? '1' : '0', asmfile);
		putc (TREE_VIA_PUBLIC (child) ? '2' : '0', asmfile);
		CHARS (2);
		if (TREE_VIA_VIRTUAL (child) && strcmp (lang_hooks.name, "GNU C++") == 0)
		  /* For a virtual base, print the (negative) offset within
		     the vtable where we must look to find the necessary
		     adjustment.  */
		  print_wide_int (tree_low_cst (BINFO_VPTR_FIELD (child), 0)
				  * BITS_PER_UNIT);
		else
		  print_wide_int (tree_low_cst (BINFO_OFFSET (child), 0)
				  * BITS_PER_UNIT);
		putc (',', asmfile);
		CHARS (1);
		dbxout_type (BINFO_TYPE (child), 0);
		putc (';', asmfile);
		CHARS (1);
	      }
	    else
	      {
		/* Print out the base class information with fields
		   which have the same names at the types they hold.  */
		dbxout_type_name (BINFO_TYPE (child));
		putc (':', asmfile);
		CHARS (1);
		dbxout_type (BINFO_TYPE (child), full);
		putc (',', asmfile);
		CHARS (1);
		print_wide_int (tree_low_cst (BINFO_OFFSET (child), 0)
				* BITS_PER_UNIT);
		putc (',', asmfile);
		CHARS (1);
		print_wide_int (tree_low_cst (DECL_SIZE
					      (TYPE_NAME
					       (BINFO_TYPE (child))),
					      0)
				* BITS_PER_UNIT);
		putc (';', asmfile);
		CHARS (1);
	      }
	  }
      }

      /* Write out the field declarations.  */
      dbxout_type_fields (type);
      if (use_gnu_debug_info_extensions && TYPE_METHODS (type) != NULL_TREE)
	{
	  have_used_extensions = 1;
	  dbxout_type_methods (type);
	}

      putc (';', asmfile);
      CHARS (1);

      if (use_gnu_debug_info_extensions && TREE_CODE (type) == RECORD_TYPE
	  /* Avoid the ~ if we don't really need it--it confuses dbx.  */
	  && TYPE_VFIELD (type))
	{
	  have_used_extensions = 1;

	  /* Tell GDB+ that it may keep reading.  */
	  putc ('~', asmfile);
	  CHARS (1);

	  /* We need to write out info about what field this class
	     uses as its "main" vtable pointer field, because if this
	     field is inherited from a base class, GDB cannot necessarily
	     figure out which field it's using in time.  */
	  if (TYPE_VFIELD (type))
	    {
	      putc ('%', asmfile);
	      CHARS (1);
	      dbxout_type (DECL_FCONTEXT (TYPE_VFIELD (type)), 0);
	    }

	  putc (';', asmfile);
	  CHARS (1);
	}
      break;

    case ENUMERAL_TYPE:
      /* We must use the same test here as we use in the DBX_NO_XREFS case
	 above.  We simplify it a bit since an enum will never have a variable
	 size.  */
      if ((TYPE_NAME (type) != 0
	   && ! (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
		 && DECL_IGNORED_P (TYPE_NAME (type)))
	   && !full)
	  || !COMPLETE_TYPE_P (type))
	{
	  fprintf (asmfile, "xe");
	  CHARS (2);
	  dbxout_type_name (type);
	  typevec[TYPE_SYMTAB_ADDRESS (type)].status = TYPE_XREF;
	  putc (':', asmfile);
	  CHARS (1);
	  return;
	}
#ifdef DBX_OUTPUT_ENUM
      DBX_OUTPUT_ENUM (asmfile, type);
#else
      if (use_gnu_debug_info_extensions
	  && TYPE_PRECISION (type) != TYPE_PRECISION (integer_type_node))
	{
	  fprintf (asmfile, "@s%d;", TYPE_PRECISION (type));
	  CHARS (5);
	}

      putc ('e', asmfile);
      CHARS (1);
      for (tem = TYPE_VALUES (type); tem; tem = TREE_CHAIN (tem))
	{
	  fprintf (asmfile, "%s:", IDENTIFIER_POINTER (TREE_PURPOSE (tem)));
	  CHARS (IDENTIFIER_LENGTH (TREE_PURPOSE (tem)) + 1);
	  if (TREE_INT_CST_HIGH (TREE_VALUE (tem)) == 0)
	    print_wide_int (TREE_INT_CST_LOW (TREE_VALUE (tem)));
	  else if (TREE_INT_CST_HIGH (TREE_VALUE (tem)) == -1
		   && (HOST_WIDE_INT) TREE_INT_CST_LOW (TREE_VALUE (tem)) < 0)
	    print_wide_int (TREE_INT_CST_LOW (TREE_VALUE (tem)));
	  else
	    print_int_cst_octal (TREE_VALUE (tem));

	  putc (',', asmfile);
	  CHARS (1);
	  if (TREE_CHAIN (tem) != 0)
	    CONTIN;
	}

      putc (';', asmfile);
      CHARS (1);
#endif
      break;

    case POINTER_TYPE:
      putc ('*', asmfile);
      CHARS (1);
      dbxout_type (TREE_TYPE (type), 0);
      break;

    case METHOD_TYPE:
      if (use_gnu_debug_info_extensions)
	{
	  have_used_extensions = 1;
	  putc ('#', asmfile);
	  CHARS (1);

	  /* Write the argument types out longhand.  */
	  dbxout_type (TYPE_METHOD_BASETYPE (type), 0);
	  putc (',', asmfile);
	  CHARS (1);
	  dbxout_type (TREE_TYPE (type), 0);
	  dbxout_args (TYPE_ARG_TYPES (type));
	  putc (';', asmfile);
	  CHARS (1);
	}
      else
	/* Treat it as a function type.  */
	dbxout_type (TREE_TYPE (type), 0);
      break;

    case OFFSET_TYPE:
      if (use_gnu_debug_info_extensions)
	{
	  have_used_extensions = 1;
	  putc ('@', asmfile);
	  CHARS (1);
	  dbxout_type (TYPE_OFFSET_BASETYPE (type), 0);
	  putc (',', asmfile);
	  CHARS (1);
	  dbxout_type (TREE_TYPE (type), 0);
	}
      else
	/* Should print as an int, because it is really just an offset.  */
	dbxout_type (integer_type_node, 0);
      break;

    case REFERENCE_TYPE:
      if (use_gnu_debug_info_extensions)
	have_used_extensions = 1;
      putc (use_gnu_debug_info_extensions ? '&' : '*', asmfile);
      CHARS (1);
      dbxout_type (TREE_TYPE (type), 0);
      break;

    case FUNCTION_TYPE:
      putc ('f', asmfile);
      CHARS (1);
      dbxout_type (TREE_TYPE (type), 0);
      break;

    default:
      abort ();
    }
}

/* Print the value of integer constant C, in octal,
   handling double precision.  */

static void
print_int_cst_octal (c)
     tree c;
{
  unsigned HOST_WIDE_INT high = TREE_INT_CST_HIGH (c);
  unsigned HOST_WIDE_INT low = TREE_INT_CST_LOW (c);
  int excess = (3 - (HOST_BITS_PER_WIDE_INT % 3));
  unsigned int width = TYPE_PRECISION (TREE_TYPE (c));

  /* GDB wants constants with no extra leading "1" bits, so
     we need to remove any sign-extension that might be
     present.  */
  if (width == HOST_BITS_PER_WIDE_INT * 2)
    ;
  else if (width > HOST_BITS_PER_WIDE_INT)
    high &= (((HOST_WIDE_INT) 1 << (width - HOST_BITS_PER_WIDE_INT)) - 1);
  else if (width == HOST_BITS_PER_WIDE_INT)
    high = 0;
  else
    high = 0, low &= (((HOST_WIDE_INT) 1 << width) - 1);

  fprintf (asmfile, "0");
  CHARS (1);

  if (excess == 3)
    {
      print_octal (high, HOST_BITS_PER_WIDE_INT / 3);
      print_octal (low, HOST_BITS_PER_WIDE_INT / 3);
    }
  else
    {
      unsigned HOST_WIDE_INT beg = high >> excess;
      unsigned HOST_WIDE_INT middle
	= ((high & (((HOST_WIDE_INT) 1 << excess) - 1)) << (3 - excess)
	   | (low >> (HOST_BITS_PER_WIDE_INT / 3 * 3)));
      unsigned HOST_WIDE_INT end
	= low & (((unsigned HOST_WIDE_INT) 1
		  << (HOST_BITS_PER_WIDE_INT / 3 * 3))
		 - 1);

      fprintf (asmfile, "%o%01o", (int) beg, (int) middle);
      CHARS (2);
      print_octal (end, HOST_BITS_PER_WIDE_INT / 3);
    }
}

static void
print_octal (value, digits)
     unsigned HOST_WIDE_INT value;
     int digits;
{
  int i;

  for (i = digits - 1; i >= 0; i--)
    fprintf (asmfile, "%01o", (int) ((value >> (3 * i)) & 7));

  CHARS (digits);
}

/* Output C in decimal while adjusting the number of digits written.  */

static void
print_wide_int (c)
     HOST_WIDE_INT c;
{
  int digs = 0;

  fprintf (asmfile, HOST_WIDE_INT_PRINT_DEC, c);

  if (c < 0)
    digs++, c = -c;

  while (c > 0)
    c /= 10; digs++;

  CHARS (digs);
}

/* Output the name of type TYPE, with no punctuation.
   Such names can be set up either by typedef declarations
   or by struct, enum and union tags.  */

static void
dbxout_type_name (type)
     tree type;
{
  tree t;
  if (TYPE_NAME (type) == 0)
    abort ();
  if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
    {
      t = TYPE_NAME (type);
    }
  else if (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL)
    {
      t = DECL_NAME (TYPE_NAME (type));
    }
  else
    abort ();

  fprintf (asmfile, "%s", IDENTIFIER_POINTER (t));
  CHARS (IDENTIFIER_LENGTH (t));
}

/* Output leading leading struct or class names needed for qualifying
   type whose scope is limited to a struct or class.  */

static void
dbxout_class_name_qualifiers (decl)
     tree decl;
{
  tree context = decl_type_context (decl);

  if (context != NULL_TREE 
      && TREE_CODE(context) == RECORD_TYPE
      && TYPE_NAME (context) != 0 
      && (TREE_CODE (TYPE_NAME (context)) == IDENTIFIER_NODE
          || (DECL_NAME (TYPE_NAME (context)) != 0)))
    {
      tree name = TYPE_NAME (context);

      /* APPLE LOCAL empty BINCL/EINCL optimization */
      emit_pending_bincls_if_required ();

      if (TREE_CODE (name) == TYPE_DECL)
	{
	  dbxout_class_name_qualifiers (name);
	  name = DECL_NAME (name);
	}
      fprintf (asmfile, "%s::", IDENTIFIER_POINTER (name));
      CHARS (IDENTIFIER_LENGTH (name) + 2);
    }
}

/* Output a .stabs for the symbol defined by DECL,
   which must be a ..._DECL node in the normal namespace.
   It may be a CONST_DECL, a FUNCTION_DECL, a PARM_DECL or a VAR_DECL.
   LOCAL is nonzero if the scope is less than the entire file.
   Return 1 if a stabs might have been emitted.  */

int
dbxout_symbol (decl, local)
     tree decl;
     int local ATTRIBUTE_UNUSED;
{
  tree type = TREE_TYPE (decl);
  tree context = NULL_TREE;
  int result = 0;

  /* APPLE LOCAL gdb only used symbols */
  /* "Intercept" dbxout_symbol() calls like we do all debug_hooks.  */
  ++dbxout_nesting;
    
  /* Cast avoids warning in old compilers.  */
  current_sym_code = (STAB_CODE_TYPE) 0;
  current_sym_value = 0;
  current_sym_addr = 0;

  /* Ignore nameless syms, but don't ignore type tags.  */

  /* APPLE LOCAL gdb only used symbols */
  if ((DECL_NAME (decl) == 0 && TREE_CODE (decl) != TYPE_DECL)
      || DECL_IGNORED_P (decl))
    DBXOUT_DECR_NESTING_AND_RETURN (0);

/* APPLE LOCAL begin gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
  /* If we are to generate only the symbols actualy used then such
     symbol nodees are flagged with TREE_USED.  Ignore any that
     aren't flaged as TREE_USED.  */
     
  if (flag_debug_only_used_symbols)
    {
      tree t;
      
      if (!TREE_USED (decl)
	  && (TREE_CODE (decl) != VAR_DECL || !DECL_INITIAL (decl)))
        DBXOUT_DECR_NESTING_AND_RETURN (0);
        
      /* We now have a used symbol.  We need to generate the info for
         the symbol's type in addition to the symbol itself.  These
         type symbols are queued to be generated after were done with
         the symbol itself (done because the symbol's info is generated
         with fprintf's, etc. as it determines what's needed).  
         
         Note, because the TREE_TYPE(type) might be something like a
         pointer to a named type we need to look for the first name
         we see following the TREE_TYPE chain.  */

      t = type;
      while (POINTER_TYPE_P (t))
        t = TREE_TYPE (t);
      
      /* RECORD_TYPE, UNION_TYPE, QUAL_UNION_TYPE, and ENUMERAL_TYPE
         need special treatment.  The TYPE_STUB_DECL field in these
         types generally represents the tag name type we want to
         output.  In addition there  could be a typedef type with
         a different name.  In that case we also want to output
         that.  */
         
      if ((TREE_CODE (t) == RECORD_TYPE
	   || TREE_CODE (t) == UNION_TYPE
	   || TREE_CODE (t) == QUAL_UNION_TYPE
	   || TREE_CODE (t) == ENUMERAL_TYPE)
	  && TYPE_STUB_DECL (t)
	  && TYPE_STUB_DECL (t) != decl
	  && TREE_CODE_CLASS (TREE_CODE (TYPE_STUB_DECL (t))) == 'd'
	  && ! DECL_IGNORED_P (TYPE_STUB_DECL (t)))
	{
	  dbxout_queue_symbol (TYPE_STUB_DECL (t));
	  if (TYPE_NAME (t)
	      && TYPE_NAME (t) != TYPE_STUB_DECL (t)
	      && TYPE_NAME (t) != decl
	      && TREE_CODE_CLASS (TREE_CODE (TYPE_NAME (t))) == 'd')
	    dbxout_queue_symbol (TYPE_NAME (t));
	}
      else if (TYPE_NAME (t)
	  && TYPE_NAME (t) != decl
	  && TREE_CODE_CLASS (TREE_CODE (TYPE_NAME (t))) == 'd')
	dbxout_queue_symbol (TYPE_NAME (t));
    }
#endif
/* APPLE LOCAL end gdb only used symbols */

  /* APPLE LOCAL end empty BINCL/EINCL optimization */
  /* Be conservative for now.  */
  emit_pending_bincls_if_required ();

  dbxout_prepare_symbol (decl);

  /* The output will always start with the symbol name,
     so always count that in the length-output-so-far.  */

  if (DECL_NAME (decl) != 0)
    current_sym_nchars = 2 + IDENTIFIER_LENGTH (DECL_NAME (decl));

  switch (TREE_CODE (decl))
    {
    case CONST_DECL:
      /* Enum values are defined by defining the enum type.  */
      break;

    case FUNCTION_DECL:
      if (DECL_RTL (decl) == 0)
	/* APPLE LOCAL gdb only used symbols */
	DBXOUT_DECR_NESTING_AND_RETURN (0);
      if (DECL_EXTERNAL (decl))
	break;
      /* Don't mention a nested function under its parent.  */
      context = decl_function_context (decl);
      if (context == current_function_decl)
	break;
      if (GET_CODE (DECL_RTL (decl)) != MEM
	  || GET_CODE (XEXP (DECL_RTL (decl), 0)) != SYMBOL_REF)
	break;
      FORCE_TEXT;

      fprintf (asmfile, "%s\"%s:%c", ASM_STABS_OP,
	       IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl)),
	       TREE_PUBLIC (decl) ? 'F' : 'f');
      result = 1;

      current_sym_code = N_FUN;
      current_sym_addr = XEXP (DECL_RTL (decl), 0);

      if (TREE_TYPE (type))
	dbxout_type (TREE_TYPE (type), 0);
      else
	dbxout_type (void_type_node, 0);

      /* For a nested function, when that function is compiled,
	 mention the containing function name
	 as well as (since dbx wants it) our own assembler-name.  */
      if (context != 0)
	fprintf (asmfile, ",%s,%s",
		 IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl)),
		 IDENTIFIER_POINTER (DECL_NAME (context)));

      dbxout_finish_symbol (decl);
      break;

    case TYPE_DECL:
#if 0
      /* This seems all wrong.  Outputting most kinds of types gives no name
	 at all.  A true definition gives no name; a cross-ref for a
	 structure can give the tag name, but not a type name.
	 It seems that no typedef name is defined by outputting a type.  */

      /* If this typedef name was defined by outputting the type,
	 don't duplicate it.  */
      if (typevec[TYPE_SYMTAB_ADDRESS (type)].status == TYPE_DEFINED
	  && TYPE_NAME (TREE_TYPE (decl)) == decl)
	/* APPLE LOCAL gdb only used symbols */
	DBXOUT_DECR_NESTING_AND_RETURN (0);
#endif
      /* Don't output the same typedef twice.
         And don't output what language-specific stuff doesn't want output.  */
      if (TREE_ASM_WRITTEN (decl) || TYPE_DECL_SUPPRESS_DEBUG (decl))
	/* APPLE LOCAL gdb only used symbols */
	DBXOUT_DECR_NESTING_AND_RETURN (0);

      FORCE_TEXT;
      result = 1;
      {
	int tag_needed = 1;
	int did_output = 0;

	if (DECL_NAME (decl))
	  {
	    /* Nonzero means we must output a tag as well as a typedef.  */
	    tag_needed = 0;

	    /* Handle the case of a C++ structure or union
	       where the TYPE_NAME is a TYPE_DECL
	       which gives both a typedef name and a tag.  */
	    /* dbx requires the tag first and the typedef second.  */
	    if ((TREE_CODE (type) == RECORD_TYPE
		 || TREE_CODE (type) == UNION_TYPE
		 || TREE_CODE (type) == QUAL_UNION_TYPE)
		&& TYPE_NAME (type) == decl
		&& !(use_gnu_debug_info_extensions && have_used_extensions)
		&& !TREE_ASM_WRITTEN (TYPE_NAME (type))
		/* Distinguish the implicit typedefs of C++
		   from explicit ones that might be found in C.  */
                && DECL_ARTIFICIAL (decl)
		/* APPLE LOCAL begin gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
                /* Do not generate a tag for incomplete records.  */
                && COMPLETE_TYPE_P (type)
#endif
		/* APPLE LOCAL end gdb only used symbols */
		/* Do not generate a tag for records of variable size,
		   since this type can not be properly described in the
		   DBX format, and it confuses some tools such as objdump.  */
		&& host_integerp (TYPE_SIZE (type), 1))
	      {
		tree name = TYPE_NAME (type);
		if (TREE_CODE (name) == TYPE_DECL)
		  name = DECL_NAME (name);

		current_sym_code = DBX_TYPE_DECL_STABS_CODE;
		current_sym_value = 0;
		current_sym_addr = 0;
		current_sym_nchars = 2 + IDENTIFIER_LENGTH (name);

		fprintf (asmfile, "%s\"%s:T", ASM_STABS_OP,
			 IDENTIFIER_POINTER (name));
		dbxout_type (type, 1);
		dbxout_finish_symbol (NULL_TREE);
	      }

	    /* Output .stabs (or whatever) and leading double quote.  */
	    fprintf (asmfile, "%s\"", ASM_STABS_OP);

	    if (use_gnu_debug_info_extensions)
	      {
		/* Output leading class/struct qualifiers.  */
		dbxout_class_name_qualifiers (decl);
	      }

	    /* Output typedef name.  */
	    fprintf (asmfile, "%s:", IDENTIFIER_POINTER (DECL_NAME (decl)));

	    /* Short cut way to output a tag also.  */
	    if ((TREE_CODE (type) == RECORD_TYPE
		 || TREE_CODE (type) == UNION_TYPE
		 || TREE_CODE (type) == QUAL_UNION_TYPE)
		&& TYPE_NAME (type) == decl
		/* Distinguish the implicit typedefs of C++
		   from explicit ones that might be found in C.  */
		&& DECL_ARTIFICIAL (decl))
	      {
		if (use_gnu_debug_info_extensions && have_used_extensions)
		  {
		    putc ('T', asmfile);
		    TREE_ASM_WRITTEN (TYPE_NAME (type)) = 1;
		  }
#if 0 /* Now we generate the tag for this case up above.  */
		else
		  tag_needed = 1;
#endif
	      }

	    putc ('t', asmfile);
	    current_sym_code = DBX_TYPE_DECL_STABS_CODE;

	    dbxout_type (type, 1);
	    dbxout_finish_symbol (decl);
	    did_output = 1;
	  }

	/* Don't output a tag if this is an incomplete type.  This prevents
	   the sun4 Sun OS 4.x dbx from crashing.  */

	if (tag_needed && TYPE_NAME (type) != 0
	    && (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE
		|| (DECL_NAME (TYPE_NAME (type)) != 0))
	    && COMPLETE_TYPE_P (type)
	    && !TREE_ASM_WRITTEN (TYPE_NAME (type)))
	  {
	    /* For a TYPE_DECL with no name, but the type has a name,
	       output a tag.
	       This is what represents `struct foo' with no typedef.  */
	    /* In C++, the name of a type is the corresponding typedef.
	       In C, it is an IDENTIFIER_NODE.  */
	    tree name = TYPE_NAME (type);
	    if (TREE_CODE (name) == TYPE_DECL)
	      name = DECL_NAME (name);

	    current_sym_code = DBX_TYPE_DECL_STABS_CODE;
	    current_sym_value = 0;
	    current_sym_addr = 0;
	    current_sym_nchars = 2 + IDENTIFIER_LENGTH (name);

	    fprintf (asmfile, "%s\"%s:T", ASM_STABS_OP,
		     IDENTIFIER_POINTER (name));
	    dbxout_type (type, 1);
	    dbxout_finish_symbol (NULL_TREE);
	    did_output = 1;
	  }

	/* If an enum type has no name, it cannot be referred to,
	   but we must output it anyway, since the enumeration constants
	   can be referred to.  */
	if (!did_output && TREE_CODE (type) == ENUMERAL_TYPE)
	  {
	    current_sym_code = DBX_TYPE_DECL_STABS_CODE;
	    current_sym_value = 0;
	    current_sym_addr = 0;
	    current_sym_nchars = 2;

	    /* Some debuggers fail when given NULL names, so give this a
	       harmless name of ` '.  */
	    fprintf (asmfile, "%s\" :T", ASM_STABS_OP);
	    dbxout_type (type, 1);
	    dbxout_finish_symbol (NULL_TREE);
	  }

	/* Prevent duplicate output of a typedef.  */
	TREE_ASM_WRITTEN (decl) = 1;
	break;
      }

    case PARM_DECL:
      /* Parm decls go in their own separate chains
	 and are output by dbxout_reg_parms and dbxout_parms.  */
      abort ();

    case RESULT_DECL:
      /* Named return value, treat like a VAR_DECL.  */
    case VAR_DECL:
      if (! DECL_RTL_SET_P (decl))
	/* APPLE LOCAL gdb only used symbols */
	DBXOUT_DECR_NESTING_AND_RETURN (0);
      /* Don't mention a variable that is external.
	 Let the file that defines it describe it.  */
      if (DECL_EXTERNAL (decl))
	break;

      /* If the variable is really a constant
	 and not written in memory, inform the debugger.  */
      if (TREE_STATIC (decl) && TREE_READONLY (decl)
	  && DECL_INITIAL (decl) != 0
	  && host_integerp (DECL_INITIAL (decl), 0)
	  && ! TREE_ASM_WRITTEN (decl)
	  && (DECL_CONTEXT (decl) == NULL_TREE
	      || TREE_CODE (DECL_CONTEXT (decl)) == BLOCK))
	{
	  if (TREE_PUBLIC (decl) == 0)
	    {
	      /* The sun4 assembler does not grok this.  */
	      const char *name = IDENTIFIER_POINTER (DECL_NAME (decl));

	      if (TREE_CODE (TREE_TYPE (decl)) == INTEGER_TYPE
		  || TREE_CODE (TREE_TYPE (decl)) == ENUMERAL_TYPE)
		{
		  HOST_WIDE_INT ival = tree_low_cst (DECL_INITIAL (decl), 0);
#ifdef DBX_OUTPUT_CONSTANT_SYMBOL
		  DBX_OUTPUT_CONSTANT_SYMBOL (asmfile, name, ival);
#else
		  fprintf (asmfile, "%s\"%s:c=i", ASM_STABS_OP, name);

		  fprintf (asmfile, HOST_WIDE_INT_PRINT_DEC, ival);
		  fprintf (asmfile, "\",0x%x,0,0,0\n", N_LSYM);
#endif
		  /* APPLE LOCAL gdb only used symbols */
		  DBXOUT_DECR_NESTING;
		  return 1;
		}
	      else if (TREE_CODE (TREE_TYPE (decl)) == REAL_TYPE)
		{
		  /* don't know how to do this yet.  */
		}
	      break;
	    }
	  /* else it is something we handle like a normal variable.  */
	}

      SET_DECL_RTL (decl, eliminate_regs (DECL_RTL (decl), 0, NULL_RTX));
#ifdef LEAF_REG_REMAP
      if (current_function_uses_only_leaf_regs)
	leaf_renumber_regs_insn (DECL_RTL (decl));
#endif

      result = dbxout_symbol_location (decl, type, 0, DECL_RTL (decl));
      break;

    default:
      break;
    }

  /* APPLE LOCAL gdb only used symbols */
  DBXOUT_DECR_NESTING;
  return result;
}

/* Output the stab for DECL, a VAR_DECL, RESULT_DECL or PARM_DECL.
   Add SUFFIX to its name, if SUFFIX is not 0.
   Describe the variable as residing in HOME
   (usually HOME is DECL_RTL (DECL), but not always).
   Returns 1 if the stab was really emitted.  */

static int
dbxout_symbol_location (decl, type, suffix, home)
     tree decl, type;
     const char *suffix;
     rtx home;
{
  int letter = 0;
  int regno = -1;

  /* APPLE LOCAL empty BINCL/EINCL optimization */
  emit_pending_bincls_if_required ();

  /* Don't mention a variable at all
     if it was completely optimized into nothingness.

     If the decl was from an inline function, then its rtl
     is not identically the rtl that was used in this
     particular compilation.  */
  if (GET_CODE (home) == SUBREG)
    {
      rtx value = home;

      while (GET_CODE (value) == SUBREG)
	value = SUBREG_REG (value);
      if (GET_CODE (value) == REG)
	{
	  if (REGNO (value) >= FIRST_PSEUDO_REGISTER)
	    return 0;
	}
      home = alter_subreg (&home);
    }
  if (GET_CODE (home) == REG)
    {
      regno = REGNO (home);
      if (regno >= FIRST_PSEUDO_REGISTER)
	return 0;
    }

  /* The kind-of-variable letter depends on where
     the variable is and on the scope of its name:
     G and N_GSYM for static storage and global scope,
     S for static storage and file scope,
     V for static storage and local scope,
     for those two, use N_LCSYM if data is in bss segment,
     N_STSYM if in data segment, N_FUN otherwise.
     (We used N_FUN originally, then changed to N_STSYM
     to please GDB.  However, it seems that confused ld.
     Now GDB has been fixed to like N_FUN, says Kingdon.)
     no letter at all, and N_LSYM, for auto variable,
     r and N_RSYM for register variable.  */

  if (GET_CODE (home) == MEM
      && GET_CODE (XEXP (home, 0)) == SYMBOL_REF)
    {
      if (TREE_PUBLIC (decl))
	{
	  letter = 'G';
	  current_sym_code = N_GSYM;
	}
      else
	{
	  current_sym_addr = XEXP (home, 0);

	  letter = decl_function_context (decl) ? 'V' : 'S';

	  /* This should be the same condition as in assemble_variable, but
	     we don't have access to dont_output_data here.  So, instead,
	     we rely on the fact that error_mark_node initializers always
	     end up in bss for C++ and never end up in bss for C.  */
	  if (DECL_INITIAL (decl) == 0
	      || (!strcmp (lang_hooks.name, "GNU C++")
		  && DECL_INITIAL (decl) == error_mark_node))
	    current_sym_code = N_LCSYM;
	  else if (DECL_IN_TEXT_SECTION (decl))
	    /* This is not quite right, but it's the closest
	       of all the codes that Unix defines.  */
	    current_sym_code = DBX_STATIC_CONST_VAR_CODE;
	  else
	    {
	      /* Some ports can transform a symbol ref into a label ref,
		 because the symbol ref is too far away and has to be
		 dumped into a constant pool.  Alternatively, the symbol
		 in the constant pool might be referenced by a different
		 symbol.  */
	      if (GET_CODE (current_sym_addr) == SYMBOL_REF
		  && CONSTANT_POOL_ADDRESS_P (current_sym_addr))
		{
		  rtx tmp = get_pool_constant (current_sym_addr);

		  if (GET_CODE (tmp) == SYMBOL_REF
		      || GET_CODE (tmp) == LABEL_REF)
		    current_sym_addr = tmp;
		}

	      /* Ultrix `as' seems to need this.  */
#ifdef DBX_STATIC_STAB_DATA_SECTION
	      data_section ();
#endif
	      current_sym_code = N_STSYM;
	    }
	}
    }
  else if (regno >= 0)
    {
      letter = 'r';
      current_sym_code = N_RSYM;
      current_sym_value = DBX_REGISTER_NUMBER (regno);
    }
  else if (GET_CODE (home) == MEM
	   && (GET_CODE (XEXP (home, 0)) == MEM
	       || (GET_CODE (XEXP (home, 0)) == REG
		   && REGNO (XEXP (home, 0)) != HARD_FRAME_POINTER_REGNUM
		   && REGNO (XEXP (home, 0)) != STACK_POINTER_REGNUM
#if ARG_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
		   && REGNO (XEXP (home, 0)) != ARG_POINTER_REGNUM
#endif
		   )))
    /* If the value is indirect by memory or by a register
       that isn't the frame pointer
       then it means the object is variable-sized and address through
       that register or stack slot.  DBX has no way to represent this
       so all we can do is output the variable as a pointer.
       If it's not a parameter, ignore it.
       (VAR_DECLs like this can be made by integrate.c.)  */
    {
      if (GET_CODE (XEXP (home, 0)) == REG)
	{
	  letter = 'r';
	  current_sym_code = N_RSYM;
	  if (REGNO (XEXP (home, 0)) >= FIRST_PSEUDO_REGISTER)
	    return 0;
	  current_sym_value = DBX_REGISTER_NUMBER (REGNO (XEXP (home, 0)));
	}
      else
	{
	  current_sym_code = N_LSYM;
	  /* RTL looks like (MEM (MEM (PLUS (REG...) (CONST_INT...)))).
	     We want the value of that CONST_INT.  */
	  current_sym_value
	    = DEBUGGER_AUTO_OFFSET (XEXP (XEXP (home, 0), 0));
	}

      /* Effectively do build_pointer_type, but don't cache this type,
	 since it might be temporary whereas the type it points to
	 might have been saved for inlining.  */
      /* Don't use REFERENCE_TYPE because dbx can't handle that.  */
      type = make_node (POINTER_TYPE);
      TREE_TYPE (type) = TREE_TYPE (decl);
    }
  else if (GET_CODE (home) == MEM
	   && GET_CODE (XEXP (home, 0)) == REG)
    {
      current_sym_code = N_LSYM;
      current_sym_value = DEBUGGER_AUTO_OFFSET (XEXP (home, 0));
    }
  else if (GET_CODE (home) == MEM
	   && GET_CODE (XEXP (home, 0)) == PLUS
	   && GET_CODE (XEXP (XEXP (home, 0), 1)) == CONST_INT)
    {
      current_sym_code = N_LSYM;
      /* RTL looks like (MEM (PLUS (REG...) (CONST_INT...)))
	 We want the value of that CONST_INT.  */
      current_sym_value = DEBUGGER_AUTO_OFFSET (XEXP (home, 0));
    }
  else if (GET_CODE (home) == MEM
	   && GET_CODE (XEXP (home, 0)) == CONST)
    {
      /* Handle an obscure case which can arise when optimizing and
	 when there are few available registers.  (This is *always*
	 the case for i386/i486 targets).  The RTL looks like
	 (MEM (CONST ...)) even though this variable is a local `auto'
	 or a local `register' variable.  In effect, what has happened
	 is that the reload pass has seen that all assignments and
	 references for one such a local variable can be replaced by
	 equivalent assignments and references to some static storage
	 variable, thereby avoiding the need for a register.  In such
	 cases we're forced to lie to debuggers and tell them that
	 this variable was itself `static'.  */
      current_sym_code = N_LCSYM;
      letter = 'V';
      current_sym_addr = XEXP (XEXP (home, 0), 0);
    }
  else if (GET_CODE (home) == CONCAT)
    {
      tree subtype;

      /* If TYPE is not a COMPLEX_TYPE (it might be a RECORD_TYPE,
	 for example), then there is no easy way to figure out
	 what SUBTYPE should be.  So, we give up.  */
      if (TREE_CODE (type) != COMPLEX_TYPE)
	return 0;

      subtype = TREE_TYPE (type);

      /* If the variable's storage is in two parts,
	 output each as a separate stab with a modified name.  */
      if (WORDS_BIG_ENDIAN)
	dbxout_symbol_location (decl, subtype, "$imag", XEXP (home, 0));
      else
	dbxout_symbol_location (decl, subtype, "$real", XEXP (home, 0));

      /* Cast avoids warning in old compilers.  */
      current_sym_code = (STAB_CODE_TYPE) 0;
      current_sym_value = 0;
      current_sym_addr = 0;
      dbxout_prepare_symbol (decl);

      if (WORDS_BIG_ENDIAN)
	dbxout_symbol_location (decl, subtype, "$real", XEXP (home, 1));
      else
	dbxout_symbol_location (decl, subtype, "$imag", XEXP (home, 1));
      return 1;
    }
  else
    /* Address might be a MEM, when DECL is a variable-sized object.
       Or it might be const0_rtx, meaning previous passes
       want us to ignore this variable.  */
    return 0;

  /* Ok, start a symtab entry and output the variable name.  */
  FORCE_TEXT;

#ifdef DBX_STATIC_BLOCK_START
  DBX_STATIC_BLOCK_START (asmfile, current_sym_code);
#endif

  dbxout_symbol_name (decl, suffix, letter);
  dbxout_type (type, 0);
  dbxout_finish_symbol (decl);

#ifdef DBX_STATIC_BLOCK_END
  DBX_STATIC_BLOCK_END (asmfile, current_sym_code);
#endif
  return 1;
}

/* Output the symbol name of DECL for a stabs, with suffix SUFFIX.
   Then output LETTER to indicate the kind of location the symbol has.  */

static void
dbxout_symbol_name (decl, suffix, letter)
     tree decl;
     const char *suffix;
     int letter;
{
  const char *name;

  if (DECL_CONTEXT (decl) && TYPE_P (DECL_CONTEXT (decl)))
    /* One slight hitch: if this is a VAR_DECL which is a static
       class member, we must put out the mangled name instead of the
       DECL_NAME.  Note also that static member (variable) names DO NOT begin
       with underscores in .stabs directives.  */
    name = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));
  else
    /* ...but if we're function-local, we don't want to include the junk
       added by ASM_FORMAT_PRIVATE_NAME.  */
    name = IDENTIFIER_POINTER (DECL_NAME (decl));

  if (name == 0)
    name = "(anon)";
  fprintf (asmfile, "%s\"%s%s:", ASM_STABS_OP, name,
	   (suffix ? suffix : ""));

  if (letter)
    putc (letter, asmfile);
}

static void
dbxout_prepare_symbol (decl)
     tree decl ATTRIBUTE_UNUSED;
{
#ifdef WINNING_GDB
  const char *filename = DECL_SOURCE_FILE (decl);

  dbxout_source_file (asmfile, filename);
#endif
}

static void
dbxout_finish_symbol (sym)
     tree sym;
{
#ifdef DBX_FINISH_SYMBOL
  DBX_FINISH_SYMBOL (sym);
#else
  int line = 0;
  if (use_gnu_debug_info_extensions && sym != 0)
    line = DECL_SOURCE_LINE (sym);

  fprintf (asmfile, "\",%d,0,%d,", current_sym_code, line);
  if (current_sym_addr)
    output_addr_const (asmfile, current_sym_addr);
  else
    fprintf (asmfile, "%d", current_sym_value);
  putc ('\n', asmfile);
#endif
}

/* Output definitions of all the decls in a chain. Return nonzero if
   anything was output */

int
dbxout_syms (syms)
     tree syms;
{
  int result = 0;
  while (syms)
    {
      result += dbxout_symbol (syms, 1);
      syms = TREE_CHAIN (syms);
    }
  return result;
}

/* The following two functions output definitions of function parameters.
   Each parameter gets a definition locating it in the parameter list.
   Each parameter that is a register variable gets a second definition
   locating it in the register.

   Printing or argument lists in gdb uses the definitions that
   locate in the parameter list.  But reference to the variable in
   expressions uses preferentially the definition as a register.  */

/* Output definitions, referring to storage in the parmlist,
   of all the parms in PARMS, which is a chain of PARM_DECL nodes.  */

void
dbxout_parms (parms)
     tree parms;
{
 
/* APPLE LOCAL gdb only used symbols */
#ifdef DBXOUT_TRACK_NESTING
  ++dbxout_nesting;
#endif

  /* APPLE LOCAL empty BINCL/EINCL optimization */
  emit_pending_bincls_if_required ();

  for (; parms; parms = TREE_CHAIN (parms))
    if (DECL_NAME (parms) && TREE_TYPE (parms) != error_mark_node)
      {
	dbxout_prepare_symbol (parms);

	/* Perform any necessary register eliminations on the parameter's rtl,
	   so that the debugging output will be accurate.  */
	DECL_INCOMING_RTL (parms)
	  = eliminate_regs (DECL_INCOMING_RTL (parms), 0, NULL_RTX);
	SET_DECL_RTL (parms, eliminate_regs (DECL_RTL (parms), 0, NULL_RTX));
#ifdef LEAF_REG_REMAP
	if (current_function_uses_only_leaf_regs)
	  {
	    leaf_renumber_regs_insn (DECL_INCOMING_RTL (parms));
	    leaf_renumber_regs_insn (DECL_RTL (parms));
	  }
#endif

	if (PARM_PASSED_IN_MEMORY (parms))
	  {
	    rtx addr = XEXP (DECL_INCOMING_RTL (parms), 0);

	    /* ??? Here we assume that the parm address is indexed
	       off the frame pointer or arg pointer.
	       If that is not true, we produce meaningless results,
	       but do not crash.  */
	    if (GET_CODE (addr) == PLUS
		&& GET_CODE (XEXP (addr, 1)) == CONST_INT)
	      current_sym_value = INTVAL (XEXP (addr, 1));
	    else
	      current_sym_value = 0;

	    current_sym_code = N_PSYM;
	    current_sym_addr = 0;

	    FORCE_TEXT;
	    if (DECL_NAME (parms))
	      {
		current_sym_nchars = 2 + IDENTIFIER_LENGTH (DECL_NAME (parms));

		fprintf (asmfile, "%s\"%s:%c", ASM_STABS_OP,
			 IDENTIFIER_POINTER (DECL_NAME (parms)),
			 DBX_MEMPARM_STABS_LETTER);
	      }
	    else
	      {
		current_sym_nchars = 8;
		fprintf (asmfile, "%s\"(anon):%c", ASM_STABS_OP,
			 DBX_MEMPARM_STABS_LETTER);
	      }

	    /* It is quite tempting to use:

	           dbxout_type (TREE_TYPE (parms), 0);

	       as the next statement, rather than using DECL_ARG_TYPE(), so
	       that gcc reports the actual type of the parameter, rather
	       than the promoted type.  This certainly makes GDB's life
	       easier, at least for some ports.  The change is a bad idea
	       however, since GDB expects to be able access the type without
	       performing any conversions.  So for example, if we were
	       passing a float to an unprototyped function, gcc will store a
	       double on the stack, but if we emit a stab saying the type is a
	       float, then gdb will only read in a single value, and this will
	       produce an erroneous value.  */
	    dbxout_type (DECL_ARG_TYPE (parms), 0);
	    current_sym_value = DEBUGGER_ARG_OFFSET (current_sym_value, addr);
	    dbxout_finish_symbol (parms);
	  }
	else if (GET_CODE (DECL_RTL (parms)) == REG)
	  {
	    rtx best_rtl;
	    char regparm_letter;
	    tree parm_type;
	    /* Parm passed in registers and lives in registers or nowhere.  */

	    current_sym_code = DBX_REGPARM_STABS_CODE;
	    regparm_letter = DBX_REGPARM_STABS_LETTER;
	    current_sym_addr = 0;

	    /* If parm lives in a register, use that register;
	       pretend the parm was passed there.  It would be more consistent
	       to describe the register where the parm was passed,
	       but in practice that register usually holds something else.

	       If we use DECL_RTL, then we must use the declared type of
	       the variable, not the type that it arrived in.  */
	    if (REGNO (DECL_RTL (parms)) < FIRST_PSEUDO_REGISTER)
	      {
		best_rtl = DECL_RTL (parms);
		parm_type = TREE_TYPE (parms);
	      }
	    /* If the parm lives nowhere, use the register where it was
	       passed.  It is also better to use the declared type here.  */
	    else
	      {
		best_rtl = DECL_INCOMING_RTL (parms);
		parm_type = TREE_TYPE (parms);
	      }
	    current_sym_value = DBX_REGISTER_NUMBER (REGNO (best_rtl));

	    FORCE_TEXT;
	    if (DECL_NAME (parms))
	      {
		current_sym_nchars = 2 + IDENTIFIER_LENGTH (DECL_NAME (parms));
		fprintf (asmfile, "%s\"%s:%c", ASM_STABS_OP,
			 IDENTIFIER_POINTER (DECL_NAME (parms)),
			 regparm_letter);
	      }
	    else
	      {
		current_sym_nchars = 8;
		fprintf (asmfile, "%s\"(anon):%c", ASM_STABS_OP,
			 regparm_letter);
	      }

	    dbxout_type (parm_type, 0);
	    dbxout_finish_symbol (parms);
	  }
	else if (GET_CODE (DECL_RTL (parms)) == MEM
		 && GET_CODE (XEXP (DECL_RTL (parms), 0)) == REG
		 && REGNO (XEXP (DECL_RTL (parms), 0)) != HARD_FRAME_POINTER_REGNUM
		 && REGNO (XEXP (DECL_RTL (parms), 0)) != STACK_POINTER_REGNUM
#if ARG_POINTER_REGNUM != HARD_FRAME_POINTER_REGNUM
		 && REGNO (XEXP (DECL_RTL (parms), 0)) != ARG_POINTER_REGNUM
#endif
		 )
	  {
	    /* Parm was passed via invisible reference.
	       That is, its address was passed in a register.
	       Output it as if it lived in that register.
	       The debugger will know from the type
	       that it was actually passed by invisible reference.  */

	    char regparm_letter;
	    /* Parm passed in registers and lives in registers or nowhere.  */

	    current_sym_code = DBX_REGPARM_STABS_CODE;
	    if (use_gnu_debug_info_extensions)
	      regparm_letter = GDB_INV_REF_REGPARM_STABS_LETTER;
	    else
	      regparm_letter = DBX_REGPARM_STABS_LETTER;

	    /* DECL_RTL looks like (MEM (REG...).  Get the register number.
	       If it is an unallocated pseudo-reg, then use the register where
	       it was passed instead.  */
	    if (REGNO (XEXP (DECL_RTL (parms), 0)) < FIRST_PSEUDO_REGISTER)
	      current_sym_value = REGNO (XEXP (DECL_RTL (parms), 0));
	    else
	      current_sym_value = REGNO (DECL_INCOMING_RTL (parms));

	    current_sym_addr = 0;

	    FORCE_TEXT;
	    if (DECL_NAME (parms))
	      {
		current_sym_nchars
		  = 2 + strlen (IDENTIFIER_POINTER (DECL_NAME (parms)));

		fprintf (asmfile, "%s\"%s:%c", ASM_STABS_OP,
			 IDENTIFIER_POINTER (DECL_NAME (parms)),
			 regparm_letter);
	      }
	    else
	      {
		current_sym_nchars = 8;
		fprintf (asmfile, "%s\"(anon):%c", ASM_STABS_OP,
			 regparm_letter);
	      }

	    dbxout_type (TREE_TYPE (parms), 0);
	    dbxout_finish_symbol (parms);
	  }
	else if (GET_CODE (DECL_RTL (parms)) == MEM
		 && GET_CODE (XEXP (DECL_RTL (parms), 0)) == MEM)
	  {
	    /* Parm was passed via invisible reference, with the reference
	       living on the stack.  DECL_RTL looks like
	       (MEM (MEM (PLUS (REG ...) (CONST_INT ...)))) or it
	       could look like (MEM (MEM (REG))).  */
	    const char *const decl_name = (DECL_NAME (parms)
				     ? IDENTIFIER_POINTER (DECL_NAME (parms))
				     : "(anon)");
	    if (GET_CODE (XEXP (XEXP (DECL_RTL (parms), 0), 0)) == REG)
	      current_sym_value = 0;
	    else
	      current_sym_value
	        = INTVAL (XEXP (XEXP (XEXP (DECL_RTL (parms), 0), 0), 1));
	    current_sym_addr = 0;
	    current_sym_code = N_PSYM;

	    FORCE_TEXT;
	    fprintf (asmfile, "%s\"%s:v", ASM_STABS_OP, decl_name);

	    current_sym_value
	      = DEBUGGER_ARG_OFFSET (current_sym_value,
				     XEXP (XEXP (DECL_RTL (parms), 0), 0));
	    dbxout_type (TREE_TYPE (parms), 0);
	    dbxout_finish_symbol (parms);
	  }
	else if (GET_CODE (DECL_RTL (parms)) == MEM
		 && XEXP (DECL_RTL (parms), 0) != const0_rtx
		 /* ??? A constant address for a parm can happen
		    when the reg it lives in is equiv to a constant in memory.
		    Should make this not happen, after 2.4.  */
		 && ! CONSTANT_P (XEXP (DECL_RTL (parms), 0)))
	  {
	    /* Parm was passed in registers but lives on the stack.  */

	    current_sym_code = N_PSYM;
	    /* DECL_RTL looks like (MEM (PLUS (REG...) (CONST_INT...))),
	       in which case we want the value of that CONST_INT,
	       or (MEM (REG ...)),
	       in which case we use a value of zero.  */
	    if (GET_CODE (XEXP (DECL_RTL (parms), 0)) == REG)
	      current_sym_value = 0;
	    else
		current_sym_value
		  = INTVAL (XEXP (XEXP (DECL_RTL (parms), 0), 1));

	    current_sym_addr = 0;

	    /* Make a big endian correction if the mode of the type of the
	       parameter is not the same as the mode of the rtl.  */
	    if (BYTES_BIG_ENDIAN
		&& TYPE_MODE (TREE_TYPE (parms)) != GET_MODE (DECL_RTL (parms))
		&& GET_MODE_SIZE (TYPE_MODE (TREE_TYPE (parms))) < UNITS_PER_WORD)
	      {
		current_sym_value +=
		    GET_MODE_SIZE (GET_MODE (DECL_RTL (parms)))
		    - GET_MODE_SIZE (TYPE_MODE (TREE_TYPE (parms)));
	      }

	    FORCE_TEXT;
	    if (DECL_NAME (parms))
	      {
		current_sym_nchars
		  = 2 + strlen (IDENTIFIER_POINTER (DECL_NAME (parms)));

		fprintf (asmfile, "%s\"%s:%c", ASM_STABS_OP,
			 IDENTIFIER_POINTER (DECL_NAME (parms)),
			 DBX_MEMPARM_STABS_LETTER);
	      }
	    else
	      {
		current_sym_nchars = 8;
		fprintf (asmfile, "%s\"(anon):%c", ASM_STABS_OP,
		DBX_MEMPARM_STABS_LETTER);
	      }

	    current_sym_value
	      = DEBUGGER_ARG_OFFSET (current_sym_value,
				     XEXP (DECL_RTL (parms), 0));
	    dbxout_type (TREE_TYPE (parms), 0);
	    dbxout_finish_symbol (parms);
	  }
      }
      
  /* APPLE LOCAL gdb only used symbols */
  DBXOUT_DECR_NESTING;
}

/* Output definitions for the places where parms live during the function,
   when different from where they were passed, when the parms were passed
   in memory.

   It is not useful to do this for parms passed in registers
   that live during the function in different registers, because it is
   impossible to look in the passed register for the passed value,
   so we use the within-the-function register to begin with.

   PARMS is a chain of PARM_DECL nodes.  */

void
dbxout_reg_parms (parms)
     tree parms;
{
/* APPLE LOCAL gdb only used symbols */
#ifdef DBXOUT_TRACK_NESTING
  ++dbxout_nesting;
#endif

  for (; parms; parms = TREE_CHAIN (parms))
    if (DECL_NAME (parms) && PARM_PASSED_IN_MEMORY (parms))
      {
	dbxout_prepare_symbol (parms);

	/* Report parms that live in registers during the function
	   but were passed in memory.  */
	if (GET_CODE (DECL_RTL (parms)) == REG
	    && REGNO (DECL_RTL (parms)) < FIRST_PSEUDO_REGISTER)
	  dbxout_symbol_location (parms, TREE_TYPE (parms),
				  0, DECL_RTL (parms));
	else if (GET_CODE (DECL_RTL (parms)) == CONCAT)
	  dbxout_symbol_location (parms, TREE_TYPE (parms),
				  0, DECL_RTL (parms));
	/* Report parms that live in memory but not where they were passed.  */
	else if (GET_CODE (DECL_RTL (parms)) == MEM
		 && ! rtx_equal_p (DECL_RTL (parms), DECL_INCOMING_RTL (parms)))
	  dbxout_symbol_location (parms, TREE_TYPE (parms),
				  0, DECL_RTL (parms));
      }

  /* APPLE LOCAL gdb only used symbols */
  DBXOUT_DECR_NESTING;
}

/* Given a chain of ..._TYPE nodes (as come in a parameter list),
   output definitions of those names, in raw form */

static void
dbxout_args (args)
     tree args;
{
  while (args)
    {
      putc (',', asmfile);
      dbxout_type (TREE_VALUE (args), 0);
      CHARS (1);
      args = TREE_CHAIN (args);
    }
}

/* Output everything about a symbol block (a BLOCK node
   that represents a scope level),
   including recursive output of contained blocks.

   BLOCK is the BLOCK node.
   DEPTH is its depth within containing symbol blocks.
   ARGS is usually zero; but for the outermost block of the
   body of a function, it is a chain of PARM_DECLs for the function parameters.
   We output definitions of all the register parms
   as if they were local variables of that block.

   If -g1 was used, we count blocks just the same, but output nothing
   except for the outermost block.

   Actually, BLOCK may be several blocks chained together.
   We handle them all in sequence.  */

static void
dbxout_block (block, depth, args)
     tree block;
     int depth;
     tree args;
{
  int blocknum = -1;

#if DBX_BLOCKS_FUNCTION_RELATIVE
  const char *begin_label;
  if (current_function_func_begin_label != NULL_TREE)
    begin_label = IDENTIFIER_POINTER (current_function_func_begin_label);
  else
    begin_label = XSTR (XEXP (DECL_RTL (current_function_decl), 0), 0);
#endif

  while (block)
    {
      /* Ignore blocks never expanded or otherwise marked as real.  */
      if (TREE_USED (block) && TREE_ASM_WRITTEN (block))
	{
	  int did_output;

#ifdef DBX_LBRAC_FIRST
	  did_output = 1;
#else
	  /* In dbx format, the syms of a block come before the N_LBRAC.
	     If nothing is output, we don't need the N_LBRAC, either.  */
	  did_output = 0;
	  if (debug_info_level != DINFO_LEVEL_TERSE || depth == 0)
	    did_output = dbxout_syms (BLOCK_VARS (block));
	  if (args)
	    dbxout_reg_parms (args);
#endif

	  /* Now output an N_LBRAC symbol to represent the beginning of
	     the block.  Use the block's tree-walk order to generate
	     the assembler symbols LBBn and LBEn
	     that final will define around the code in this block.  */
	  if (depth > 0 && did_output)
	    {
	      char buf[20];
	      blocknum = BLOCK_NUMBER (block);
	      ASM_GENERATE_INTERNAL_LABEL (buf, "LBB", blocknum);

	      if (BLOCK_HANDLER_BLOCK (block))
		{
		  /* A catch block.  Must precede N_LBRAC.  */
		  tree decl = BLOCK_VARS (block);
		  while (decl)
		    {
#ifdef DBX_OUTPUT_CATCH
		      DBX_OUTPUT_CATCH (asmfile, decl, buf);
#else
		      fprintf (asmfile, "%s\"%s:C1\",%d,0,0,", ASM_STABS_OP,
			       IDENTIFIER_POINTER (DECL_NAME (decl)), N_CATCH);
		      assemble_name (asmfile, buf);
		      fprintf (asmfile, "\n");
#endif
		      decl = TREE_CHAIN (decl);
		    }
		}

#ifdef DBX_OUTPUT_LBRAC
	      DBX_OUTPUT_LBRAC (asmfile, buf);
#else
	      fprintf (asmfile, "%s%d,0,0,", ASM_STABN_OP, N_LBRAC);
	      assemble_name (asmfile, buf);
#if DBX_BLOCKS_FUNCTION_RELATIVE
	      putc ('-', asmfile);
	      assemble_name (asmfile, begin_label);
#endif
	      fprintf (asmfile, "\n");
#endif
	    }

#ifdef DBX_LBRAC_FIRST
	  /* On some weird machines, the syms of a block
	     come after the N_LBRAC.  */
	  if (debug_info_level != DINFO_LEVEL_TERSE || depth == 0)
	    dbxout_syms (BLOCK_VARS (block));
	  if (args)
	    dbxout_reg_parms (args);
#endif

	  /* Output the subblocks.  */
	  dbxout_block (BLOCK_SUBBLOCKS (block), depth + 1, NULL_TREE);

	  /* Refer to the marker for the end of the block.  */
	  if (depth > 0 && did_output)
	    {
	      char buf[20];
	      ASM_GENERATE_INTERNAL_LABEL (buf, "LBE", blocknum);
#ifdef DBX_OUTPUT_RBRAC
	      DBX_OUTPUT_RBRAC (asmfile, buf);
#else
	      fprintf (asmfile, "%s%d,0,0,", ASM_STABN_OP, N_RBRAC);
	      assemble_name (asmfile, buf);
#if DBX_BLOCKS_FUNCTION_RELATIVE
	      putc ('-', asmfile);
	      assemble_name (asmfile, begin_label);
#endif
	      fprintf (asmfile, "\n");
#endif
	    }
	}
      block = BLOCK_CHAIN (block);
    }
}

/* Output the information about a function and its arguments and result.
   Usually this follows the function's code,
   but on some systems, it comes before.  */

#if defined (DBX_DEBUGGING_INFO)
static void
dbxout_begin_function (decl)
     tree decl;
{
/* APPLE LOCAL gdb only used symbols */
#ifdef DBX_ONLY_USED_SYMBOLS
  int saved_tree_used1 = TREE_USED (decl);
  TREE_USED (decl) = 1;
  if (DECL_NAME (DECL_RESULT (decl)) != 0)
    {
      int saved_tree_used2 = TREE_USED (DECL_RESULT (decl));
      TREE_USED (DECL_RESULT (decl)) = 1;
      dbxout_symbol (decl, 0);
      TREE_USED (DECL_RESULT (decl)) = saved_tree_used2;
    }
  else
      dbxout_symbol (decl, 0);
  TREE_USED (decl) = saved_tree_used1;
#else
  dbxout_symbol (decl, 0);
#endif
  dbxout_parms (DECL_ARGUMENTS (decl));
  if (DECL_NAME (DECL_RESULT (decl)) != 0)
    /* APPLE LOCAL begin Constructors return THIS  turly 20020315  */
#ifdef POSSIBLY_COMPILING_APPLE_KEXT_P
    /* We cheat with kext constructors: DECL_RESULT is "this", but "this"
       is actually the first parameter, so don't confuse matters by
       outputting the same parameter twice.  */
    if (!(POSSIBLY_COMPILING_APPLE_KEXT_P ()
	  && DECL_RESULT (decl) == DECL_ARGUMENTS (decl)))
#endif
    /* APPLE LOCAL end Constructors return THIS  turly 20020315  */
    dbxout_symbol (DECL_RESULT (decl), 1);
}
#endif /* DBX_DEBUGGING_INFO */

#endif /* DBX_DEBUGGING_INFO || XCOFF_DEBUGGING_INFO */

#include "gt-dbxout.h"
