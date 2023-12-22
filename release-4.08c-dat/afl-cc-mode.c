#include "afl-cc.h"

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

  if (strcmp(aflcc->callname, "afl-clang") == 0 ||
      strcmp(aflcc->callname, "afl-clang++") == 0) {

    aflcc->clang_mode = 1;
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

    } else

      FATAL("Unknown AFL_CC_COMPILER mode: %s\n", ptr);

  }

}
