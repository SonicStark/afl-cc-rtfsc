#include "afl-cc.h"

void add_misc_args(aflcc_state_t *aflcc) {

  if (!aflcc->have_pic) { insert_param(aflcc, "-fPIC"); }

  if (getenv("AFL_HARDEN")) {

    insert_param(aflcc, "-fstack-protector-all");

    if (!aflcc->fortify_set) ctrl_fortification(aflcc, 2);

  }

  if (!getenv("AFL_DONT_OPTIMIZE")) {

    insert_param(aflcc, "-g");
    if (!aflcc->have_o)   insert_param(aflcc, "-O3");
    if (!aflcc->have_unroll)  insert_param(aflcc, "-funroll-loops");
    // if (strlen(aflcc->march_opt) > 1 && aflcc->march_opt[0] == '-')
    //     insert_param(aflcc, aflcc->march_opt);

  }

  if (aflcc->x_set) {

    insert_param(aflcc, "-x");
    insert_param(aflcc, "none");

  }

}

param_st handle_misc_args(aflcc_state_t *aflcc, u8 *cur_argv, u8 scan) {

  param_st final_ = PARAM_MISS;

#define SCAN_ORIG(dst, src) \
  do {                      \
                            \
    if (scan) {             \
                            \
      dst = src;            \
      final_ = PARAM_SCAN;  \
                            \
    } else {                \
                            \
      final_ = PARAM_ORIG;  \
                            \
    }                       \
                            \
  } while (0)

  if (!strncasecmp(cur_argv, "-fpic", 5)) {

    SCAN_ORIG(aflcc->have_pic, 1);

  } else if (cur_argv[0] != '-') { 

    SCAN_ORIG(aflcc->non_dash, 1);

  } else if (!strcmp(cur_argv, "-m32") ||
             !strcmp(cur_argv, "armv7a-linux-androideabi")) {

    SCAN_ORIG(aflcc->bit_mode, 32);
  
  } else if (!strcmp(cur_argv, "-m64")) {

    SCAN_ORIG(aflcc->bit_mode, 64);
   
  } else if (strstr(cur_argv, "FORTIFY_SOURCE")) {
    
    SCAN_ORIG(aflcc->fortify_set, 1);
  
  } else if (!strcmp(cur_argv, "-x")) {
    
    SCAN_ORIG(aflcc->x_set, 1);

  } else if (!strcmp(cur_argv, "-E")) {

    SCAN_ORIG(aflcc->preprocessor_only, 1);

  } else if (!strcmp(cur_argv, "--target=wasm32-wasi")) {
    
    SCAN_ORIG(aflcc->passthrough, 1);

  } else if (!strcmp(cur_argv, "-c")) {
    
    SCAN_ORIG(aflcc->have_c, 1);

  } else if (!strncmp(cur_argv, "-O", 2)) {
    
    SCAN_ORIG(aflcc->have_o, 1);

  } else if (!strncmp(cur_argv, "-funroll-loop", 13)) {

    SCAN_ORIG(aflcc->have_unroll, 1);

  } else if (!strncmp(cur_argv, "--afl", 5)) {

    if (scan)
      final_ = PARAM_SCAN;
    else      
      final_ = PARAM_DROP;

  } else if (!strncmp(cur_argv, "-fno-unroll", 11)) {

    if (scan)
      final_ = PARAM_SCAN;
    else      
      final_ = PARAM_DROP;

  } else if (!strcmp(cur_argv, "-pipe") && 
            aflcc->compiler_mode == GCC_PLUGIN) {
    if (scan)
      final_ = PARAM_SCAN;
    else      
      final_ = PARAM_DROP;
  
  } else if (!strncmp(cur_argv, "-stdlib=", 8) &&
            (aflcc->compiler_mode == GCC || 
             aflcc->compiler_mode == GCC_PLUGIN)) {
    
    if (scan) {

      final_ = PARAM_SCAN;

    } else {
      
      if (!be_quiet) WARNF("Found '%s' - stripping!", cur_argv);   
      final_ = PARAM_DROP;

    }

  }

#undef SCAN_ORIG

  if (final_ == PARAM_ORIG)
    insert_param(aflcc, cur_argv);

  return final_;

}
