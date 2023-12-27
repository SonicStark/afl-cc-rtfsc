#include "afl-cc.h"

static void add_aflpplib(aflcc_state_t *aflcc) {

  if (!aflcc->need_aflpplib) return;

  u8 *afllib = find_object(aflcc, "libAFLDriver.a");

  if (!be_quiet) {

    OKF("Found '-fsanitize=fuzzer', replacing with libAFLDriver.a");

  }

  if (!afllib) {

    if (!be_quiet) {

      WARNF(
          "Cannot find 'libAFLDriver.a' to replace '-fsanitize=fuzzer' in "
          "the flags - this will fail!");

    }

  } else {

    INSERT_PARAM(aflcc, afllib);

#ifdef __APPLE__
    INSERT_PARAM(aflcc, "-Wl,-undefined");
    INSERT_PARAM(aflcc, "dynamic_lookup");
#endif

  }

}

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

  add_aflpplib(aflcc);

#if defined(USEMMAP) && !defined(__HAIKU__) && !__APPLE__
  INSERT_PARAM(aflcc, "-Wl,-lrt");
#endif

}

void add_lto_linker(aflcc_state_t *aflcc) {

  unsetenv("AFL_LD");
  unsetenv("AFL_LD_CALLER");

  u8 *ld_path = NULL;
  if (getenv("AFL_REAL_LD")) {

    ld_path = strdup(getenv("AFL_REAL_LD"));

  } else {

    ld_path = strdup(AFL_REAL_LD);

  }

  if (!ld_path || !*ld_path) {

    if (ld_path) {

      // Freeing empty string
      free(ld_path);

    }

    ld_path = strdup("ld.lld");

  }

  if (!ld_path) { PFATAL("Could not allocate mem for ld_path"); }
#if defined(AFL_CLANG_LDPATH) && LLVM_MAJOR >= 12
  INSERT_PARAM(aflcc, alloc_printf("--ld-path=%s", ld_path));
#else
  INSERT_PARAM(aflcc, alloc_printf("-fuse-ld=%s", ld_path));
#endif
  free(ld_path);

}
