#include "afl-cc.h"

void parse_fsanitize(char *string) {

  char *p, *ptr = string + strlen("-fsanitize=");
  char *new = malloc(strlen(string) + 1);
  char *tmp = malloc(strlen(ptr));
  u32   count = 0, len, ende = 0;

  if (!new || !tmp) { FATAL("could not acquire memory"); }
  strcpy(new, "-fsanitize=");

  do {

    p = strchr(ptr, ',');
    if (!p) {

      p = ptr + strlen(ptr) + 1;
      ende = 1;

    }

    len = p - ptr;
    if (len) {

      strncpy(tmp, ptr, len);
      tmp[len] = 0;
      // fprintf(stderr, "Found: %s\n", tmp);
      ptr += len + 1;
      if (*tmp) {

        u32 copy = 1;
        if (!strcmp(tmp, "fuzzer")) {

          need_aflpplib = 1;
          copy = 0;

        } else if (!strncmp(tmp, "fuzzer", 6)) {

          copy = 0;

        }

        if (copy) {

          if (count) { strcat(new, ","); }
          strcat(new, tmp);
          ++count;

        }

      }

    } else {

      ptr++;                                    /*fprintf(stderr, "NO!\n"); */

    }

  } while (!ende);

  strcpy(string, new);
  // fprintf(stderr, "string: %s\n", string);
  // fprintf(stderr, "new: %s\n", new);

}

static void process_params(u32 argc, char **argv) {

  if (cc_par_cnt + argc >= MAX_PARAMS_NUM) {

    FATAL("Too many command line parameters, please increase MAX_PARAMS_NUM.");

  }

  if (lto_mode && argc > 1) {

    u32 idx;
    for (idx = 1; idx < argc; idx++) {

      if (!strncasecmp(argv[idx], "-fpic", 5)) have_pic = 1;

    }

  }

  // for (u32 x = 0; x < argc; ++x) fprintf(stderr, "[%u] %s\n", x, argv[x]);

  /* Process the argument list. */

  u8 skip_next = 0;
  while (--argc) {

    u8 *cur = *(++argv);

    if (skip_next) {

      skip_next = 0;
      continue;

    }

    if (cur[0] != '-') { non_dash = 1; }
    if (!strncmp(cur, "--afl", 5)) continue;

    if (lto_mode && !strncmp(cur, "-flto=thin", 10)) {

      FATAL(
          "afl-clang-lto cannot work with -flto=thin. Switch to -flto=full or "
          "use afl-clang-fast!");

    }

    if (lto_mode && !strncmp(cur, "-fuse-ld=", 9)) continue;
    if (lto_mode && !strncmp(cur, "--ld-path=", 10)) continue;
    if (!strncmp(cur, "-fno-unroll", 11)) continue;
    if (strstr(cur, "afl-compiler-rt") || strstr(cur, "afl-llvm-rt")) continue;
    if (!strcmp(cur, "-Wl,-z,defs") || !strcmp(cur, "-Wl,--no-undefined") ||
        !strcmp(cur, "--no-undefined")) {

      continue;

    }

    if (compiler_mode == GCC_PLUGIN && !strcmp(cur, "-pipe")) { continue; }

    if (!strcmp(cur, "-z") || !strcmp(cur, "-Wl,-z")) {

      u8 *param = *(argv + 1);
      if (!strcmp(param, "defs") || !strcmp(param, "-Wl,defs")) {

        skip_next = 1;
        continue;

      }

    }

    if ((compiler_mode == GCC || compiler_mode == GCC_PLUGIN) &&
        !strncmp(cur, "-stdlib=", 8)) {

      if (!be_quiet) { WARNF("Found '%s' - stripping!", cur); }
      continue;

    }

    if (!strncmp(cur, "-fsanitize-coverage-", 20) && strstr(cur, "list=")) {

      have_instr_list = 1;

    }

    if (!strncmp(cur, "-fsanitize=", strlen("-fsanitize=")) &&
        strchr(cur, ',')) {

      parse_fsanitize(cur);
      if (!cur || strlen(cur) <= strlen("-fsanitize=")) { continue; }

    } else if ((!strncmp(cur, "-fsanitize=fuzzer-",

                         strlen("-fsanitize=fuzzer-")) ||
                !strncmp(cur, "-fsanitize-coverage",
                         strlen("-fsanitize-coverage"))) &&
               (strncmp(cur, "sanitize-coverage-allow",
                        strlen("sanitize-coverage-allow")) &&
                strncmp(cur, "sanitize-coverage-deny",
                        strlen("sanitize-coverage-deny")) &&
                instrument_mode != INSTRUMENT_LLVMNATIVE)) {

      if (!be_quiet) { WARNF("Found '%s' - stripping!", cur); }
      continue;

    }

    if (need_aflpplib || !strcmp(cur, "-fsanitize=fuzzer")) {

      u8 *afllib = find_object("libAFLDriver.a", argv[0]);

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

        cc_params[cc_par_cnt++] = afllib;

#ifdef __APPLE__
        cc_params[cc_par_cnt++] = "-undefined";
        cc_params[cc_par_cnt++] = "dynamic_lookup";
#endif

      }

      if (need_aflpplib) {

        need_aflpplib = 0;

      } else {

        continue;

      }

    }

    if (!strcmp(cur, "-m32")) bit_mode = 32;
    if (!strcmp(cur, "armv7a-linux-androideabi")) bit_mode = 32;
    if (!strcmp(cur, "-m64")) bit_mode = 64;

    if (!strcmp(cur, "-fsanitize=address") || !strcmp(cur, "-fsanitize=memory"))
      asan_set = 1;

    if (strstr(cur, "FORTIFY_SOURCE")) fortify_set = 1;

    if (!strcmp(cur, "-x")) x_set = 1;
    if (!strcmp(cur, "-E")) preprocessor_only = 1;
    if (!strcmp(cur, "-shared")) shared_linking = 1;
    if (!strcmp(cur, "-dynamiclib")) shared_linking = 1;
    if (!strcmp(cur, "--target=wasm32-wasi")) passthrough = 1;
    if (!strcmp(cur, "-Wl,-r")) partial_linking = 1;
    if (!strcmp(cur, "-Wl,-i")) partial_linking = 1;
    if (!strcmp(cur, "-Wl,--relocatable")) partial_linking = 1;
    if (!strcmp(cur, "-r")) partial_linking = 1;
    if (!strcmp(cur, "--relocatable")) partial_linking = 1;
    if (!strcmp(cur, "-c")) have_c = 1;

    if (!strncmp(cur, "-O", 2)) have_o = 1;
    if (!strncmp(cur, "-funroll-loop", 13)) have_unroll = 1;

    if (*cur == '@') {

      // response file support.
      // we have two choices - move everything to the command line or
      // rewrite the response files to temporary files and delete them
      // afterwards. We choose the first for easiness.
      // We do *not* support quotes in the rsp files to cope with spaces in
      // filenames etc! If you need that then send a patch!
      u8 *filename = cur + 1;
      if (debug) { DEBUGF("response file=%s\n", filename); }
      FILE       *f = fopen(filename, "r");
      struct stat st;

      // Check not found or empty? let the compiler complain if so.
      if (!f || fstat(fileno(f), &st) < 0 || st.st_size < 1) {

        cc_params[cc_par_cnt++] = cur;
        continue;

      }

      u8    *tmpbuf = malloc(st.st_size + 2), *ptr;
      char **args = malloc(sizeof(char *) * (st.st_size >> 1));
      int    count = 1, cont = 0, cont_act = 0;

      while (fgets(tmpbuf, st.st_size + 1, f)) {

        ptr = tmpbuf;
        // fprintf(stderr, "1: %s\n", ptr);
        //  no leading whitespace
        while (isspace(*ptr)) {

          ++ptr;
          cont_act = 0;

        }

        // no comments, no empty lines
        if (*ptr == '#' || *ptr == '\n' || !*ptr) { continue; }
        // remove LF
        if (ptr[strlen(ptr) - 1] == '\n') { ptr[strlen(ptr) - 1] = 0; }
        // remove CR
        if (*ptr && ptr[strlen(ptr) - 1] == '\r') { ptr[strlen(ptr) - 1] = 0; }
        // handle \ at end of line
        if (*ptr && ptr[strlen(ptr) - 1] == '\\') {

          cont = 1;
          ptr[strlen(ptr) - 1] = 0;

        }

        // fprintf(stderr, "2: %s\n", ptr);

        // remove whitespace at end
        while (*ptr && isspace(ptr[strlen(ptr) - 1])) {

          ptr[strlen(ptr) - 1] = 0;
          cont = 0;

        }

        // fprintf(stderr, "3: %s\n", ptr);
        if (*ptr) {

          do {

            u8 *value = ptr;
            while (*ptr && !isspace(*ptr)) {

              ++ptr;

            }

            while (*ptr && isspace(*ptr)) {

              *ptr++ = 0;

            }

            if (cont_act) {

              u32 len = strlen(args[count - 1]) + strlen(value) + 1;
              u8 *tmp = malloc(len);
              snprintf(tmp, len, "%s%s", args[count - 1], value);
              free(args[count - 1]);
              args[count - 1] = tmp;
              cont_act = 0;

            } else {

              args[count++] = strdup(value);

            }

          } while (*ptr);

        }

        if (cont) {

          cont_act = 1;
          cont = 0;

        }

      }

      if (count) { process_params(count, args); }

      // we cannot free args[]
      free(tmpbuf);

      continue;

    }

    cc_params[cc_par_cnt++] = cur;

  }

}

/* Copy argv to cc_params, making the necessary edits. */

static void edit_params(aflcc_state_t *aflcc, u32 argc, char **argv, char **envp) {

  cc_params = ck_alloc(MAX_PARAMS_NUM * sizeof(u8 *));

  if (lto_mode) {

    if (lto_flag[0] != '-')
      FATAL(
          "Using afl-clang-lto is not possible because Makefile magic did not "
          "identify the correct -flto flag");
    else
      compiler_mode = LTO;

  }

  set_real_argv0(aflcc);

  if (compiler_mode == GCC || compiler_mode == CLANG) {

    cc_params[cc_par_cnt++] = "-B";
    cc_params[cc_par_cnt++] = obj_path;

    if (compiler_mode == CLANG) {

      cc_params[cc_par_cnt++] = "-no-integrated-as";

    }

  }

  if (compiler_mode == GCC_PLUGIN) {

    char *fplugin_arg;

    if (cmplog_mode) {

      fplugin_arg =
          alloc_printf("-fplugin=%s/afl-gcc-cmplog-pass.so", obj_path);
      cc_params[cc_par_cnt++] = fplugin_arg;
      fplugin_arg =
          alloc_printf("-fplugin=%s/afl-gcc-cmptrs-pass.so", obj_path);
      cc_params[cc_par_cnt++] = fplugin_arg;

    }

    fplugin_arg = alloc_printf("-fplugin=%s/afl-gcc-pass.so", obj_path);
    cc_params[cc_par_cnt++] = fplugin_arg;
    cc_params[cc_par_cnt++] = "-fno-if-conversion";
    cc_params[cc_par_cnt++] = "-fno-if-conversion2";

  }

  if (compiler_mode == LLVM || compiler_mode == LTO) {

    cc_params[cc_par_cnt++] = "-Wno-unused-command-line-argument";

    if (lto_mode && have_instr_env) {

      load_llvm_pass(aflcc, "afl-llvm-lto-instrumentlist.so");

    }

    if (getenv("AFL_LLVM_DICT2FILE")) {

      load_llvm_pass(aflcc, "afl-llvm-dict2file.so");

    }

    // laf
    if (getenv("LAF_SPLIT_SWITCHES") || getenv("AFL_LLVM_LAF_SPLIT_SWITCHES")) {

      load_llvm_pass(aflcc, "split-switches-pass.so");

    }

    if (getenv("LAF_TRANSFORM_COMPARES") ||
        getenv("AFL_LLVM_LAF_TRANSFORM_COMPARES")) {

      load_llvm_pass(aflcc, "compare-transform-pass.so");

    }

    if (getenv("LAF_SPLIT_COMPARES") || getenv("AFL_LLVM_LAF_SPLIT_COMPARES") ||
        getenv("AFL_LLVM_LAF_SPLIT_FLOATS")) {

      load_llvm_pass(aflcc, "split-compares-pass.so");

    }

    // /laf

    unsetenv("AFL_LD");
    unsetenv("AFL_LD_CALLER");

    if (cmplog_mode) {

      cc_params[cc_par_cnt++] = "-fno-inline";

      load_llvm_pass(aflcc, "cmplog-switches-pass.so");
      // reuse split switches from laf
      load_llvm_pass(aflcc, "split-switches-pass.so");

    }

    // #if LLVM_MAJOR >= 13
    //     // Use the old pass manager in LLVM 14 which the AFL++ passes still
    //     use. cc_params[cc_par_cnt++] = "-flegacy-pass-manager";
    // #endif

    if (lto_mode && !have_c) {

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
      cc_params[cc_par_cnt++] = alloc_printf("--ld-path=%s", ld_path);
#else
      cc_params[cc_par_cnt++] = alloc_printf("-fuse-ld=%s", ld_path);
#endif
      free(ld_path);

#if defined(AFL_CLANG_LDPATH) && LLVM_MAJOR >= 15
      // The NewPM implementation only works fully since LLVM 15.
      cc_params[cc_par_cnt++] = alloc_printf(
          "-Wl,--load-pass-plugin=%s/SanitizerCoverageLTO.so", obj_path);
#elif defined(AFL_CLANG_LDPATH) && LLVM_MAJOR >= 13
      cc_params[cc_par_cnt++] = "-Wl,--lto-legacy-pass-manager";
      cc_params[cc_par_cnt++] =
          alloc_printf("-Wl,-mllvm=-load=%s/SanitizerCoverageLTO.so", obj_path);
#else
      cc_params[cc_par_cnt++] = "-fno-experimental-new-pass-manager";
      cc_params[cc_par_cnt++] =
          alloc_printf("-Wl,-mllvm=-load=%s/SanitizerCoverageLTO.so", obj_path);
#endif

      cc_params[cc_par_cnt++] = "-Wl,--allow-multiple-definition";
      cc_params[cc_par_cnt++] = lto_flag;

    } else {

      if (instrument_mode == INSTRUMENT_PCGUARD) {

#if LLVM_MAJOR >= 13
  #if defined __ANDROID__ || ANDROID
        cc_params[cc_par_cnt++] = "-fsanitize-coverage=trace-pc-guard";
        instrument_mode = INSTRUMENT_LLVMNATIVE;
  #else
        if (have_instr_list) {

          if (!be_quiet)
            SAYF(
                "Using unoptimized trace-pc-guard, due usage of "
                "-fsanitize-coverage-allow/denylist, you can use "
                "AFL_LLVM_ALLOWLIST/AFL_LLMV_DENYLIST instead.\n");
          cc_params[cc_par_cnt++] = "-fsanitize-coverage=trace-pc-guard";
          instrument_mode = INSTRUMENT_LLVMNATIVE;

        } else {

    #if LLVM_MAJOR >= 13                            /* use new pass manager */
      #if LLVM_MAJOR < 16
          cc_params[cc_par_cnt++] = "-fexperimental-new-pass-manager";
      #endif
          cc_params[cc_par_cnt++] = alloc_printf(
              "-fpass-plugin=%s/SanitizerCoveragePCGUARD.so", obj_path);
    #else
          cc_params[cc_par_cnt++] = "-Xclang";
          cc_params[cc_par_cnt++] = "-load";
          cc_params[cc_par_cnt++] = "-Xclang";
          cc_params[cc_par_cnt++] =
              alloc_printf("%s/SanitizerCoveragePCGUARD.so", obj_path);
    #endif

        }

  #endif
#else
  #if LLVM_MAJOR >= 4
        if (!be_quiet)
          SAYF(
              "Using unoptimized trace-pc-guard, upgrade to LLVM 13+ for "
              "enhanced version.\n");
        cc_params[cc_par_cnt++] = "-fsanitize-coverage=trace-pc-guard";
        instrument_mode = INSTRUMENT_LLVMNATIVE;
  #else
        FATAL("pcguard instrumentation requires LLVM 4.0.1+");
  #endif
#endif

      } else if (instrument_mode == INSTRUMENT_LLVMNATIVE) {

#if LLVM_MAJOR >= 4
        if (instrument_opt_mode & INSTRUMENT_OPT_CODECOV) {

  #if LLVM_MAJOR >= 6
          cc_params[cc_par_cnt++] =
              "-fsanitize-coverage=trace-pc-guard,bb,no-prune,pc-table";
  #else
          FATAL("pcguard instrumentation with pc-table requires LLVM 6.0.1+");
  #endif

        } else {

          cc_params[cc_par_cnt++] = "-fsanitize-coverage=trace-pc-guard";

        }

#else
        FATAL("pcguard instrumentation requires LLVM 4.0.1+");
#endif

      } else {

        load_llvm_pass(aflcc, "afl-llvm-pass.so");

      }

    }

    if (cmplog_mode) {

      load_llvm_pass(aflcc, "cmplog-instructions-pass.so");
      load_llvm_pass(aflcc, "cmplog-routines-pass.so");

    }

    // cc_params[cc_par_cnt++] = "-Qunused-arguments";

    if (lto_mode && argc > 1) {

      u32 idx;
      for (idx = 1; idx < argc; idx++) {

        if (!strncasecmp(argv[idx], "-fpic", 5)) have_pic = 1;

      }

    }

  }

  /* Inspect the command line parameters. */

  process_params(argc, argv);

  if (!have_pic) { cc_params[cc_par_cnt++] = "-fPIC"; }

  // in case LLVM is installed not via a package manager or "make install"
  // e.g. compiled download or compiled from github then its ./lib directory
  // might not be in the search path. Add it if so.
  u8 *libdir = strdup(LLVM_LIBDIR);
  if (plusplus_mode && strlen(libdir) && strncmp(libdir, "/usr", 4) &&
      strncmp(libdir, "/lib", 4)) {

    cc_params[cc_par_cnt++] = "-Wl,-rpath";
    cc_params[cc_par_cnt++] = libdir;

  } else {

    free(libdir);

  }

  if (getenv("AFL_HARDEN")) {

    cc_params[cc_par_cnt++] = "-fstack-protector-all";

    if (!fortify_set) cc_params[cc_par_cnt++] = "-D_FORTIFY_SOURCE=2";

  }

  if (!asan_set) {

    if (getenv("AFL_USE_ASAN")) {

      if (getenv("AFL_USE_MSAN")) FATAL("ASAN and MSAN are mutually exclusive");

      if (getenv("AFL_HARDEN"))
        FATAL("ASAN and AFL_HARDEN are mutually exclusive");

      cc_params[cc_par_cnt++] = "-U_FORTIFY_SOURCE";
      cc_params[cc_par_cnt++] = "-fsanitize=address";

    } else if (getenv("AFL_USE_MSAN")) {

      if (getenv("AFL_USE_ASAN")) FATAL("ASAN and MSAN are mutually exclusive");

      if (getenv("AFL_HARDEN"))
        FATAL("MSAN and AFL_HARDEN are mutually exclusive");

      cc_params[cc_par_cnt++] = "-U_FORTIFY_SOURCE";
      cc_params[cc_par_cnt++] = "-fsanitize=memory";

    }

  }

  if (getenv("AFL_USE_UBSAN")) {

    cc_params[cc_par_cnt++] = "-fsanitize=undefined";
    cc_params[cc_par_cnt++] = "-fsanitize-undefined-trap-on-error";
    cc_params[cc_par_cnt++] = "-fno-sanitize-recover=all";
    cc_params[cc_par_cnt++] = "-fno-omit-frame-pointer";

  }

  if (getenv("AFL_USE_TSAN")) {

    cc_params[cc_par_cnt++] = "-fsanitize=thread";
    cc_params[cc_par_cnt++] = "-fno-omit-frame-pointer";

  }

  if (getenv("AFL_USE_LSAN")) {

    cc_params[cc_par_cnt++] = "-fsanitize=leak";
    cc_params[cc_par_cnt++] = "-includesanitizer/lsan_interface.h";
    cc_params[cc_par_cnt++] =
        "-D__AFL_LEAK_CHECK()={if(__lsan_do_recoverable_leak_check() > 0) "
        "_exit(23); }";
    cc_params[cc_par_cnt++] = "-D__AFL_LSAN_OFF()=__lsan_disable();";
    cc_params[cc_par_cnt++] = "-D__AFL_LSAN_ON()=__lsan_enable();";

  }

  if (getenv("AFL_USE_CFISAN")) {

    if (compiler_mode == GCC_PLUGIN || compiler_mode == GCC) {

      cc_params[cc_par_cnt++] = "-fcf-protection=full";

    } else {

      if (!lto_mode) {

        uint32_t i = 0, found = 0;
        while (envp[i] != NULL && !found)
          if (strncmp("-flto", envp[i++], 5) == 0) found = 1;
        if (!found) cc_params[cc_par_cnt++] = "-flto";

      }

      cc_params[cc_par_cnt++] = "-fsanitize=cfi";
      cc_params[cc_par_cnt++] = "-fvisibility=hidden";

    }

  }

  if (!getenv("AFL_DONT_OPTIMIZE")) {

    cc_params[cc_par_cnt++] = "-g";
    if (!have_o) cc_params[cc_par_cnt++] = "-O3";
    if (!have_unroll) cc_params[cc_par_cnt++] = "-funroll-loops";
    // if (strlen(march_opt) > 1 && march_opt[0] == '-')
    //  cc_params[cc_par_cnt++] = march_opt;

  }

  if (getenv("AFL_NO_BUILTIN") || getenv("AFL_LLVM_LAF_TRANSFORM_COMPARES") ||
      getenv("LAF_TRANSFORM_COMPARES") || getenv("AFL_LLVM_LAF_ALL") ||
      lto_mode) {

    cc_params[cc_par_cnt++] = "-fno-builtin-strcmp";
    cc_params[cc_par_cnt++] = "-fno-builtin-strncmp";
    cc_params[cc_par_cnt++] = "-fno-builtin-strcasecmp";
    cc_params[cc_par_cnt++] = "-fno-builtin-strncasecmp";
    cc_params[cc_par_cnt++] = "-fno-builtin-memcmp";
    cc_params[cc_par_cnt++] = "-fno-builtin-bcmp";
    cc_params[cc_par_cnt++] = "-fno-builtin-strstr";
    cc_params[cc_par_cnt++] = "-fno-builtin-strcasestr";

  }

#if defined(USEMMAP) && !defined(__HAIKU__) && !__APPLE__
  if (!have_c) cc_params[cc_par_cnt++] = "-lrt";
#endif

  cc_params[cc_par_cnt++] = "-D__AFL_COMPILER=1";
  cc_params[cc_par_cnt++] = "-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1";

  /* As documented in instrumentation/README.persistent_mode.md, deferred
     forkserver initialization and persistent mode are not available in afl-gcc
     and afl-clang. */
  if (compiler_mode != GCC && compiler_mode != CLANG) {

    cc_params[cc_par_cnt++] = "-D__AFL_HAVE_MANUAL_CONTROL=1";

    /* When the user tries to use persistent or deferred forkserver modes by
       appending a single line to the program, we want to reliably inject a
       signature into the binary (to be picked up by afl-fuzz) and we want
       to call a function from the runtime .o file. This is unnecessarily
       painful for three reasons:

       1) We need to convince the compiler not to optimize out the signature.
          This is done with __attribute__((used)).

       2) We need to convince the linker, when called with -Wl,--gc-sections,
          not to do the same. This is done by forcing an assignment to a
          'volatile' pointer.

       3) We need to declare __afl_persistent_loop() in the global namespace,
          but doing this within a method in a class is hard - :: and extern "C"
          are forbidden and __attribute__((alias(...))) doesn't work. Hence the
          __asm__ aliasing trick.

     */

    cc_params[cc_par_cnt++] =
        "-D__AFL_FUZZ_INIT()="
        "int __afl_sharedmem_fuzzing = 1;"
        "extern unsigned int *__afl_fuzz_len;"
        "extern unsigned char *__afl_fuzz_ptr;"
        "unsigned char __afl_fuzz_alt[1048576];"
        "unsigned char *__afl_fuzz_alt_ptr = __afl_fuzz_alt;";

  }

  if (plusplus_mode) {

    cc_params[cc_par_cnt++] =
        "-D__AFL_COVERAGE()=int __afl_selective_coverage = 1;"
        "extern \"C\" void __afl_coverage_discard();"
        "extern \"C\" void __afl_coverage_skip();"
        "extern \"C\" void __afl_coverage_on();"
        "extern \"C\" void __afl_coverage_off();";

  } else {

    cc_params[cc_par_cnt++] =
        "-D__AFL_COVERAGE()=int __afl_selective_coverage = 1;"
        "void __afl_coverage_discard();"
        "void __afl_coverage_skip();"
        "void __afl_coverage_on();"
        "void __afl_coverage_off();";

  }

  cc_params[cc_par_cnt++] =
      "-D__AFL_COVERAGE_START_OFF()=int __afl_selective_coverage_start_off = "
      "1;";
  cc_params[cc_par_cnt++] = "-D__AFL_COVERAGE_ON()=__afl_coverage_on()";
  cc_params[cc_par_cnt++] = "-D__AFL_COVERAGE_OFF()=__afl_coverage_off()";
  cc_params[cc_par_cnt++] =
      "-D__AFL_COVERAGE_DISCARD()=__afl_coverage_discard()";
  cc_params[cc_par_cnt++] = "-D__AFL_COVERAGE_SKIP()=__afl_coverage_skip()";
  cc_params[cc_par_cnt++] =
      "-D__AFL_FUZZ_TESTCASE_BUF=(__afl_fuzz_ptr ? __afl_fuzz_ptr : "
      "__afl_fuzz_alt_ptr)";
  cc_params[cc_par_cnt++] =
      "-D__AFL_FUZZ_TESTCASE_LEN=(__afl_fuzz_ptr ? *__afl_fuzz_len : "
      "(*__afl_fuzz_len = read(0, __afl_fuzz_alt_ptr, 1048576)) == 0xffffffff "
      "? 0 : *__afl_fuzz_len)";

  if (compiler_mode != GCC && compiler_mode != CLANG) {

    cc_params[cc_par_cnt++] =
        "-D__AFL_LOOP(_A)="
        "({ static volatile const char *_B __attribute__((used,unused)); "
        " _B = (const char*)\"" PERSIST_SIG
        "\"; "
        "extern int __afl_connected;"
#ifdef __APPLE__
        "__attribute__((visibility(\"default\"))) "
        "int _L(unsigned int) __asm__(\"___afl_persistent_loop\"); "
#else
        "__attribute__((visibility(\"default\"))) "
        "int _L(unsigned int) __asm__(\"__afl_persistent_loop\"); "
#endif                                                        /* ^__APPLE__ */
        // if afl is connected, we run _A times, else once.
        "_L(__afl_connected ? _A : 1); })";

    cc_params[cc_par_cnt++] =
        "-D__AFL_INIT()="
        "do { static volatile const char *_A __attribute__((used,unused)); "
        " _A = (const char*)\"" DEFER_SIG
        "\"; "
#ifdef __APPLE__
        "__attribute__((visibility(\"default\"))) "
        "void _I(void) __asm__(\"___afl_manual_init\"); "
#else
        "__attribute__((visibility(\"default\"))) "
        "void _I(void) __asm__(\"__afl_manual_init\"); "
#endif                                                        /* ^__APPLE__ */
        "_I(); } while (0)";

  }

  if (x_set) {

    cc_params[cc_par_cnt++] = "-x";
    cc_params[cc_par_cnt++] = "none";

  }

  // prevent unnecessary build errors
  if (compiler_mode != GCC_PLUGIN && compiler_mode != GCC) {

    cc_params[cc_par_cnt++] = "-Wno-unused-command-line-argument";

  }

  if (preprocessor_only || have_c || !non_dash) {

    /* In the preprocessor_only case (-E), we are not actually compiling at
       all but requesting the compiler to output preprocessed sources only.
       We must not add the runtime in this case because the compiler will
       simply output its binary content back on stdout, breaking any build
       systems that rely on a separate source preprocessing step. */
    cc_params[cc_par_cnt] = NULL;
    return;

  }

#ifndef __ANDROID__

  if (compiler_mode != GCC && compiler_mode != CLANG) {

    switch (bit_mode) {

      case 0:
        if (!shared_linking && !partial_linking)
          cc_params[cc_par_cnt++] =
              alloc_printf("%s/afl-compiler-rt.o", obj_path);
        if (lto_mode)
          cc_params[cc_par_cnt++] =
              alloc_printf("%s/afl-llvm-rt-lto.o", obj_path);
        break;

      case 32:
        if (!shared_linking && !partial_linking) {

          cc_params[cc_par_cnt++] =
              alloc_printf("%s/afl-compiler-rt-32.o", obj_path);
          if (access(cc_params[cc_par_cnt - 1], R_OK))
            FATAL("-m32 is not supported by your compiler");

        }

        if (lto_mode) {

          cc_params[cc_par_cnt++] =
              alloc_printf("%s/afl-llvm-rt-lto-32.o", obj_path);
          if (access(cc_params[cc_par_cnt - 1], R_OK))
            FATAL("-m32 is not supported by your compiler");

        }

        break;

      case 64:
        if (!shared_linking && !partial_linking) {

          cc_params[cc_par_cnt++] =
              alloc_printf("%s/afl-compiler-rt-64.o", obj_path);
          if (access(cc_params[cc_par_cnt - 1], R_OK))
            FATAL("-m64 is not supported by your compiler");

        }

        if (lto_mode) {

          cc_params[cc_par_cnt++] =
              alloc_printf("%s/afl-llvm-rt-lto-64.o", obj_path);
          if (access(cc_params[cc_par_cnt - 1], R_OK))
            FATAL("-m64 is not supported by your compiler");

        }

        break;

    }

  #if !defined(__APPLE__) && !defined(__sun)
    if (!shared_linking && !partial_linking)
      cc_params[cc_par_cnt++] =
          alloc_printf("-Wl,--dynamic-list=%s/dynamic_list.txt", obj_path);
  #endif

  #if defined(__APPLE__)
    if (shared_linking || partial_linking) {

      cc_params[cc_par_cnt++] = "-Wl,-U";
      cc_params[cc_par_cnt++] = "-Wl,___afl_area_ptr";
      cc_params[cc_par_cnt++] = "-Wl,-U";
      cc_params[cc_par_cnt++] = "-Wl,___sanitizer_cov_trace_pc_guard_init";

    }

  #endif

  }

  #if defined(USEMMAP) && !defined(__HAIKU__) && !__APPLE__
  cc_params[cc_par_cnt++] = "-lrt";
  #endif

#endif

  cc_params[cc_par_cnt] = NULL;

}

/* Main entry point */

int main(int argc, char **argv, char **envp) {

  aflcc_state_t *aflcc = malloc(sizeof(aflcc_state_t));
  aflcc_state_init(aflcc);

  init_callname(aflcc, argv[0]);

  if (getenv("AFL_DEBUG")) {

    aflcc->debug = 1;
    if (strcmp(getenv("AFL_DEBUG"), "0") == 0) unsetenv("AFL_DEBUG");

  } else if (getenv("AFL_QUIET"))

    be_quiet = 1;

  if ((getenv("AFL_PASSTHROUGH") || getenv("AFL_NOOPT")) && (!aflcc->debug))

    be_quiet = 1;

  check_environment_vars(envp);

  int   i;
  char *ptr = NULL;

  if ((ptr = find_object("as", argv[0])) != NULL) {

    have_gcc = 1;
    ck_free(ptr);

  }

#if (LLVM_MAJOR >= 3)

  if ((ptr = find_object("SanitizerCoverageLTO.so", argv[0])) != NULL) {

    have_lto = 1;
    ck_free(ptr);

  }

  if ((ptr = find_object("cmplog-routines-pass.so", argv[0])) != NULL) {

    have_llvm = 1;
    ck_free(ptr);

  }

#endif

#ifdef __ANDROID__
  have_llvm = 1;
#endif

  if ((ptr = find_object("afl-gcc-pass.so", argv[0])) != NULL) {

    have_gcc_plugin = 1;
    ck_free(ptr);

  }

  compiler_mode_by_callname(aflcc);
  compiler_mode_by_environ(aflcc);
  compiler_mode_by_cmdline(aflcc, argc, argv);

  instrument_mode_by_environ(aflcc);
  instrument_opt_mode_mutex(aflcc);

  if (instrument_opt_mode && instrument_mode == INSTRUMENT_DEFAULT &&
      (compiler_mode == LLVM || compiler_mode == UNSET)) {

    instrument_mode = INSTRUMENT_CLASSIC;
    compiler_mode = LLVM;

  }

  if (!compiler_mode) {

    // lto is not a default because outside of afl-cc RANLIB and AR have to
    // be set to LLVM versions so this would work
    if (have_llvm)
      compiler_mode = LLVM;
    else if (have_gcc_plugin)
      compiler_mode = GCC_PLUGIN;
    else if (have_gcc)
#ifdef __APPLE__
      // on OSX clang masquerades as GCC
      compiler_mode = CLANG;
#else
      compiler_mode = GCC;
#endif
    else if (have_lto)
      compiler_mode = LTO;
    else
      FATAL("no compiler mode available");

  }

  /* if our PCGUARD implementation is not available then silently switch to
     native LLVM PCGUARD */
  if (compiler_mode == CLANG &&
      (instrument_mode == INSTRUMENT_DEFAULT ||
       instrument_mode == INSTRUMENT_PCGUARD) &&
      find_object("SanitizerCoveragePCGUARD.so", argv[0]) == NULL) {

    instrument_mode = INSTRUMENT_LLVMNATIVE;

  }

  if (compiler_mode == GCC) {

    instrument_mode = INSTRUMENT_GCC;

  }

  if (compiler_mode == CLANG) {

    instrument_mode = INSTRUMENT_CLANG;
    setenv(CLANG_ENV_VAR, "1", 1);  // used by afl-as

  }

  maybe_show_help(aflcc, argc, argv);

  if (compiler_mode == LTO) {

    if (instrument_mode == 0 || instrument_mode == INSTRUMENT_LTO ||
        instrument_mode == INSTRUMENT_CFG ||
        instrument_mode == INSTRUMENT_PCGUARD) {

      lto_mode = 1;
      // force CFG
      // if (!instrument_mode) {

      instrument_mode = INSTRUMENT_PCGUARD;
      // ptr = instrument_mode_string[instrument_mode];
      // }

    } else if (instrument_mode == INSTRUMENT_CLASSIC) {

      lto_mode = 1;

    } else {

      if (!be_quiet) {

        WARNF("afl-clang-lto called with mode %s, using that mode instead",
              instrument_mode_string[instrument_mode]);

      }

    }

  }

  if (instrument_mode == 0 && compiler_mode < GCC_PLUGIN) {

#if LLVM_MAJOR >= 7
  #if LLVM_MAJOR < 11 && (LLVM_MAJOR < 10 || LLVM_MINOR < 1)
    if (have_instr_env) {

      instrument_mode = INSTRUMENT_AFL;
      if (!be_quiet) {

        WARNF(
            "Switching to classic instrumentation because "
            "AFL_LLVM_ALLOWLIST/DENYLIST does not work with PCGUARD < 10.0.1.");

      }

    } else

  #endif
      instrument_mode = INSTRUMENT_PCGUARD;

#else
    instrument_mode = INSTRUMENT_AFL;
#endif

  }

  if (instrument_opt_mode && compiler_mode != LLVM)
    FATAL("CTX, CALLER and NGRAM can only be used in LLVM mode");

  if (!instrument_opt_mode) {

    if (lto_mode && instrument_mode == INSTRUMENT_CFG)
      instrument_mode = INSTRUMENT_PCGUARD;
    ptr = instrument_mode_string[instrument_mode];

  } else {

    char *ptr2 = alloc_printf(" + NGRAM-%u", ngram_size);
    char *ptr3 = alloc_printf(" + K-CTX-%u", ctx_k);

    ptr = alloc_printf(
        "%s%s%s%s%s", instrument_mode_string[instrument_mode],
        (instrument_opt_mode & INSTRUMENT_OPT_CTX) ? " + CTX" : "",
        (instrument_opt_mode & INSTRUMENT_OPT_CALLER) ? " + CALLER" : "",
        (instrument_opt_mode & INSTRUMENT_OPT_NGRAM) ? ptr2 : "",
        (instrument_opt_mode & INSTRUMENT_OPT_CTX_K) ? ptr3 : "");

    ck_free(ptr2);
    ck_free(ptr3);

  }

#ifndef AFL_CLANG_FLTO
  if (lto_mode)
    FATAL(
        "instrumentation mode LTO specified but LLVM support not available "
        "(requires LLVM 11 or higher)");
#endif

  if (instrument_opt_mode && instrument_opt_mode != INSTRUMENT_OPT_CODECOV &&
      instrument_mode != INSTRUMENT_CLASSIC)
    FATAL(
        "CALLER, CTX and NGRAM instrumentation options can only be used with "
        "the LLVM CLASSIC instrumentation mode.");

  if (getenv("AFL_LLVM_SKIP_NEVERZERO") && getenv("AFL_LLVM_NOT_ZERO"))
    FATAL(
        "AFL_LLVM_NOT_ZERO and AFL_LLVM_SKIP_NEVERZERO can not be set "
        "together");

#if LLVM_MAJOR < 11 && (LLVM_MAJOR < 10 || LLVM_MINOR < 1)
  if (instrument_mode == INSTRUMENT_PCGUARD && have_instr_env) {

    FATAL(
        "Instrumentation type PCGUARD does not support "
        "AFL_LLVM_ALLOWLIST/DENYLIST! Use LLVM 10.0.1+ instead.");

  }

#endif

  u8 *ptr2;

  if ((ptr2 = getenv("AFL_LLVM_DICT2FILE")) != NULL && *ptr2 != '/')
    FATAL("AFL_LLVM_DICT2FILE must be set to an absolute file path");

  if ((isatty(2) && !be_quiet) || debug) {

    SAYF(cCYA
         "afl-cc" VERSION cRST
         " by Michal Zalewski, Laszlo Szekeres, Marc Heuse - mode: %s-%s\n",
         compiler_mode_string[compiler_mode], ptr);

  }

  if (!be_quiet && (compiler_mode == GCC || compiler_mode == CLANG)) {

    WARNF(
        "You are using outdated instrumentation, install LLVM and/or "
        "gcc-plugin and use afl-clang-fast/afl-clang-lto/afl-gcc-fast "
        "instead!");

  }

  if (aflcc->debug) 
    debugf_args(argc, argv);

  if (getenv("AFL_LLVM_LAF_ALL")) {

    setenv("AFL_LLVM_LAF_SPLIT_SWITCHES", "1", 1);
    setenv("AFL_LLVM_LAF_SPLIT_COMPARES", "1", 1);
    setenv("AFL_LLVM_LAF_SPLIT_FLOATS", "1", 1);
    setenv("AFL_LLVM_LAF_TRANSFORM_COMPARES", "1", 1);

  }

  cmplog_mode = getenv("AFL_CMPLOG") || getenv("AFL_LLVM_CMPLOG") ||
                getenv("AFL_GCC_CMPLOG");

#if !defined(__ANDROID__) && !defined(ANDROID)
  ptr = find_object("afl-compiler-rt.o", argv[0]);

  if (!ptr) {

    FATAL(
        "Unable to find 'afl-compiler-rt.o'. Please set the AFL_PATH "
        "environment variable.");

  }

  if (debug) { DEBUGF("rt=%s obj_path=%s\n", ptr, obj_path); }

  ck_free(ptr);
#endif

  edit_params(aflcc, argc, argv, envp);

  if (aflcc->debug) 
    debugf_args((s32)aflcc->cc_par_cnt, aflcc->cc_params);

  if (passthrough) {

    argv[0] = cc_params[0];
    execvp(cc_params[0], (char **)argv);

  } else {

    execvp(cc_params[0], (char **)cc_params);

  }

  FATAL("Oops, failed to execute '%s' - check your PATH", cc_params[0]);

  return 0;

}

