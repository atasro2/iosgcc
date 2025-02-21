(* ALQAAHIRA LOCAL file v7 support. Merge from Codesourcery *)
(* Auto-generate ARM Neon intrinsics tests.
   Copyright (C) 2006 Free Software Foundation, Inc.
   Contributed by CodeSourcery.

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
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   This is an O'Caml program.  The O'Caml compiler is available from:

     http://caml.inria.fr/

   Or from your favourite OS's friendly packaging system. Tested with version
   3.09.2, though other versions will probably work too.
  
   Compile with:
     ocamlc -c neon.ml
     ocamlc -o neon-testgen neon.cmo neon-testgen.ml
*)

open Neon

type c_type_flags = Pointer | Const

(* Open a test source file.  *)
let open_test_file dir name =
  try
    open_out (dir ^ "/" ^ name ^ ".c")
  with Sys_error str ->
    failwith ("Could not create test source file " ^ name ^ ": " ^ str)

(* Emit prologue code to a test source file.  *)
let emit_prologue chan test_name =
  Printf.fprintf chan "/* Test the `%s' ARM Neon intrinsic.  */\n" test_name;
  Printf.fprintf chan "/* This file was autogenerated by neon-testgen.  */\n\n";
  Printf.fprintf chan "/* { dg-do assemble } */\n";
  Printf.fprintf chan "/* { dg-require-effective-target arm_neon_ok } */\n";
  Printf.fprintf chan
                 "/* { dg-options \"-save-temps -O0 -mfpu=neon -mfloat-abi=softfp\" } */\n";
  Printf.fprintf chan "\n#include \"arm_neon.h\"\n\n";
  Printf.fprintf chan "void test_%s (void)\n{\n" test_name

(* Emit declarations of local variables that are going to be passed
   to an intrinsic, together with one to take a returned value if needed.  *)
let emit_automatics chan c_types =
  let emit () =
    ignore (
      List.fold_left (fun arg_number -> fun (flags, ty) ->
                        let pointer_bit =
                          if List.mem Pointer flags then "*" else ""
                        in
                          (* Const arguments to builtins are directly
                             written in as constants.  *)
                          if not (List.mem Const flags) then
                            Printf.fprintf chan "  %s %sarg%d_%s;\n"
                                           ty pointer_bit arg_number ty;
                        arg_number + 1)
                     0 (List.tl c_types))
  in
    match c_types with
      (_, return_ty) :: tys ->
        if return_ty <> "void" then
          (* The intrinsic returns a value.  *)
          (Printf.fprintf chan "  %s out_%s;\n" return_ty return_ty;
           emit ())
        else
          (* The intrinsic does not return a value.  *)
          emit ()
    | _ -> assert false

(* Emit code to call an intrinsic.  *)
let emit_call chan const_valuator c_types name elt_ty =
  (if snd (List.hd c_types) <> "void" then
     Printf.fprintf chan "  out_%s = " (snd (List.hd c_types))
   else
     Printf.fprintf chan "  ");
  Printf.fprintf chan "%s_%s (" (intrinsic_name name) (string_of_elt elt_ty);
  let print_arg chan arg_number (flags, ty) =
    (* If the argument is of const type, then directly write in the
       constant now.  *)
    if List.mem Const flags then
      match const_valuator with
        None ->
          if List.mem Pointer flags then
            Printf.fprintf chan "0"
          else
            Printf.fprintf chan "1"
      | Some f -> Printf.fprintf chan "%s" (string_of_int (f arg_number))
    else
      Printf.fprintf chan "arg%d_%s" arg_number ty
  in
  let rec print_args arg_number tys =
    match tys with
      [] -> ()
    | [ty] -> print_arg chan arg_number ty
    | ty::tys ->
      print_arg chan arg_number ty;
      Printf.fprintf chan ", ";
      print_args (arg_number + 1) tys
  in
    print_args 0 (List.tl c_types);
    Printf.fprintf chan ");\n"

(* Emit epilogue code to a test source file.  *)
let emit_epilogue chan features regexps =
  let no_op = List.exists (fun feature -> feature = No_op) features in
    Printf.fprintf chan "}\n\n";
    (if not no_op then
       List.iter (fun regexp ->
                   Printf.fprintf chan
                     "/* { dg-final { scan-assembler \"%s\" } } */\n" regexp)
                regexps
     else
       ()
    );
    Printf.fprintf chan "/* { dg-final { cleanup-saved-temps } } */\n"

(* Check a list of C types to determine which ones are pointers and which
   ones are const.  *)
let check_types tys =
  let tys' =
    List.map (fun ty ->
                let len = String.length ty in
                  if len > 2 && String.get ty (len - 2) = ' '
                             && String.get ty (len - 1) = '*'
                  then ([Pointer], String.sub ty 0 (len - 2))
                  else ([], ty)) tys
  in
    List.map (fun (flags, ty) ->
                if String.length ty > 6 && String.sub ty 0 6 = "const "
                then (Const :: flags, String.sub ty 6 ((String.length ty) - 6))
                else (flags, ty)) tys'

(* Given an intrinsic shape, produce a regexp that will match
   the right-hand sides of instructions generated by an intrinsic of
   that shape.  *)
let rec analyze_shape shape =
  let rec n_things n thing =
    match n with
      0 -> []
    | n -> thing :: (n_things (n - 1) thing)
  in
  let rec analyze_shape_elt elt =
    match elt with
      Dreg -> "\\[dD\\]\\[0-9\\]+"
    | Qreg -> "\\[qQ\\]\\[0-9\\]+"
    | Corereg -> "\\[rR\\]\\[0-9\\]+"
    | Immed -> "#\\[0-9\\]+"
    | VecArray (1, elt) ->
        let elt_regexp = analyze_shape_elt elt in
          "((\\\\\\{" ^ elt_regexp ^ "\\\\\\})|(" ^ elt_regexp ^ "))"
    | VecArray (n, elt) ->
      let elt_regexp = analyze_shape_elt elt in
      let alt1 = elt_regexp ^ "-" ^ elt_regexp in
      let alt2 = commas (fun x -> x) (n_things n elt_regexp) "" in
        "\\\\\\{((" ^ alt1 ^ ")|(" ^ alt2 ^ "))\\\\\\}"
    | (PtrTo elt | CstPtrTo elt) ->
      "\\\\\\[" ^ (analyze_shape_elt elt) ^ "\\\\\\]"
    | Element_of_dreg -> (analyze_shape_elt Dreg) ^ "\\\\\\[\\[0-9\\]+\\\\\\]"
    | Element_of_qreg -> (analyze_shape_elt Qreg) ^ "\\\\\\[\\[0-9\\]+\\\\\\]"
    | All_elements_of_dreg -> (analyze_shape_elt Dreg) ^ "\\\\\\[\\\\\\]"
  in
    match shape with
      All (n, elt) -> commas analyze_shape_elt (n_things n elt) ""
    | Long -> (analyze_shape_elt Qreg) ^ ", " ^ (analyze_shape_elt Dreg) ^
              ", " ^ (analyze_shape_elt Dreg)
    | Long_noreg elt -> (analyze_shape_elt elt) ^ ", " ^ (analyze_shape_elt elt)
    | Wide -> (analyze_shape_elt Qreg) ^ ", " ^ (analyze_shape_elt Qreg) ^
              ", " ^ (analyze_shape_elt Dreg)
    | Wide_noreg elt -> analyze_shape (Long_noreg elt)
    | Narrow -> (analyze_shape_elt Dreg) ^ ", " ^ (analyze_shape_elt Qreg) ^
                ", " ^ (analyze_shape_elt Qreg)
    | Use_operands elts -> commas analyze_shape_elt (Array.to_list elts) ""
    | By_scalar Dreg ->
        analyze_shape (Use_operands [| Dreg; Dreg; Element_of_dreg |])
    | By_scalar Qreg ->
        analyze_shape (Use_operands [| Qreg; Qreg; Element_of_dreg |])
    | By_scalar _ -> assert false
    | Wide_lane ->
        analyze_shape (Use_operands [| Qreg; Dreg; Element_of_dreg |])
    | Wide_scalar ->
        analyze_shape (Use_operands [| Qreg; Dreg; Element_of_dreg |])
    | Pair_result elt ->
      let elt_regexp = analyze_shape_elt elt in
        elt_regexp ^ ", " ^ elt_regexp
    | Unary_scalar _ -> "FIXME Unary_scalar"
    | Binary_imm elt -> analyze_shape (Use_operands [| elt; elt; Immed |])
    | Narrow_imm -> analyze_shape (Use_operands [| Dreg; Qreg; Immed |])
    | Long_imm -> analyze_shape (Use_operands [| Qreg; Dreg; Immed |])

(* Generate tests for one intrinsic.  *)
let test_intrinsic dir opcode features shape name munge elt_ty =
  (* Open the test source file.  *)
  let test_name = name ^ (string_of_elt elt_ty) in
  let chan = open_test_file dir test_name in
  (* Work out what argument and return types the intrinsic has.  *)
  let c_arity, new_elt_ty = munge shape elt_ty in
  let c_types = check_types (strings_of_arity c_arity) in
  (* Extract any constant valuator (a function specifying what constant
     values are to be written into the intrinsic call) from the features
     list.  *)
  let const_valuator =
    try
      match (List.find (fun feature -> match feature with
                                         Const_valuator _ -> true
				       | _ -> false) features) with
        Const_valuator f -> Some f
      | _ -> assert false
    with Not_found -> None
  in
  (* Work out what instruction name(s) to expect.  *)
  let insns = get_insn_names features name in
  let no_suffix = (new_elt_ty = NoElts) in
  let insns =
    if no_suffix then insns
                 else List.map (fun insn ->
                                  let suffix = string_of_elt_dots new_elt_ty in
                                    insn ^ "\\." ^ suffix) insns
  in
  (* Construct a regexp to match against the expected instruction name(s).  *)
  let insn_regexp =
    match insns with
      [] -> assert false
    | [insn] -> insn
    | _ ->
      let rec calc_regexp insns cur_regexp =
        match insns with
          [] -> cur_regexp
        | [insn] -> cur_regexp ^ "(" ^ insn ^ "))"
        | insn::insns -> calc_regexp insns (cur_regexp ^ "(" ^ insn ^ ")|")
      in calc_regexp insns "("
  in
  (* Construct regexps to match against the instructions that this
     intrinsic expands to.  Watch out for any writeback character and
     comments after the instruction.  *)
  let regexps = List.map (fun regexp -> insn_regexp ^ "\\[ \t\\]+" ^ regexp ^
			  "!?\\(\\[ \t\\]+@\\[a-zA-Z0-9 \\]+\\)?\\n")
                         (analyze_all_shapes features shape analyze_shape)
  in
    (* Emit file and function prologues.  *)
    emit_prologue chan test_name;
    (* Emit local variable declarations.  *)
    emit_automatics chan c_types;
    Printf.fprintf chan "\n";
    (* Emit the call to the intrinsic.  *)
    emit_call chan const_valuator c_types name elt_ty;
    (* Emit the function epilogue and the DejaGNU scan-assembler directives.  *)
    emit_epilogue chan features regexps;
    (* Close the test file.  *)
    close_out chan

(* Generate tests for one element of the "ops" table.  *)
let test_intrinsic_group dir (opcode, features, shape, name, munge, types) =
  List.iter (test_intrinsic dir opcode features shape name munge) types

(* Program entry point.  *)
let _ =
  let directory = if Array.length Sys.argv <> 1 then Sys.argv.(1) else "." in
    List.iter (test_intrinsic_group directory) (reinterp @ ops)

