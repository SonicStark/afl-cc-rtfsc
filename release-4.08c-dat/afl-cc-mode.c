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

  if (getenv("AFL_PASSTHROUGH") || getenv("AFL_NOOPT")) {

    aflcc->passthrough = 1;

  }

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

// If it can be inferred, instrument_mode would also be set
void compiler_mode_by_cmdline(aflcc_state_t *aflcc, int argc, char **argv) {

  char *ptr = NULL;

  for (int i = 1; i < argc; i++) {

    if (strncmp(argv[i], "--afl", 5) == 0) {

      if (!strcmp(argv[i], "--afl_noopt") || !strcmp(argv[i], "--afl-noopt")) {

        aflcc->passthrough = 1;
        argv[i] = "-g";  // we have to overwrite it, -g is always good
        continue;

      }

      if (aflcc->compiler_mode && !be_quiet) {

        WARNF(
            "--afl-... compiler mode supersedes the AFL_CC_COMPILER and "
            "symlink compiler selection!");

      }

      ptr = argv[i];
      ptr += 5;
      while (*ptr == '-')
        ptr++;

      if (strncasecmp(ptr, "LTO", 3) == 0) {

        aflcc->compiler_mode = LTO;

      } else if (strncasecmp(ptr, "LLVM", 4) == 0) {

        aflcc->compiler_mode = LLVM;

      } else if (strncasecmp(ptr, "PCGUARD", 7) == 0 ||

                 strncasecmp(ptr, "PC-GUARD", 8) == 0) {

        aflcc->compiler_mode = LLVM;
        aflcc->instrument_mode = INSTRUMENT_PCGUARD;

      } else if (strcasecmp(ptr, "INSTRIM") == 0 ||

                 strcasecmp(ptr, "CFG") == 0) {

        FATAL(
            "InsTrim instrumentation was removed. Use a modern LLVM and "
            "PCGUARD (default in afl-cc).\n");

      } else if (strcasecmp(ptr, "AFL") == 0 ||

                 strcasecmp(ptr, "CLASSIC") == 0) {

        aflcc->compiler_mode = LLVM;
        aflcc->instrument_mode = INSTRUMENT_CLASSIC;

      } else if (strcasecmp(ptr, "LLVMNATIVE") == 0 ||

                 strcasecmp(ptr, "NATIVE") == 0 ||
                 strcasecmp(ptr, "LLVM-NATIVE") == 0) {

        aflcc->compiler_mode = LLVM;
        aflcc->instrument_mode = INSTRUMENT_LLVMNATIVE;

      } else if (strncasecmp(ptr, "GCC_P", 5) == 0 ||

                 strncasecmp(ptr, "GCC-P", 5) == 0 ||
                 strncasecmp(ptr, "GCCP", 4) == 0) {

        aflcc->compiler_mode = GCC_PLUGIN;

      } else if (strcasecmp(ptr, "GCC") == 0) {

        aflcc->compiler_mode = GCC;

      } else if (strncasecmp(ptr, "CLANG", 5) == 0) {

        aflcc->compiler_mode = CLANG;

      } else

        FATAL("Unknown --afl-... compiler mode: %s\n", argv[i]);

    }

  }

}

static void instrument_mode_old_environ(aflcc_state_t *aflcc) {

  if (getenv("AFL_LLVM_INSTRIM") || getenv("INSTRIM") ||
      getenv("INSTRIM_LIB")) {

    FATAL(
        "InsTrim instrumentation was removed. Use a modern LLVM and PCGUARD "
        "(default in afl-cc).\n");

  }

  if (getenv("USE_TRACE_PC") || getenv("AFL_USE_TRACE_PC") ||
      getenv("AFL_LLVM_USE_TRACE_PC") || getenv("AFL_TRACE_PC")) {

    if (aflcc->instrument_mode == 0)
      aflcc->instrument_mode = INSTRUMENT_PCGUARD;
    else if (aflcc->instrument_mode != INSTRUMENT_PCGUARD)
      FATAL("you cannot set AFL_LLVM_INSTRUMENT and AFL_TRACE_PC together");

  }

  if (getenv("AFL_LLVM_CTX")) aflcc->instrument_opt_mode |= INSTRUMENT_OPT_CTX;
  if (getenv("AFL_LLVM_CALLER")) aflcc->instrument_opt_mode |= INSTRUMENT_OPT_CALLER;

  if (getenv("AFL_LLVM_NGRAM_SIZE")) {

    aflcc->instrument_opt_mode |= INSTRUMENT_OPT_NGRAM;
    aflcc->ngram_size = atoi(getenv("AFL_LLVM_NGRAM_SIZE"));
    if (aflcc->ngram_size < 2 || aflcc->ngram_size > NGRAM_SIZE_MAX)
      FATAL(
          "NGRAM instrumentation mode must be between 2 and NGRAM_SIZE_MAX "
          "(%u)",
          NGRAM_SIZE_MAX);

  }

  if (getenv("AFL_LLVM_CTX_K")) {

    aflcc->ctx_k = atoi(getenv("AFL_LLVM_CTX_K"));
    if (aflcc->ctx_k < 1 || aflcc->ctx_k > CTX_MAX_K)
      FATAL("K-CTX instrumentation mode must be between 1 and CTX_MAX_K (%u)",
            CTX_MAX_K);
    if (aflcc->ctx_k == 1) {

      setenv("AFL_LLVM_CALLER", "1", 1);
      unsetenv("AFL_LLVM_CTX_K");
      aflcc->instrument_opt_mode |= INSTRUMENT_OPT_CALLER;

    } else {

      aflcc->instrument_opt_mode |= INSTRUMENT_OPT_CTX_K;

    }

  }

}

// compiler_mode would also be set if depended by the instrument_mode
static void instrument_mode_new_environ(aflcc_state_t *aflcc) {

  if (!getenv("AFL_LLVM_INSTRUMENT")) return;

  u8 *ptr2 = strtok(getenv("AFL_LLVM_INSTRUMENT"), ":,;");

  while (ptr2) {

    if (strncasecmp(ptr2, "afl", strlen("afl")) == 0 ||
        strncasecmp(ptr2, "classic", strlen("classic")) == 0) {

      if (aflcc->instrument_mode == INSTRUMENT_LTO) {

        aflcc->instrument_mode = INSTRUMENT_CLASSIC;
        aflcc->lto_mode = 1;

      } else if (!aflcc->instrument_mode || 
                  aflcc->instrument_mode == INSTRUMENT_AFL) {

        aflcc->instrument_mode = INSTRUMENT_AFL;

      } else {

        FATAL("main instrumentation mode already set with %s",
              instrument_mode_2str(aflcc->instrument_mode));

      }

    }

    if (strncasecmp(ptr2, "pc-guard", strlen("pc-guard")) == 0 ||
        strncasecmp(ptr2, "pcguard", strlen("pcguard")) == 0) {

      if (!aflcc->instrument_mode || 
          aflcc->instrument_mode == INSTRUMENT_PCGUARD)

        aflcc->instrument_mode = INSTRUMENT_PCGUARD;

      else
        FATAL("main instrumentation mode already set with %s",
              instrument_mode_2str(aflcc->instrument_mode));

    }

    if (strncasecmp(ptr2, "llvmnative", strlen("llvmnative")) == 0 ||
        strncasecmp(ptr2, "llvm-native", strlen("llvm-native")) == 0 ||
        strncasecmp(ptr2, "native", strlen("native")) == 0) {

      if (!aflcc->instrument_mode || 
          aflcc->instrument_mode == INSTRUMENT_LLVMNATIVE)

        aflcc->instrument_mode = INSTRUMENT_LLVMNATIVE;

      else
        FATAL("main instrumentation mode already set with %s",
              instrument_mode_2str(aflcc->instrument_mode));

    }

    if (strncasecmp(ptr2, "llvmcodecov", strlen("llvmcodecov")) == 0 ||
        strncasecmp(ptr2, "llvm-codecov", strlen("llvm-codecov")) == 0) {

      if (!aflcc->instrument_mode ||
          aflcc->instrument_mode == INSTRUMENT_LLVMNATIVE) {

        aflcc->instrument_mode = INSTRUMENT_LLVMNATIVE;
        aflcc->instrument_opt_mode |= INSTRUMENT_OPT_CODECOV;

      } else {

        FATAL("main instrumentation mode already set with %s",
              instrument_mode_2str(aflcc->instrument_mode));

      }

    }

    if (strncasecmp(ptr2, "cfg", strlen("cfg")) == 0 ||
        strncasecmp(ptr2, "instrim", strlen("instrim")) == 0) {

      FATAL(
          "InsTrim instrumentation was removed. Use a modern LLVM and "
          "PCGUARD (default in afl-cc).\n");

    }

    if (strncasecmp(ptr2, "lto", strlen("lto")) == 0) {

      aflcc->lto_mode = 1;
      if (!aflcc->instrument_mode || 
          aflcc->instrument_mode == INSTRUMENT_LTO)

        aflcc->instrument_mode = INSTRUMENT_LTO;

      else
        FATAL("main instrumentation mode already set with %s",
              instrument_mode_2str(aflcc->instrument_mode));

    }

    if (strcasecmp(ptr2, "gcc") == 0) {

      if (!aflcc->instrument_mode || 
          aflcc->instrument_mode == INSTRUMENT_GCC)

        aflcc->instrument_mode = INSTRUMENT_GCC;

      else if (aflcc->instrument_mode != INSTRUMENT_GCC)
        FATAL("main instrumentation mode already set with %s",
              instrument_mode_2str(aflcc->instrument_mode));

      aflcc->compiler_mode = GCC;

    }

    if (strcasecmp(ptr2, "clang") == 0) {

      if (!aflcc->instrument_mode ||
          aflcc->instrument_mode == INSTRUMENT_CLANG)

        aflcc->instrument_mode = INSTRUMENT_CLANG;

      else if (aflcc->instrument_mode != INSTRUMENT_CLANG)
        FATAL("main instrumentation mode already set with %s",
              instrument_mode_2str(aflcc->instrument_mode));
      
      aflcc->compiler_mode = CLANG;

    }

    if (strncasecmp(ptr2, "ctx-", strlen("ctx-")) == 0 ||
        strncasecmp(ptr2, "kctx-", strlen("c-ctx-")) == 0 ||
        strncasecmp(ptr2, "k-ctx-", strlen("k-ctx-")) == 0) {

      u8 *ptr3 = ptr2;
      while (*ptr3 && (*ptr3 < '0' || *ptr3 > '9'))
        ptr3++;

      if (!*ptr3) {

        if ((ptr3 = getenv("AFL_LLVM_CTX_K")) == NULL)
          FATAL(
              "you must set the K-CTX K with (e.g. for value 2) "
              "AFL_LLVM_INSTRUMENT=ctx-2");

      }

      aflcc->ctx_k = atoi(ptr3);
      if (aflcc->ctx_k < 1 || aflcc->ctx_k > CTX_MAX_K)
        FATAL(
            "K-CTX instrumentation option must be between 1 and CTX_MAX_K "
            "(%u)",
            CTX_MAX_K);

      if (aflcc->ctx_k == 1) {

        aflcc->instrument_opt_mode |= INSTRUMENT_OPT_CALLER;
        setenv("AFL_LLVM_CALLER", "1", 1);
        unsetenv("AFL_LLVM_CTX_K");

      } else {

        aflcc->instrument_opt_mode |= (INSTRUMENT_OPT_CTX_K);
        u8 *ptr4 = alloc_printf("%u", aflcc->ctx_k);
        setenv("AFL_LLVM_CTX_K", ptr4, 1);

      }

    }

    if (strcasecmp(ptr2, "ctx") == 0) {

      aflcc->instrument_opt_mode |= INSTRUMENT_OPT_CTX;
      setenv("AFL_LLVM_CTX", "1", 1);

    }

    if (strncasecmp(ptr2, "caller", strlen("caller")) == 0) {

      aflcc->instrument_opt_mode |= INSTRUMENT_OPT_CALLER;
      setenv("AFL_LLVM_CALLER", "1", 1);

    }

    if (strncasecmp(ptr2, "ngram", strlen("ngram")) == 0) {

      u8 *ptr3 = ptr2 + strlen("ngram");
      while (*ptr3 && (*ptr3 < '0' || *ptr3 > '9'))
        ptr3++;

      if (!*ptr3) {

        if ((ptr3 = getenv("AFL_LLVM_NGRAM_SIZE")) == NULL)
          FATAL(
              "you must set the NGRAM size with (e.g. for value 2) "
              "AFL_LLVM_INSTRUMENT=ngram-2");

      }

      aflcc->ngram_size = atoi(ptr3);
      if (aflcc->ngram_size < 2 || aflcc->ngram_size > NGRAM_SIZE_MAX)
        FATAL(
            "NGRAM instrumentation option must be between 2 and "
            "NGRAM_SIZE_MAX (%u)",
            NGRAM_SIZE_MAX);
      aflcc->instrument_opt_mode |= (INSTRUMENT_OPT_NGRAM);
      u8 *ptr4 = alloc_printf("%u", aflcc->ngram_size);
      setenv("AFL_LLVM_NGRAM_SIZE", ptr4, 1);

    }

    ptr2 = strtok(NULL, ":,;");

  }

}

void instrument_mode_by_environ(aflcc_state_t *aflcc) {

  if (getenv("AFL_LLVM_INSTRUMENT_FILE") || getenv("AFL_LLVM_WHITELIST") ||
      getenv("AFL_LLVM_ALLOWLIST") || getenv("AFL_LLVM_DENYLIST") ||
      getenv("AFL_LLVM_BLOCKLIST")) {

    aflcc->have_instr_env = 1;

  }

  if (aflcc->have_instr_env && getenv("AFL_DONT_OPTIMIZE") && !be_quiet) {

    WARNF(
        "AFL_LLVM_ALLOWLIST/DENYLIST and AFL_DONT_OPTIMIZE cannot be combined "
        "for file matching, only function matching!");

  }

  instrument_mode_old_environ(aflcc);
  instrument_mode_new_environ(aflcc);  

}

void instrument_opt_mode_mutex(aflcc_state_t *aflcc) {

  if ((aflcc->instrument_opt_mode & INSTRUMENT_OPT_CTX) &&
      (aflcc->instrument_opt_mode & INSTRUMENT_OPT_CALLER)) {

    FATAL("you cannot set CTX and CALLER together");

  }

  if ((aflcc->instrument_opt_mode & INSTRUMENT_OPT_CTX) &&
      (aflcc->instrument_opt_mode & INSTRUMENT_OPT_CTX_K)) {

    FATAL("you cannot set CTX and K-CTX together");

  }

  if ((aflcc->instrument_opt_mode & INSTRUMENT_OPT_CALLER) &&
      (aflcc->instrument_opt_mode & INSTRUMENT_OPT_CTX_K)) {

    FATAL("you cannot set CALLER and K-CTX together");

  }

}
