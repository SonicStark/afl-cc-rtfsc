/*
   american fuzzy lop++ - compiler instrumentation wrapper
   -------------------------------------------------------

   Written by Michal Zalewski, Laszlo Szekeres and Marc Heuse

   Copyright 2015, 2016 Google Inc. All rights reserved.
   Copyright 2019-2023 AFLplusplus Project. All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     https://www.apache.org/licenses/LICENSE-2.0

 */

#define AFL_MAIN

#ifndef AFL_CC_HEADER
#define AFL_CC_HEADER

#include "common.h"
#include "config.h"
#include "types.h"
#include "debug.h"
#include "alloc-inl.h"
#include "llvm-alternative-coverage.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>

#if (LLVM_MAJOR - 0 == 0)
  #undef LLVM_MAJOR
#endif
#if !defined(LLVM_MAJOR)
  #define LLVM_MAJOR 0
#endif
#if (LLVM_MINOR - 0 == 0)
  #undef LLVM_MINOR
#endif
#if !defined(LLVM_MINOR)
  #define LLVM_MINOR 0
#endif

#ifndef MAX_PARAMS_NUM
  #define MAX_PARAMS_NUM 2048
#endif

typedef enum {

  PARAM_MISS, // not matched
  PARAM_SCAN, // scan without any changes
  PARAM_ORIG, // kept as-is
  PARAM_FROM, // modifed before kept
  PARAM_DROP, // ignored

} param_st;

typedef enum {

  INSTRUMENT_DEFAULT = 0,
  INSTRUMENT_CLASSIC = 1,
  INSTRUMENT_AFL = 1,
  INSTRUMENT_PCGUARD = 2,
  INSTRUMENT_CFG = 3,
  INSTRUMENT_LTO = 4,
  INSTRUMENT_LLVMNATIVE = 5,
  INSTRUMENT_GCC = 6,
  INSTRUMENT_CLANG = 7,
  INSTRUMENT_OPT_CTX = 8,
  INSTRUMENT_OPT_NGRAM = 16,
  INSTRUMENT_OPT_CALLER = 32,
  INSTRUMENT_OPT_CTX_K = 64,
  INSTRUMENT_OPT_CODECOV = 128,

} instrument_mode_id;

typedef enum {

  UNSET = 0,
  LTO = 1,
  LLVM = 2,
  GCC_PLUGIN = 3,
  GCC = 4,
  CLANG = 5

} compiler_mode_id;

u8 *instrument_mode_2str(instrument_mode_id);
u8 *compiler_mode_2str(compiler_mode_id);

typedef struct aflcc_state
{

  u8 **cc_params;                 /* Parameters passed to the real CC  */
  u32  cc_par_cnt;                /* Param count, including argv0      */

  u8 *argv0;                      /* Original argv0 (by strdup)        */
  u8 *callname;                   /* Executable file argv0 indicated   */

  u8 debug;

  u8 compiler_mode,
      plusplus_mode,
      lto_mode;

  u8 *lto_flag;

  u8 instrument_mode,
      instrument_opt_mode, 
      ngram_size, 
      ctx_k; 
  
  u8 cmplog_mode;

  u8 have_instr_env,
      have_gcc, 
      have_llvm, 
      have_gcc_plugin,
      have_lto, 
      have_optimized_pcguard,
      have_instr_list;

  u8 fortify_set,
      asan_set,
      x_set,
      bit_mode,
      preprocessor_only,
      have_unroll,
      have_o,
      have_pic,
      have_c,
      shared_linking,
      partial_linking,
      non_dash;

  // u8 *march_opt;
  u8 need_aflpplib;
  int passthrough;

  u8 use_stdin;                   /* dummy */
  u8 *argvnull;                   /* dummy */

} aflcc_state_t;

void aflcc_state_init(aflcc_state_t *);

u8 *getthecwd();

void init_callname(aflcc_state_t *, u8 *argv0);

void maybe_show_help(aflcc_state_t *, int argc, char **argv);

/* Try to find a specific runtime we need, the path to obj would be 
  allocated and returned. Otherwise it returns NULL on fail. */
u8 *find_object(aflcc_state_t *, u8 *obj);

void find_built_deps(aflcc_state_t *);

#define INSERT_PARAM(cc, pa)                                        \
  do {                                                              \
                                                                    \
    cc->cc_params[cc->cc_par_cnt++] = pa;                           \
                                                                    \
  } while (0)

#define INSERT_OBJECT(cc, obj, fmt, msg)                            \
  do {                                                              \
                                                                    \
    u8 *_rt_path = find_object(cc, obj);                            \
    if (!_rt_path) {                                                \
                                                                    \
      if (msg)                                                      \
        FATAL(STRINGIFY(msg));                                      \
      else                                                          \
        FATAL("Unable to find '%s'", obj);                          \
                                                                    \
    } else {                                                        \
                                                                    \
      if (fmt) {                                                    \
                                                                    \
        u8 *_rt_path_fmt = alloc_printf(STRINGIFY(fmt), _rt_path);  \
        ck_free(_rt_path);                                          \
        cc->cc_params[cc->cc_par_cnt++] = _rt_path_fmt;             \
                                                                    \
      } else {                                                      \
                                                                    \
        cc->cc_params[cc->cc_par_cnt++] = _rt_path;                 \
                                                                    \
      }                                                             \
                                                                    \
    }                                                               \
                                                                    \
  } while (0)

static inline void load_llvm_pass(aflcc_state_t *aflcc, u8 *pass) {

#if LLVM_MAJOR >= 11              /* use new pass manager */
  #if LLVM_MAJOR < 16
      INSERT_PARAM(aflcc, "-fexperimental-new-pass-manager");
  #endif
      INSERT_OBJECT(aflcc, pass, "-fpass-plugin=%s", 0);
#else
      INSERT_PARAM(aflcc, "-Xclang");
      INSERT_PARAM(aflcc, "-load");
      INSERT_PARAM(aflcc, "-Xclang");
      INSERT_OBJECT(aflcc, pass, 0, 0);
#endif

}

static inline void debugf_args(int argc, char **argv) {

  DEBUGF("cd '%s';", getthecwd());
  for (int i = 0; i < argc; i++)
    SAYF(" '%s'", argv[i]);
  SAYF("\n");
  fflush(stdout);
  fflush(stderr);

}

void set_real_argv0(aflcc_state_t *);
void compiler_mode_by_callname(aflcc_state_t *);
void compiler_mode_by_environ(aflcc_state_t *);
void compiler_mode_by_cmdline(aflcc_state_t *, int argc, char **argv);
void instrument_mode_by_environ(aflcc_state_t *);
void mode_final_checkout(aflcc_state_t *, int argc, char **argv);
void mode_notification(aflcc_state_t *);

/* provided for preprocessor */

void add_macro(aflcc_state_t *);
void set_fortification(aflcc_state_t *, u8);
void add_lsan_ctrl(aflcc_state_t *);

/* for generic compiler driver */

param_st handle_fsanitize(aflcc_state_t *, u8 *cur_argv, u8 scan);
void add_sanitizers(aflcc_state_t *, char **envp);

void add_no_builtin(aflcc_state_t *);
void add_assembler(aflcc_state_t *);
void add_gcc_plugin(aflcc_state_t *);

/* linking */

void add_runtime(aflcc_state_t *);
void add_lto_linker(aflcc_state_t *);

#endif
