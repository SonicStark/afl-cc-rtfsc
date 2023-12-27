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


  if (getenv("AFL_HARDEN")) {

    cc_params[cc_par_cnt++] = "-fstack-protector-all";

    if (!fortify_set) ctrl_fortification(aflcc, 2);

  }

  if (!asan_set) {

    if (getenv("AFL_USE_ASAN")) {

      if (getenv("AFL_USE_MSAN")) FATAL("ASAN and MSAN are mutually exclusive");

      if (getenv("AFL_HARDEN"))
        FATAL("ASAN and AFL_HARDEN are mutually exclusive");

      set_fortification(aflcc, 0);
      cc_params[cc_par_cnt++] = "-fsanitize=address";

    } else if (getenv("AFL_USE_MSAN")) {

      if (getenv("AFL_USE_ASAN")) FATAL("ASAN and MSAN are mutually exclusive");

      if (getenv("AFL_HARDEN"))
        FATAL("MSAN and AFL_HARDEN are mutually exclusive");

      set_fortification(aflcc, 0);
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
    add_lsan_ctrl(aflcc);

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

  if (x_set) {

    cc_params[cc_par_cnt++] = "-x";
    cc_params[cc_par_cnt++] = "none";

  }

  // prevent unnecessary build errors
  if (compiler_mode != GCC_PLUGIN && compiler_mode != GCC) {

    cc_params[cc_par_cnt++] = "-Wno-unused-command-line-argument";

  }

  add_macro(aflcc);

  add_runtime(aflcc);

  aflcc->cc_params[aflcc->cc_par_cnt] = NULL;

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

  find_built_deps(aflcc);

  compiler_mode_by_callname(aflcc);
  compiler_mode_by_environ(aflcc);
  compiler_mode_by_cmdline(aflcc, argc, argv);

  instrument_mode_by_environ(aflcc);

  mode_final_checkout(aflcc, argc, argv);

  maybe_show_help(aflcc, argc, argv);

  mode_notification(aflcc);

  if (aflcc->debug) 
    debugf_args(argc, argv);

  edit_params(aflcc, argc, argv, envp);

  if (aflcc->debug) 
    debugf_args((s32)aflcc->cc_par_cnt, aflcc->cc_params);

  if (aflcc->passthrough) {

    argv[0] = aflcc->cc_params[0];
    execvp(aflcc->cc_params[0], (char **)argv);

  } else {

    execvp(aflcc->cc_params[0], (char **)aflcc->cc_params);

  }

  FATAL("Oops, failed to execute '%s' - check your PATH", 
          aflcc->cc_params[0]);

  return 0;

}

