#include "afl-cc.h"

void add_runtime(aflcc_state_t *aflcc) {

  if (aflcc->preprocessor_only || aflcc->have_c || !aflcc->non_dash) {

    /* In the preprocessor_only case (-E), we are not actually compiling at
       all but requesting the compiler to output preprocessed sources only.
       We must not add the runtime in this case because the compiler will
       simply output its binary content back on stdout, breaking any build
       systems that rely on a separate source preprocessing step. */
    return;

  }

  // in case LLVM is installed not via a package manager or "make install"
  // e.g. compiled download or compiled from github then its ./lib directory
  // might not be in the search path. Add it if so.
  u8 *libdir = strdup(LLVM_LIBDIR);
  if (aflcc->plusplus_mode && strlen(libdir) && 
      strncmp(libdir, "/usr", 4) &&
      strncmp(libdir, "/lib", 4)) {

    INSERT_PARAM(aflcc, "-Wl,-rpath");
    INSERT_PARAM(aflcc, libdir);

  } else {

    free(libdir);

  }

#ifndef __ANDROID__

  #define M32_ERR_MSG "-m32 is not supported by your compiler"
  #define M64_ERR_MSG "-m64 is not supported by your compiler"

  if (aflcc->compiler_mode != GCC && aflcc->compiler_mode != CLANG) {

    switch (aflcc->bit_mode) {

      case 0:
        if (!aflcc->shared_linking && !aflcc->partial_linking)
          INSERT_OBJECT(aflcc, "afl-compiler-rt.o", 0, 0);
        if (aflcc->lto_mode)
          INSERT_OBJECT(aflcc, "afl-llvm-rt-lto.o", 0, 0);
        break;

      case 32:
        if (!aflcc->shared_linking && !aflcc->partial_linking)
          INSERT_OBJECT(aflcc, "afl-compiler-rt-32.o", 0, M32_ERR_MSG);
        if (aflcc->lto_mode)
          INSERT_OBJECT(aflcc, "afl-llvm-rt-lto-32.o", 0, M32_ERR_MSG);
        break;

      case 64:
        if (!aflcc->shared_linking && !aflcc->partial_linking)
          INSERT_OBJECT(aflcc, "afl-compiler-rt-64.o", 0, M64_ERR_MSG);
        if (aflcc->lto_mode)
          INSERT_OBJECT(aflcc, "afl-llvm-rt-lto-64.o", 0, M64_ERR_MSG);
        break;

    }

  #if !defined(__APPLE__) && !defined(__sun)
    if (!aflcc->shared_linking && !aflcc->partial_linking)
      INSERT_OBJECT(aflcc, "dynamic_list.txt", "-Wl,--dynamic-list=%s", 0);
  #endif

  #if defined(__APPLE__)
    if (aflcc->shared_linking || aflcc->partial_linking) {

      INSERT_PARAM(aflcc, "-Wl,-U");
      INSERT_PARAM(aflcc, "-Wl,___afl_area_ptr");
      INSERT_PARAM(aflcc, "-Wl,-U");
      INSERT_PARAM(aflcc, "-Wl,___sanitizer_cov_trace_pc_guard_init");

    }
  #endif

  }

#endif

#if defined(USEMMAP) && !defined(__HAIKU__) && !__APPLE__
  INSERT_PARAM(aflcc, "-Wl,-lrt");
#endif

}
