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

  u8  *obj_path;                  /* Path to runtime libraries         */
  u8 **cc_params;                 /* Parameters passed to the real CC  */
  u32  cc_par_cnt;                /* Param count, including argv0      */

  u8 debug;

  u8 llvm_fullpath[PATH_MAX];

  u8 compiler_mode,
      plusplus_mode,
      clang_mode,                 /* Invoked as afl-clang*?            */
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
      have_instr_list;

  u8 fortify_set,
      asan_set,
      x_set,
      bit_mode,
      hared_linking,
      preprocessor_only,
      have_unroll,
      have_o,
      have_pic,
      have_c,
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

#endif
