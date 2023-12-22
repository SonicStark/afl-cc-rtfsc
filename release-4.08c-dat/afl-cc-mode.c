#include "afl-cc.h"

void set_real_argv0(aflcc_state_t *aflcc) {

  static u8 llvm_fullpath[PATH_MAX];

  if (aflcc->plusplus_mode) {

    u8 *alt_cxx = getenv("AFL_CXX");

    if (!alt_cxx) {

      if (aflcc->compiler_mode == GCC ||
          aflcc->compiler_mode == GCC_PLUGIN) {
          
        alt_cxx = "g++";

      } else if (aflcc->compiler_mode == CLANG) {

        alt_cxx = "clang++";

      } else {

        if (USE_BINDIR)
          snprintf(llvm_fullpath, sizeof(llvm_fullpath), "%s/clang++",
                   LLVM_BINDIR);
        else
          snprintf(llvm_fullpath, sizeof(llvm_fullpath), CLANGPP_BIN);
        alt_cxx = llvm_fullpath;

      }

    }

    aflcc->cc_params[0] = alt_cxx;

  } else {

    u8 *alt_cc = getenv("AFL_CC");

    if (!alt_cc) {

      if (aflcc->compiler_mode == GCC ||
          aflcc->compiler_mode == GCC_PLUGIN) {
          
        alt_cc = "gcc";

      } else if (aflcc->compiler_mode == CLANG) {

        alt_cc = "clang";

      } else {

        if (USE_BINDIR)
          snprintf(llvm_fullpath, sizeof(llvm_fullpath), "%s/clang",
                   LLVM_BINDIR);
        else
          snprintf(llvm_fullpath, sizeof(llvm_fullpath), CLANG_BIN);
        alt_cc = llvm_fullpath;

      }

    }

    aflcc->cc_params[0] = alt_cc;

  }

}

void compiler_mode_by_callname(aflcc_state_t *aflcc) {

#if (LLVM_MAJOR >= 3)

  if (strncmp(aflcc->callname, "afl-clang-fast", 14) == 0) {

    aflcc->compiler_mode = LLVM;

  } else if (strncmp(aflcc->callname, "afl-clang-lto", 13) == 0 ||

             strncmp(aflcc->callname, "afl-lto", 7) == 0) {

    aflcc->compiler_mode = LTO;

  } else

#endif
      if (strncmp(aflcc->callname, "afl-gcc-fast", 12) == 0 ||

          strncmp(aflcc->callname, "afl-g++-fast", 12) == 0) {

    aflcc->compiler_mode = GCC_PLUGIN;

  } else if (strncmp(aflcc->callname, "afl-gcc", 7) == 0 ||

             strncmp(aflcc->callname, "afl-g++", 7) == 0) {

    aflcc->compiler_mode = GCC;

  } else if (strcmp(aflcc->callname, "afl-clang") == 0 ||

             strcmp(aflcc->callname, "afl-clang++") == 0) {

    aflcc->compiler_mode = CLANG;

  }

}

void compiler_mode_by_environ(aflcc_state_t *aflcc) {

  char *ptr = getenv("AFL_CC_COMPILER");

  if (!ptr) return;

  if (aflcc->compiler_mode) {

    if (!be_quiet) {

      WARNF(
          "\"AFL_CC_COMPILER\" is set but a specific compiler was already "
          "selected by command line parameter or symlink, ignoring the "
          "environment variable!");

    }

  } else {

    if (strncasecmp(ptr, "LTO", 3) == 0) {

      aflcc->compiler_mode = LTO;

    } else if (strncasecmp(ptr, "LLVM", 4) == 0) {

      aflcc->compiler_mode = LLVM;

    } else if (strncasecmp(ptr, "GCC_P", 5) == 0 ||

                strncasecmp(ptr, "GCC-P", 5) == 0 ||
                strncasecmp(ptr, "GCCP", 4) == 0) {

      aflcc->compiler_mode = GCC_PLUGIN;

    } else if (strcasecmp(ptr, "GCC") == 0) {

      aflcc->compiler_mode = GCC;

    } else if (strcasecmp(ptr, "CLANG") == 0) {

      aflcc->compiler_mode = CLANG;

    } else

      FATAL("Unknown AFL_CC_COMPILER mode: %s\n", ptr);

  }

}
