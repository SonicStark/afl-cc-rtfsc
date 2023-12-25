#include "afl-cc.h"

#define RT_INSERT(cc, rt)                                     \
  do {                                                        \
                                                              \
    if (!cc->shared_linking && !cc->partial_linking)          \
      INSERT_PARAM(cc, alloc_printf("%s/" rt, cc->obj_path)); \
                                                              \
  } while (0)

#define RT_CHECK(cc, ar)                                    \
  do {                                                      \
                                                            \
    if (access(cc->cc_params[cc->cc_par_cnt - 1], R_OK))    \
      FATAL("-m" ar " is not supported by your compiler");  \
                                                            \
  } while (0)

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

  if (aflcc->compiler_mode != GCC && aflcc->compiler_mode != CLANG) {

    switch (aflcc->bit_mode) {

      case 0:
        if (!aflcc->shared_linking && !aflcc->partial_linking)
          RT_INSERT(aflcc, "afl-compiler-rt.o");
        if (aflcc->lto_mode)
          RT_INSERT(aflcc, "afl-llvm-rt-lto.o");
        break;

      case 32:
        if (!aflcc->shared_linking && !aflcc->partial_linking) {

          RT_INSERT(aflcc, "afl-compiler-rt-32.o");
          RT_CHECK(aflcc, "32");

        }

        if (aflcc->lto_mode) {

          RT_INSERT(aflcc, "afl-llvm-rt-lto-32.o");
          RT_CHECK(aflcc, "32");

        }

        break;

      case 64:
        if (!aflcc->shared_linking && !aflcc->partial_linking) {

          RT_INSERT(aflcc, "afl-compiler-rt-64.o");
          RT_CHECK(aflcc, "64");

        }

        if (aflcc->lto_mode) {

          RT_INSERT(aflcc, "afl-llvm-rt-lto-64.o");
          RT_CHECK(aflcc, "64");

        }

        break;

    }

  #if !defined(__APPLE__) && !defined(__sun)
    if (!aflcc->shared_linking && !aflcc->partial_linking)
      INSERT_PARAM(aflcc, 
        alloc_printf("-Wl,--dynamic-list=%s/dynamic_list.txt", aflcc->obj_path));
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
