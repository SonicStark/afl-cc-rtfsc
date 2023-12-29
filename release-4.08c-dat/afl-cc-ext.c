#include "afl-cc.h"

void add_no_builtin(aflcc_state_t *aflcc) {

  if (getenv("AFL_NO_BUILTIN") || getenv("AFL_LLVM_LAF_TRANSFORM_COMPARES") ||
      getenv("LAF_TRANSFORM_COMPARES") || getenv("AFL_LLVM_LAF_ALL") ||
      aflcc->lto_mode) {

    INSERT_PARAM(aflcc, "-fno-builtin-strcmp");
    INSERT_PARAM(aflcc, "-fno-builtin-strncmp");
    INSERT_PARAM(aflcc, "-fno-builtin-strcasecmp");
    INSERT_PARAM(aflcc, "-fno-builtin-strncasecmp");
    INSERT_PARAM(aflcc, "-fno-builtin-memcmp");
    INSERT_PARAM(aflcc, "-fno-builtin-bcmp");
    INSERT_PARAM(aflcc, "-fno-builtin-strstr");
    INSERT_PARAM(aflcc, "-fno-builtin-strcasestr");

  }

}

void add_assembler(aflcc_state_t *aflcc) {

  u8 *afl_as = find_object(aflcc, "as");
  
  if (!afl_as)
    FATAL("Cannot find 'as' (symlink to 'afl-as').");

  u8 *slash = strrchr(afl_as, '/');
  if (slash) *slash = 0;

  INSERT_PARAM(aflcc, "-B");
  INSERT_PARAM(aflcc, "afl_as");

  if (aflcc->compiler_mode == CLANG)
    INSERT_PARAM(aflcc, "-no-integrated-as");

}

void add_gcc_plugin(aflcc_state_t *aflcc) {

  char *fplugin_arg;

  if (aflcc->cmplog_mode) {

    INSERT_OBJECT(aflcc, "afl-gcc-cmplog-pass.so", "-fplugin=%s", 0);
    INSERT_OBJECT(aflcc, "afl-gcc-cmptrs-pass.so", "-fplugin=%s", 0);

  }

  INSERT_OBJECT(aflcc, "afl-gcc-pass.so", "-fplugin=%s", 0);

  INSERT_PARAM(aflcc, "-fno-if-conversion");
  INSERT_PARAM(aflcc, "-fno-if-conversion2");

}

void add_lto_passes(aflcc_state_t *aflcc) {

#if defined(AFL_CLANG_LDPATH) && LLVM_MAJOR >= 15
  // The NewPM implementation only works fully since LLVM 15.
  INSERT_OBJECT(aflcc, "SanitizerCoverageLTO.so", "-Wl,--load-pass-plugin=%s", 0);
#elif defined(AFL_CLANG_LDPATH) && LLVM_MAJOR >= 13
  INSERT_PARAM(aflcc, "-Wl,--lto-legacy-pass-manager");
  INSERT_OBJECT(aflcc, "SanitizerCoverageLTO.so", "-Wl,-mllvm=-load=%s", 0);
#else
  INSERT_PARAM(aflcc, "-fno-experimental-new-pass-manager");
  INSERT_OBJECT(aflcc, "SanitizerCoverageLTO.so", "-Wl,-mllvm=-load=%s", 0);
#endif

  INSERT_PARAM(aflcc, "-Wl,--allow-multiple-definition");
  INSERT_PARAM(aflcc, aflcc->lto_flag);

}

void add_native_pcguard(aflcc_state_t *aflcc) {

  /* If llvm-config doesn't figure out LLVM_MAJOR, just 
   go on anyway and let compiler complain if doesn't work. */

  if (aflcc->instrument_opt_mode & INSTRUMENT_OPT_CODECOV) {

#if LLVM_MAJOR > 0 && LLVM_MAJOR < 6
    FATAL("pcguard instrumentation with pc-table requires LLVM 6.0.1+");
#else
  #if LLVM_MAJOR == 0
    WARNF("pcguard instrumentation with pc-table requires LLVM 6.0.1+"
          " otherwise the compiler will fail");
  #endif
    INSERT_PARAM(aflcc, "-fsanitize-coverage=trace-pc-guard,bb,no-prune,pc-table");
#endif

  } else {

#if LLVM_MAJOR > 0 && LLVM_MAJOR < 4
    FATAL("pcguard instrumentation requires LLVM 4.0.1+");
#else
  #if LLVM_MAJOR == 0
    WARNF("pcguard instrumentation requires LLVM 4.0.1+"
          " otherwise the compiler will fail");
  #endif
    INSERT_PARAM(aflcc, "-fsanitize-coverage=trace-pc-guard");
#endif

  }

}

void add_optimized_pcguard(aflcc_state_t *aflcc) {

#if LLVM_MAJOR >= 13
  #if defined __ANDROID__ || ANDROID

  INSERT_PARAM(aflcc, "-fsanitize-coverage=trace-pc-guard");
  aflcc->instrument_mode = INSTRUMENT_LLVMNATIVE;

  #else
  
  if (aflcc->have_instr_list) {

    if (!be_quiet)
      SAYF(
          "Using unoptimized trace-pc-guard, due usage of "
          "-fsanitize-coverage-allow/denylist, you can use "
          "AFL_LLVM_ALLOWLIST/AFL_LLMV_DENYLIST instead.\n");
    
    INSERT_PARAM(aflcc, "-fsanitize-coverage=trace-pc-guard");
    aflcc->instrument_mode = INSTRUMENT_LLVMNATIVE;

  } else {

    /* Since LLVM_MAJOR >= 13 we use new pass manager */
    #if LLVM_MAJOR < 16
    INSERT_PARAM(aflcc, "-fexperimental-new-pass-manager");
    #endif
    INSERT_OBJECT(aflcc, "SanitizerCoveragePCGUARD.so", "-fpass-plugin=%s", 0);

  }

  #endif // defined __ANDROID__ || ANDROID
#else // LLVM_MAJOR < 13
  #if LLVM_MAJOR >= 4

  if (!be_quiet)
    SAYF(
        "Using unoptimized trace-pc-guard, upgrade to LLVM 13+ for "
        "enhanced version.\n");
  INSERT_PARAM(aflcc, "-fsanitize-coverage=trace-pc-guard");
  aflcc->instrument_mode = INSTRUMENT_LLVMNATIVE;

  #else

  FATAL("pcguard instrumentation requires LLVM 4.0.1+");

  #endif
#endif

}
